#include "editor_window.hpp"
#include "dialog_fit.hpp"
#include "../project/junction_template_codec.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/street_segment_commands.hpp"
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

// S3.5 (D309, docs/reference/STREET_SEGMENTS.md §5, §18): Find crossings' batch dialog. Each place where StreetSegments on
// one level cross or touch is a row with Junction (the default) or Overpass, the latter only where no
// StreetSegment ends; Confirm applies every row in one Undo step. The tolerance is STREET_SEGMENTS §1's 1-2 m snap, 2 m by
// default, and Refresh searches again at the value typed. A place with a StreetSegment that has neither a
// template nor Links is marked in the warning colour, with one note: its Node is sized without that
// StreetSegment's width (2 x tolerance when no StreetSegment there has one, D382). The note does not block Confirm (D385).
namespace multra {
void EditorWindow::findCrossingsDialog() {
    constexpr double defaultTolerance = 2;
    if (findCrossings(history_.document().network, defaultTolerance).empty()) {
        statusBar()->showMessage(text("editorFindCrossingsNone"), 5000); return;
    }
    std::vector<JunctionTemplate> templates;
    try {for(auto& row:resolveJunctionTemplates(history_.document().definition.value_or(AuthoringDefinition{}),data_))templates.push_back(std::move(row.type));}
    catch(const std::exception& e){showError(e);return;}
    QDialog dialog(this); dialog.setObjectName("editorCrossingsDialog"); dialog.setWindowTitle(text("editorFindCrossings"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* form = new QFormLayout; layout->addLayout(form);
    auto* tolerance = new QDoubleSpinBox(&dialog); tolerance->setObjectName("editorCrossingTolerance");
    tolerance->setRange(0.1, 10); tolerance->setDecimals(1); tolerance->setSingleStep(0.5); tolerance->setSuffix(" m");
    tolerance->setValue(defaultTolerance);
    auto* refresh = new QPushButton(text("editorCrossingRefresh"), &dialog); refresh->setObjectName("editorCrossingRefresh");
    form->addRow(text("editorCrossingTolerance"), tolerance); form->addRow(QString(), refresh);
    auto* table = new QTableWidget(0, 7, &dialog); table->setObjectName("editorCrossingTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->setHorizontalHeaderLabels({text("editorCrossingLevel"), QStringLiteral("x"), QStringLiteral("y"), text("editorStreetSegmentTable"),
                                      text("editorCrossingLegs"), text("editorCrossingChoice"),text("editorTransitionTemplate")});
    layout->addWidget(table, 1);
    auto* warning = new QLabel(&dialog); warning->setObjectName("editorCrossingTemplateWarning"); warning->setWordWrap(true);
    auto palette = warning->palette(); palette.setColor(QPalette::WindowText, editorDesign::semantic(editorDesign::Semantic::warning));
    warning->setPalette(palette); layout->addWidget(warning);
    std::vector<StreetSegmentCrossing> found;
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    const auto validate=[&] {
        bool ready=!found.empty();
        for(std::size_t r=0;r<found.size();++r)if(found[r].transition) {
            const auto* select=qobject_cast<QComboBox*>(table->cellWidget(static_cast<int>(r),6));
            ready=ready&&select&&!select->currentData().toString().isEmpty();
        }
        buttons->button(QDialogButtonBox::Ok)->setEnabled(ready);
    };
    const auto fill = [&] {
        const auto& network = history_.document().network;
        found = findCrossings(network, tolerance->value());
        table->clearContents();
        table->setRowCount(static_cast<int>(found.size()));
        bool anyBare = false;
        for (int r = 0; r < table->rowCount(); ++r) {
            const auto& c = found[static_cast<std::size_t>(r)];
            QStringList roads, bare;
            for (const auto& x : c.streetSegments) {
                roads << QString::fromStdString(x.streetSegmentId);
                const auto road = std::find_if(network.streetSegments.begin(), network.streetSegments.end(), [&](const auto& y) { return y.id == x.streetSegmentId; });
                if (road != network.streetSegments.end() && !road->crossSectionId && road->links.empty()) bare << QString::fromStdString(x.streetSegmentId);
            }
            anyBare = anyBare || !bare.isEmpty();
            const QStringList values{QString::number(c.level), editorDesign::formatValue(c.at.x, 1), editorDesign::formatValue(c.at.y, 1),
                                     roads.join(", "), QString::number(c.legs)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]);
                if (col != 3) editorDesign::setNumericText(cell, true);
                if (!bare.isEmpty()) {
                    cell->setForeground(editorDesign::semantic(editorDesign::Semantic::warning));
                    cell->setToolTip(text("editorCrossingNoTemplateStreetSegment").arg(bare.join(", ")));
                }
                table->setItem(r, col, cell);
            }
            auto* choice = new QComboBox(table); choice->setObjectName("editorCrossingChoice");
            choice->addItem(text(c.transition?"editorCrossingTransition":"editorCrossingJunction"),static_cast<int>(c.transition?CrossingChoice::transition:CrossingChoice::junction));
            if (!c.endpoint&&!c.transition) choice->addItem(text("editorCrossingOverpass"), static_cast<int>(CrossingChoice::overpass));
            table->setCellWidget(r, 5, choice);
            if(c.transition) {
                auto* select=new QComboBox(table);select->setObjectName("editorTransitionTemplate");
                select->addItem(text("editorTransitionNone"),QString());
                for(const auto& t:templates)if(t.kind=="transition")select->addItem(QString::fromStdString(t.name),QString::fromStdString(t.id));
                table->setCellWidget(r,6,select);
                connect(select,&QComboBox::currentIndexChanged,&dialog,validate);
            } else table->setItem(r,6,new QTableWidgetItem);

        }
        table->setColumnHidden(6,std::none_of(found.begin(),found.end(),[](const auto& c){return c.transition;}));
        validate();
        table->resizeColumnsToContents();
        warning->setText(text("editorCrossingNoTemplate").arg(editorDesign::formatValue(2 * tolerance->value(), 1)));
        warning->setVisible(anyBare);
    };
    fill();
    connect(refresh, &QPushButton::clicked, &dialog, fill);

    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(table->isColumnHidden(6)?560:760, 360);fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted || found.empty()) return;
    std::vector<CrossingChoice> choices;std::vector<std::optional<NodeJunction>> recipes;
    for (int r = 0; r < table->rowCount(); ++r) {
        choices.push_back(static_cast<CrossingChoice>(qobject_cast<QComboBox*>(table->cellWidget(r,5))->currentData().toInt()));
        if(choices.back()==CrossingChoice::transition) {
            NodeJunction recipe;recipe.templateId=qobject_cast<QComboBox*>(table->cellWidget(r,6))->currentData().toString().toStdString();recipes.push_back(recipe);
        } else recipes.push_back(std::nullopt);
    }
    const auto installed = crossSectionRows();
    std::size_t junctions = 0;
    if (execute("editorFindCrossings", [&](auto& d) { junctions = applyCrossings(d, found, choices, tolerance->value(), installed,recipes,templates).size(); })) {
        const auto transitions=static_cast<std::size_t>(std::count(choices.begin(),choices.end(),CrossingChoice::transition));
        statusBar()->showMessage(transitions?text("editorFindCrossingsTransitionsDone").arg(junctions-transitions).arg(transitions).arg(choices.size()-junctions):
            text("editorFindCrossingsDone").arg(junctions).arg(choices.size()-junctions),5000);
    }
}
}
