# UI_GEOMETRY — dialog geometry checked on Linux (contract)

Written 2026-11-27 (D678). The owner chose U1 and U2, both implemented (§5, D680).

## 1. The problem

Several owner findings were geometry that a test could have seen:
- F8: a cell editor sat 4 px right and 2 px down of a 99 × 23 cell, and was 24 px tall (D676);
- earlier F items: clipped labels and buttons at 150 %.

Linux tests check behaviour, plus `requireFitsScreen` in 13 dialog tests (`tests/dialog_fit_support.hpp`): the
dialog shrinks to a laptop screen and Confirm stays visible. Nothing checks that what is inside a dialog is laid out.

## 2. Checks proposed

Each check runs offscreen in English and Thai, at scale 1 and 1.5 (`QT_SCALE_FACTOR`).

| Check | Fails when |
|---|---|
| G1 cell editor | An editable table's editor geometry differs from its cell's rect (D676's row, generalised) |
| G2 clipping | A visible child's rect leaves its visible parent's rect, outside a scroll area |
| G3 overlap | Two visible siblings in one layout overlap by more than 1 px |
| G4 text fit | A label without word wrap or elision is narrower than its text's width |

## 3. Options

| Option | What | Cost and risk |
|---|---|---|
| **U1** | A helper `requireLaidOut(QWidget&)` running G1–G4, called where the 13 dialog tests already call `requireFitsScreen` | Small. It covers only dialogs that tests already open |
| **U2** | U1, plus a sweep test that triggers every dialog action by object name, runs the checks and saves PNGs for review | Medium. It needs a list of actions that open a modal, and one fixture project that enables them all |
| U3 | Pixel snapshots compared with golden images | Breaks with every font, Qt or style change. **Not recommended** |

**Not measured yet:** how many false positives G2–G4 give today. U1's first slice measures it on the 13 dialogs and
reports the count before any fix.

## 4. Recommendation

**U1, then U2.**
- U1 is cheap and gives the false-positive count.
- U2 then reaches every dialog, so the owner's Windows look is left for what a person must judge: wording, colour
  and whether a layout makes sense.

## 5. The owner's choice and the implementation (D680)

The owner chose "UI_GEOMETRY U1, U2" on 2026-11-27.

- **U1:**
  - `tests/ui_geometry_support.hpp` holds `uiGeometry::findings` (G1–G4). `requireFitsScreen` (`tests/dialog_fit_support.hpp`)
    now also calls `requireLaidOut`, so the 22 dialog checks in 12 UI suites run G2–G4.
  - G1 runs in the sweep only. Opening an editor moves a table's current cell, which a test still driving the dialog
    must not have changed. In `junction-recipe-ui`, G1 also met `EditorInteraction`'s focus rule (an editor's
    focus sets its table's current cell) in a loop; the sweep sets the current cell first, as a click does.
  - The rules exempt:
    - Qt's own parts: scroll bars, headers, tab, combo and spin internals;
    - the direct children of a scroll area's viewport, which is scrolling;
    - a label's mnemonic `&`, which is not drawn.
  - G1 makes the first editable cell of each enabled table current, opens its editor and reverts it with `closeEditor`.
  - With `MULTRA_UI_GEOMETRY_REPORT` set, findings are reported and do not fail.
- **Forcing rows UG1–UG5** (`ui-geometry`):
  - each check fires on a dialog built to break it: Qt's delegate under the style's cell padding, a child past its
    parent's edge, two widgets in one grid cell, and a label and a button narrower than their text;
  - a plain form passes.
- **U2** (`ui-dialog-sweep-1x`, `-1.5x`, `-2x`): `tests/ui_dialog_sweep_tests.cpp` opens
  `four-leg-signalised` and `macro-four-step`.
  - In English and Thai, it triggers every enabled editor action that is not a run, file, tool, toggle or delete action.
  - It checks each dialog that opens, then opens the Add/Edit dialogs that dialog's buttons open (two levels).
  - It checks each dialog twice: as opened, and again with every box ticked, so gated fields and tables are laid out.
  - `MULTRA_UI_SWEEP_PNG=<dir>` saves each dialog for review.
- **Measured** (one pass per scale):
  - U1: 111 checks in 11 suites at scales 1 and 1.5, **0 findings**.
  - U2: 104 dialog views per scale (26 dialogs × 2 projects × 2 languages) at scales 1, 1.5 and 2, **0 findings**.
  - One false-positive kind was found and removed: Qt's colour-picker labels measured with their mnemonic `&`.
- **Forcing on real code:**
  - With D676's cell-editor delegate removed, the sweep reports G1 on `editorCatalogSpeedCurve` and
    `editorCatalogAccelerationCurve` with every box ticked: "editor 4,2 91x24 on cell 0,0 99x23", the owner's F8 finding.
  - With it in place, the sweep passes.
- **Not reached:** dialogs whose action needs a selected object (Link, Connector, conflict area, zone, Node, counter,
  speed decision, OD matrix and others; about 40 actions are disabled without a selection), and dialogs opened from
  panels rather than actions. Selecting one object of each kind before the sweep is the next extension.
- **A defect found (D681):** U1's layout pass made `junction-recipe-ui`'s dialogs active before the test drove them.
  *Add lane* in the junction lane override then recursed through `EditorInteraction`'s table focus rule until the
  stack overflowed. The rule is fixed.
- Nothing here validates a look on Windows (rule 3). It removes geometry from what the owner must check.
