# Contract: the metadata-driven object table (ObjectTable, D525)

2026-11-12. The owner asked for object lists as Vissim and SUMO build them: each object type
declares its columns, and one table serves every type. This contract fixes that table before any
code; it is piloted on Study periods, and the other tabs follow only after the owner has seen it.

## Owner's choices for the first version

1. The table looks as it does now (rows, headers, formats, selection).
2. Cells can be edited in place.
3. Columns can be hidden and shown.
4. Rows can be filtered.

Header sorting stays out (D490: code writes rows by position).

## Column metadata (`src/shell/object_columns.hpp`)

```cpp
enum class CellKind { id, text, number };   // id, number: tabular digits; number right-aligned
template <class T> struct ObjectColumn {
    std::string key;     // stable attribute name ("name", "start"); the column's identity
    std::string header;  // locale key of the header ("editorColumnName")
    CellKind kind;
    std::function<QString(const T&)> display;
    // Empty: read-only. Otherwise an in-place edit: one History step named `undo`; it may throw
    // (a parse or validation refusal), and the cell then shows the stored value again.
    std::function<void(ProjectDocument&, const std::string& id, const QString&)> edit;
    std::string undo;
};
template <class T> struct ObjectTableSpec {
    std::function<const std::vector<T>&(const ProjectDocument&)> objects;
    std::function<std::string(const T&)> id;
    std::vector<ObjectColumn<T>> columns;
};
```

## ObjectTable (`src/shell/object_table.hpp`)

A widget that holds the tab's existing `QTableWidget`, so object names and every test that finds
it are unchanged. Typed rows reach it as `{id, cells}` through a template adapter.

**Owns:** headers and their translation; rebuilding the rows only when the revision moves or the
row count differs (the `ObjectTab` rule); the id in `Qt::UserRole` on every cell; keeping the
selected id across a rebuild; formatting by `CellKind`; `fitTableColumns`; `selected()`,
`selectedIds()`, `selectRow(id)`, `clearSelection()`, as `ObjectTabPanel`'s.

**Stays with the panel:** its toolbar, actions and their enabled states; its dialogs; a
double-click on a read-only cell, which opens the dialog as now.

**In-place edit:** only a cell whose column has `edit` is editable (double-click, F2 or typing).
Committing runs the window's `execute(undo, ...)`, one Undo step. A refusal shows its error as the
window does and the cell shows the stored value again. An unchanged text commits nothing.

**Hide and show columns:** right-click a header for a checkable list of the columns (titled by
`editorTableColumns`). The id column cannot be hidden. The choice lasts for the session, per
table. Copy already leaves hidden columns out (`tableSelectionText`).

**Row filter:** a field above the table, named after it with `Filter` appended, placeholder
`editorTableFilter`. It hides the rows where no visible cell contains the text, ignoring case,
and never reorders rows. A hidden row loses its selection. `selectRow(id)` from outside (canvas,
diagnostics) clears the filter first, so a selected row is always visible.

## Out of scope

Sorting; editing several rows at once; column choices saved in the project; the other twelve
tabs (one per commit, after the owner has seen the pilot); tabs that are not one object per row
(Conflict groups, the Nodes Study-area tick, Zones' connector table), which wait for a revision
of this contract.

## Order

1. This contract (D525).
2. ObjectTable with Study periods on it, no visible change.
3. In-place edit (Study periods: name, start, end; times as `HH:MM:SS`, hours may pass 24).
4. Hide and show columns.
5. Row filter.

Each step is one commit with its tests (`tests/object_table_ui_tests.cpp`); the owner then tries
the pilot on Windows (OWNER_SITTING) and decides whether the other tabs follow.

Return to [the plans index](README.md).
