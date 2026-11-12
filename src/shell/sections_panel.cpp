#include "table_layout.hpp"
#include "sections_panel.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/right_of_way_commands.hpp"
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// M5.4b (D133): the Travel-time sections tab. The Section tool's second Ctrl+right-click commits
// through context_.execute(), so a placed section is one Undo step and invalidates the run, as rename and
// delete are. M5.5 (D134): the dialog also sets the control type LOS reads, in the same step.
namespace multra {
namespace {
QString describe(const Network& n, const SectionLine& l) {
    const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == l.linkId; });
    const auto name = link == n.links.end() || link->name.empty() ? l.linkId : link->name;
    return QString("%1 @ %2 m").arg(QString::fromStdString(name)).arg(l.station, 0, 'f', 1);
}
}
SectionsPanel::SectionsPanel(QWidget* parent, ObjectTabContext context) : ObjectTabPanel(parent, std::move(context)) {
    auto* body = this; auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 5, body); tab_.table->setObjectName("editorSectionTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    bar->addAction(action("editorEditSection", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(action("editorDeleteSection", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteSection", [&](auto& d) { deleteTravelTimeSection(d, id); });
    }));
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selected(); if (!id.empty()) edit(id); });
    connect(tab_.table, &QTableWidget::activated, this, [this](const QModelIndex&) { // Enter
        const auto id = selected(); if (!id.empty()) edit(id); });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void SectionsPanel::translate() {
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorSectionControl"),
                                              text("editorSectionStart"), text("editorSectionEnd")});
    tab_.invalidate();
}
std::string SectionsPanel::selected() const {
    const auto* cell = tab_.table->item(tab_.table->currentRow(), 0);
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void SectionsPanel::refresh() {
    const auto& n = context_.document().network;
    const auto keep = selected();
    if (tab_.revision != context_.revision() || tab_.table->rowCount() != static_cast<int>(n.travelTimeSections.size())) {
        tab_.revision = context_.revision();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(n.travelTimeSections.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& s = n.travelTimeSections[static_cast<std::size_t>(r)];
            const auto control = !s.controlType ? text("editorSectionControlNone")
                : text(*s.controlType == SectionControl::signalised ? "editorSectionSignalised" : "editorSectionUnsignalised");
            const QStringList values{QString::fromStdString(s.id), QString::fromStdString(s.name), control, describe(n, s.start), describe(n, s.end)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(s.id));
                if (col == 0 || col >= 3) editorDesign::setNumericText(cell, col >= 3);
                tab_.table->setItem(r, col, cell);
            }
            if (s.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
    }
    const bool chosen = !selected().empty();
    for (const auto* key : {"editorEditSection", "editorDeleteSection"}) actions_.at(key)->setEnabled(chosen);
}
void SectionsPanel::edit(const std::string& id) {
    const auto& sections = context_.document().network.travelTimeSections;
    const auto s = std::find_if(sections.begin(), sections.end(), [&](const auto& x) { return x.id == id; });
    if (s == sections.end()) return;
    QDialog dialog(this); dialog.setObjectName("editorSectionDialog"); dialog.setWindowTitle(text("editorEditSection"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(s->name), &dialog); name->setObjectName("editorSectionName");
    form->addRow(text("editorColumnName"), name);
    auto* control = new QComboBox(&dialog); control->setObjectName("editorSectionControl");
    control->addItem(text("editorSectionControlNone"), -1);
    control->addItem(text("editorSectionSignalised"), static_cast<int>(SectionControl::signalised));
    control->addItem(text("editorSectionUnsignalised"), static_cast<int>(SectionControl::unsignalised));
    control->setCurrentIndex(control->findData(s->controlType ? static_cast<int>(*s->controlType) : -1));
    form->addRow(text("editorSectionControl"), control);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto edited = *s; edited.name = name->text().toStdString();
    const int chosen = control->currentData().toInt();
    edited.controlType = chosen < 0 ? std::nullopt : std::optional(static_cast<SectionControl>(chosen));
    context_.execute("editorEditSection", [&](auto& d) { putTravelTimeSection(d, edited); });
}
void SectionsPanel::commitTool(SectionLine start, SectionLine end) {
    std::string id;
    if (!context_.execute("editorSectionTool", [&](auto& d) { id = putTravelTimeSection(d, {"", "", start, end}); })) return;
    context_.show(this);
    for (int r = 0; r < tab_.table->rowCount(); ++r)
        if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) tab_.table->selectRow(r);
}
}
