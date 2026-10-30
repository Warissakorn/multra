# PROGRESS archive — 2026-10-29

Whole entries moved from [PROGRESS](../PROGRESS.md); historical, not the current queue.

## 2026-10-29 — J3b fill checkpoint, owner choices and all-way-stop question (D397, D398)

- **Scope:** continue J3b only. The owner kept signals in J4 and selected a named refusal with
  existing → required counts when full lanes differ. JT8 uses the two-way-stop fill in Micro,
  then manual Commands for test-only signals to inspect HCM lane groups. No preset execution,
  timing defaults, template editor, transition or UTDF writer is introduced.
- **Implementation:** pure per-call plans list missing site values/codes/paths and build cuts,
  physical pockets and merged movement ranges. The junction offer follows F3; F4 cannot bypass
  a recipe. Shared F4 right-of-way ranks remain unchanged. Recipe arm ids stay; the final storage
  Link keeps the Street segment association. Re-apply walks generated predecessors and refuses
  named edited detail/attachments. Link.generated uses feature-selected 62, older field paths
  refuse it, and fill transactions preserve program ownership while manual edits refine it.
  Coarse still disappears only through the existing detail transition, with recipe retained.
- **Unfinished:** D388 requires generated all-way stop, but existing StopControl cannot name an
  independent stop line. OWNER_SITTING B31 asks whether to extend that representation or defer
  generation. Until answered it is disabled with F4's existing code. This checkpoint does not
  complete J3b or authorize J4; remaining work is in NEXT. Windows look is E26.
- **Rows/verification:** the first six JT6–JT8 checkpoint cases failed before implementation. Extra red cases
  exposed missing Link export marking and a copied Link retaining generated; both are fixed.
  Desktop build, new checkpoint cases **10/10**, desktop CTest **250/250** (242.41 s) and
  `check` **250/250** (221.90 s) pass on Linux (GCC 14.2.0, Qt 6.8.2 offscreen). The JSON
  comparisons include revision for exact Undo/Redo. Node-analysis CLI bytes match before/after
  except buildCommit. Headless is not configured; Windows and the new junction-specific UI
  rows were not driven. Existing desktop UI suites passed. No J3b completion/gate is claimed.

## 2026-10-29 — J3a review: keep coarse until detail refine (D393)

- **Asked:** keep coarse beside the template recipe so choosing/Detach cannot change Macro/Meso
  before the fill; update D393, verify, push and stop again before J3b.
- **Fixed:** `setNodeJunction` still carries coarse.major when no major is supplied, and leaves
  coarse untouched for both choice and Detach. Document validation accepts coarse plus recipe.
  The existing detail-refine rule alone handles the transition; no J3b fill was implemented.
- **Rows:** a forcing-first signalised coarse Node with a banned turn compares the entire Meso
  and Macro graphs and diagnostics before/after selection, Detach, save/reopen and Undo, on both
  driving sides. The setup first proves that removing coarse changes both compiles. Updated JT5
  checks coexistence/round-trip and coarse disappearing only when actual interior detail appears.
  Three checks failed before the implementation fix. No new schema field or UI.
- **Verified:** desktop build, JT5 19/19, CTest 249/249 and `check` 249/249 pass on Linux
  (GCC 14.2.0, Qt 6.8.2 offscreen). Node-analysis CLI bytes match before/after except `buildCommit`.
  Headless is not configured; Windows and manual UI driving were not run. Owner re-review is next.

## 2026-10-29 — Junction catalog and Node recipes, J3a (D393–D396)

- **Asked:** continue after J2 review; stop again after J3a, no PR. Owner approved all three
  clarifications ("เอาตามแนะนำ"): major/lane override home, independent Link-keyed compass
  overrides, and partial site values with no defaults.
- **Changed:** strict structural catalog/loader, filename/field diagnostics, 12 installed
  junction templates with no site numbers; owned catalog presence includes an empty array.
  A further forcing-first catalog row separates protected far turns for P2/P3 (D396, §8); no
  phasing/signal implementation is included.
  Schema 61 stores the recipe and directions only when used; older files still read and fields
  before 61 refuse at their paths, even null. Node value types were split from the 500-line header.
- **Commands:** choosing carries coarse major and clears coarse in one Undo; detach, catalog
  capture and directions are undoable. Link splits carry direction ids downstream; Street segment
  splits carry recipe keys at the moved end. Recipe Nodes survive detail removal / two-leg
  lifecycle. Fill, signals and editor stay J3b/J4/J6; HCM storage and B29 keys are unchanged.
- **Rows:** JT5 forcing-first load failures, ownership/order, malformed JSON/duplicate ids,
  schema floors/paths, partial recipes, coarse exclusion, Commands/Undo and direction/lifecycle.
- **Verified:** desktop build, JT5 18/18, CTest 249/249 and `check` 249/249 pass on Linux
  (GCC 14.2.0, Qt 6.8.2 offscreen). CLI Node analysis before/after is byte-identical except
  `buildCommit`. Headless is not configured; Windows and manual UI driving were not run.
  No new editor surface.

## 2026-10-29 — J2 review: preserve authored ambiguity and stop pockets at Nodes (D391)

- **Asked:** fix the two J2 review items, update D391, verify and push; stop again before J3a.
- **Fixed:** only `addTurnPocket`'s still-generated 1→2 widening skips its zero-source-width
  path in an implied Link-to-Link step. Hand-drawn/refined widenings retain pre-J2 ambiguity;
  explicit Connector selection remains unchanged.
- **Fixed:** `pocketOf` stops before a Connector with an inside run in any Node, including a
  straight-through movement. A pocket lane upstream of a junction is not a pocket at the next.
- **Rows:** both new JT3 regressions failed first after asserting their setup: a hand-drawn 1→2
  really has a zero-width source path; the straight continuation really belongs to the first
  Node and that Node's pocket exists. Both driving sides, and kerb/far pockets, are covered.
- **Verified:** desktop build and focused suite 9/9 pass on Linux (GCC 14.2.0, Qt 6.8.2 offscreen).
  Full CTest and `check` pass 248/248 each. CLI Node analysis is byte-identical apart from `buildCommit`.
  No new UI or fields; headless is unconfigured and Windows was not run.

## 2026-10-29 — Kerb/far pockets and derived junction layouts (D391, D392, M14 J2)

- **Asked:** continue J2 only on `claude/upbeat-ptolemy-qe1x84`, stop for review, no PR; owner
  approved the two clarifications ("ได้ เอาตามแนะนำ"): physical taper then storage, 45°/135° naming.
- **Changed:** `addTurnPocket` draws a kerb/far widening taper and full-width storage, on both
  driving sides, inside one Command. Original lane footprints, route/head ids, external heads,
  boundary/taper heads and one-step Undo/Redo survive. The legacy split tool is unchanged.
  An implied route continues along the ordinary path of a widening Connector, preserving its
  old outer lane; two ordinary bridges remain ambiguous, explicit Connector selection unchanged.
- **Derived:** `approachLayout` gives lane turns/widths/stop lines, pocket storage/taper (also over
  a bend and a later split), travel bearings and NB/EB/SB/WB + L/T/R. Duplicate/tied directions
  and U-turns are refused; per-call direction overrides change only labels. J3/J6 own persistence/UI.
- **Decided:** D391 uses existing geometry/fields and generated/refine, so no schema step; 61 is
  still unused. D392 fixes 45°/135° boundaries; HCM's current lane-group/storage derivation is
  retained pending NA1–NA23 byte parity and B29 (e).
- **Verified:** Linux desktop build (CMake 3.31.6, GCC 14.2.0, Qt 6.8.2 offscreen), CTest
  **248/248** (247 + `junctionlayout`, seven new cases). JT3/JT4 failed before implementation;
  `check` also passes 248/248. `multra-cli --node-analysis` output is byte-identical before/after except
  `buildCommit`. No new UI or Windows appearance claim; headless has no configured build.

## 2026-10-29 — J3b complete: safe fills, historical parity and explicit deferrals (D399–D401)

- **Owner review:** B31 defers D388 JT-4a generation until A5 or U1 needs it; the row stays
  disabled (`FILL_NODE_ALL_WAY_STOP`). StopControl and schema 63 are untouched. The owner kept
  the orthogonal-T/wide-median `EDIT_CONNECTOR_GAP` limitation and accepted the fill dialog's
  generatedCommand mode, requesting historical F3/F4 parity.
- **Finished:** Street segment Apply template over junction detail now refuses with a junction
  code and explicit Undo/earlier-copy → cross-section → junction fill instructions. Re-apply
  refuses if the base's shape no longer matches the cross-section, keeping the detached
  upstream Link intact. The tested recovery changes the full lane count before a new fill,
  whose complete output undoes in one step. Node recipes/Link keys stay D393/D394's; no new
  stored field or analytical method.
- **Reference protection:** cuts refuse named stationed data they would truncate or rebase;
  re-apply refuses new pocket origins/routing targets and conflicting direction keys, including
  two removed pocket pieces with no base override. Retained heads, route ids, volumes/directions
  and zone destinations survive, with complete JSON Undo.
  Red tests exposed the old generic lane error, changed-base/head movement and reference losses.
- **Rows:** JT6–JT8 and new JT13 pass in 23 junctionfill cases. The test-only command bodies are
  frozen from 33a856a5, checked unchanged apart from include/symbol isolation; F3/F4 priority and
  signalised/banned-turn outputs match in the same build on both driving sides, with complete
  IDs/revision/flags and F4 Street segment re-apply. No TypeScript baseline was regenerated.
- **Verification:** desktop build, CTest **250/250** (276.85 s) and `check` **250/250**
  (260.54 s) pass on Linux (GCC 14.2.0, Qt 6.8.2 offscreen), with junctionfill **23/23**.
  Node-analysis CLI bytes match before/after except `buildCommit`. Headless is not configured;
  Windows and manual driving of the new junction UI were not run (OWNER_SITTING E26).
  Existing desktop UI suites passed. J4 was not started.

## 2026-10-29 — J3b review: F4 recipe parity and recovery after reopen (D402)

- **Asked:** narrow the Street segment guard, explain recovery with no Undo after reopen,
  document measurement order and book the one-Command junction cross-section change in J6
  before U1. JT13 is accepted unchanged; stop before J4.
- **Fixed:** only the segment's junction-generated Links trigger its Apply refusal. Choosing
  a recipe on F4 detail keeps the existing laneWidth Apply behaviour. The native test first
  fails with the over-broad junction code and compares complete documents against the isolated
  pre-J3b Apply implementation, with/without recipe pockets on both driving sides.
- **Recovery:** after reopen without Undo, delete the junction's generated Links, Apply the
  Street segment template and fill again. JT15 checks the recipe, changed widths, checkLayout
  and exact Undo/Redo; deletion removes the attached travel-time measurement. English/Thai
  text states that cascade. §5 and OWNER_SITTING E26 say to fill before stationed measurements.
- **Scope:** no new stored field or schema; StopControl, JT13 and its frozen references stay
  unchanged. The future reference-preserving one-Command change is booked, not implemented.
- **Verification:** desktop build, junctionfill **25/25**, CTest **250/250** (274.60 s) and
  `check` **250/250** (263.19 s) pass on Linux (GCC 14.2.0, Qt 6.8.2 offscreen).
  Node-analysis CLI bytes match before/after except `buildCommit`. Headless is not configured;
  Windows and manual driving of the new junction UI were not run (OWNER_SITTING E26).
  Existing desktop UI suites passed. J4 was not started.
