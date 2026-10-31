# PROGRESS archive — 2026-10-30

Whole entries moved from the live PROGRESS log.

## 2026-10-30 — J6 command/seam/corridor checkpoint (D410, D411)

- **B35 answered:** all recommendations selected: explicit listed seam replacement with
  reference guards, automatic complete shared-Link closure, preserve incompatible site drafts.
- **Implemented:** private previews/atomic explicit re-apply and corridor cross-section Commands;
  restore both ends before F3/regeneration, including pockets. Preserve retained measurements/
  inputs, refuse edited neighbours or measured generated heads atomically, and keep ordinary
  Fill/Apply guards. A unique generated pocket-prefix query stops at Nodes; no association is saved.
- **Seams:** pure lane/movement/pocket/transition/control/timing comparison; the same advisory
  in Micro/Meso/Macro Readiness. Changed major control is detected even when lane rows match.
  Template-id changes keep all entered site fields as a draft, with disabled Fill and full Undo.
- **UI:** optional matching Junction template column; explicit seam replacement list, Cancel,
  Apply again/Detach; staged corridor overrides and complete affected-set preview in one Undo.
  Linux Qt drives both sides. Reopening a revision-zero document now invalidates the touched
  object-table caches; otherwise the valid corridor file's Street segment table stayed empty.
- **Tests first:** initial JT11/JT16 cases failed, then targeted timing/control/readiness,
  crossing and Qt preview cases failed before their fixes. Positive pocket corridors required
  resolving the detached out-leg; an input fixture's mutually exclusive route target and a
  nested-modal test's lifetime were corrected after forcing assertions identified setup errors.
- **Checkpoint:** template catalog/Node recipe forms are not complete. B36 asks the same-template
  site-edit generated flag rule; no answer is assumed. NEXT names the remaining free-function
  forms and tests. Pause at the context boundary, not final J6 approval. No J4T/UTDF writer.
- **Docs:** J6 detail split before the 500-line limit; B35/B36, D410/D411/index, E29, schemas,
  readiness and module maps updated. Archive the oldest whole entry, retaining ten.
- **Verified:** desktop build, native `junctioneditor` **11/11**, Qt crossing/seam/corridor
  rows on both driving sides, CTest **255/255** (247.09 s, parallel 5) and `check` **255/255**
  (245.20 s) pass, including architecture/file-size/docs guards. CLI node-analysis matches
  except `buildCommit`. Linux GCC 14.2.0/Qt 6.8.2 offscreen; headless unconfigured;
  Windows/manual UI not run (E29). Logs and comparison: `/workspace/j6-main/`.

## 2026-10-30 — J6 first carry-over: overlap preview (D409)

- **Owner:** J5 d5e2bbe approved; start J6 including the D402/D403 corridor Command. First
  commit must make a transition's neighbour-overlap refusal visible before Apply.
- **Implemented:** after containTransition, compare the private polygon with the other Nodes
  using the existing overlap predicate. NODE_OVERLAP names each neighbour, disabling the Fill
  offer; the whole-document validator remains a backstop. Add Thai/English text for the code.
- **Tests first:** the new JT10 row failed on the enabled offer after proving that the original
  polygons were disjoint and the planned enlargement overlaps, on both driving sides. All
  14 native cases now pass, including unchanged preview/History and direct generation refusing
  before publishing a document. No new field/schema or geometry policy; frozen JT13 untouched.
- **Main J6:** re-read §3–§5/§10/JT11, D402/D403 and the seam/ownership contracts. Asked B35's
  three choices before main code: Apply-again versus edited-detail refusal, the affected
  corridor set, and incompatible entered site values. Recommendations are not answers; await
  the owner while finishing the independent carry-over. No J4T or UTDF writer work.
- **Docs:** §7/JT10, J6 status, E28, B35, D409/index and NEXT updated. Archived the oldest
  whole PROGRESS entry, retaining ten. Main J6 remains pending the B35 answers.
- **Verified:** desktop build, JT10 **14/14**, Qt transition selector on both driving sides,
  CTest **253/253** (253.57 s, parallel 5) and `check` **253/253** (253.16 s) pass, including
  architecture/file-size/docs guards. CLI node-analysis matches except `buildCommit`.
  Linux GCC 14.2.0/Qt 6.8.2 offscreen; headless unconfigured; Windows/manual UI not run (E28).

## 2026-10-30 — J5 review: refuse polygon captures (D408)

- **Fixed:** after generated transition polygon enlargement, every resulting Link leg must
  already have been a leg, with the same entry/exit direction. New legs refuse with
  `TRANSITION_POLYGON_CAPTURES` at each captured Link. Thai/English recovery says to shorten
  the taper or draw the Node polygon by hand. Existing missing-arm and NODE_OVERLAP guards stay.
- **Tests first:** JT10 failed on the owner's real side-road setup before the guard. The
  forcing asserts establish original 2 Street segment / 4 Link legs and prospective 3 / 6
  membership before checking the refusal; 60 m and both driving sides are covered, with the
  side road mirrored for right-hand traffic. The y=25 control stays outside and fills with
  one complete JSON Undo/Redo. A neighbouring Node still refuses NODE_OVERLAP.
- **Atomicity:** capture refusal preserves the complete document, revision, saved state and
  Undo/Redo history. Planning/preview publish nothing. No stored field/schema, taper/envelope
  geometry policy, core, HCM, J6/J4T or UTDF writer changes; frozen JT13 files remain untouched.
- **Docs:** §7/JT10, Street segment workflow, command map, E28, D408/index and NEXT updated.
  Archived the oldest whole PROGRESS entry, retaining ten. Stop for J5 review.
- **Verified:** desktop build, JT10 **13/13**, Qt transition selector on both driving sides,
  CTest **253/253** (238.25 s, parallel 5) and `check` **253/253** (234.08 s) pass, including
  architecture, file-size and documentation guards. CLI node-analysis matches except
  `buildCommit`. Linux GCC 14.2.0/Qt 6.8.2 offscreen only; headless is not configured, and
  Windows/manual UI driving were not run (E28).

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
