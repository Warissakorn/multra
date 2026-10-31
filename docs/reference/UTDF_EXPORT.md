# UTDF export for Synchro 12 and Vistro 2026 (M14, D386, D387)

Contract before code (D145), docs only, written 2026-10-28. The owner approved exporting the
network built for HCM to PTV Vistro 2026 and Synchro 12 (D386). This is the competitor-format
scope revision that ROADMAP M1.23 requires; imports from those programs stay out. The editor side
is [JUNCTION_TEMPLATES](JUNCTION_TEMPLATES.md); Road became Street segment in slice SR (D389). **Nothing here is implemented.** §3's
names come from one Synchro UTDF 8 file; no import by either program has been verified. U0 (§8)
fixes them from files the owner's programs write and read.

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

## 3. Sections (seen in one Synchro file, fixed by U0)

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
| `[Links]`, `Link Data` | `RECORDNAME,INTID,NB,SB,EB,WB,NE,NW,SE,SW` | for each Node's inbound Link by direction: `Up ID`, `Lanes`, `Name`, `Distance`, `Speed`, `Time`, `Grade`, `Median`. External ends list the Link arriving at them |
| `[Lanes]`, `Lane Group Data` | `RECORDNAME,INTID,NBL,NBT,NBR,SBL,…,WBR,PED,HOLD` (the sample adds `NE…`/`SW…` and second-turn columns such as `EBL2`) | `Up Node`, `Dest Node`, `Lanes`, `Shared`, `Width`, `Storage`, `Taper`, `StLanes`, `Grade`, `Speed`, `Phase1`, `PermPhase1`, `IdealFlow`, `Volume`, `PHF`, `HeavyVehicles` (per cent), `BusStops` |
| `[Timeplans]`, `Timing Plan Settings` | `RECORDNAME,INTID,DATA` | `Control Type`, `Cycle Length`, `Reference Phase`, `Offset` |
| `[Phases]`, `Phasing Data` | `RECORDNAME,INTID,D1,…,D8` | `BRP`, `MinGreen`, `MaxGreen`, `Yellow`, `AllRed`, `Recall`, `Start`, `End`, `Yield` |

The sample carries more rows than MULTRA authors. MULTRA leaves out the detector rows, `Growth`,
`LostTime`, `Allow RTOR`, `SatFlow*`, `Lane Group Flow`, the walk and pedestrian rows, the Link
mandatory and positioning distances, and the curve points. The curve points are written when U1
reaches them (§4). The codes the hand-written file guesses (shared lanes, control type, recall,
`BRP`) are listed in [UTDF_PROBE §5](../plans/UTDF_PROBE.md#5-what-the-hand-written-file-assumes-checked-by-step-4).

A field MULTRA has no value for is written empty, so the receiving program applies its own default
and says so; the export report lists each one. MULTRA never writes a value it did not author.

## 4. Units and coordinates

- **Metric** (metres, km/h), the file declaring it, so no conversion is written.
- **X, Y:** world metres. When the project has a map reference (MAP_BACKGROUND, schema 29), the
  UTM easting and northing are added, so both programs' backgrounds align (§9 UX-3).
- Distances are centreline lengths between Node centres along the Links; curve points are the
  Link's interior vertices.

## 5. Node and Link numbers

UTDF identifies intersections by integer. MULTRA's ids are strings and a re-export must give the
same numbers, or a study re-imported into Synchro mismatches its edits.
- **Stored (§9 UX-1):** an optional integer `number` on the Node, written only when set, at the
  next schema step. Applying a junction template assigns the next free one; *Number Nodes* fills the rest in one
  Undo step. Export refuses an unnumbered Node with `UTDF_NODE_UNNUMBERED`, naming the fill.
- **External ends** (a leg's far end) take derived numbers above the largest Node number, in
  Link-id order; they are not stored, as nobody edits them in the target.

## 6. Signals and driving side

- Only **pretimed** controllers are exported as timed; their phases come from `nemaCheck`. A
  controller that fails it is `UTDF_NEMA_INVALID` with the check's codes; the Node is exported as
  unsignalised only if the user confirms, never silently.
- Two-way stop: control type unsignalised with Stop on the minor approaches; all-way stop likewise.
- **Left-hand traffic is the main risk.** HCM, NEMA numbering and both programs are written for
  right-hand traffic. The sample's `[Network]` section has no driving-side record. U0 asks the
  owner three things about Synchro 12 and Vistro 2026 (UTDF_PROBE §4 step 5):
  - whether each program has a left-hand setting;
  - whether the file carries it;
  - whether the far turn is then written under `R` or `L`. Until answered, the movement
  names are geometric (L/T/R) and the phase numbers follow JUNCTION_TEMPLATES §8's left-hand rule.

## 7. Slices

| Slice | Scope | Rows |
|---|---|---|
| U0 | Probe (§8); this page's §3 fixed from real files; left-hand answer | UT0 |
| U1 | Pure writer `utdfDocument(document)` in `src/project/`: Network, Nodes, Links, Lanes geometry; Node `number`; CLI `--export-utdf` | UT1–UT5 |
| U2 | Lanes volumes, Timeplans, Phases; desktop *Export → UTDF…* with the report dialog, atomic write as the CSV tables | UT6–UT9 |
| U3 | Round trip on the owner's machine (OWNER_SITTING item), fixes | UT10 |

## 8. U0: the probe (first, before any writer)

**Prepared 2026-10-28; waiting on the owner.** The sheet is [UTDF_PROBE](../plans/UTDF_PROBE.md),
MULTRA's hand-written file is [`tests/data/utdf/u0-handwritten.csv`](../../tests/data/utdf/README.md),
and the sitting items are OWNER_SITTING I. The steps:

1. The session writes a **reference junction** sheet: the Thai four-leg fixture of
   JUNCTION_TEMPLATES JT6 (two crossing Street segments, kerb pockets, a shared kerb-through lane), with
   volumes and a P2 timing.
2. The owner draws it in **Synchro 12**, exports UTDF (combined CSV, metric), and saves it as the
   golden file, with Synchro's HCM report.
3. The owner imports that file into **Vistro 2026** and notes what came through, what was dropped,
   and the HCM result.
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

## 10. Acceptance rows (planned)

| Row | Asserts |
|---|---|
| UT0 | U0's golden file parses with the writer's section reader, and every field §3 names is in it |
| UT1 | A two-Node corridor exports Nodes, Links and Lanes with the numbers, units and distances by hand |
| UT2 | Re-exporting an unchanged document gives the same bytes; adding a Node keeps the others' numbers |
| UT3 | Every refusal (unnumbered, ambiguous direction, missing layout) names its Node and writes nothing for it |
| UT4 | Empty fields are listed in the report; no field carries a value MULTRA did not author |
| UT5 | The export changes no byte of the document |
| UT6 | Lanes volumes, PHF and %HV equal `Node::analysis` per movement |
| UT7 | Each preset's Phases rows equal the controller's greens, yellows and all-reds |
| UT8 | A controller failing `nemaCheck` is refused with its codes |
| UT9 | The file written by the desktop equals the CLI's |
| UT10 | (owner) Synchro 12 and Vistro 2026 import the fixture; geometry, lanes, volumes and timing match the sheet |
