#include "object_table.hpp"
#include "table_grid.hpp"
#include "table_layout.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

// D525 (docs/plans/OBJECT_TABLE.md): the metadata-driven object table.
namespace multra {
ObjectTable::ObjectTable(QWidget* parent, ObjectTabContext context, ObjectTableModel model, const QString& objectName, bool severalRows)
    : QWidget(parent), context_(std::move(context)), model_(std::move(model)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
    table_ = new QTableWidget(0, static_cast<int>(model_.columns.size()), this); table_->setObjectName(objectName);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(severalRows ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
    table_->horizontalHeader()->setStretchLastSection(true); table_->verticalHeader()->hide();
    layout->addWidget(table_);
}
void ObjectTable::translate() {
    QStringList headers;
    for (const auto& c : model_.columns) headers << context_.text(c.header);
    table_->setHorizontalHeaderLabels(headers);
    invalidate(); // translated cells are rebuilt on the next refresh
}
std::string ObjectTable::selected() const {
    const auto* cell = table_->item(table_->currentRow(), 0);
    return cell && !table_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
std::vector<std::string> ObjectTable::selectedIds() const {
    std::vector<std::string> ids;
    for (const int row : selectedTableRows(table_)) ids.push_back(table_->item(row, 0)->data(Qt::UserRole).toString().toStdString());
    return ids;
}
bool ObjectTable::selectRow(const std::string& id) {
    for (int r = 0; r < table_->rowCount(); ++r)
        if (table_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) { table_->selectRow(r); return true; }
    return false;
}
void ObjectTable::clearSelection() {
    const QSignalBlocker block(table_);
    table_->clearSelection(); table_->setCurrentItem(nullptr);
}
void ObjectTable::refresh() {
    if (revision_ == context_.revision() && table_->rowCount() == static_cast<int>(model_.count(context_.document()))) return;
    const auto rows = model_.rows(context_.document());
    const auto keep = selected();
    const QSignalBlocker block(table_);
    table_->setRowCount(static_cast<int>(rows.size()));
    for (int r = 0; r < table_->rowCount(); ++r) {
        const auto& row = rows[static_cast<std::size_t>(r)];
        for (int col = 0; col < row.cells.size(); ++col) {
            auto* cell = new QTableWidgetItem(row.cells[col]); cell->setData(Qt::UserRole, QString::fromStdString(row.id));
            const auto kind = model_.columns[static_cast<std::size_t>(col)].kind;
            if (kind != CellKind::text) editorDesign::setNumericText(cell, kind == CellKind::number);
            table_->setItem(r, col, cell);
        }
        if (row.id == keep) table_->selectRow(r);
    }
    fitTableColumns(table_);
    revision_ = context_.revision();
}
}
