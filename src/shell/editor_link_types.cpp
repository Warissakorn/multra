#include "editor_window.hpp"
#include "link_types_panel.hpp"
#include "../commands/catalog_commands.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSignalBlocker>
#include <algorithm>

// S2.4 (MACRO_NETWORK §20, D277): the Link's type and posted-speed fields in Properties. The Link
// types tab is LinkTypesPanel since D521.
namespace multra {
void EditorWindow::buildLinkTypeField(QFormLayout* form) {
    linkType_ = new QComboBox(this); linkType_->setObjectName("editorLinkType");
    // D463: a long type name must not widen Properties past its panel; the field shrinks and
    // the list still shows each name in full.
    linkType_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); linkType_->setMinimumContentsLength(8); label(form, "editorLinkType", linkType_);
    // Every selected Link at once, as one Undo step; "(none)" clears it (Macro then refuses the Link).
    connect(linkType_, &QComboBox::activated, this, [this] {
        const auto chosen = linkType_->currentData().toString().toStdString();
        std::vector<std::string> links;
        const auto& n = history_.document().network;
        for (const auto& id : canvas_->selection())
            if (std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; })) links.push_back(id);
        if (links.empty()) return;
        execute("editorApplyLinkType", [&](auto& d) { for (const auto& id : links) setLinkType(d, id, chosen.empty() ? std::nullopt : std::optional(chosen)); });
    });
    // J1 (D390): the posted speed, km/h, on every selected Link as one Undo step; empty clears it.
    linkSpeed_ = new QLineEdit(this); linkSpeed_->setObjectName("editorLinkSpeed"); label(form, "editorLinkSpeed", linkSpeed_);
    connect(linkSpeed_, &QLineEdit::editingFinished, this, [this] {
        const auto typed = linkSpeed_->text().trimmed();
        bool ok = typed.isEmpty();
        const auto speed = ok ? std::nullopt : std::optional(typed.toDouble(&ok));
        std::vector<std::string> links;
        const auto& n = history_.document().network;
        for (const auto& id : canvas_->selection())
            if (std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id && l.speedLimit != speed; })) links.push_back(id);
        if (!ok) { showError(std::invalid_argument("INVALID_LINK_SPEED")); refreshLinkTypeField(); return; }
        if (links.empty()) return;
        execute("editorApplyLinkSpeed", [&](auto& d) { for (const auto& id : links) setLinkSpeedLimit(d, id, speed); });
    });
}
void EditorWindow::refreshLinkTypeField() {
    if (!linkType_) return;
    const auto& d = history_.document();
    std::optional<std::string> current; bool link = false;
    for (const auto& id : canvas_->selection())
        for (const auto& l : d.network.links) if (l.id == id && !link) { current = l.linkTypeId; link = true; }
    const QSignalBlocker block(linkType_);
    linkType_->clear(); linkType_->addItem(text("editorLinkTypeNone"), QString());
    for (const auto& t : linkTypeRowsOf(d, data_))
        linkType_->addItem(t.name.empty() ? QString::fromStdString(t.id) : QString::fromStdString(t.name + " (" + t.id + ")"), QString::fromStdString(t.id));
    const auto value = QString::fromStdString(current.value_or(""));
    if (linkType_->findData(value) < 0) linkType_->addItem(value + " " + text("editorBehaviourMissing"), value);
    linkType_->setCurrentIndex(linkType_->findData(value));
    linkType_->setEnabled(link);
    std::optional<double> speed; bool seen = false;
    for (const auto& id : canvas_->selection())
        for (const auto& l : d.network.links) if (l.id == id && !seen) { speed = l.speedLimit; seen = true; }
    const QSignalBlocker blockSpeed(linkSpeed_);
    linkSpeed_->setText(speed ? QString::number(*speed) : QString());
    linkSpeed_->setEnabled(link);
}
}
