# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-11-03 — Synchro is right-hand only; left-hand UTDF export modes (D437)

- **Owner answer (U0 item 4):** Synchro builds right-hand traffic only. The upload `2569-P01.csv`
  was an encrypted Synchro 12 project, not UTDF; nothing was read from it or committed.
- **Owner choice (UX-5, D437):** the user picks at export.
  - Geometric (default): real map and legs, far turn in `R`, and the report says the network is left-hand.
  - Mirrored: reflected to right-hand, far turn in `L`, right-hand NEMA numbers.
- **Updated:** UTDF_EXPORT §6/§9/UT11, UTDF_PROBE §6 rows 1 and 7, OWNER_SITTING I.4,
  utdf-real-exports and NEXT.
- **Still open:** the §1 export, both imports of the hand-written file, and whether Links `Lanes`
  counts pockets. U1 stays blocked. `multra-check-docs` passes.

## 2026-11-03 — U0 partly answered from real Synchro/Vistro exports

- **Owner supplied:** UTDF exports and project files from two real studies: Synchro 10.1, Vistro 2026
  (with its `.vistromerge`) and an encrypted Synchro 12.0 `.syn`. None is the UTDF_PROBE §1 junction.
- **Recorded, not committed:** licence and project data are unknown, so only section and field names
  and codes went into [utdf-real-exports](evidence/utdf-real-exports.md). UTDF_PROBE §6 rows 3/7/8,
  OWNER_SITTING I and NEXT now say what is settled.
- **Findings:**
  - Node TYPE 3 is unsignalised.
  - A phase that ends at the cycle wrap is written `End 0`.
  - `Yield` = End − yellow − all-red.
  - Vistro's dialect adds U/L2/R2 movements, `D1…D32` phases and `Phase1…4`.
  - Vistro has a left-hand setting, but UTDF does not carry it.
- **Still open:** the §1 export, the far-turn column, Synchro's left-hand setting, both imports of
  the hand-written file, and whether Links `Lanes` counts pockets. U1 stays blocked; UTDF_EXPORT
  is unchanged.

## 2026-11-03 — drawing level is not separation (D436, D201 B3)

- **Owner request:** continue NEXT. Every other open line waits on the owner, so this session took the
  booked fix. Owner choice: separation is a confirmed overpass, or Street segments on different levels.
- **Change:** `automaticConflicts` no longer skips path pairs because their drawing `level` differs
  (M3_CONTRACT §1). Pairs of Links generated from an overpass pair or from Street segments on
  different levels stay apart, so Find crossings and D382 keep their meaning. Hand-drawn Links and
  Connectors on different levels now get a passive crossing, as the contract says.
- **Failure first:** the old `levels_apart_do_not_conflict…` became
  `drawing_level_is_not_separation…`; two new rows cover the overpass and level separators. All three
  failed before the fix. Shipped projects have every Link at level 0, so nothing they derive changes.
- **Verification:** release build, `ctest` 272/272, desktop/headless builds and `check`; `multra-check-docs`.

## 2026-11-02 — main CI green again: node-ui and object-groups-ui

- **Owner request:** fix the failing build (Native C++ run 692 on main, Linux and Windows) and rebuild.
- **node-ui:** the test still expected a resting Node outlined in ink; D434 draws it as a shade with
  no pen. The test now checks that the cleared Node has no outline.
- **object-groups-ui:** the fixed icons-only rail (palette max width = its width, 96ad27a) made Qt
  hold the canvas dock at its old height when Results → Problems showed the docks again, and the
  window grew 860 → 903 px. `setWorkspacePreset` now lifts the rail cap before showing/hiding docks;
  `resizePalette` sets it again, so the rail stays fixed afterwards.
- **Verification:** release preset build, `ctest` 272/272 and `check` pass locally (Qt 6.4, offscreen).

## 2026-11-02 — session-path docs: one copy of each standing rule and owner item

- **Owner request:** optimize docs/AI context; owner chose the recommended items 3 → 2 → 1.
- **AGENTS:** dropped the finished D15 (migration) and D16 (M1.1–M1.3) authorizations, which read
  as a live exception to one system per session; their two live rules stay. The CLA note, the
  owner-only appearance/gate rule and the new-dialog convention (D318/D319) moved here from NEXT.
- **NEXT:** removed restated OWNER_SITTING items (E22–E24, F4, G1/G2, H1/H2, C13, E11, B29) and
  finished history (A1–A3, S5, Node detail measurements now pointed to ENGINEERING_NOTES); every
  open item stays in order. No decision, gate or owner item changed.
- **Measured (characters, ≈ tokens ÷ 4):** session path CLAUDE+AGENTS+NEXT+docs/README
  36,619 → 34,722 (≈ 9,150 → 8,680 tokens); NEXT 14,430 → 12,425. The rest of NEXT is open work.
- **Not changed, for the owner:** OWNER_SITTING's 14-day carve-out rule is not being applied
  (17 items listed in September/early October remain on the sheet).
- **Verification:** `multra-check-docs` and `multra-check-file-sizes` (built from `tools/`) pass;
  no code or build change. Oldest whole entry archived, retaining ten.

## 2026-11-02 — all 34 UI audit corrections (D432)

- **Owner request:** detailed UI review, additional sizing/proportion review, then fix everything.
  [Evidence](evidence/ui-audit-fixes.md) maps all 34 findings to code and reproducible regressions.
- **Data safety:** results/automation exports protect project/recovery paths; document generations
  invalidate all list/diagnostic caches; rejected Problems clear on replacement; input/route tint
  ownership is explicit. Staged forms validate before closing and keep errors/drafts; untouched
  numbers, day-two/fractional periods and language-switch Properties drafts retain exact values.
  Empty width tokens refuse consistently, clicked cell editors own Remove, and 200 Unicode scalar
  names work for Thai and surrogate-pair characters.
- **Space:** Drawing opens without Objects; Tables reserves a readable network preview; Results
  uses the dock area with per-tab Macro/Meso/batch bars and stacked movement/queue tables below
  1250 px. Only the active tab imposes its vertical minimum. Common Properties shrinks when folded
  and scrolls when expanded. Object columns retain manual sizes and cap initial prose at 320 px;
  period editors fit their cells, seeds reserve 104 px, and growing speed forms keep fixed footers.
  The compact palette gives its released width to the canvas, with thin group separators; named
  tools use short captions and a separate chord column. Lab uses a 320 px panel and aligned,
  stacked fields. Session Compact/Comfortable/Large density provides 24/28/32 px rows/controls.
- **Canvas:** Fit reserves pixel room for labels, including cached roads during Run. Labels avoid
  viewport edges/HUD/map overlays after redraw/pan/resize and see the same road shapes in cached
  and live rendering. PNG scene export keeps independent placement; canvas destruction detaches
  its scene before label-member teardown. Scientific values, schemas and engine algorithms stay
  unchanged, with all not-yet-validated markers retained.
- **Measured:** Drawing at 1366×768 is 936×575 (51.3% of the window); compact releases/reclaims
  123 px English / 119 px Thai with zero Properties growth. At 1024, movement viewports are
  992×208 / 992×220 (8/9 full rows), queue names 726 / 667 px; both locales fit 800×600 folded
  and expanded. Screenshots and commands are linked from the evidence note.
- **Verification:** desktop build and `check` pass: 271/271 CTest entries (381.90 s, parallel 5),
  plus architecture, file-size and documentation guards. New data/layout regressions cover both
  locales and layout at 1×/1.5×/2×; existing core/CLI/analytical CSV parity, docking, gestures,
  cached/live Run rendering and PNG export remain green. Linux GCC 14.2.0 / Qt 6.8.2 offscreen;
  headless unconfigured, Windows/native manual appearance not driven (E34).
- **Contracts:** D432/index, workflow, periods, canvas plan, module maps, evidence, NEXT and E34
  updated; oldest PROGRESS entry archived whole. Windows owner appearance remains open; this
  does not start canvas style C, another M13 slice or UTDF, or close a milestone gate.

## 2026-10-31 — A9 finish: explicit manual-count comparison windows (D422)

- **Owner B41:** explicit per-Node session-only confirmation before comparison, matching batch
  start/end and Node T, labelled manual counts, nothing stored. Checkpoints a6d0a25/d663bf5 are
  accepted so far: m2.6 totals 2040 veh/h over all three reviewed windows, source-64 reopen,
  manual-edit schema/source clearing and exact Undo, CBD/source retention and old-output parity.
- **Implemented:** Results → Node analysis → Micro side by side → Confirm manual count window…
  selects a Node by id/name and shows batch window/effective T. Required start/end preview uses
  the same project comparison guards as the result. Missing/non-finite/reversed/mismatched or
  duplicate confirmations refuse with localized named Node paths; no duration/name inference.
  Cancel keeps prior state, Confirm replaces only that Node’s session window. No project field,
  schema step, Command/revision or implicit run. Re-Analyse, batch changes/cancel, edits/Undo
  and New/reopen clear confirmations; saved project and analytical report bytes stay unchanged.
- **Presentation:** Node/approach rows label Manual counts versus Model demand in both locales.
  The comparison CSV alone appends quoted countsSource through shared formatters and marking;
  HCM/CLI formats, values, missing estimates, n/CI/unfinished and distinct measures stay as before.
- **Tests first:** both native manual cases fail on missing rows after asserting real completed
  batches/entered T; all four UI cases fail on the missing confirmation button. A test setup
  compile error (multiplying an optional T without dereferencing) was fixed before recording
  product reds. Coverage includes absolute-window mismatch with equal duration, correct window
  with wrong T, wrong/duplicate ids, labels, Cancel, no History/Save change, CSV equality,
  re-Analyse, edit/Undo, new batches and reopen, on both driving sides and English/Thai.
- **Verification:** native nodeanalysis 46/46, Micro UI 8/8, Fill UI 4/4 and JT12 UI 8/8
  pass, including expanded lifecycle rows. Desktop build (108 tasks) and CTest 260/260
  (303.56 s, parallel 5) pass; check 260/260 (301.67 s) also passes with architecture,
  file-size and documentation guards. Five legacy project/CSV comparisons are byte-exact and raw JSON is
  exact except buildCommit. Linux GCC 14.2.0/Qt 6.8.2 offscreen; headless unconfigured,
  Windows/manual/high-DPI appearance not driven (E32). No HCM/core/other-level algorithm change.
- **Docs:** D422/index, answered B41/E32, contracts/schema/module maps and ROADMAP/NEXT updated.
  Oldest PROGRESS entry archived whole, ten live entries retained. A9 is implemented and stops
  for owner review; no open A9 contract question, M13 gate closure or UTDF writer. U0 unanswered.

## 2026-10-31 — A9 staged Fill, source schema and Micro comparison (D421)

- **Implemented answered B40:** Analysis → Fill from model… stages explicit window/HV choices,
  StudyPeriod minus clockStart, all-turn before/after including zero/stale removal, T=window
  hours and PHF=1. Child Confirm stages only; parent Confirm is one Command/Undo. Preserve
  adjustments/saturation, and make drawn approach Clear stay visible like drawn turns (E31).
- **Provenance:** feature-selected schema 64, closed/version-gated historical source fields,
  exact captured project SHA plus compiler manifest file hashes/fallbacks and explicit choices.
  Older/source-absent files keep their schema/bytes. Command and raw History demand-row/T/PHF
  edits clear an unchanged claim; site-only edits retain it, Undo restores it. Source is exposed
  in the form/settings and conditional analytical JSON/CSV, with no method or old-format change.
- **Micro:** reuse the explicitly requested completed batch. Capture decoded source and ordered
  Node/in-Link ids at compile; duplicate names are harmless. Match source/window/T, present
  Node/approach HCM and Micro measures beside each other with n/CI/unfinished/blanks, and
  localized named refusals. Shared comparison CSV keeps marker/marking; active-view export is
  disabled without rows. No implicit run, subtraction, turn matching or new delay formula.
  Edits/Undo, seed changes, cancellation and reopen discard comparisons under batch lifecycle.
- **Failure first:** source codec/replacement cases failed before schema/T/PHF implementation;
  both-locale/side Fill and Micro suites failed on missing controls, native comparison on empty
  rows. Compile/setup mistakes (missing hash include, helper name colliding with namespace test,
  requiring completed vehicles where a valid seed had none) were corrected before claiming reds.
- **Coverage:** native nodeanalysis 44/44; new Qt Fill 4/4 and Micro 4/4, both sides/locales,
  retaining JT12 8/8 and real CLI source-64 JSON/CSV parity. Exact snapshot source, site retention,
  staged Cancel/no-op, one Undo/Redo/reopen, demand-edit clearing, batch/window/stale/refusal and
  actual export cases pass. Legacy five project/report byte comparisons pass: project/CSV exact,
  raw JSON exact after replacing only buildCommit. No core/HCM/Micro/Meso/Macro algorithm change.
- **Pending:** B41 manual-count window confirmation remains unanswered; no window is inferred
  from T, and manual missing-window rows give NODE_MICRO_NO_WINDOW. This is an A9 checkpoint for
  review, not a claim that B41 or A9's remaining contract is closed. U0 unanswered; no UTDF writer.
- **Verification:** desktop build passes (210 tasks). CTest 260/260 (289.67 s, parallel 5) and check
  260/260 (285.80 s), with architecture/file-size/docs guards, pass. Linux GCC 14.2.0/Qt 6.8.2 offscreen only; headless unconfigured, Windows/manual
  appearance not driven (E32). Docs/schema/module maps, D421/index, ROADMAP/NEXT and owner sheet
  updated. Oldest PROGRESS entry archived whole, ten live entries retained.

## 2026-10-31 — A9 review: resolve compiled lane-change stubs (D420)

- **Owner:** native preview accepted so far; remaining B40 choices answered as recommended:
  explicit window/StudyPeriod placement, replace T with duration and PHF with 1 before Confirm;
  reuse completed requested batches for Node/approach n/CI/unfinished, keeping quantities distinct.
  B41 asks the manual-count window only; no answer is assumed. U0 remains unanswered, no writer.
- **Failure first:** shipped m2.6 plus Suggest nodes refused its singleton-origin path-4 stub,
  as did the hand-counted pocket on both sides. Corrected the ambiguous fixture's forcing setup
  to enter on a real source and give positive stub demand two complete, different Node exits.
- **Fixed:** follow every compiled laneChanges target through further stubs and accept only
  agreeing complete traversals. A stub with targets cannot fall back to a broader origin family.
  Target-free stubs retain the exact-origin fallback. Visit graph cycles once; stage all results
  before installation, and copy queue ids before appending so vector growth cannot alter a choice.
  Determined demand totals are conserved; different/partial target traversals still refuse.
- **Tests:** six new both-side cases cover real m2.6 windows, the 1200 veh/h pocket's hand totals
  720/240/240 and the ambiguous control. Native nodeanalysis 38/38 passes. No stored field/schema,
  method, geometry or level output change. Desktop build, CTest 258/258 (274.43 s) and check
  258/258 (270.13 s) pass, with architecture/file-size/docs guards. Five fixture/project JSON
  reports differ only in buildCommit; CSV and project bytes are unchanged. Linux offscreen;
  headless unconfigured, Windows/manual not run.
- **Docs:** native contract, B40 answers/B41, D420/index and NEXT updated; B33–B36 and oldest
  PROGRESS entry archived whole, ten live entries retained. Next remains the rest of A9.

## 2026-10-31 — A9 native expected-demand preview checkpoint (D419)

- **Owner:** J7 fe827c8 approved: real CLI/editor JSON/CSV parity for saved and unsaved
  Save-representation snapshots; shipped projects/node analysis unchanged except buildCommit.
  M14 junction slices are complete; U0 remains unanswered and no writer starts.
- **B40:** asked six contract choices before code. Owner chose expected Micro demand, explicit
  HV types, whole-set replacement/provenance cleared by later manual demand edits, and named
  unresolved-demand refusals (1/4/5/6). T/PHF and the comparison action (2/3) remain unanswered;
  their recommendations are not assumed. Schema 64 is booked for provenance, not consumed.
- **Independent implementation:** read-only expected-demand preview captures/parses the same
  bytes, reuses the compiler, integrates requested input intervals and maps complete runtime
  traversals to derived turns. Use reference-to-lane stations and exact compiler origins for
  entry-lane stubs; never multiply a turn by possible lane paths. Explicit HV weighting, zero
  rows/retained optional %HV and source revision/manifest; no sampled traffic or achieved counts.
  Refuse partial/unresolved traversals and affected potential parking releases, including
  lot-to-lot sources. A provably empty lot without gate demand stays harmless.
- **Tests:** NA24's direct interval/lane/stub rows failed against the empty interface on both
  sides before implementation. Extended guards exposed a parking fixture without its required
  gate; fixed the forcing setup. The empty-lot control then failed on an overbroad refusal,
  before its guard fix. Eight new native cases cover those paths, clock-placed OD/compositions,
  scheduled decision intervals, invalid requests/HV types, partial routes and runtime choices.
  Native nodeanalysis now passes 32/32, including existing method rows.
- **Unfinished:** no stored field/codec/version, Fill dialog/Command, T/PHF policy or Micro
  comparison. Both-locale UI rows and NA26–NA30 are still planned, not claimed. E31 records the
  cheap drawn-approach clear inconsistency for the next dialog touch; J7 approval stays intact.
- **Verification:** desktop build, CTest 258/258 (280.29 s) and check 258/258 (283.04 s) pass.
  A separate non-PCH header compile caught a missing complete Json include; fixed it, then
  passed standalone compilation, another desktop build and native nodeanalysis 32/32.
  Architecture/file-size/docs guards pass. Linux offscreen only, including existing JT12
  en/th cases; this is not a claim of the still-unimplemented new UI rows.
  The configured four-leg fixture and four shipped projects retain project bytes, and their
  before/after --node-analysis JSON bytes differ only in buildCommit. No Windows/manual
  verdict; headless is unconfigured. NEXT names the two pending answers and exact remaining work.
  B39 and the oldest PROGRESS entry moved whole to indexed archives; ten live entries retained.
