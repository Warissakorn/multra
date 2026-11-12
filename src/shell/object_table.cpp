#include "object_table.hpp"
#include "table_grid.hpp"
#include "table_layout.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QMetaObject>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

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
    columnMenu_ = new QMenu(this); columnMenu_->setObjectName(objectName + "Columns");
    for (std::size_t i = 0; i < model_.columns.size(); ++i) {
        const auto& c = model_.columns[i];
        auto* entry = columnMenu_->addAction(QString()); entry->setObjectName(QString::fromStdString(c.key));
        entry->setCheckable(true); entry->setChecked(true); entry->setEnabled(c.kind != CellKind::id);
        connect(entry, &QAction::toggled, this, [this, key = c.key](bool on) { setColumnShown(key, on); });
    }
    table_->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table_->horizontalHeader(), &QWidget::customContextMenuRequested, this, [this](const QPoint& at) {
        columnMenu_->popup(table_->horizontalHeader()->mapToGlobal(at)); });
    // An edit commits after the editor closes: the commit rebuilds the rows, the cell included.
    if (std::any_of(model_.columns.begin(), model_.columns.end(), [](const auto& c) { return bool(c.edit); }))
        table_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    connect(table_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* cell) {
        const auto id = cell->data(Qt::UserRole).toString().toStdString(); const int column = cell->column(); const auto text = cell->text();
        QMetaObject::invokeMethod(this, [this, id, column, text] { commit(id, column, text); }, Qt::QueuedConnection);
    });
}
void ObjectTable::setColumnShown(const std::string& key, bool shown) {
    for (std::size_t i = 0; i < model_.columns.size(); ++i) {
        if (model_.columns[i].key != key || model_.columns[i].kind == CellKind::id) continue;
        table_->setColumnHidden(static_cast<int>(i), !shown);
        auto* entry = columnMenu_->actions().value(static_cast<int>(i));
        if (entry && entry->isChecked() != shown) { const QSignalBlocker block(entry); entry->setChecked(shown); }
    }
}
bool ObjectTable::columnShown(const std::string& key) const {
    for (std::size_t i = 0; i < model_.columns.size(); ++i)
        if (model_.columns[i].key == key) return !table_->isColumnHidden(static_cast<int>(i));
    return false;
}
bool ObjectTable::editable(int column) const {
    return column >= 0 && column < static_cast<int>(model_.columns.size()) && bool(model_.columns[static_cast<std::size_t>(column)].edit);
}
void ObjectTable::commit(const std::string& id, int column, const QString& text) {
    if (!editable(column)) return;
    const auto row = std::find_if(shown_.begin(), shown_.end(), [&](const auto& r) { return r.id == id; });
    if (row == shown_.end() || row->cells.value(column) == text) return; // unchanged: nothing to commit
    const auto& c = model_.columns[static_cast<std::size_t>(column)];
    // A refusal (a parse error included) is shown by execute(); the cell then reads the stored value.
    if (!context_.execute(c.undo, [&](ProjectDocument& d) { c.edit(d, id, text); })) { invalidate(); refresh(); }
}
void ObjectTable::translate() {
    QStringList headers;
    for (const auto& c : model_.columns) headers << context_.text(c.header);
    columnMenu_->setTitle(context_.text("editorTableColumns"));
    const auto entries = columnMenu_->actions();
    for (int i = 0; i < entries.size(); ++i) entries[i]->setText(headers.value(i));
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
            if (editable(col)) cell->setFlags(cell->flags() | Qt::ItemIsEditable); else cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
            table_->setItem(r, col, cell);
        }
        if (row.id == keep) table_->selectRow(r);
    }
    fitTableColumns(table_);
    shown_ = rows;
    revision_ = context_.revision();
}
}
