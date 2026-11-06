#include "trip_rate_dialog.hpp"
#include "dialog_fit.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace multra {
std::optional<std::vector<TripRateSet>> editTripRateSets(QWidget* parent, const ProjectDocument& d, const DialogText& text,
                                                         const TripRateFiles& files) {
    QDialog dialog(parent); dialog.setObjectName("editorTripRateSetsDialog"); dialog.setWindowTitle(text("editorTripRateSets"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* table = new QTableWidget(0, 4, &dialog); table->setObjectName("editorTripRateSetsTable");
    table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorTripRateUnit"), text("editorTripRatePurposes")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setMinimumSize(520, 200); layout->addWidget(table, 1);
    std::vector<TripRateSet> sets = d.tripRateSets;
    const auto fill = [&] {
        table->setRowCount(0);
        for (const auto& s : sets) {
            const int row = table->rowCount(); table->insertRow(row);
            QStringList purposes;
            for (const auto& p : s.purposes) purposes << QString::fromStdString(p.id);
            int column = 0;
            for (const auto& cell : {QString::fromStdString(s.id), QString::fromStdString(s.name), QString::fromStdString(s.unit), purposes.join(", ")})
                table->setItem(row, column++, new QTableWidgetItem(cell));
        }
    };
    fill();
    auto* row = new QHBoxLayout;
    auto* importButton = new QPushButton(text("editorTripRateImport"), &dialog); importButton->setObjectName("editorTripRateImport");
    auto* exportButton = new QPushButton(text("editorTripRateExport"), &dialog); exportButton->setObjectName("editorTripRateExport");
    auto* removeButton = new QPushButton(text("editorTripRateRemove"), &dialog); removeButton->setObjectName("editorTripRateRemove");
    row->addWidget(importButton); row->addWidget(exportButton); row->addWidget(removeButton); row->addStretch(); layout->addLayout(row);
    auto* error = new QLabel(&dialog); error->setObjectName("editorTripRateError"); error->setWordWrap(true); layout->addWidget(error);
    const auto refuse = [&](const QString& code, const QString& path) { error->setText(text(code.toUtf8().constData()) + " (" + path + ")"); };
    QObject::connect(importButton, &QPushButton::clicked, &dialog, [&] {
        const auto path = files.open ? files.open() : QString();
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) { refuse("EDIT_CATALOG_READ", path); return; }
        try {
            auto set = parseTripRateSet(Json::parse(file.readAll().toStdString()), "file", false);
            if (std::any_of(sets.begin(), sets.end(), [&](const auto& s) { return s.id == set.id; })) { refuse("DUPLICATE_ID", "file.id"); return; }
            sets.push_back(std::move(set)); fill(); table->selectRow(table->rowCount() - 1); error->clear();
        } catch (const ValidationError& e) { refuse(QString::fromStdString(e.issues.front().code), QString::fromStdString(e.issues.front().path)); }
        catch (const std::exception&) { refuse("INVALID_TRIP_RATE_SET", "file"); }
    });
    QObject::connect(exportButton, &QPushButton::clicked, &dialog, [&] {
        const int r = table->currentRow();
        if (r < 0 || r >= static_cast<int>(sets.size()) || !files.save) return;
        const auto path = files.save(QString::fromStdString(sets[r].id) + ".json");
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
            file.write(QByteArray::fromStdString(tripRateSetJson(sets[r]).dump(2) + "\n")) < 0) refuse("editorTripRateExportFailed", path);
    });
    QObject::connect(removeButton, &QPushButton::clicked, &dialog, [&] {
        const int r = table->currentRow();
        if (r < 0 || r >= static_cast<int>(sets.size())) return;
        sets.erase(sets.begin() + r); fill();
    });
    auto* footer = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    footer->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    footer->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(footer, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(footer, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(footer);
    dialog.resize(640, 360); fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return sets;
}
}
