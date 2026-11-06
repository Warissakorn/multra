#include "owned_list_dialog.hpp"
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
std::optional<std::vector<Json>> editOwnedList(QWidget* parent, const OwnedListSpec& spec, std::vector<Json> items, const DialogText& text,
                                               const TripRateFiles& files) {
    QDialog dialog(parent); dialog.setObjectName(spec.dialogName); dialog.setWindowTitle(text(spec.titleKey));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* table = new QTableWidget(0, static_cast<int>(spec.columnKeys.size()), &dialog); table->setObjectName(spec.tableName);
    QStringList headers;
    for (const auto* key : spec.columnKeys) headers << text(key);
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setMinimumSize(520, 200); layout->addWidget(table, 1);
    const auto fill = [&] {
        table->setRowCount(0);
        for (const auto& item : items) {
            const int row = table->rowCount(); table->insertRow(row);
            int column = 0;
            for (const auto& cell : spec.cells(item)) table->setItem(row, column++, new QTableWidgetItem(cell));
        }
    };
    fill();
    auto* row = new QHBoxLayout;
    auto* importButton = new QPushButton(text("editorTripRateImport"), &dialog); importButton->setObjectName(spec.importName);
    auto* exportButton = new QPushButton(text("editorTripRateExport"), &dialog); exportButton->setObjectName(spec.exportName);
    auto* removeButton = new QPushButton(text("editorTripRateRemove"), &dialog); removeButton->setObjectName(spec.removeName);
    row->addWidget(importButton); row->addWidget(exportButton); row->addWidget(removeButton); row->addStretch(); layout->addLayout(row);
    auto* error = new QLabel(&dialog); error->setObjectName(spec.errorName); error->setWordWrap(true); layout->addWidget(error);
    const auto refuse = [&](const QString& code, const QString& path) { error->setText(text(code.toUtf8().constData()) + " (" + path + ")"); };
    QObject::connect(importButton, &QPushButton::clicked, &dialog, [&] {
        const auto path = files.open ? files.open() : QString();
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) { refuse("EDIT_CATALOG_READ", path); return; }
        try {
            auto item = spec.parse(Json::parse(file.readAll().toStdString()));
            const auto id = spec.id(item);
            if (std::any_of(items.begin(), items.end(), [&](const auto& x) { return spec.id(x) == id; })) { refuse("DUPLICATE_ID", "file.id"); return; }
            items.push_back(std::move(item)); fill(); table->selectRow(table->rowCount() - 1); error->clear();
        } catch (const ValidationError& e) { refuse(QString::fromStdString(e.issues.front().code), QString::fromStdString(e.issues.front().path)); }
        catch (const std::exception&) { refuse(spec.invalidCode, "file"); }
    });
    QObject::connect(exportButton, &QPushButton::clicked, &dialog, [&] {
        const int r = table->currentRow();
        if (r < 0 || r >= static_cast<int>(items.size()) || !files.save) return;
        const auto path = files.save(QString::fromStdString(spec.id(items[r])) + ".json");
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
            file.write(QByteArray::fromStdString(items[r].dump(2) + "\n")) < 0) refuse(spec.exportFailedKey, path);
    });
    QObject::connect(removeButton, &QPushButton::clicked, &dialog, [&] {
        const int r = table->currentRow();
        if (r < 0 || r >= static_cast<int>(items.size())) return;
        items.erase(items.begin() + r); fill();
    });
    auto* footer = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    footer->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    footer->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(footer, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(footer, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(footer);
    dialog.resize(640, 360); fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return items;
}
}
