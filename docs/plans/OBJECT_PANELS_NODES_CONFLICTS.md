# Contract: the Nodes and Conflicts tabs as panels (D523)

2026-11-12. The last two steps of D521 (object tabs as panels, part of the `EditorWindow` split, D505).
The owner chose to finish the split and asked for this contract before any code.

## Rules

- No behaviour change: object names, action keys, Undo names, dialogs, rows, enabled states,
  shortcuts and selection stay as they are; the full desktop suite passes after each step.
- A panel holds no pointer to the window. What it needs from the window is a named hook, listed
  below; anything not listed stays on the window.
- The window calls only the public methods listed for each panel.

## NodesPanel (`src/shell/nodes_panel.hpp`)

**Owns** the `editorNodeTable` table (9 columns); the actions `editorRenameNode`,
`editorNumberNodes`, `editorDeleteNode`, `editorEditCoarse`, `editorNodeAnalysis`,
`editorAnalyseNodes`, `editorJunctionTemplates`, `editorNodeJunction`, `editorMergeNodes`,
`editorGateFlows` and `editorSuggestNodes`; the rename dialog; the Study area tick (column 7);
double-click; the enabled states; `gateFlowsEditable`.

**Hooks** (`NodesHooks`):

| Hook | Window code behind it |
|---|---|
| `editCoarse(id)` | the coarse-values dialog, `editor_coarse.cpp` |
| `projectFile()` | the open project file, for `editNodeAnalysis` |
| `analyseAll()` | runs the Node analysis panel and brings the Results page forward |
| `crossSectionRows()`, `priorityDefaults()` | the catalogs the junction dialog fills from |
| `choices` | `OdChoices` (composition, period, mode) for the gate-flow dialog |
| `status(text, ms)` | the status bar |
| `highlight(id)` | the canvas's highlighted Node |
| `extend(layout)`, `refreshExtension()`, `translateExtension()` | Node proposals (`editor_proposals.cpp`), which read the Macro and Meso results and so stay on the window; placed under the table as now |

**Public:** `commitTool(polygon)` (the new id, empty when refused; the window shows the page, then
selects the row), `selected()`, `refresh()`, `translate()`, and from `ObjectTabPanel`
`selectRow(id)`, `invalidate()`, `clearSelection()`.

**Selection:** the panel follows a cleared canvas selection (`followCanvasClear`, D522);
diagnostics and proposals select a row with `selectRow(id)`.

## ConflictsPanel (`src/shell/conflicts_panel.hpp`)

**Owns** the `editorConflictTable` table; the automatic areas and their revision; the actions
`editorAddCrossing`, `editorTakeOverMerge`, `editorEditConflict`, `editorCyclePriority` (P),
`editorRestorePriority` and `editorDeleteConflict`; the edit and crossing dialogs; authoring an
automatic area; cycling a priority; selecting a row by any id that names an area, rule, control or
waiting line.

**Hooks** (`ConflictsHooks`):

| Hook | Window code behind it |
|---|---|
| `priorityDefaults()` | `data/priority-rules`, as Run reads them (also used by Fill, seams and street segments) |
| `automaticShown()` | the Conflict area tool is chosen, or this page is current |
| `selectedConnector()` | the selected Connector's id, empty when none |
| `showAutomatic(areas)` | the canvas's automatic areas |
| `highlight(id)` | the canvas's highlighted area |
| `reveal()` | shows the objects dock on this page |

The canvas selection comes from `ObjectTabContext::selection`.

**Public:** `cyclePriority(id)`, `authorConflict(key)`, `select(id)`, `selected()`,
`cycleAction()` (the window also adds it to the canvas), `invalidate()` (the rows and the
automatic areas), `refresh()`, `translate()`, and `clearSelection()`.

**Window keeps:** the run-protection note under the run status, `priorityDefaults()`,
`showConflicts()`, the canvas callbacks (pick, cycle, author, move waiting line) and the refresh
when the page becomes current.

## Order

1. This contract (D523).
2. Nodes as a panel; `editor_nodes.cpp` goes.
3. Conflicts as a panel; `editor_priority.cpp` goes.

Return to [the plans index](README.md).
