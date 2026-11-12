#pragma once
#include "object_columns.hpp"
#include "object_tab_context.hpp"
#include <QWidget>
#include <cstdint>
#include <string>
#include <vector>

class QAbstractItemView;
class QTableWidget;
namespace multra {
// D525 (docs/plans/OBJECT_TABLE.md): one object tab's table, built from its columns' metadata. It
// owns the headers, the rows (rebuilt when the revision moves), the id on every cell, the kept
// selection and the cell formats; the panel keeps its toolbar, actions and dialogs.
class ObjectTable final : public QWidget {
public:
    ObjectTable(QWidget* parent, ObjectTabContext, ObjectTableModel, const QString& objectName, bool severalRows = false);
    QTableWidget* table() const { return table_; }
    void translate();  // headers; the rows follow on the next refresh
    void refresh();    // rebuilds when the revision moved or the row count differs
    void invalidate() { revision_ = UINT64_MAX; }
    std::string selected() const;              // the current row's id when a row is selected
    std::vector<std::string> selectedIds() const;
    bool selectRow(const std::string& id);     // false when no row has it
    void clearSelection();
    // In-place edit (D525): whether a column's cells take an edit; a double-click elsewhere is the panel's.
    bool editable(int column) const;
private:
    void commit(const std::string& id, int column, const QString& text);
    std::vector<TableRow> shown_; // the texts the cells were built with, to tell an edit from none
    ObjectTabContext context_;
    ObjectTableModel model_;
    QTableWidget* table_{};
    std::uint64_t revision_{UINT64_MAX};
};
}
