#pragma once
#include <QString>
#include <vector>
class QAbstractItemView;
class QTableView;
namespace multra {
// The spreadsheet behaviour every table view gets with the table recipe (D490): a numbered row
// header, Ctrl+C / Ctrl+A that a window shortcut cannot take from a focused table, and
// cell-by-cell selection in a table that allows several selected cells (a single-selection table
// keeps selecting its row). Idempotent; applyTableRecipe calls it.
void makeSpreadsheetTable(QTableView* table);
// The rows holding at least one selected cell, ascending and without repeats. Use this, not
// selectionModel()->selectedRows(), which returns only rows with every cell selected.
std::vector<int> selectedTableRows(const QAbstractItemView* view);
// The selection as Excel reads it from the clipboard: tab-separated, one line per row, the
// column headers first; the bounding block of the selected cells with unselected cells empty,
// hidden rows and columns left out, and row labels first when the table names its rows.
QString tableSelectionText(const QTableView* table);
}
