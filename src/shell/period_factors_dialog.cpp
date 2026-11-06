#include "period_factors_dialog.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cmath>

namespace multra {
std::optional<std::vector<PeriodFactor>> editPeriodFactors(QWidget* parent, const ProjectDocument& d, const DialogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorPeriodFactorsDialog"); dialog.setWindowTitle(text("editorPeriodFactors"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* note = new QLabel(text("editorPeriodFactorsNote"), &dialog); note->setWordWrap(true); layout->addWidget(note);
    auto* table = new QTableWidget(0, 4, &dialog); table->setObjectName("editorPeriodFactorsTable");
    table->setHorizontalHeaderLabels({text("editorTripPurpose"), text("editorPeriodColumn"), text("editorPeriodFactorPA"), text("editorPeriodFactorAP")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setMinimumSize(520, 200); layout->addWidget(table, 1);
    const auto addRow = [&](const PeriodFactor& f) {
        const int row = table->rowCount(); table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(f.purposeId)));
        auto* period = new QComboBox(table);
        for (const auto& p : d.zones.studyPeriods)
            period->addItem(QString::fromStdString(p.name.empty() ? p.id : p.name + " (" + p.id + ")"), QString::fromStdString(p.id));
        if (const int at = period->findData(QString::fromStdString(f.periodId)); at >= 0) period->setCurrentIndex(at);
        else if (!f.periodId.empty()) { period->addItem(QString::fromStdString(f.periodId), QString::fromStdString(f.periodId)); period->setCurrentIndex(period->count() - 1); }
        table->setCellWidget(row, 1, period);
        table->setItem(row, 2, new QTableWidgetItem(QString::number(f.productionToAttraction)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(f.attractionToProduction)));
    };
    for (const auto& f : d.periodFactors) addRow(f);
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton(text("editorPeriodFactorAdd"), &dialog); add->setObjectName("editorPeriodFactorAdd");
    auto* remove = new QPushButton(text("editorTripRateRemove"), &dialog); remove->setObjectName("editorPeriodFactorRemove");
    add->setEnabled(!d.zones.studyPeriods.empty());
    buttons->addWidget(add); buttons->addWidget(remove); buttons->addStretch(); layout->addLayout(buttons);
    auto* error = new QLabel(&dialog); error->setObjectName("editorPeriodFactorsError"); error->setWordWrap(true); layout->addWidget(error);
    QObject::connect(add, &QPushButton::clicked, &dialog, [&] {
        addRow({"", d.zones.studyPeriods.front().id, 0, 0}); table->selectRow(table->rowCount() - 1);
    });
    QObject::connect(remove, &QPushButton::clicked, &dialog, [&] { if (table->currentRow() >= 0) table->removeRow(table->currentRow()); });
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addWidget(box);
    std::vector<PeriodFactor> result;
    QObject::connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(box, &QDialogButtonBox::accepted, &dialog, [&] {
        std::vector<PeriodFactor> rows;
        for (int r = 0; r < table->rowCount(); ++r) {
            const auto at = QString("periodFactors[%1]").arg(r);
            const auto share = [&](int column, double& out) {
                bool ok = false;
                out = table->item(r, column) ? table->item(r, column)->text().trimmed().toDouble(&ok) : 0;
                return ok && std::isfinite(out) && out >= 0 && out <= 1;
            };
            PeriodFactor f;
            f.purposeId = table->item(r, 0) ? table->item(r, 0)->text().trimmed().toStdString() : std::string();
            f.periodId = static_cast<QComboBox*>(table->cellWidget(r, 1))->currentData().toString().toStdString();
            if (f.purposeId.empty() || !share(2, f.productionToAttraction) || !share(3, f.attractionToProduction) ||
                f.productionToAttraction + f.attractionToProduction > 1) {
                error->setText(text("PERIOD_FACTOR_INVALID") + " (" + at + ")"); table->selectRow(r); return;
            }
            rows.push_back(std::move(f));
        }
        result = std::move(rows); dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return result;
}
}
