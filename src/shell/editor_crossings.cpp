#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/road_commands.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

// S3.5 (D309, docs/reference/ROADS.md §5, §18): Find crossings' batch dialog. Each place where Roads on
// one level cross or touch is a row with Junction (the default) or Overpass, the latter only where no
// Road ends; Confirm applies every row in one Undo step. The tolerance is ROADS §1's 1-2 m snap, 2 m by
// default, and Refresh searches again at the value typed. A place with a Road that has neither a
// template nor Links is marked in the warning colour, with one note: its Node is sized without that
// Road's width (2 x tolerance when no Road there has one, D382). The note does not block Confirm (D385).
namespace multra {
void EditorWindow::findCrossingsDialog() {
    constexpr double defaultTolerance = 2;
    if (findCrossings(history_.document().network, defaultTolerance).empty()) {
        statusBar()->showMessage(text("editorFindCrossingsNone"), 5000); return;
    }
    QDialog dialog(this); dialog.setObjectName("editorCrossingsDialog"); dialog.setWindowTitle(text("editorFindCrossings"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* form = new QFormLayout; layout->addLayout(form);
    auto* tolerance = new QDoubleSpinBox(&dialog); tolerance->setObjectName("editorCrossingTolerance");
    tolerance->setRange(0.1, 10); tolerance->setDecimals(1); tolerance->setSingleStep(0.5); tolerance->setSuffix(" m");
    tolerance->setValue(defaultTolerance);
    auto* refresh = new QPushButton(text("editorCrossingRefresh"), &dialog); refresh->setObjectName("editorCrossingRefresh");
    form->addRow(text("editorCrossingTolerance"), tolerance); form->addRow(QString(), refresh);
    auto* table = new QTableWidget(0, 6, &dialog); table->setObjectName("editorCrossingTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->setHorizontalHeaderLabels({text("editorCrossingLevel"), QStringLiteral("x"), QStringLiteral("y"), text("editorRoadTable"),
                                      text("editorCrossingLegs"), text("editorCrossingChoice")});
    layout->addWidget(table, 1);
    auto* warning = new QLabel(&dialog); warning->setObjectName("editorCrossingTemplateWarning"); warning->setWordWrap(true);
    auto palette = warning->palette(); palette.setColor(QPalette::WindowText, editorDesign::semantic(editorDesign::Semantic::warning));
    warning->setPalette(palette); layout->addWidget(warning);
    std::vector<RoadCrossing> found;
    const auto fill = [&] {
        const auto& network = history_.document().network;
        found = findCrossings(network, tolerance->value());
        table->setRowCount(static_cast<int>(found.size()));
        bool anyBare = false;
        for (int r = 0; r < table->rowCount(); ++r) {
            const auto& c = found[static_cast<std::size_t>(r)];
            QStringList roads, bare;
            for (const auto& x : c.roads) {
                roads << QString::fromStdString(x.roadId);
                const auto road = std::find_if(network.roads.begin(), network.roads.end(), [&](const auto& y) { return y.id == x.roadId; });
                if (road != network.roads.end() && !road->crossSectionId && road->links.empty()) bare << QString::fromStdString(x.roadId);
            }
            anyBare = anyBare || !bare.isEmpty();
            const QStringList values{QString::number(c.level), editorDesign::formatValue(c.at.x, 1), editorDesign::formatValue(c.at.y, 1),
                                     roads.join(", "), QString::number(c.legs)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]);
                if (col != 3) editorDesign::setNumericText(cell, true);
                if (!bare.isEmpty()) {
                    cell->setForeground(editorDesign::semantic(editorDesign::Semantic::warning));
                    cell->setToolTip(text("editorCrossingNoTemplateRoad").arg(bare.join(", ")));
                }
                table->setItem(r, col, cell);
            }
            auto* choice = new QComboBox(table); choice->setObjectName("editorCrossingChoice");
            choice->addItem(text("editorCrossingJunction"), static_cast<int>(CrossingChoice::junction));
            if (!c.endpoint) choice->addItem(text("editorCrossingOverpass"), static_cast<int>(CrossingChoice::overpass));
            table->setCellWidget(r, 5, choice);
        }
        table->resizeColumnsToContents();
        warning->setText(text("editorCrossingNoTemplate").arg(editorDesign::formatValue(2 * tolerance->value(), 1)));
        warning->setVisible(anyBare);
    };
    fill();
    connect(refresh, &QPushButton::clicked, &dialog, fill);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(560, 320);
    if (dialog.exec() != QDialog::Accepted || found.empty()) return;
    std::vector<CrossingChoice> choices;
    for (int r = 0; r < table->rowCount(); ++r)
        choices.push_back(static_cast<CrossingChoice>(qobject_cast<QComboBox*>(table->cellWidget(r, 5))->currentData().toInt()));
    const auto installed = crossSectionRows();
    std::size_t junctions = 0;
    if (execute("editorFindCrossings", [&](auto& d) { junctions = applyCrossings(d, found, choices, tolerance->value(), installed).size(); }))
        statusBar()->showMessage(text("editorFindCrossingsDone").arg(junctions).arg(choices.size() - junctions), 5000);
}
}
