# UI_GEOMETRY — dialog geometry checked on Linux (contract)

Written 2026-11-27 (D678). It is a contract with options; **nothing is implemented**, and the owner chooses (§4).

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
