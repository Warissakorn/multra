# UTDF export for Synchro 12 and Vistro 2026 (M14, D386, D387)

Contract before code (D145), written 2026-10-28; U1, the geometry writer, shipped 2026-11-03 (§7). The owner approved exporting the
network built for HCM to PTV Vistro 2026 and Synchro 12 (D386). This is the competitor-format
scope revision that ROADMAP M1.23 requires; imports from those programs stay out. The editor side
is [JUNCTION_TEMPLATES](JUNCTION_TEMPLATES.md); Road became Street segment in slice SR (D389). U0 (§8) fixed §3's names from files the owner's
programs write and read. U1 writes [Network], [Nodes], [Links] and [Lanes] geometry
(`src/project/utdf_export.hpp`, `multra-cli --export-utdf`). Volumes, timing, the desktop dialog,
UTM and the Vistro median patch are U2. No MULTRA-written file has been imported by the owner yet (UT10).

## 1. Why UTDF

- Vistro's `.vistro` and Synchro's `.syn` are closed formats; MULTRA does not write them.
- **UTDF** (Universal Traffic Data Format) is Synchro's documented text interchange: CSV
  sections for network settings, nodes, links, lanes and volumes, timing plans and phases.
  Synchro reads it natively. Vistro offers a Synchro import; whether Vistro 2026 reads the UTDF
  combined file, and which sections, is U0's first question.
- One writer serves both programs. SIDRA and Aimsun stay out until the owner asks (D386).

## 2. What is exported

From the saved document, read only, never changed by the export:
- every Node with Micro detail and a valid `approachLayout` (JUNCTION_TEMPLATES §6);
- the Links between those Nodes, and the external ends of their legs;
- lanes, storage, taper, widths, posted speed, names, coordinates;
- volumes, PHF and %HV from `Node::analysis` when present;
- for signalised Nodes, the controller's timing through `nemaCheck` (JUNCTION_TEMPLATES §8).

Not exported: vehicle inputs, routes, Micro behaviour, conflict areas, Macro and Meso data, and
results. A Node outside the rules is left out and named in the export report, never guessed.

## 3. Sections (seen in Synchro and Vistro files, fixed by U0)

UTDF's combined file is one CSV with six bracketed sections. Each section starts with a title line
and a header row. Rows are keyed by record name and intersection id (`INTID`), and the cells run by
direction, movement or phase.

**Source, 2026-10-28.** The names below are copied from a Synchro-written UTDF 8 file, imperial and
right-hand, in the MIT-licensed `utdf2gmns` 1.2.5 package from PyPI (`engine/sigma_x/UTDF8.csv`).
- The file is CRLF, with no BOM.
- Synchro's own documentation and PTV's Visum notes were not reachable from the session.
- The file is not vendored here.

| Section, title line | Header | MULTRA writes (source) |
|---|---|---|
| `[Network]`, `Network Settings` | `RECORDNAME,DATA` | `UTDFVERSION` 8, `Metric` 1 (§4), and the defaults the project has: `yellowTime`, `allRedTime`, `HV` (a fraction), `PHF`, `DefWidth`, `DefFlow` |
| `[Nodes]`, `Node Data` | `INTID,TYPE,X,Y,Z,DESCRIPTION,CBD,Inside Radius,Outside Radius,Roundabout Lanes,Circle Speed` | the Node number (§5), `TYPE` (0 signalised, 1 external; unsignalised unknown), X and Y (§4), the name, `CBD` |
| `[Links]`, `Link Data` | `RECORDNAME,INTID,NB,SB,EB,WB,NE,NW,SE,SW` | for each Node's inbound Link by direction: `Up ID`, `Lanes` (full lanes plus pockets at the stop line, D440; Vistro reads it, row 17), `Name`, `Distance`, `Speed`, `Time`, `Grade`, `Median`. External ends list the Link arriving at them |
| `[Lanes]`, `Lane Group Data` | `RECORDNAME,INTID,NBL,NBT,NBR,SBL,…,WBR,PED,HOLD` (the sample adds `NE…`/`SW…` and second-turn columns such as `EBL2`) | `Up Node`, `Dest Node`, `Lanes`, `Shared`, `Width`, `Storage`, `Taper`, `StLanes` (on the turn column that owns the pocket; Vistro reads it there, though its writer puts it on the through column), `Grade`, `Speed`, `Phase1`, `PermPhase1`, `IdealFlow`, `Volume`, `PHF`, `HeavyVehicles` (per cent), `BusStops` |
| `[Timeplans]`, `Timing Plan Settings` | `RECORDNAME,INTID,DATA` | every Synchro 10.1 record in its order: `Control Type`, `Cycle Length`, `Reference Phase` and `Offset` authored, the rest per D439 |
| `[Phases]`, `Phasing Data` | `RECORDNAME,INTID,D1,…,D8` | every Synchro 10.1 record in its order: `BRP`, `MinGreen`, `MaxGreen`, `Yellow`, `AllRed`, `Recall`, `Start`, `End` and `Yield` authored, the rest per D439 |

The sample carries more rows than MULTRA authors. MULTRA leaves out the detector rows, `Growth`,
`LostTime`, `Allow RTOR`, `SatFlow*`, `Lane Group Flow`, the walk and pedestrian rows, the Link
mandatory and positioning distances, and the curve points. The curve points are written when U1
reaches them (§4). The codes the hand-written file guesses (shared lanes, control type, recall,
`BRP`) are listed in [UTDF_PROBE §5](../plans/UTDF_PROBE.md#5-what-the-hand-written-file-assumes-checked-by-step-4).

A field MULTRA has no value for is written empty, so the receiving program applies its own default;
the export report lists each one. The exception is the timing records, which Vistro refuses when
empty: they get D439's neutral pretimed values, also listed.

**Vistro import (U0, 2026-11-03):** `u0-handwritten.csv`'s geometry, lanes, pockets and volumes
import; its timing is refused (`CellNotFound: Row not found: Row = 3, Column = 1`). It lacks
`[Timeplans]` and `[Phases]` records that both real writers emit, in a fixed order
([utdf-real-exports](../evidence/utdf-real-exports.md)). The v2 probe files (UTDF_PROBE §6 row 13)
answered this the same day (UTDF_PROBE §6 row 13): **every record must be present and non-empty.**
v2-empty is refused at the first empty cell (`VehExt … D1 is invalid`); v2-filled imports the
cycle, rings and splits exactly as sheeted.

**Timing records MULTRA does not author (D439)** are written with neutral pretimed values, all
listed in the export report:
- **Constants:**
  - `Lock Timings`, `Referenced To`, `Master` and `Yield` (Timeplans): 0.
  - `VehExt`, `TimeBeforeReduce`, `TimeToReduce` and `MinGap`: 0.0.
  - `PedCalls` 0, `DualEntry` 0, `InhibitMax` 1.
  - `Walk` and `DontWalk` stay empty; v2-filled imported with them empty.
- **Copies of authored values:**
  - `MinSplit` = split; `ActGreen` = green.
  - `Yield170` and `LocalYield170` = `Yield`; `LocalStart` = `Start`; `LocalYield` = `Yield`.
- **Records in Synchro 10.1's order:** `Node 0` = the Node's number and `Node 1` = 0.

None of these values changes a fixed-time plan. Vistro's own extra records (`StartTime`,
`MaxInitial` …) are not written: v2-filled imported without them.

Links `Median` is not shown by Vistro after import (2.0 m on Main St, owner 2026-11-03); MULTRA
still writes it, and the report says Vistro drops it.

No UTDF record names the HCM method, in either writer's files. Vistro opens an import as HCM 2010;
the user sets HCM 7th in the program after import (owner, 2026-11-03). The export report should say so.

## 4. Units and coordinates

- **Metric** (metres, km/h), the file declaring it, so no conversion is written. Vistro reads
  `Metric,1`, as `u0-handwritten.csv` showed (2026-11-03), although its own writer always emits
  feet ([utdf-real-exports](../evidence/utdf-real-exports.md)).
- **X, Y:** world metres. When the project has a map reference (MAP_BACKGROUND, schema 29), the
  UTM easting and northing are added, so both programs' backgrounds align (§9 UX-3).
- Distances are centreline lengths between Node centres along the Links; curve points are the
  Link's interior vertices.

## 5. Node and Link numbers

UTDF identifies intersections by integer. MULTRA's ids are strings and a re-export must give the
same numbers, or a study re-imported into Synchro mismatches its edits.
- **Stored (§9 UX-1):** an optional positive integer `number` on the Node, written only when set
  (schema 65, D442; a repeat is `NODE_NUMBER_DUPLICATE`). Applying a junction template assigns the next free one; *Number Nodes* fills the rest in one
  Undo step. Export refuses an unnumbered Node with `UTDF_NODE_UNNUMBERED`, naming the fill.
- **External ends** (a leg's far end) take derived numbers above the largest Node number, in
  Link-id order; they are not stored, as nobody edits them in the target.

## 6. Signals and driving side

- Only **pretimed** controllers are exported as timed; their phases come from `nemaCheck`. A
  controller that fails it is `UTDF_NEMA_INVALID` with the check's codes; the Node is exported as
  unsignalised only if the user confirms, never silently.
- Two-way stop: control type unsignalised with Stop on the minor approaches; all-way stop likewise.
- **Left-hand traffic is the main risk.** HCM and NEMA numbering assume right-hand traffic.
  What U0 settled (2026-11-03, [utdf-real-exports](../evidence/utdf-real-exports.md)):
  - **Synchro has no left-hand setting** (owner). It models right-hand traffic only.
  - **Vistro 2026 has one** (`LEFTHANDTRAFFIC` in its own network file), but its UTDF export drops it,
    and **importing UTDF switches a left-hand project to right-hand** (owner, 2026-11-03). The export
    report must tell the user to set left-hand again after import in geometric mode.
  - **A UTDF import can never be left-hand in Vistro** (U0 rows 18–20). Merging a patch sets
    the median (`$LEG` `HASCISLAND`/`CISLANDWIDTH`) but not `LEFTHANDTRAFFIC` or the HCM method.
    Vistro also refuses to switch to left-hand while a right-hand junction exists.
  - **Vistro route (D441):** the UTDF file plus a `.vistromerge` patch for Vistro's *Merge Data*,
    carrying the medians (`$LEG` `HASCISLAND`/`CISLANDWIDTH`; proven after the import, row 20).
    - The patch must address the link numbers Vistro itself gives after the import (rows 19–20).
      U1's contract must settle how: from Vistro's numbering rule, or by asking the user to export
      *Merge Data* first. If neither works, the median becomes a report step.
    - A whole network in Vistro's own format does not work: Merge Data duplicates the junction and
      ignores `LEFTHANDTRAFFIC` (row 21).
    - **For a correct HCM treatment of the Thai far turn, the user picks mirrored mode.** In
      geometric mode Vistro analyses the network as right-hand.
    - The report tells the user to set HCM 7th after import.
  - **The UTDF file never carries the driving side**: neither writer has a record for it.
- **The user chooses a mode at export (D437):**
  - **Geometric (default).** Vistro's own writer does the same with left-hand traffic on (far turn
    under `R`, 2026-11-03).
    - Real coordinates and compass legs.
    - Movement names stay geometric (L/T/R), so the far turn of left-hand traffic stays in `R`.
    - Phase numbers follow JUNCTION_TEMPLATES §8's left-hand rule.
    - The report, and a comment line if the readers accept one (U0 import check), say the network is left-hand.
    - A right-hand-only reader will misread `R` as the kerb turn.
  - **Mirrored.**
    - The network is reflected into its right-hand equivalent before writing, so the far turn is in `L`.
    - NEMA numbering is the right-hand standard, and HCM in Synchro treats the far turn correctly.
    - Compass legs swap across the mirror axis. The axis is fixed in U1's contract; the default is x → −x, so E↔W, and NE↔NW, SE↔SW.
    - The report says the map is mirrored.

## 7. Slices

| Slice | Scope | Rows |
|---|---|---|
| U0 | Probe (§8); this page's §3 fixed from real files; left-hand answer | UT0 |
| U1 (done 2026-11-03) | Pure writer `utdfDocument(document, mode)` in `src/project/`: Network, Nodes, Links, Lanes geometry, geometric and mirrored; Node `number` (schema 65); CLI `--export-utdf`, `--utdf-mode` | UT0–UT5, UT11 geometry |
| U2 | Lanes volumes, Timeplans, Phases (right-hand NEMA numbers when mirrored); *Number Nodes* and numbers on template Apply; desktop *Export → UTDF…* with the report dialog, UTM (PROJ), atomic write as the CSV tables; the Vistro median patch once its link numbers are settled (D441) | UT6–UT9, UT11 timing, UT12 |
| U3 | Round trip on the owner's machine (OWNER_SITTING item), fixes | UT10 |

### U1 contract (D442, 2026-11-03)

- **Call.** `utdfDocument(document, mode)` in `src/project/utdf_export.hpp` returns `{csv, report}`.
  - It is pure: it reads the document and never changes it (UT5).
  - The CSV is CRLF, has no BOM, and uses the C locale.
  - Each report note is `{code, node, text}`.
  - `mode` is geometric (default) or mirrored (D437); both are in U1.
- **Which Nodes are written.** A Node is written when it has Micro detail (`nodeHasDetail`), a valid
  `approachLayout`, and a stored `number` (§5). Any other Node is left out with one note (UX-4a):
  - `UTDF_NO_DETAIL`;
  - `UTDF_LAYOUT_INVALID`, followed by the layout's codes;
  - `UTDF_NODE_UNNUMBERED`.
- **Legs.**
  - **Walking a leg.** A written Node's in-leg is walked upstream through the unique Connector
    that feeds it, provided that Connector lies outside every Node. The walk stops at the out-leg of
    another written Node, which becomes the upstream Node. Otherwise it stops at the first Link's
    start, which is an **external end**. Out-legs are walked downstream the same way.
  - **External ends and their numbers.**
    - An in-leg and the out-leg leaving in the opposite compass direction share one external end,
      placed at the in-leg's start.
    - External ends are numbered above the largest Node number, in the order of the Link id that
      keys them (the in-leg's first Link, or the out-leg's last Link).
  - **Distance** runs along the leg's Link polylines, from the upstream Node's centroid (or the
    external end) to the Node's centroid, with straight joins across Connectors.
- **Values and rounding.**
  - **[Network]:** `UTDFVERSION` 8, `Metric` 1, `DefWidth` and `DefFlow`, written empty. The
    project has no network-wide default for either, so the report lists both. Yellow, all-red, HV
    and PHF are U2.
  - **[Nodes]:**
    - X and Y are the polygon's area centroid in world metres, to 0.1. UTM waits for U2 on the
      desktop, because PROJ is not in the headless build (D442), and the report says so.
    - `TYPE`: 0 when the membership is signalised; 3 when it is unsignalised (Vistro's code, which
      [utdf-real-exports](../evidence/utdf-real-exports.md) records); 1 for an external end.
    - `DESCRIPTION` is the Node's name. `CBD` comes from `analysis`; it is empty without one.
  - **[Links]**, written per in-leg under its compass column:
    - `Up ID`;
    - `Lanes`: the in-Link's lanes at the stop line, pockets included (D440);
    - `Name`: the Street segment's name, or else the Link's;
    - `Distance`, to 1 m;
    - `Speed`: the in-Link's `speedLimit`;
    - `Time` = Distance × 3.6 / Speed, to 0.1 s;
    - `Grade`: from `analysis`;
    - `Median`: the Street segment's override, or else its cross-section's, to 0.1.

    External ends list the out-leg that arrives at them. NE, NW, SE and SW stay empty, because
    layouts have only four compass directions.
  - **[Lanes]**, by movement:
    - `Up Node` and `Dest Node` are written for every movement the layout has.
    - **Grouping.** A lane serving the through movement belongs to T. A lane serving only one turn
      belongs to that turn. A lane serving both turns and no through movement belongs to L.
    - `Lanes` is the group's count, 0 included.
    - `Shared` is written when the count is above 0:
      - on T, 1 if a T lane also serves L, plus 2 if one also serves R;
      - on L, 2 if an L lane also serves R;
      - on R, 0.
    - `Width` is the mean width of the group's lanes, or of the lanes serving the movement when the
      group is empty.
    - `Storage`, `Taper` and `StLanes` go on the column of the group that owns the pocket (D440).
    - `Grade` and `Speed` go on T, or on the approach's first movement when it has no T.
  - Lengths are to 0.1 m, widths to 0.01 m, and trailing zeros are trimmed. `Time` and `Median`
    keep one decimal place.
- **Empty values.** Every cell the contract authors that has no value is written empty and listed
  as `UTDF_FIELD_EMPTY` with its record and column (UT4). Cells for directions or movements that
  do not exist are not listed.
- **Mode notes.**
  - **Mirrored:** x → −x; E and W swap in every column name; L and R swap in movement names. The
    report says the map is mirrored. Right-hand NEMA numbers come with Phases in U2.
  - **Geometric, left-hand network:** the report says the network is left-hand and the far turn is
    under `R`.
- **Not in U1:** Node numbering in the editor (*Number Nodes*, numbers on template Apply), volumes,
  timing, UTM and the Vistro median patch. They are U2; the patch's link numbers are still open
  (D441).

## 8. U0: the probe (first, before any writer)

**Prepared 2026-10-28; waiting on the owner.** The sheet is [UTDF_PROBE](../plans/UTDF_PROBE.md),
MULTRA's hand-written file is [`tests/data/utdf/u0-handwritten.csv`](../../tests/data/utdf/README.md),
and the sitting items are OWNER_SITTING I. The steps:

1. The session writes a **reference junction** sheet: the Thai four-leg fixture of
   JUNCTION_TEMPLATES JT6 (two crossing Street segments, kerb pockets, a shared kerb-through lane), with
   volumes and a P2 timing.
2. The owner draws it in **Vistro 2026** with left-hand traffic on, exports UTDF (combined CSV,
   metric) as the golden file `u0-vistro.csv`, and saves Vistro's HCM report. This was revised
   2026-11-03 (D438): the owner cannot create UTDF in Synchro, and Synchro is right-hand only.
   Synchro's dialect comes from a real Synchro 10.1 export
   ([utdf-real-exports](../evidence/utdf-real-exports.md)).
3. If Synchro can import UTDF, the owner imports the golden file into it and notes what came
   through and what was dropped.
4. The owner checks the left-hand setting in both programs (§6).
5. The session hand-writes the same junction as MULTRA would and the owner imports it into both,
   to find the first difference.

The golden file goes to `tests/data/utdf/` only if the owner agrees that it may be redistributed
(GPL repository); otherwise only the field names and units are recorded here.

## 9. Owner choices (answered 2026-10-28, D388: UX-1a to UX-4a)

1. **UX-1 Node numbers.**
   - **(a) Stored optional `number`, assigned by template Apply and *Number Nodes* (recommended).**
   - (b) Derived on each export (sorted ids): no schema step, but numbers move when Nodes change.
2. **UX-2 Volumes.**
   - **(a) From `Node::analysis` only; a Node without it exports geometry and timing (recommended).**
   - (b) Also from Micro vehicle inputs and routes (*Fill from model*, a later slice).
3. **UX-3 Coordinates.**
   - **(a) UTM when the project has a map reference, else world metres (recommended).**
   - (b) Always world metres.
4. **UX-4 A Node the rules cannot export.**
   - **(a) Leave it out and list it in the report; the export still writes (recommended).**
   - (b) Refuse the whole export.

5. **UX-5 Left-hand network** (answered 2026-11-03, D437): the user picks **geometric** or
   **mirrored** at export (§6); geometric is the default.
6. **UX-6 What UTDF loses in Vistro** (answered 2026-11-03, D441):
   - **Medians:** a `.vistromerge` median patch alongside the UTDF file.
   - **Left-hand:** mirrored mode when HCM must treat the far turn correctly.
   - **HCM 7th:** a report step after import.

## 10. Acceptance rows

UT0–UT5 and UT11's geometry pass since U1 (2026-11-03) in `tests/utdf_export_tests.cpp` (group
`utdf`) and `cli-utdf` (`tests/utdf_cli.cmake`). UT1's U0 junction reproduces every Node 1
`[Links]` line and the ten `[Lanes]` geometry lines of `u0-handwritten-v3.csv`, which Vistro
imported as sheeted. The CLI fixture and its expected files are
`tests/data/utdf/u1-u0-junction*.{traffic.json,csv}`. UT6–UT10 and UT12 are planned.

| Row | Asserts |
|---|---|
| UT0 | U0's golden file (`u0-vistro.csv`, D438; committed 2026-11-03) parses with the writer's section reader, and every field §3 names is in it. Its timing is Vistro's own numbering and splits, so UT0 compares fields and codes, not timing values |
| UT1 | A two-Node corridor exports Nodes, Links and Lanes with the numbers, units and distances by hand |
| UT2 | Re-exporting an unchanged document gives the same bytes; adding a Node keeps the others' numbers |
| UT3 | Every refusal (unnumbered, ambiguous direction, missing layout) names its Node and writes nothing for it |
| UT4 | Empty fields are listed in the report; every value is either authored or one of D439's neutral pretimed values, each listed in the report |
| UT5 | The export changes no byte of the document |
| UT6 | Lanes volumes, PHF and %HV equal `Node::analysis` per movement |
| UT7 | Each preset's Phases rows equal the controller's greens, yellows and all-reds |
| UT8 | A controller failing `nemaCheck` is refused with its codes |
| UT9 | The file written by the desktop equals the CLI's |
| UT11 | A left-hand fixture exported geometric keeps compass legs and puts the far turn in `R`; mirrored swaps E↔W legs, puts it in `L` and uses right-hand NEMA numbers; one golden file per mode (D437) |
| UT12 | The Vistro median patch has one `$LEG` row per junction leg with a median, addressed by Vistro's post-import link numbers (D441), and the report lists it with the HCM 7th and right-hand notes |
| UT10 | (owner) Vistro 2026, and Synchro if it imports UTDF (D438), import the fixture; geometry, lanes, volumes and timing match the sheet |
