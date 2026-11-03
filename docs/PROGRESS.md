# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-11-03 — Lateral crossing at the window end (D444)

- **Owner:** agreed to the smallest fix first, after the review of the lateral audit: reproduce
  Finding 1, unify the crossing test and clamp the body. The lane-change state machine waits.
- **Cause (m2.6 seed 45, vehicle 613):** the span window [240, 277] ends at a join on the target route.
  At the dead end, `frontPart` put the mapped front on `connector-53`, so `crossingOn` found no crossing
  and the centre clamp let the body stand on the line. `lateralStart` read the same part, which gives
  a target of 0 to a change started there.
- **Change:**
  - `targetSectionAt` (`lateral.cpp`) is the one target-lane lookup for the start and the crossing.
  - `spanHolds` (`lanes.hpp`) replaces four copies of the coverage test.
  - On a side no span covers, the whole body stays in its lane (LATERAL §7; BA44 now pins this).
- **Rows:** BA158 and BA159 failed before the fix and pass after.
- **Result, switched on:** m2.6 seed 45 completes 1531 → 1903 (off: 1908). No vehicle straddles at the
  end of any seed. m2.6 clamps fall from 33 to 28. The lab, four-leg and T-junction runs on equal their
  off runs.
- **Open:** Finding 2 (seed 42: 13 clamps) and Finding 3 (the swap lock) are not addressed. The
  `followerFilter` window-end lookup is recorded in the evidence.
- **Verified (Linux, headless only):** `ctest --preset headless` 174/174 and `check` pass. There was
  no desktop build here: Qt is not installed in this container.

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
