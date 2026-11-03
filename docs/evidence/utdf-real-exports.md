# Real UTDF exports against the U0 assumptions — 2026-11-03

The owner supplied files from two of their own studies. These were real projects, not the UTDF_PROBE §1
reference junction:
- a Synchro project and its UTDF export;
- a Vistro 2026 project, its UTDF export, its native `.vistromerge` network export and an SVG drawing.

Their licence and project data are unknown, so **none of them is committed**. This note records
only section names, field names and codes, as UTDF_PROBE §4.6 allows. No coordinate, volume or
timing value from the files appears here.

## The files

| Writer | File | Header facts |
|---|---|---|
| Synchro 10.1 (`%%Synchro10.1%%` binary) | UTDF combined CSV | `UTDFVERSION,8`, `Metric,1`, `ScenarioDate`; 1 signalised node |
| PTV Vistro 2026.00-08 | UTDF combined CSV | `UTDFVERSION,8`, `Metric,0` (feet), `ScenarioTime`; 6 signalised nodes |
| PTV Vistro 2026.00-08 | `.vistromerge` (Visum net format, version 19) | `$NETWORK:…;LEFTHANDTRAFFIC;…` = `1` |
| Synchro 12.0 | `.syn` | Encrypted (`Salted__`); not readable |

## Sections and fields

Both writers emit the six sections in this order: `[Network] [Nodes] [Links] [Lanes] [Timeplans] [Phases]`.

- **Shared by both writers:**
  - Network records `UTDFVERSION Metric yellowTime allRedTime Walk DontWalk HV PHF DefWidth DefFlow
    vehLength heavyvehlength criticalgap followuptime stopthresholdspeed criticalmergegap growth PedSpeed
    LostTimeAdjust`.
  - Node columns `INTID TYPE X Y Z DESCRIPTION CBD Inside Radius Outside Radius Roundabout Lanes Circle Speed`.
  - Links records `Up ID Lanes Name Distance Speed Time Grade Median Offset TWLTL Crosswalk Width
    Mandatory Distance(2) Positioning Distance(2) Curve Pt X/Y/Z Link Is Hidden Street Name Is Hidden`.
- **Synchro 10.1 only:**
  - Movement columns `NBL…WBR`, plus `NEU NEL NET NER SWU SWL SWT SWR` for the diagonal legs it uses.
  - Phase columns `D1…D8`.
  - `Phase1` and `PermPhase1`.
- **Vistro only:**
  - Movement columns `U L2 L T R R2` for each of 8 compass directions.
  - Phase columns `D1…D32`.
  - `Phase1…4` and `PermPhase1…4`.
  - Extra records:
    - Lanes: `Parking`, `Right Radius`, `Add Lanes`, `ExtendPhase`, detectors 1–5 and `Traffic in shared lane`.
    - Phases: `StartTime`, `MaxInitial`, `AddedMin`, `MinActuation`, `MaxActuation`, `MaxExtension`, `ReduceBy` and `ReduceEvery`.
    - Timeplans: `Node 0…7`.

## Codes observed

| Assumption (UTDF_PROBE §5) | Observed | Status |
|---|---|---|
| `UTDFVERSION,8`; `Metric,1` = metric | Both writers; Synchro metric 1, Vistro 0 with feet defaults | Confirmed |
| Node `TYPE` 0 = signalised, 1 = external | 0 is the only type with timeplans. 3 carries `SignControl` rows, so it is unsignalised. 1 and 2 have neither; external and bend are likely but not shown | 0 confirmed, 3 new, 1/2 inferred |
| `Control Type` 0 = pretimed | 0 on a pretimed plan | Confirmed |
| `Reference Phase` 206 = phases 2 and 6 | Single phases (`2`, `6`) also written | 206 still unverified; a single phase is valid |
| `Recall` 3 | Used on the Synchro plan's phases | Value present; its meaning (max) not shown |
| `Start`/`End` bound the split; `Yield` = end of green | `End` of a phase ending at the cycle wrap is written `0`; `Yield = End − Yellow − AllRed` holds | Confirmed |
| A shared lane is coded on the through movement | Vistro also writes `Shared` on a right-turn column | Differs: not only on the through movement |
| Links `Lanes` counts full lanes (2), not pockets | No case settles it | Open |
| Movement columns named by compass and geometry, whatever the driving side | No driving-side field in either UTDF file | Inferred |
| Left-hand setting carried by the file | Vistro has the setting (`LEFTHANDTRAFFIC` = 1 in `.vistromerge`); its UTDF has no such field | **Not carried**: a reader must be told the side |
| Far turn in `NBR` or `NBL` | Needs the §1 junction | Open |

## What remains for U0

- The §1–§3 junction drawn in Synchro (12.0 or 10.1, whichever is used), with its metric export and HCM 7 report.
- The import of `tests/data/utdf/u0-handwritten.csv` into each program.
- The far-turn column under left-hand traffic.
- Whether Synchro has a left-hand setting.
