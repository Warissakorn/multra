# UI presentation delivery slices — 2026-10-15

Phase 1 output, with the recommended direction accepted by the owner on 2026-10-15
(“ทำตามแนะนำ”). Follow
[B16–B18](OWNER_SITTING.md#b--decisions-in-chat-one-line-each) against the
[audit](../audits/UI_PRESENTATION_AUDIT.md) and [proposed spec](UI_PRESENTATION_SPEC.md).
One implementation session per slice. Slice 0 follows the owner reply; later visual slices
remain separate sessions. No D-row is created for this proposal or behaviour-preserving slice 0.

## Common acceptance and evidence

Every implementation slice starts with failure-first tests for its changed behaviour, then
changes presentation code only. A forced-failure test first proves its forcing setup worked.
Keep a before/after source diff: src/core, src/model, src/commands, codecs, dependency manifests,
lockfiles, file schemas, numerical outputs, action keys, objectName, gestures and shortcuts
remain unchanged. Translation additions go in both data/locales/en.json and th.json.
No token changes without a new owner decision and matching test change in the same commit.
No new hex colours, shadows, animations, second fonts or speculative caches.

From the repository root, after activating the prepared toolchain:

```bash
cmake --preset desktop
cmake --build --preset desktop --parallel 4
QT_QPA_PLATFORM=offscreen ctest --preset desktop --parallel 4
QT_QPA_PLATFORM=offscreen CTEST_PARALLEL_LEVEL=4 cmake --build build/desktop --target check --parallel 4
```

Required gates: full desktop suite (including workspace-ui, design-system-ui,
design-system-ui-1.5x and design-system-ui-2x) and check. Retain exact passed/failed counts and
logs for that run. Check includes CTest, so a repeated suite is intentional only when executing
the required target or validating later edits. Do not substitute a zero-test selection.
Linux offscreen is the only local GUI claim; Windows/high-DPI appearance stays owner review.
No benchmark or Release/sanitizer matrix is required for this presentation proposal; measure
paint cost if a canvas change could affect it, without asserting an unmeasured speed-up.

Current workspace screenshot harness, with an output prefix whose parent exists:

```bash
QT_QPA_PLATFORM=offscreen ./build/desktop/bin/trafficsim-workspace-tests \
  /workspace/trafficsim/data /tmp/trafficsim-workspace-after
```

It emits `-en.png` at 1360×860, `-th.png` at 1024×768 and `-focus.png` in Thai. It runs 100
steps first; these are run-state layout screenshots, not an empty editor mockup. Validate its
exit code and inspect actual output images. Every later slice records images, decision for
visible changes, PROGRESS, specific NEXT and a visual checklist in OWNER_SITTING §E. It cannot
mark that checklist passed itself. File budget remains approximately 500 lines.

## Slice 0 — stable page identity (must precede any tab movement)

**Behaviour:** no visible change, no tab reordering. Retain the current 14 outer and four inner
page orders. Replace positional identity with pointers (preferred) or existing object names.
Store actual body/page pointers at construction; do not assume table->parentWidget() always
means the page. Resolve names with indexOf(page) only at QTabWidget APIs that require an index.
Tool mode values 0–12 and their shortcut map are explicitly exempt from this refactor.

**Files:** src/shell/editor_window.hpp, editor_tables.cpp, editor_diagnostics.cpp,
editor_demand.cpp, editor_priority.cpp, editor_counters.cpp, editor_sections.cpp,
editor_zones.cpp, editor_od.cpp, editor_results.cpp, editor_run.cpp, editor_batch.cpp,
editor_compare.cpp and editor_window.cpp. Audit all currentChanged handlers and visibility
comparisons as well as setCurrentIndex and setTabText. Preserve page ownership/lifetime.

**Tests first:** update navigation helpers in tests/table_ui_tests.cpp, priority_ui_tests.cpp,
priority_canvas_tests.cpp, queue_counter_ui_tests.cpp, section_ui_tests.cpp,
od_lot_ui_tests.cpp and results_tabs_ui_tests.cpp to resolve the intended page through its
widget. Add regression coverage for translations and Show in table on every object category,
Problems jumps after failed run/batch/compare, Signal-head-only toolbar visibility and Results'
four inner pages. Any test that temporarily moves a tab must first assert it moved, then
assert navigation and translated labels still address the right widget, then restore it.
This is test-local mutation, not changed application ordering.

**Accept:** all names/labels/order/screenshots unchanged; no positional page identity left in
production or relevant tests; current layout and all desktop/check tests pass. Numeric loops
for row counts and the stable tool-mode protocol remain valid. Audit remaining numeric tab
calls by meaning rather than replacing them indiscriminately.
**Risk:** active-conflict logic or Problems paths silently opening the wrong page; stale page
pointers after ownership changes; a test navigating by its new order masking the same defect.
Do not start slice c until this slice is green and committed.

## Slice a — icon vocabulary

**Files:** src/shell/editor_style.cpp/.hpp; icon assignment in editor_palette.cpp and
editor_workspace.cpp; existing object toolbar/table construction where a glyph is appropriate;
tests/design_system_ui_tests.cpp. Add a small presentation helper only if needed to keep files
within budget; no new dependency or persistent cache.
**Tests first:** distinct Connector/Route and Rotate/Reset rasters at 16 px; correct 1×/2×/3×
sizes and palette roles; repeated object glyph reused across existing surfaces. Keep the test
honest: differing pixels do not prove users recognize the shapes.
**Accept:** a monochrome contact sheet at actual 16 px, owner visual row in §E, existing icon
metrics unchanged and common gates pass. Fit/Focus and Counter/Section distinguishable by
shape. No gratuitous icons for data types lacking an agreed glyph.
**Risk:** crowded strokes at 16 px, icon-only actions losing their accessible name, static icon
cache invalidation if future themes are added (theme work deferred).

## Slice b — palette and Simulation toolbar

Done as [D193](../decisions/RECORD.md#d193) on 2026-10-16: five groups, the transient icons-only
toggle, the Simulation bar order and Delete behind a separator. A b1-only variant (no toggle) was
written in parallel and replaced by this one when both merged.

**Depends:** owner B17, slice a; slice 0 recommended even though no object page moves here.
**Files:** editor_palette.cpp, editor_workspace.cpp, editor_window.cpp, editor_run.cpp,
editor_style.cpp; both locales; tests/workspace_ui_tests.cpp, design_system_ui_tests.cpp and
editor_workflow_ui_tests.cpp. Keep new helper files separately indexed if documentation grows.
**Tests first:** all 13 modes retain their old number/chord in both palette views; five proposed
groups contain the intended modes; keyboard traversal, tooltip names/chords and active/focus
states work in icon-only mode. Update the current four-group expectation deliberately.
**Accept:** fixed control heights in every state, destructive separators, Run/Pause/Step/Reset
visible in focus, no overflow at 1024 in either language; wide single-row and narrow two-row
behaviour remains. No new remembered preference or independent Edit/Run state variable.
**Risk:** extra palette row minimum height and toolbar text squeezing the canvas/language control.

## Slice c — grouped Objects navigation

Done as [D194](../decisions/RECORD.md#d194) on 2026-10-16: layout A with the group combo as the
page strip's corner widget, so it adds no dock height; the group follows the current page.

**Depends:** green slice 0 and owner's B16 choice. Implement A unless the owner selects B;
B first needs a concrete objectName/saveState compatibility design.
**Files for A:** editor_workspace.cpp, editor_tables.cpp, editor_window.hpp, page show helpers
in editor_demand/diagnostics/priority/counters/sections/zones/od/results.cpp; both locales;
tests/workspace_ui_tests.cpp, table_ui_tests.cpp, results_tabs_ui_tests.cpp and related page
suites. Register pages once by pointer and translation key rather than duplicating mappings.
**Tests first:** every task/page reachable in English/Thai; locale switch while each page is
active; Show in table reveals the owning task; Problems from failed checks/run; Results batch,
comparison and Cancel still replace page 0 only. Test all 14 pages rather than just default
Links. D161 horizontal Zones/connectors stays intact.
**Accept:** 1024×768 both languages with viewport >=400×250 for every relevant group;
1360×860 baseline viewport >=700×350 retained where the current workspace test requires it;
focus restores hidden/moved/floating panels and current group; Reset returns default layout;
layout operations preserve revision, selection and run tick. New group labels in both locales.
**Risk:** hidden-page minimum size, stale currentIndex assumptions, refreshResults visibility
side effects, a new selector row increasing bottom dock height. Prefer an overflow control
to shrinking text. No persistent presets or remembered expanded sections.

## Slice d1 — Properties presentation

Done as [D195](../decisions/RECORD.md#d195) on 2026-10-16: one 120 px label column per group,
all-rows wrap below two columns, the Apply divider and eyebrow headings. The dialog forms and the
Lab were not touched.

**Files:** editor_inspector.cpp, editor_connectors.cpp, editor_behaviour.cpp,
editor_interaction.cpp, editor_style.cpp and actual dialog form builders identified by the
changed group; both locales; workspace-ui/design-system-ui/editor-workflow-ui and affected
inspector/connector suites. Verify filenames against current checkout before editing.
**Tests first:** common label alignment at wide width and reflow at narrow width; ID/Name
remain pinned; immediate vs Apply commit boundaries unchanged; Thai tone marks and field
baselines at 1/1.5/2×; invalid range feedback and existing wheel protection retained.
**Accept:** role map in Results/dialog/Lab adopted only for touched headings, no new values;
long labels wrap without forcing window growth; primary-object editing semantics unchanged.
**Risk:** headings consume vertical space, QFormLayout minimum hints, applying a presentation
helper accidentally changing editingFinished/valueChanged behaviour.

## Slice d2 — table and numeric presentation

Done as [D196](../decisions/RECORD.md#d196) on 2026-10-16: the recipe at polish time, elided-cell
popups and figures through `formatValue`; header units and table action separators were not changed.

A separate session from d1 because late dialog tables and Results are a different surface.
**Files:** editor_tables.cpp, editor_results.cpp, editor_batch_cells.hpp, editor_compare.cpp,
editor_counters.cpp, editor_sections.cpp, editor_zones.cpp, editor_od.cpp,
behaviour_lab_panel.cpp/behaviour_lab_telemetry.cpp, affected dialog table builders;
ui_design_tokens.hpp only for a helper using existing values; both locales.
**Tests first:** right-aligned fixed-precision figures through formatValue/QLocale; C-locale
Latin digits remain on English/Thai switching; unavailable/null/zero differ; full-text tooltip
on elided IDs/names; 24 px rows in tables created after startup; header units and action groups.
**Accept:** results-tabs-ui, batch-run-ui, compare-run-ui, queue-counter-ui, section-ui,
od-lot-ui, tables-ui and behaviour-lab-ui plus common gates; CLI/export/clipboard data unchanged.
Use a representative before/after export byte comparison, not a new baseline fixture.
**Risk:** format helper changing export bytes or parsing tests, unit duplication, text/numeric
IDs misclassified, too many rows of batch tables reducing readability.

## Slice e1 — overlay readability and HUD

HUD part done as [D197](../decisions/RECORD.md#d197) on 2026-10-16: scale bar, px/m and north from
the map projection. The casings and R/A/G cue below are slice e1b.

**Files:** src/editor/canvas.hpp, canvas_map.cpp, canvas_run.cpp, canvas_feedback.cpp and a
small canvas HUD helper if needed; src/shell map/HUD translation binding; both locales;
workspace_ui_tests.cpp, map_ui_tests.cpp, design_system_ui_tests.cpp and relevant run/gesture
suites. Check actual CMake target/source names before changing build registration.
**Tests first:** HUD scale follows inverse transform, units and valid/invalid georeference;
rotated/negative axes; marker/status/attribution non-overlap at both sizes/languages; HUD
receives no gesture hits. Existing R/A/G state supplies any additional glyph pattern; no new
signal logic. Background fixtures must be local images or fake tiles, not live OSM dependency.
**Accept:** high-contrast neutral casings on plain and aerial-like backgrounds, state cues
visible in grayscale; signal/vehicle positions and hit paths unchanged; Lab heading-up
supported; attribution/validation always present; owner §E review added.
**Risk:** wrong “north” from projection convergence, m11 scale error under rotation, casings
obscuring road markings. If north cannot be computed from existing supported interfaces,
report/defer it to a separate geo task rather than change src/model/core or invent accuracy.

## Slice e1b — run vehicle casings and R/A/G shape cue

Done as [D199](../decisions/RECORD.md#d199) on 2026-10-16.

Carved out of e1 on 2026-10-16 (D197), because it is a second system.
**Files:** canvas_run.cpp, canvas_heads.cpp, vehicle_shape.cpp only if a casing path is needed,
canvas_style.hpp; design_system_ui_tests.cpp, behaviour-lab and run suites.
**Tests first:** run vehicles carry a neutral edge casing readable on a local aerial-like image and
on the plain grid; R/A/G heads differ in shape or pattern when rendered in grayscale; vehicle poses,
hit paths, crossbars and snapshots unchanged; no new signal logic.
**Accept:** owner §E row with grayscale and aerial-like images; common gates.
**Risk:** casings hiding lane markings at low zoom; a pattern too small to read at 16 px.

## Slice e2 — measured LOD refinement

Done as [D200](../decisions/RECORD.md#d200) on 2026-10-16.

**Files:** canvas.cpp, canvas_run.cpp, relevant canvas overlay files and tests; reuse existing
viewScale/grid helpers. Read D28 before any cache proposal.
**Tests first:** threshold-boundary rendering at <3/3 px lane width and <14/14 px vehicle
length, selected handles still available, Ctrl+A drawing/picking unchanged; no hidden traffic
controls or misreported results. Use existing tags/geometry assertions plus screenshots.
**Accept:** intended decoration reduction only, documented before/after frame measurements on
the same fixture/toolchain if making a performance claim; no new cache, spatial index, GPU
renderer, snapshot semantics or gesture changes. Common gates remain mandatory.
**Risk:** detail suppression breaking selected-object feedback or making a head/counter vanish.

## Separate state/logic proposals — B18, not part of these slices

| Idea | Why separate | Likely files, subject to investigation |
|---|---|---|
| Problems badge; Snap/level/file status | New derived state, updates and localization lifecycle | editor_diagnostics.cpp, editor_window.cpp, editor_workspace.cpp; both locales |
| Shared/mixed-value Properties | New multi-object validation/transaction semantics | editor_inspector.cpp, editor_window.hpp; separate commands design if needed |
| Remember folded/expanded panels | Preference persistence and versioned restore policy | editor_palette.cpp, editor_inspector.cpp, editor_recovery.cpp or new settings helper |
| Named workspace modes | New preset state, persistence and dock migration | editor_workspace.cpp, editor_window.hpp, settings helper |
| Uniform table context menus with new actions | New command surface and selection/enable rules | editor_tables.cpp and each affected table controller; tests/table_ui_tests.cpp |
| Colour/dark themes | New palette selection and icon/cache refresh lifecycle | ui_design_tokens.hpp, editor_style.cpp, editor_appearance.cpp and canvas style helpers |

These are options, not accepted scope or promises. If selected, design/test each as a separate
session and keep current model/file constraints until explicitly authorized otherwise.
