# PROGRESS archive — 2026-11-03

Whole entries moved from the live PROGRESS log.

## 2026-11-03 — Vistro draft 2 of U0; Links Lanes counts pockets (D440)

- **Owner:** re-exported the U0 junction from Vistro. Draft 2 matches §1–§2: lanes, pockets,
  names, volumes, PHF 0.92 and HV 5 %. The timing is still 60 s with four phases; the owner will
  set §3 and export again.
- **Owner choice (D440):** `[Links] Lanes` counts the pocket, 3 at the U0 junction. Vistro's own
  writer counts only full lanes (2) and puts pocket storage on the through column. MULTRA keeps
  storage on the turn column, which Vistro reads.
- **Committed:** `u0-vistro-draft2.csv` byte for byte, and the probe `u0-handwritten-v3.csv`, which
  is v2-filled with the junction legs' `Lanes` 3 (UTDF_PROBE §6 row 17).
- **Fixed:** UTDF_EXPORT §3 still said MULTRA never writes unauthored values; it now cites D439.
  Rows 15–16 of UTDF_PROBE §6 had text outside their last cell.
- **Check:** `multra-check-docs` passes.

## 2026-11-03 — Vistro switches an import to right-hand; no median

- **Owner:**
  - The project was set left-hand before import; Vistro switched it to right-hand itself.
  - No median is shown.
  - The redone `u0-vistro.csv` arrived empty after the first column of every section, so it was not committed and a re-export is pending.
- **Recorded:**
  - UTDF_PROBE §6 rows 14–16.
  - UTDF_EXPORT §3/§6: in geometric mode the report must tell the user to set left-hand again after import, and that Vistro drops the median.
  - OWNER_SITTING I.3 and NEXT.
- **Check:** `multra-check-docs` passes.

## 2026-11-03 — Vistro imports the U0 timing; neutral values for unauthored records (D439)

- **Owner imports into Vistro 2026:**
  - `u0-handwritten-v2-empty.csv` is refused: `VehExt … D1 is invalid`. Empty cells are not allowed.
  - `u0-handwritten-v2-filled.csv` imports the timing exactly: controller 1, cycle 100, Ring 1 =
    1·2|4, Ring 2 = 5·6|8, splits 14/44/42/14/44/42. HCM 2010 then gives 186.72 s, LOS F, v/c 0.988.
- **Owner choice, D439:** the timing records MULTRA does not author get v2-filled's neutral
  pretimed values: constants and copies of authored values, each listed in the report. UT4 is amended.
- **Noticed:** the imports look drawn right-hand (row 15, to confirm).
- **Updated:** UTDF_PROBE §6 rows 13/15, UTDF_EXPORT §3/UT4, the utdf README, OWNER_SITTING I.3
  and NEXT. `multra-check-docs` passes.

## 2026-11-03 — U0: the refused timing leaves no cycle; HCM method is not in UTDF

- **Owner screenshot** of the original `u0-handwritten.csv` imported into Vistro: no cycle, empty
  rings, *ICA error*, all delays 0. This confirms the refused timing; the v2 files are not yet tried.
- **HCM method:** Vistro opens an import as HCM 2010. No UTDF record carries the method, and the
  owner sets HCM 7th after import. The export report should say so.
- **Updated:** UTDF_PROBE §6 row 6, UTDF_EXPORT §3 and utdf-real-exports. `multra-check-docs` passes.

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
  and codes went into [utdf-real-exports](../evidence/utdf-real-exports.md). UTDF_PROBE §6 rows 3/7/8,
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
