#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/street_segment_commands.hpp"
#include "../model/demand/seam_checks.hpp"
#include "../project/link_type_codec.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// S3.7 (D312, docs/reference/STREET_SEGMENTS.md §7): the Seam checks dialog, on demand from the StreetSegments tab. One
// row per seamChecks row for the chosen level; a lane-count row at Micro offers Use Link and Use
// Node, each one Undo step, and the list is checked again after it. The speed-jump threshold is a
// display choice kept for the session.
namespace multra {
namespace {
std::string objectAt(const Network& n, const std::string& path) {
    const auto open = path.find('['), close = path.find(']');
    if (open == std::string::npos || close == std::string::npos) return {};
    const auto list = path.substr(0, open); const auto i = std::stoul(path.substr(open + 1, close - open - 1));
    if (list == "network.streetSegments" && i < n.streetSegments.size()) return n.streetSegments[i].id;
    if (list == "network.links" && i < n.links.size()) return n.links[i].id;
    if (list == "network.connectors" && i < n.connectors.size()) return n.connectors[i].id;
    return {};
}
}
void EditorWindow::seamChecksDialog() {
    QDialog dialog(this); dialog.setObjectName("editorSeamDialog"); dialog.setWindowTitle(text("editorSeamChecks"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* form = new QFormLayout; layout->addLayout(form);
    auto* level = new QComboBox(&dialog); level->setObjectName("editorSeamLevel");
    level->addItem(text("editorSeamMicro"), 0); level->addItem(text("editorSeamMeso"), 1); level->addItem(text("editorSeamMacro"), 2);
    form->addRow(text("editorSeamLevel"), level);
    auto* jump = new QDoubleSpinBox(&dialog); jump->setObjectName("editorSeamSpeedJump");
    jump->setRange(1, 200); jump->setDecimals(0); jump->setSuffix(" km/h"); jump->setValue(seamSpeedJump_);
    form->addRow(text("editorSeamSpeedJump"), jump);
    auto* table = new QTableWidget(0, 4, &dialog); table->setObjectName("editorSeamTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->hide(); table->horizontalHeader()->setStretchLastSection(true);
    table->setHorizontalHeaderLabels({text("editorSeamSeverity"), text("editorSeamProblem"), text("editorColumnId"), text("editorSeamFix")});
    layout->addWidget(table);
    auto* none = new QLabel(text("editorSeamNone"), &dialog); none->setObjectName("editorSeamNone"); layout->addWidget(none);
    std::function<void()> fill = [&] {
        const auto& d = history_.document();
        std::vector<LinkType> types;
        try { for (auto& r : resolveLinkTypes(d.definition.value_or(AuthoringDefinition{}), data_)) types.push_back(std::move(r.type)); }
        catch (const std::exception&) {} // an unreadable catalog skips the speed check; Run reports it
        const auto sections = crossSectionRows();
        const auto chosen = static_cast<SeamLevel>(level->currentData().toInt());
        const auto rows = seamChecks(d.network, types, sections, chosen, jump->value());
        table->setRowCount(static_cast<int>(rows.size()));
        for (int r = 0; r < table->rowCount(); ++r) {
            const auto& row = rows[static_cast<std::size_t>(r)];
            const auto id = objectAt(d.network, row.path);
            const QStringList values{text(row.severity == SeamSeverity::warning ? "editorSeamWarning" : "editorSeamAdvisory"),
                                     text(row.code), QString::fromStdString(id)};
            for (int c = 0; c < values.size(); ++c) table->setItem(r, c, new QTableWidgetItem(values[c]));
            table->setCellWidget(r, 3, nullptr);
            if (row.code != "SEAM_LANE_COUNT" || chosen != SeamLevel::micro) continue;
            auto* fixes = new QWidget(table); auto* buttons = new QHBoxLayout(fixes); buttons->setContentsMargins(0, 0, 0, 0);
            auto* useLink = new QPushButton(text("editorSeamUseLink"), fixes); useLink->setObjectName("editorSeamUseLink");
            auto* useNode = new QPushButton(text("editorSeamUseNode"), fixes); useNode->setObjectName("editorSeamUseNode");
            buttons->addWidget(useLink); buttons->addWidget(useNode);
            table->setCellWidget(r, 3, fixes);
            connect(useLink, &QPushButton::clicked, &dialog, [&, id] {
                execute("editorSeamUseLink", [&](auto& x) { useLinkLanes(x, id); }); QMetaObject::invokeMethod(&dialog, fill, Qt::QueuedConnection); });
            connect(useNode, &QPushButton::clicked, &dialog, [&, id] {
                const auto installed = crossSectionRows();
                execute("editorSeamUseNode", [&](auto& x) { useStreetSegmentLanes(x, id, installed); }); QMetaObject::invokeMethod(&dialog, fill, Qt::QueuedConnection); });
        }
        table->resizeColumnsToContents();
        // A cell widget sits inside QTableView::item's padding (editor_style.cpp), which the content
        // width leaves out: give the fix buttons' column that padding back, or their text is cut.
        table->setColumnWidth(3, table->columnWidth(3) + 2 * editorDesign::space1);
        none->setVisible(rows.empty());
        seamSpeedJump_ = jump->value();
    };
    fill();
    connect(level, &QComboBox::currentIndexChanged, &dialog, [&] { fill(); });
    connect(jump, &QDoubleSpinBox::valueChanged, &dialog, [&] { fill(); });
    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    close->button(QDialogButtonBox::Close)->setText(text("editorClose"));
    connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(close);
    dialog.resize(editorDesign::space3 * 50, editorDesign::space3 * 25);
    dialog.exec();
}
}
