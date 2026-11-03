# UTDF probe — the U0 reference junction (M14, UTDF_EXPORT §8)

Written 2026-10-28 for the owner's Synchro 12 and Vistro 2026; since D438 (2026-11-03) Vistro writes
the golden file. The answers fix
[UTDF_EXPORT](../reference/UTDF_EXPORT.md) §3 (field names, units) and §6 (left-hand traffic) before
any writer code (D145). The sitting items are [OWNER_SITTING I](OWNER_SITTING.md#i--utdf-probe-m14-u0);
this sheet holds the junction and the answer lines. MULTRA's own attempt at the file is
[`tests/data/utdf/u0-handwritten.csv`](../../tests/data/utdf/u0-handwritten.csv).

## 1. The junction

Left-hand traffic (Thailand). Main St runs east–west and Side St north–south; they cross at
right angles. It is JUNCTION_TEMPLATES JT6's fixture: a far-turn pocket, a kerb-turn pocket and a
shared kerb-through lane. *Far turn* is the right turn here (it crosses the opposing traffic);
*kerb turn* is the left turn.

| | Main St (EB, WB) | Side St (NB, SB) |
|---|---|---|
| Cross-section between junctions | 2 + 2, divided, median 2.0 m | 2 + 2, undivided |
| Lane width | 3.25 m | 3.25 m |
| Leg length (junction centre to the end node) | 300 m | 300 m |
| Posted speed | 60 km/h | 50 km/h |
| Grade | 0 % | 0 % |
| Lanes at the stop line, far side to kerb | right-turn pocket · through · through + left | through + right · through · left-turn pocket |
| Pocket | right turn, storage 60 m, taper 20 m | left turn, storage 40 m, taper 15 m |

```
                 N (node 2)
                     |  SB: [L pocket][T][T+R]   (kerb ... far)
                     |
 W (node 5) -------- 1 -------- E (node 3)
  EB: [T+L][T][R pocket]   (kerb ... far)
                     |
                 S (node 4)
```

**Node numbers** (UTDF_EXPORT §5): the junction is 1; the end nodes are 2 north, 3 east, 4 south,
5 west. Coordinates in metres: 1 at (1000, 1000), each end node 300 m out along its axis.

## 2. Volumes (veh/h, the analysis hour)

| Approach | Left (kerb) | Through | Right (far) |
|---|---|---|---|
| EB (from 5) | 120 | 900 | 200 |
| WB (from 3) | 100 | 850 | 180 |
| NB (from 4) | 90 | 300 | 110 |
| SB (from 2) | 80 | 280 | 120 |

Total 3,330 veh/h. Every movement: PHF 0.92, heavy vehicles 5 %, ideal saturation flow
1,900 pc/h/ln, no parking, no buses stopping, not CBD.

## 3. Signal timing (P2, pretimed)

Cycle 100 s, offset 0, referenced to the start of phases 2 and 6. Every phase has 3.0 s yellow and
1.0 s all-red. Phases 1 and 5 lead: they run at the end of the cycle, just before 2 and 6.

| Phase | Movements | Type | Green | Split | Start → end |
|---|---|---|---|---|---|
| 1 | EB right (far) | protected only | 10 s | 14 s | 86 → 100 |
| 2 | WB through + WB left | — | 40 s | 44 s | 0 → 44 |
| 4 | SB through + right; SB left pocket permitted | — | 38 s | 42 s | 44 → 86 |
| 5 | WB right (far) | protected only | 10 s | 14 s | 86 → 100 |
| 6 | EB through + EB left | — | 40 s | 44 s | 0 → 44 |
| 8 | NB through + right; NB left pocket permitted | — | 38 s | 42 s | 44 → 86 |

The numbering follows Synchro's own convention seen in a Synchro UTDF file (UTDF_EXPORT §3):
2 is WB through, 6 is EB through, 4 is SB, 8 is NB. Phase 1 is the far turn that crosses phase 2.
In left-hand traffic, that is EB's right turn. The Side St right turns are permitted against the
opposing through; that is HCM's permitted far turn.

## 4. What the owner does

**Revised 2026-11-03 (D438):** the owner cannot create UTDF in Synchro, and Synchro is right-hand only.
So **Vistro 2026 writes the golden file**. Synchro's dialect is recorded from a real Synchro 10.1
export ([utdf-real-exports](../evidence/utdf-real-exports.md)), and Synchro is only an import check,
if it can import UTDF. Menu names are guesses; correct them on the answer lines.

1. **Draw in Vistro 2026.**
   - Start a new project.
   - Turn left-hand traffic on and use metric units.
   - Draw §1, enter §2 and §3, and run the HCM 7 signalised report.
   - Save the report as PDF or text: delay and LOS per approach and for the junction.
2. **Export.** Export UTDF (combined CSV), metric, as `u0-vistro.csv`.
3. **Far-turn column.** Find which column `u0-vistro.csv` holds the far turn (EB right, phase 1) in: `EBR` or `EBL`.
   If Vistro lets you, export the same junction again with left-hand traffic off, and note whether
   the column changes.
4. **Import MULTRA's file.** Import
   [`u0-handwritten.csv`](../../tests/data/utdf/u0-handwritten.csv) into Vistro. Note the first error,
   or the first place where the drawing differs from §1–§3.
5. **Synchro, if it can import UTDF.** If Synchro has a UTDF import (perhaps *Transfer → Load/Merge
   UTDF*), import `u0-vistro.csv` and `u0-handwritten.csv`. Note the first error or difference.
   If it has no UTDF import, write "no import".
6. **Licence.** Can `u0-vistro.csv` be committed to this GPL repository?
   - If yes, send the file.
   - If not, send only its section and field names. Units and values may be blanked.

## 5. What the hand-written file assumes (checked by step 4)

Each line below is a guess, taken from one Synchro UTDF 8 file written by a right-hand, imperial
network. The owner's file settles each one.

- `UTDFVERSION,8` and `Metric,1` declare the version and metric units (m, km/h).
- Node `TYPE` 0 is a signalised junction and 1 an external end. An unsignalised junction's code is
  unknown.
- **Shared lanes:** a shared lane is coded on the through movement, with `Shared` set to 1 (shares
  left) or 2 (shares right). The turning movement then gets `Lanes` 0.
- `StLanes` with `Storage`/`Taper` make a pocket. The Links `Lanes` value is the number of full
  lanes between the junctions: 2. The sample's own value of 4 suggests Synchro may count the
  pockets.
- `Phase1` holds protected phases and `PermPhase1` permitted phases. Movements carried in a shared
  lane leave both empty.
- **Timeplans:** `Control Type` 0 is pretimed. `Reference Phase` 206 means phases 2 and 6 together.
  `Recall` 3 is max recall.
- **Phases:** `BRP` (barrier, ring, position) is copied from the sample. `Start`/`End` bound the
  split; `Yield` is the end of green.
- Movement columns are named by compass and geometry (L/T/R), whatever the driving side.
- **Rows written empty or not at all:** detectors, pedestrians, `Growth`, `LostTime`,
  `Allow RTOR` and the other Network defaults. MULTRA does not author them (UTDF_EXPORT §3).

## 6. Answers (verbatim, dated)

| # | Question | Owner |
|---|---|---|
| 1 | Synchro 12: units set to metric, left-hand setting found? (menu path) | 2026-11-03 (owner): "Synchro สร้างได้แค่แบบ การจราจรวิ่งขวา": Synchro builds right-hand traffic only. No left-hand setting. Metric: open. |
| 2 | Synchro 12: HCM 7 delay and LOS per approach and junction | 2026-11-03: not possible; Synchro cannot create UTDF (owner). Replaced by row 9 (D438). |
| 3 | `u0-synchro.csv` exported; section and field names differ from §5 where? | 2026-11-03, partial, from a real Synchro 10.1 export (not §1): sections and Synchro field names in [utdf-real-exports](../evidence/utdf-real-exports.md); TYPE 3 = unsignalised; `End` 0 at the cycle wrap; single `Reference Phase` values occur. The §1 export is still needed. |
| 4 | Vistro 2026 import of `u0-synchro.csv`: kept, dropped, HCM delay and LOS | 2026-11-03: no Synchro file (D438). See rows 9–11. |
| 5 | `u0-handwritten.csv` into Synchro 12: first error or difference | Now §4 step 5: only if Synchro imports UTDF. |
| 6 | `u0-handwritten.csv` into Vistro 2026: first error or difference | |
| 7 | Left-hand: setting in each program, carried by the file, far-turn column | 2026-11-03: Synchro right-hand only (owner); Vistro 2026 has the setting (`LEFTHANDTRAFFIC` = 1 in its `.vistromerge`) but its UTDF drops it; UTDF carries the side in neither ([utdf-real-exports](../evidence/utdf-real-exports.md)). The far-turn column is now the export mode's (D437: geometric `R`, mirrored `L`); how each program imports it is still open. |
| 8 | May `u0-synchro.csv` be committed (GPL)? | 2026-11-03: no Synchro file (D438); the real-study exports supplied are not committed, only their field names are recorded. See row 12. |
| 9 | Vistro 2026: left-hand on, metric; HCM 7 delay and LOS per approach and junction | |
| 10 | `u0-vistro.csv` exported; far turn (EB right) in `EBR` or `EBL`; with left-hand off, does it change? | |
| 11 | Synchro: does it import UTDF? If yes, first error or difference for `u0-vistro.csv` and `u0-handwritten.csv` | |
| 12 | May `u0-vistro.csv` be committed (GPL)? | |
