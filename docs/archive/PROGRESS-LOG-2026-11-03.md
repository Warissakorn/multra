# PROGRESS archive — 2026-11-03

Whole entries moved from the live PROGRESS log.

## 2026-11-03 — UTDF U2b: desktop export, Number Nodes, UTM

- **Numbering:** template Apply numbers an unnumbered Node (`generateJunction`), and re-Apply keeps
  the number. `numberNodes` and `setNodeNumber` are in `node_commands`.
- **Nodes page:** a Number column, a Number field in *Rename*, and *Number Nodes* (one Undo step).
- **Export:** *File → UTDF for Synchro/Vistro…* (`utdf_export_dialog.*`).
  - It shows the report, with the mode and the NEMA opt-in, before the file dialog.
  - It writes atomically and refuses the open project.
  - It writes UTM through `GeoProjection::worldToGrid` when the project has a map reference.
- **Translations:** en/th strings for every `UTDF_*` code and the dialog.
- **Tests:**
  - `utdf-export-ui` (UT9): the desktop writes the CLI's expected files byte for byte in both
    modes. It also covers cancel, protection, Number Nodes and Thai.
  - `map-geo` checks `worldToGrid`.
  - `node-ui` now expects the 9th column.
  - The jt6 fill now writes schema 65.
- **Verified:** headless `utdf`, `junctionfill` and `nodeanalysis` pass, and so do desktop
  `utdf-export-ui`, `map-geo`, `node-ui` and `object-export-ui`. `multra-check-docs` passes.

## 2026-11-03 — UTDF U2a: phases, volumes and timing (D443)

- **Owner:**
  - U2a, then U2b, in one session;
  - a Node whose controller fails NEMA is left out unless the user opts to export it unsignalised;
  - the Vistro median patch stays a report step until Vistro's link numbering is confirmed.
- **Writer:** `utdf_signals.*` reads the Node's controller through its heads and checks it with
  `nemaCheck`.
  - **[Timeplans]/[Phases]:** every Synchro 10.1 record. All-red is the ring gap, and the D439
    values fill the records MULTRA does not author.
  - **Lanes:** `Phase1`/`PermPhase1` per UTDF_PROBE §5. Volumes, PHF, HV, ideal flow and bus stops
    come from `analysis`, with A1's HCM defaults listed. A1 and UTDF now share one PHF rule
    (`nodeAnalysisPeakHourFactor`).
  - **Report steps:** median (`UTDF_MEDIAN_STEP`) and HCM 7th. Mirrored mode keeps the group
    numbers, which are already right-hand NEMA numbers.
- **CLI:** `--utdf-unsignalised`.
- **Tests:** the U0 junction now carries the §2 volumes and the §3 controller.
  - It reproduces `u0-handwritten-v3.csv` except `[Network]`'s values and the external ends'
    `Grade`. `[Timeplans]`/`[Phases]` match byte for byte.
  - P1–P4 cover both driving sides (UT7).
  - The four-leg A1 fixture covers the NEMA refusal and the opt-in (UT8).
  - Mirrored mode is covered (UT11).
  - The CLI fixture is renamed `u2-u0-junction*`.
- **Verified:** headless `check` passes 174/174; `multra-check-docs` passes.

## 2026-11-03 — UTDF U1: the geometry writer (D442)

- **Owner:** chose world metres for U1, with UTM in U2 on the desktop, because PROJ is not in the
  headless build. Mirrored mode is in U1. Recorded as D442; the contract is UTDF_EXPORT §7.
- **Schema 65:** Node `number`, an optional positive integer. A repeat is `NODE_NUMBER_DUPLICATE`.
- **Writer:** `utdfDocument(document, mode)` (`utdf_export.*`, with the leg walk in `utdf_legs.*`)
  writes [Network], [Nodes], [Links] and [Lanes]:
  - legs walked to the next Node or to an external end;
  - D440 lane counts, with pockets on the owning turn;
  - every empty authored field listed in the report.
- **CLI:** `multra-cli --project P --export-utdf FILE [--utdf-mode geometric|mirrored]`. It refuses
  an existing file and writes nothing when no Node can be exported.
- **Tests:** group `utdf` covers UT0–UT5 and UT11's geometry. The U0 junction rebuilt in MULTRA
  reproduces every Node 1 `[Links]` line and the ten `[Lanes]` geometry lines of
  `u0-handwritten-v3.csv`. `cli-utdf` compares the CLI's files byte for byte.
- **Verified:** headless `ctest` 174/174 and `check` pass; `multra-check-docs` passes.
- **Next:** the owner imports `u1-u0-junction-geometric.csv` into Vistro (UT10). After that comes U2.

## 2026-11-03 — Vistro limits closed (D441)

- **Owner:** both whole-network merges (`u0-full-lh-a/-b`) create a second, overlapping junction and
  stay right-hand; only `a` gets the median. The owner stopped probing.
- **D441:** Vistro gets the UTDF file plus a `.vistromerge` median patch. Mirrored mode (D437)
  gives a correct far-turn HCM. The report covers HCM 7th and the right-hand result of geometric mode.
- **Updated:** UTDF_PROBE row 21, UTDF_EXPORT §6/§9 UX-6/UT12, the README, NEXT and OWNER_SITTING I.4.
- **Open for U1:** how the patch learns Vistro's post-import link numbers. `multra-check-docs` passes.

## 2026-11-03 — UTDF cannot be left-hand in Vistro; whole-network probe

- **Owner (patch v2):**
  - The median merged.
  - Left-hand and HCM 7th did not merge.
  - Vistro refuses to switch to left-hand while a right-hand junction exists.
- **Owner choice:** try the whole network in Vistro's own format, merged into an empty left-hand project.
- **Added:** `u0-full-lh-a/-b.vistromerge`, generated by a script from the structure of the owner's
  post-import export (not committed) and holding U0 sheet data only. `b` mirrors lane indices.
  A script check finds every lane-turn and signal-group index in `$LANE`.
- **Updated:** UTDF_PROBE rows 20–21, UTDF_EXPORT §6, the README and NEXT. `multra-check-docs` passes.

## 2026-11-03 — Merge Data patch v1 keeps right-hand; v2

- **Owner:** merging `u0-patch-v1.vistromerge` after the v3 import left the project right-hand.
  They also sent Vistro's post-import network export; it is not committed (owner).
- **Recorded (structure only):**
  - Vistro renumbers the end nodes (100100…) and numbers the links itself (227 N, 318 E, 409 S, 500 W).
  - Default values are omitted from the export.
  - Pockets are `$LANE` rows with a length.
  - The signal groups match §3. The offset is restated as 44 s LAGEND.
- **Added:** `u0-patch-v2.vistromerge`, in the export's own shape, with HCM 7th and a 2 m median on
  legs 318/500. UTDF_PROBE rows 18–20, utdf-real-exports, the README and NEXT are updated.
  `multra-check-docs` passes.

## 2026-11-03 — U0 limits: a Vistro Merge Data patch

- **Owner:** asked how to handle UTDF's limits: the import switches to right-hand, there is no
  median, and Vistro opens imports as HCM 2010. Vistro has *File → Import → Merge Data*.
- **Found:** the owner's Vistro network export (Visum net format 19) has `$NETWORK
  LEFTHANDTRAFFIC`, `$NODE ICAMETHODSIGNALIZED`/`USELOCALICAMETHOD` and `$LEG HASCISLAND`/
  `CISLANDWIDTH`/`CISLANDLEN`.
- **Added:** `u0-patch-v1.vistromerge`, which sets left-hand and HCM 7th for node 1. `.gitattributes`
  now covers `*.vistromerge`. UTDF_PROBE rows 18–19, UTDF_EXPORT §6 (proposed companion file),
  the README and NEXT are updated. The median patch waits on Vistro's link numbers after import (row 19).
- **Check:** `multra-check-docs` passes.

## 2026-11-03 — U0 golden file committed

- **Owner:** exported again from Vistro and accepted it as the golden file, `u0-vistro.csv`
  (committed byte for byte). It matches §1–§2 with a 100 s cycle and protected-only far turns:
  EBR is phase 3 and WBR phase 7. The phase numbers, the lagging order and the splits are
  Vistro's own, not §3's, so UT0 compares fields and codes, not timing values.
- **Updated:** UTDF_PROBE §6 row 16, UTDF_EXPORT UT0, the utdf README, OWNER_SITTING I and NEXT.
- **Row 17:** `u0-handwritten-v3.csv` (Links `Lanes` 3) imports with the pockets as pockets and the
  same HCM 2010 result as v2-filled: 186.72 s, LOS F. D440 holds, and U0's probe questions are answered.

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
