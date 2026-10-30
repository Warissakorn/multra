# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-10-30 — J5 review: generated transition polygon (D407)

- **Fixed:** the real Find crossings → Transition → F3 → explicit tapers → Fill workflow
  enlarges a still-generated Node polygon around both tapers and Connector rails, with its
  Links, Connectors and controls in the same Command and one Undo. A person's polygon stays
  unchanged and refuses outside tapers at `network.nodes[index].polygon`.
- **Envelope:** reuse the convex hull and widest-attached-lane padding convention from
  suggested Nodes (D265); preserve the old polygon and generated flag, keep external legs,
  and retain atomic reference guards. No new stored field/schema, timing default or core change.
- **Tests first:** the new end-to-end JT10 row failed on the actual generated polygon on
  both driving sides; the edited-polygon and existing outside row failed on the missing
  polygon path. All 11 native cases now pass, including 15/30/60 m, kerb/far, painted rails,
  unchanged preview, re-apply, save/reopen and complete JSON Undo/Redo. No ±60 m fixture is used
  for the new workflow. Frozen JT13 files are unchanged.
- **Docs:** §7/JT10, generated/schema/street references, D407/index and Windows E28 updated:
  no manual polygon edit is needed for a generated Node. Archived the oldest whole PROGRESS
  entry, retaining ten. Stop for J5 review; no J6, J4T or UTDF writer work.
- **Verified:** desktop build, JT10 **11/11** and Qt transition selector on both driving
  sides pass on Linux (GCC 14.2.0/Qt 6.8.2 offscreen). CTest **253/253** (450.41 s) and
  `check` **253/253** (241.64 s) pass, including architecture, file-size and documentation
  guards. CLI node-analysis output matches except `buildCommit`. Headless is not configured;
  Windows and manual UI driving were not run (E28).

## 2026-10-30 — J5 transition implementation (D406)

- **Owner B34:** taper entirely upstream, Transition-only selector in J5, two symmetric
  installed alignments and owned mixed directions. J4T remains documentation only (D405).
- **Implemented:** strict transition catalog shape and schema-63 field guards; old recipe
  fields keep their existing floors. Differing end-to-start pairs are Transition rows; the
  selector starts empty and Confirm needs a choice. Confirm and fill each use one Command.
- **Geometry:** cut the incoming arm at its entered centreline taper, keep the outgoing start,
  map aligned lanes one-to-one, then merge/diverge the extra lanes on the opposite side.
  Existing merge order/defaults, generated/refine and named reference protection apply.
  No Node kind/layout is duplicated, no site length is defaulted, no core/HCM method changes.
- **Tests first:** the first three JT10 cases failed on the loader, missing row and fill.
  Extended cases cover both driving sides, mixed alignment, one-way/unused site values,
  bounds/missing values, generated replacement/refine, stationed heads and full JSON Undo.
- **Docs/UI:** PROJECT_SCHEMA, street workflow, module maps and OWNER_SITTING E28 updated.
  Street segment/transition Qt test registration was split before the 500-line limit.
- **Fixture corrections:** allocate new Node ids through the API; a detailed two-leg layout
  still has approaches, while HCM refuses its unsignalised control. Micro compilation needs
  inputs and emits conflict zones for authored merges, not legacy priority rules.
- **Verification:** desktop build, native JT10 **9/9**, and the new Qt selector test on both
  driving sides pass. CTest **253/253** (253.04 s) and `check` **253/253** (241.42 s) pass. CLI output
  matches before/after except the single `buildCommit`. Headless is not configured. Linux,
  GCC 14.2.0/Qt 6.8.2 offscreen; Windows/manual UI were not run (E28). Stop for J5 review.

## 2026-10-30 — J5 preparation and signalised T booking (D405)

- **Asked:** J4 5ab3ea3 approved; start J5 transitions, then stop for review. Add B33 for
  signalised T templates as documentation only, proposing a small slice before U1.
- **B33 answered:** one protected three-stage preset first, retaining J4's compass NEMA
  numbers, omitting unused phases and keeping the 2/6 zero anchor. Recorded D405 and booked
  J4T before U1 with its own contract/rows and review; no T code is added during J5.
- **Before transition code:** re-read §7/JT10, D387/D388, crossings, lifecycle, F4 cuts and
  generated rules. §7 does not fix where taper is placed or how template selection crosses
  the J5/J6 boundary. B34 asks those choices and the initial directional catalog/structure,
  with concrete recommendations; no answer is assumed. JT10 remains unimplemented and 63 unused.
- **Verification:** desktop build and standalone documentation, file-size, architecture and
  whitespace checks pass on Linux. CTest, the full `check` target and headless were not run
  for this documentation-only checkpoint. No Windows or manual UI run; no new UI or stored field.
  The build stays green at the approved J4 implementation while B34 is pending.

## 2026-10-29 — J4 template signals and NEMA checks (D404)

- **Owner B32:** all recommendations selected: numbered rings and corrected barriers; P4
  4 then 8; explicit clearance separate from derived waiting; rotated compass mapping and
  time zero at 2/6, with leading far turns at the preceding cycle's end. U0 remains unanswered;
  the internal implementation does not establish Synchro/Vistro interchange.
- **Implemented:** pure timing construction from every entered green/yellow/allRed, with
  grid, phase-set and cycle-closure refusals; read-only network/layout NEMA checks. The fill
  generates ordinary controllers/groups/heads and overlapping-green priority, with no Stops
  at signals. No schema step, stored layout, timing default, core or HCM-method change.
- **Ownership:** generated signals replace safely in the fill transaction. Manual edits,
  controller reuse outside the junction and queue measurements naming generated heads refuse
  atomically with their ids. Exact Undo/Redo covers the whole regenerated document.
- **Tests first:** the three initial JT9 cases failed before implementation. Extended cases
  cover every preset/side/major axis/pocket variant, clearances versus waiting, HCM source
  greens/P3 result g/C, Micro seed 42 and generated/reference rules. The protected-shared test
  initially forced a lane-count error; its full-lane setup was corrected before the check.
  A further red case exposed compass labels masking adjacent major legs; reciprocal geometric
  through movements/bearings now establish physical opposition. A red case also keeps the
  checker usable on hand-drawn Links without Street segment associations.
- **Docs:** §8 records the chosen construction; the full decision index was split along M14
  navigation before exceeding 500 lines. Windows look is E27. J6's corridor booking stays docs
  only; neither a selection aid nor the combined Command nor UTDF writer is implemented.
- **Verification:** desktop build, JT9 **9/9**, junctionfill **25/25**, CTest **251/251**
  (304.08 s) and `check` **251/251** (274.93 s) pass on Linux (GCC 14.2.0, Qt 6.8.2
  offscreen). Node-analysis CLI bytes match before/after except the single `buildCommit`.
  Headless is not configured; Windows and manual driving of the signal fill UI were not run
  (OWNER_SITTING E27). Existing desktop UI suites passed. Stop for J4 review before J5.

## 2026-10-29 — J4 preparation: U0 checked, timing questions and corridor booking (D403)

- **Asked:** J3b a52476b approved; start J4, stop for review, no timing defaults or writer.
  Extend J6's cross-section Command to corridors before U1; docs only for that booking.
- **Checked before code:** JUNCTION_TEMPLATES §8/JT9, D395/D396 and U0's answer table.
  UTDF_PROBE §6 and OWNER_SITTING I are still blank. Reported that J4 can establish MULTRA
  contract support only, while Synchro/Vistro mapping and left-hand interchange remain unverified.
- **Pending B32:** §8's barrier/P4 contradiction, entered allRed versus barrier-wait red,
  and leg-number/time-zero conventions change stored timing and results. Asked the owner in
  Thai with options/recommendations before code; no answer or default has been assumed.
  No J4 implementation or JT9 pass is claimed.
- **Corridor booking:** a generated Link can belong to two filled junctions; deletion also
  removes the neighbour's leg and attached objects. §5 and NEXT now require a shared-Link
  selection aid or corridor-aware J6 Command, keeping both ends consistent. No J6 code.
- **Verification:** desktop build, documentation links/indexes, file-size, architecture and
  diff checks pass. Node-analysis CLI bytes match except buildCommit. Runtime/test source is
  unchanged; JT9, CTest and full `check` were not run for this documentation-only checkpoint.
  Headless is not configured; Windows/manual UI was not run. J4 remains pending B32.

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
