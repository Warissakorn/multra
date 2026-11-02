# UI audit remediation — 2026-11-02 (D432)

The owner requested a detailed UI review, then a sizing/proportion review, then correction of
all findings. This note maps those 34 findings to the implementation and reproducible checks.
The review used `c0ca9d16` as its baseline. Measurements are logical pixels on Linux,
GCC 14.2.0 / Qt 6.8.2, offscreen, with bundled Noto Sans Thai. Windows appearance and owner
acceptance remain separate; no simulation-validation or milestone gate closes here.

## Finding coverage

`data` and `layout` below mean the new `ui-audit-data` and `ui-audit-layout` CTest suites.
Existing suites still cover the broader editing, docking, playback and rendering contracts.

| # | Reproduced problem | Correction and check |
|---|---|---|
| 1 | Results CSV could replace the open project/recovery file | Common export destination guard, including automation CSV/screenshot. `data`: both paths refused, original bytes intact, ordinary CSV succeeds. |
| 2 | Equal-revision, equal-row-count Open reused previous list cells | Document-generation invalidation of all revision-keyed list/diagnostics views. `data`: different Zones, periods and speed decisions at the same revision. |
| 3 | Confirming 25:00–26:00 rewrote it to 01:00–02:00 | Explicit day selectors and exact unchanged clock seconds. `data`: day-two no-op preserves document/revision. |
| 4 | Remove could follow the earlier row while its combo was clicked | Embedded editor mouse/focus events establish the current row. `data`: a visible three-row lane form removes the clicked first row, retains the other two in order. The original hidden-host probe was insufficient; the regression asserts visibility first. |
| 5 | Untouched input 123.456789 became 123.457 | Preserve original numeric values until edited. `data`: original precision and History survive no-op and invalid/retry. |
| 6 | Head at 99.9997 m displayed 100.000 and refused confirmation | Preserve original station; roundoff-close typed endpoint clamps to actual length. `data`: unchanged endpoint confirms without an edit. |
| 7 | Validation after modal acceptance lost staged input | A validating dialog accepts only after successful atomic History commit; errors remain beside the staged values. Applied to Input, Decision, speed decision, signal head/program/controller, run settings and StudyPeriod. `data`: invalid interval then retry. |
| 8 | Missing EDIT_NODE_INVALID produced a blank refusal | Added English/Thai diagnostic text; existing junction/Node validation keeps its named refusal. |
| 9 | Input selection retained the previously selected route's tint | Clear the competing table selection before highlight synchronization. `data`: route B then input for route A. |
| 10 | Rejected Problems survived Open of a valid document | Clear rejected/readiness/diagnostic state at document replacement. `data`: failed widths Apply then reopen. |
| 11 | Many vehicle types made speed form 1664 px tall | Scrolling body, bounded dialog, fixed footer. `data`: installed types plus 45 extra types, screen-fit/footer assertions. |
| 12 | Expanding common Properties forced height above 768 px | Scrollable common section above scrolling property tabs. `layout`: expanded Appearance at 1024×768 and 800×600. |
| 13 | Fit clipped label boxes | Fit world geometry with pixel space reserved for labels; all live candidates stay inside viewport. `layout`: all 12 shipped rates after Fit; existing transformed label suite retained. |
| 14 | HUD covered labels | Reserve visible HUD/map attribution/status rectangles during placement; re-place on pan/resize. `layout`: no visible label/HUD intersection. Isolated PNG scene export retains its own placement without live-viewport culling. |
| 15 | Thai names hit a byte limit labelled 200 characters | Count Unicode scalars in the Qt-free rename command and allow surrogate pairs in the field. `data`: 100 Thai and 200 non-BMP characters accepted. |
| 16 | Language switch discarded pending Properties values | Snapshot/restore editable text, numbers and choices around translation refresh; revalidate restored values. `data`: pending lane widths/count survive. |
| 17 | Selection refresh reset manually resized columns | Fit object columns once, then preserve widths. `data`: 290 px Name survives another selection. |
| 18 | 4,,4 was marked invalid but Apply accepted it | Hint and commit both retain empty tokens. `data`: rejected Apply leaves document unchanged. |
| 19 | Global scope marker said the app had no LOS | Marker distinguishes diagnostic simulated delay/LOS from HCM control delay, retains not-yet-validated wording; scientific result semantics unchanged. |
| 20 | Icons-only grew Properties instead of the canvas | Resize palette/canvas/Properties together, preserve Properties width. `layout`: released palette width reclaimed by canvas (within 3 px), Properties stable. |
| 21 | Icons-only retained 120 px of blank category rows | Thin 4 px separators replace text-sized root rows. `layout`: five separators total at most 25 px. |
| 22 | Default Drawing canvas consumed only 19–44% of window | Opening Drawing layout hides Objects; Tables/Results presets allocate space for the current task. `layout`: Drawing size floors and task visibility without data/run changes. |
| 23 | Default Results showed one complete movement row | Results preset gives the dock area to tables; Drawing no longer reserves a mostly empty bottom dock. `layout`: at least six full movement rows at 1366/1024. |
| 24 | Queue names had 47 px beside 339 px of numeric columns | Stack movement/queue tables when Results content is below 1250 px. `layout`: direction and approach-name width at least 140 px. |
| 25 | Period columns were 100 px with 166 px editors | Size columns and rows for embedded numeric editor minima, including newly added rows. `layout`: no editor exceeds its column. |
| 26 | A legal 200-W name made a 2430 px column | Cap automatic initial object columns at 320 px; user resizing remains available. `data`: legal long-name fixture. |
| 27 | Seed 4294967295 clipped its first digit | 104 px main/Lab seed fields. `layout`: full ten-digit text fits content rectangle. |
| 28 | Results mixed every Macro/Meso/batch command on one crowded bar | Per-tab command bars; common Export/Copy only. `layout`: correct visible toolbar on Macro, Meso and Micro tabs. |
| 29 | Named palette cut long Split/Calibrate/Section captions | Short translated captions, separate shortcut column, full tooltip/accessibility text. `workspace-ui`: mode/chord/name and keyboard traversal. |
| 30 | Normal 800×600 grew to 800×758 | Common Properties scrolling and inactive-tab minimum isolation remove the height constraint. `layout`: exact requested window size folded/expanded. |
| 31 | Lab panel occupied 41–45% of 1024 px | Initial 320 px dock and bounded numeric fields. `layout`: dock at most 360 px. |
| 32 | Lab group field columns did not align | Consistently stacked, wrapped labels and left-aligned fields across groups. `layout`: visible numeric field x positions agree within 2 px. |
| 33 | Only one small font/row density | Session Compact/Comfortable/Large menu, independent of world zoom/model values. `layout`: row sizes and unchanged document/revision/run. |
| 34 | All click targets stayed at 24 px | Density control/row targets 24/28/32 px. `layout`: actual playback button hit areas at all three densities. |

## Representative measurements and images

At 1366×768, Drawing has a 936×575 viewport in both locales (51.3% of the window, versus the
review's Thai 847×334 / 27%). Toggling names → icons releases and gives the canvas 123 px in
English / 119 px in Thai, with zero Properties width growth. At 1024, movement viewports are
992×208 English / 992×220 Thai (8 / 9 complete rows), and queue-name columns are 726 / 667 px.
The review showed about one complete row and a 47 px name column at that size.

Linux offscreen, Thai, 1024×768: [Drawing](../images/ui-audit-after/drawing-th-1024.png),
[Results](../images/ui-audit-after/results-th-1024.png), [Lab](../images/ui-audit-after/lab-th-1024.png).
Drawing is captured after the appearance fold closes and its layout settles. The validation
marker remains present; these are layout evidence, not a Windows or simulation-fidelity claim.

## Reproduction

Configure/build the desktop preset using the documented Qt toolchain, then run:

```sh
ctest --preset desktop -R 'ui-audit-|workspace-ui|object-groups-ui|properties-layout-ui|canvas-labels-ui|design-system-ui' --output-on-failure
build/desktop/bin/multra-ui-audit-layout-tests data /path/to/existing/screenshots
```

The optional second argument writes English/Thai Drawing (1024 and 800), Results (1366 and
1024), density and Lab screenshots. The layout suite prints table viewport/name widths.
The data suite creates its fixtures in temporary storage, including protected export paths.
All EditorWindow suites call `useTestStorage()` after QApplication.

Desktop build and `check` passed all 271 CTest entries (381.90 s, parallel 5), including
architecture, documentation and file-size guards. Both locales and layout at 100/150/200% are
covered offscreen. Existing CLI/core/analysis output-parity, pointer/docking, cached/live rendering
and PNG export suites pass. [PROGRESS](../PROGRESS.md) records the session and platform limits.
