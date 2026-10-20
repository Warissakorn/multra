# TrafficSim UI presentation audit — 2026-10-15

Dated phase 1 inventory and proposals. The owner subsequently accepted the recommended
direction (“ทำตามแนะนำ”); slice 0 removes the hazards in §2 without moving pages. Read with the
[proposed specification](../plans/UI_PRESENTATION_SPEC.md),
[slice plan](../plans/UI_PRESENTATION_SLICES.md) and [owner choices B16–B18](../plans/OWNER_SITTING.md#b--decisions-in-chat-one-line-each).
Baseline source: the checkout's HEAD recorded in §8. No application code, translations, tokens,
shortcuts, gestures, models, commands, schemas or numerical outputs change in this phase.

## 1. Actual information architecture

The constructor in `src/shell/editor_window.cpp` builds the object pages in this order.
`editorObjectTabs`, inside `editorObjectsDock`, has **14** pages:

| Index | English title / translation key | Page contents | Construction / translation |
|---|---|---|---|
| 0 | Links / editorLinkTable | editorLinkTable | editor_tables.cpp |
| 1 | Connectors / editorConnectorTable | editorConnectorTable | editor_tables.cpp |
| 2 | Signal heads / editorSignalTable | editorSignalTable; toolbar outside tabs | editor_tables.cpp; editor_demand.cpp |
| 3 | Problems / editorProblemTable | editorProblemTable | editor_diagnostics.cpp; editor_tables.cpp |
| 4 | Routes / editorRouteTable | editorRouteTable; Add/Edit/Delete | editor_demand.cpp |
| 5 | Vehicle inputs / editorInputTable | editorInputTable; preview/catalog/library actions | editor_demand.cpp |
| 6 | Signal control / editorProgramTable | editorProgramTable; controllers and legacy programs | editor_demand.cpp |
| 7 | Routing decisions / editorDecisionTable | editorDecisionTable | editor_demand.cpp |
| 8 | Results / editorResultsTable | editorResultsTabs and shared results toolbar | editor_results.cpp |
| 9 | Conflict areas / editorConflictTable | editorConflictTable and priority actions | editor_priority.cpp |
| 10 | Queue counters / editorCounterTable | editorCounterTable | editor_counters.cpp |
| 11 | Travel-time sections / editorSectionTable | editorSectionTable | editor_sections.cpp |
| 12 | Zones / editorZoneTable | editorZoneTable + editorZoneConnectorTable, horizontal 3:2 | editor_zones.cpp |
| 13 | OD matrices / editorOdTable | editorOdTable | editor_od.cpp |

All filenames in the last column are under `src/shell/`. Tabs use document mode, scroll
buttons and right elision (`buildWorkspace`). This mitigates overflow but does not group work.
The base table pages and later body pages have different ownership shapes; do not infer a
page pointer by taking a table's parent without checking its actual container.

Results has **4** inner pages (`editor_results.cpp`, `translateResults`):

| Index | English title / key | Tables |
|---|---|---|
| 0 | Movements and queues / editorResultsTabMovements | movement + queue side by side; batch or comparison replaces these |
| 1 | Queue discharge / editorResultsTabDischarge | editorDischargeTable |
| 2 | Safety clamps / editorResultsTabClamps | editorClampTable |
| 3 | Parking lots / editorResultsTabLots | editorLotTable |
| 4 | Nodes / editorResultsTabNodes | editorNodeResultTable: Node, leg and turn rows of the run or batch (S0.6, D278) |
| 5 | Macro links / editorResultsTabMacro | editorMacroTable (S2.4, D277) |

Run seeds and Compare with… are toolbar actions, not pages. Their tables live inside page 0;
Cancel is shared. Export and Copy retain completion guards and use existing export formatters.

**Menus:** seven, built in `editor_workspace.cpp`: File, Edit, View, Map, Simulation, Help,
Language. Four direct main-window toolbars: editorFiles, editorTools, editorRunToolbar,
editorWorkspace. Nested table toolbars are separate. The main bars use icons except Run's
text+icon; narrow windows insert a break before Run at width <1180. Focus hides docks, not the
central scope/run labels or toolbars. Reset restores palette left (180), Properties right (310),
Objects bottom (185); History is tabified with Properties and hidden. These are requested dock
sizes, not measured final sizes.

**Palette:** 13 stable modes, currently in four groups (`editor_palette.cpp`):

| Group | Modes and shortcuts |
|---|---|
| Geometry | 0 Select S; 1 Link L; 5 Connector C; 2 Split X |
| Controls | 6 Routes R; 7 Vehicle inputs V; 8 Signal heads H; 9 Conflict areas A; 10 Queue counters Q; 11 Travel-time sections T |
| Demand | 12 Zone Z |
| Tools | 3 Measure M; 4 Calibrate K |

Routing and measurement are currently mixed into Controls. The hidden editorTool combo remains
an index-based mode protocol; changing that protocol is outside this proposal.

## 2. Navigation hazards and layout recommendation

Fixed indices appear in translations, show-in-table navigation, error jumps, active-conflict
checks and Signal-head toolbar visibility. Beyond kConflictTab=9, kCounterTab=10,
kSectionTab=11, kZoneTab=12 and kOdTab=13, inspect:

- editor_tables.cpp: translation loop 0–3.
- editor_demand.cpp: translation 4–7, selection navigation ternary, toolbar index==2.
- editor_diagnostics.cpp, editor_window.cpp, editor_run.cpp, editor_batch.cpp and
  editor_compare.cpp: Problems index 3.
- editor_results.cpp: Results index 8; editor_batch.cpp already locates resultsPage_ by pointer,
  but still selects inner page 0 numerically.
- Tests: table_ui_tests, priority_ui_tests, priority_canvas_tests, queue_counter_ui_tests,
  section_ui_tests, od_lot_ui_tests and results_tabs_ui_tests. Some already locate the outer
  Results page by pointer; inner Results indices still need conversion.

**Proposal A, recommended and subsequently accepted:** retain the existing Objects dock and editorObjectTabs instance;
add a task selector that exposes the matching existing pages: Network, Control, Demand,
Measurement, Results, Problems. Preserve page widgets, action keys and object names. Selecting
an object through canvas/context menu/Problems must reveal its group first, then its page.
A compact group combo plus the existing page strip at 1024 avoids a second vertical sidebar.
This is navigation state only; it must not edit document, selection or run state. It still needs
owner selection and tests before implementation. The selector must not become a second source
of page identity; derive its mapping from page pointers.

**Proposal B:** separate docks per task, tabified into the current bottom area by default.
This gives detachable task panels but adds dock identities, reset/focus/restore paths and window
minimum-size risks. Never open six bottom panels at once. Retaining existing objectName
contracts makes A the smaller migration. Neither proposal is claimed to pass before execution.

Recommended palette: Geometry {0,1,5,2}, Control {8,9}, Demand {6,7,12}, Measurement {10,11},
Tools {3,4}. The change from four to five groups deliberately needs a workspace-ui expectation
update. Icon-only mode is proposed as a transient presentation toggle with accessible tool names,
not remembered fold state or a saved workspace preset.

## 3. Buttons and typography

D81/D83/D84 already implement the 24 px control, 32 px toolbar, 24 px table-row tokens and
constant invalid/focus box model in editor_style.cpp. Treat this as an existing invariant,
not a new achievement. Nested toolbars and Behaviour Lab need explicit measurements rather
than assuming the direct-child toolbar pass reaches them. Delete currently follows Snap/grid
in editorTools, and Add/Edit/Delete are adjacent in several table bars: add separators without
changing confirmations or action enabled conditions.

The bundled Noto Sans Thai 400/600 is registered in editor_appearance.cpp. Body 13, numeric 12,
label 11 and title 18 are used; fontSizeSection=14 exists but the current QSS substitution list
does not use it. Apply the proposed role map to Results, dialogs and Lab without a second face
or changed token values. English label tracking is deliberate; Thai remains untracked.

`formatValue` and numericFont already exist. C-locale QString::number remains in
editor_tables.cpp (metres), editor_results.cpp (figures), editor_compare.cpp and
behaviour_lab_telemetry.cpp. `EditorWindow` explicitly sets QLocale::c() (D83): language switching
must not silently switch numerical locale. Normalize UI formatting through existing helpers
while retaining C-locale Latin digits, decimals, missing-value distinctions and exported bytes.
Some cells tagged as numeric use setNumericText (font only); check alignment separately.
Long-name cells generally lack explicit full-text tooltips in their fill helpers. The D151
application tooltip filter bounds existing tooltips; it does not supply missing cell content.

## 4. Icon audit

`EditorIcon` declares **30** entries. `editorIcon()` paints each at 16 logical px from a 20-unit
grid, 1.5 px stroke and 1×/2×/3× rasters. Exact duplicates in source:

- connector and route share the same curved arrow path.
- rotate and reset share the same circular arrow path.

Near pairs to review at 16 px in monochrome: fit/focus (only the central plus differs),
link/split (horizontal cross-cut), counter/section (small crossbars). Measure/image are currently
distinct; Calibrate deliberately uses image. Most table/menu objects have no icon assignment,
whereas the palette assigns all tool icons. Reuse one object glyph across existing surfaces;
keep text labels for controllers, OD and other objects until a deliberate glyph is approved.
See the complete shape catalogue in the proposal. Distinguish source-identical evidence from
perceptual judgement: a 16 px monochrome contact-sheet review belongs in slice a, not a claim
that all current icons were visually recognized. Slice a ([D192](../decisions/RECORD.md#d192))
separated both duplicate pairs and Fit/Focus; this section describes the 2026-10-15 baseline.

## 5. Viewport, paint order and state feedback

Actual order is not the supplied spec's global Z=0–100 stack. `canvas.cpp` draws grid/map
in the background; raster image is -200000; zones -150 (label -149); each road level uses
L=level*100. Links are L+1, markings +2, feedback +2.5; Connector surfaces +4, markings +4.5;
conflict bands approximately +5.5–7; signal Edit bars +10/+10.5; vehicles +11; counters +11.5;
sections +11.6; Run signal casing +11.9 and bar +12. Demand tint is 200004, marker/labels
200005–7; drafts/rotation/copy approximately 200007–10; lane grips 200011, point grips 200012.
`canvas_map.cpp` places status at viewport top-left and OSM attribution bottom-left as widgets.

LOD already exists for the grid (minor >=8 screen px, major 10×), and vehicle decoration:
body floors 4×2.5 px; windshield/cab at approximately 14 px length. Manual Ctrl+A wireframe
changes both drawing and picking (D100); it is not an automatic Full/Simplified mode system.
No current scale-bar/zoom/north HUD was found in canvas paint/overlay code. Do not invent GPU,
large-network culling or whole-road LOD evidence from the supplied spec.

Selection uses an outline, fill and direction arrowheads; primary outline 2 px vs hover 1.5,
with different alpha. Drafts often use dashes. Signal Run bars already have a Base casing;
vehicles normally have no pen, so aerial backgrounds can reduce edge contrast. Add neutral
casings/shape distinctions in a later presentation slice and use a local image/fake tiles for
verification. Signal phases currently differ by semantic colour on the same shape: propose
an additional R/A/G shape/pattern cue with no new timing semantics. Never change hit paths,
road crossbars, fixed gestures, snapshots or vehicle poses to improve appearance.

A proposed HUD must reserve the existing map attribution and status corners. Show scale in
metres and px/m zoom; show north only when the existing georeference supports the conversion,
including the Lab's heading-up rotation. Read transforms via viewScale()/inverse transform,
not abs(m11). No new persistent geometry cache; D28 applies before any cache proposal.

## 6. Properties and table consistency

ID/Name and selection summary are above property tabs. Name commits on editingFinished;
Network and appearance folds in memory and includes drivingSide (immediate) and
level/display Apply (explicit). Behaviour assignment precedes the three scrollable property
pages: Links, Connectors, Background. `WrapLongRows` and ReflowLabel are already present.
Keep these commit boundaries; a visual group must not turn Apply fields into immediate edits.

Object tables have read-only rows, varied single/extended selection policies and different
header resize strategies. buildWorkspace sets alternating rows/no grid and row=24 on then-existing
tables, not every dialog created later. Standardize presentation explicitly for late dialogs
and Lab, retain selection policy and editing actions. Units belong in translated headers;
null/empty/zero remain distinct. Zones and Zone connectors stay side by side (D161).

## 7. Prompt and historical-document differences

The new brief's traps match current source. Additional findings:

- There are seven menus, including Map; EDITOR_WORKFLOW's list omits Map.
- Results' first title is “Movements and queues”; Parking lots is the fourth page, beyond D144's
  original three-page record and NETWORK_EDITOR's older summary.
- The prompt cites results_tabs_ui_tests as index-dependent: its outer Results navigation already
  uses parentWidget identity, while inner pages use indices. Audit both levels in slice 0.
- VISSIM_PARITY §2b has dated missing-feature statements for positioned routing and parking;
  current code has these. Its queue/conflict click descriptions also predate D84's Ctrl+right
  rule. Use current EDITOR_WORKFLOW and code for gestures.
- Supplied spec §13 multi-selection, auto-commit and per-object colour editing are proposals;
  current Properties edits the primary object and road/vehicle colours remain data.
- Supplied spec §4 MSAA/GPU/render modes and global layer numbers do not describe this
  QPainter implementation. Grid/vehicle LOD exists, but full automatic rendering modes do not.

No decision record is added: all new layouts, grouping and glyph changes are proposals.
B18 records the six non-presentation ideas separately with reasons and likely files.

## 8. Baseline evidence

Baseline HEAD: `3e74c1c4c45ba7661ea06c75e7bd943c718d6948`. Linux Debian 13, GCC 14.2.0,
CMake 3.31.6, Ninja 1.12.1, Qt 6.8.2, Debug desktop, four build/test workers. No Windows
verification. Existing docs/images/workspace-*.png are historical examples, not this run.

| Current-run capture | Before slice 0 | After slice 0 |
|---|---|---|
| English 1360×860 | [before](../images/ui-presentation-before/workspace-en.png) | [after](../images/ui-presentation-after/workspace-en.png) |
| Thai 1024×768 | [before](../images/ui-presentation-before/workspace-th.png) | [after](../images/ui-presentation-after/workspace-th.png) |
| Thai focus 1024×768 | [before](../images/ui-presentation-before/workspace-focus.png) | [after](../images/ui-presentation-after/workspace-focus.png) |

All three before/after PNG pairs are byte-identical. Captured using the existing workspace
harness after 100 Steps. Reported viewports: English 811×390; Thai 475×248. The test's earlier
1024×768 checks in both languages pass >=400×250 **before** those Steps; the later Thai Run
summary wraps and leaves 248 px in the capture. This is an existing layout limitation, retained
by slice 0, not a claim that every Run state meets 250 px. Slice c must extend coverage to this
state and recover the height without shrinking text or hiding validation.

Source inspection and the Thai capture show the crowded tab strip, with several labels reduced
to fragments; palette content also scrolls. The inspection supports task grouping but does not
validate the proposed new layout. Scope marker, Run summary and control protection remain
visible; no live aerial/OSM legibility or north-accuracy claim is made.

Initial current-image setup problems were diagnosed without weakening assertions: system
Noto Sans Thai displaced the application's tabular Latin glyphs, and Qt test-mode recovery
storage was unwritable. A local fontconfig selecting bundled fonts and a test-only storage
symlink into /workspace restored the intended environment. The initial pre-correction suite
was 145/148 (workspace-ui, design-system-ui and m1-workflow failed); this is superseded by the
final run recorded below, not described as a passing baseline suite.

Failure-first slice 0 regression: prove outer/inner page order actually changed, then check
all 18 labels against the locale catalogs. Original code fails “Reordered object page translated
by position”; fixed code also passes tool/page selection, Signal-head toolbar visibility,
Check/failed Run/failed batch jumps and batch Results selection under the changed order.
The stress window is separate from the untouched screenshot/layout window.

CLI seed 42 still produces 45 completed, 1 active, 0 pending, one safety clamp and mean delay
31.75870154816726 s. This is reproducibility evidence, not scientific validation.
Final `cmake --preset desktop` and full desktop build pass. `ctest --preset desktop --parallel 4`
passes **148/148**, no skipped tests (93.12 s on this instance), including workspace-ui,
design-system-ui, design-system-ui-1.5x and design-system-ui-2x. Required target `check` also passes: architecture, file sizes, documentation and
**148/148** CTest entries. Logs and tested setup helpers are outside the checkout under
/workspace/.onboarding; install_script and start_skill were saved successfully to the cloud
environment draft. This confirms current-instance setup and draft persistence only; publication
and restoration in a fresh task remain user/platform operations.
