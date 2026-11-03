# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-11-03 — Vistro draft 1 of the U0 junction

- **Owner:** exported the U0 sheet from Vistro 2026 (metric, left-hand on), allowed the commit, and
  will redo it to match the sheet.
- **Committed** as `tests/data/utdf/u0-vistro-draft1.csv`, byte for byte. It is a draft, not the golden file:
  cycle 60 with phases 2/4/6/8, PHF 1.00, HV 2 %, and different NB/SB lanes.
- **Settled:**
  - Vistro writes UTDF in feet even for a metric project, but reads `Metric,1`.
  - With left-hand on, Vistro writes the far turn under `EBR` (geometric, as D437's default).
  - Its 4-phase numbering is NB 6, SB 2, EB 8, WB 4.
  - Its `End` for the phase that ends the cycle is the cycle length; Synchro writes 0.
- **Updated:** UTDF_PROBE §6 rows 9/10/12/16, utdf-real-exports, UTDF_EXPORT §4/§6,
  OWNER_SITTING I.1 and NEXT. `multra-check-docs` passes.

## 2026-11-03 — Vistro imports u0-handwritten.csv; timing probe v2

- **Owner:** found Vistro's UTDF menus: *File → Export / Import → Synchro…*. They imported
  `u0-handwritten.csv`.
- **Came through as sheeted:** geometry, speeds, 3.25 m lanes, all four pockets, shared lanes,
  volumes and PHF.
- **Not shown:** the intersection median.
- **Timing refused:** `CellNotFound: Row not found: Row = 3, Column = 1`.
- **Added:** `u0-handwritten-v2-empty.csv` and `-v2-filled.csv`. Their `[Timeplans]` and `[Phases]`
  carry every record Synchro 10.1 writes, in its order; one leaves the unauthored rows empty, the
  other fills them. The import tells whether UT4 ("no unauthored values") survives.
- **Updated:** UTDF_PROBE §4/§6 rows 6 and 13–15, UTDF_EXPORT §3, OWNER_SITTING I.3 and NEXT.
- **Status:** U1 stays blocked. `multra-check-docs` passes.

## 2026-11-03 — U0 golden file moves to Vistro (D438)

- **Owner:** Synchro cannot create UTDF. They asked for another route and chose Vistro as the
  reference (D438).
- **Changed:**
  - UTDF_PROBE §4 now has Vistro steps: draw left-hand and metric, run HCM 7, export `u0-vistro.csv`, find the far-turn column, import the hand-written file, then the licence question.
  - UTDF_PROBE §6: rows 2/4/5/8 are closed as not possible; rows 9–12 are added.
  - UTDF_EXPORT §8, UT0 and UT10, OWNER_SITTING I.1–I.3, `tests/data/utdf/README.md` and NEXT are updated to match.
- **Kept:** Synchro's dialect stays recorded from the real 10.1 export. Synchro only checks the
  import, if it can import UTDF. U1 stays blocked. `multra-check-docs` passes.

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
