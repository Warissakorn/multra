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
2. **Export.** *File → Export → Synchro…* writes the UTDF combined CSV (the owner's real `test2.csv`
   came from this menu, 2026-11-03; Vistro has no item named UTDF). Use metric units and save it as `u0-vistro.csv`.
3. **Far-turn column.** Find which column `u0-vistro.csv` holds the far turn (EB right, phase 1) in: `EBR` or `EBL`.
   If Vistro lets you, export the same junction again with left-hand traffic off, and note whether
   the column changes.
4. **Import MULTRA's file.** Import
   [`u0-handwritten.csv`](../../tests/data/utdf/u0-handwritten.csv) into Vistro (*File → Import →
   Synchro…*). Note the first error, or the first place where the drawing differs from §1–§3. Its
   timing was refused on 2026-11-03 (row 6), so also import the two v2 files (row 13). They differ
   only in `[Timeplans]` and `[Phases]`: both carry every record Synchro 10.1 writes, in its order.
   - **v2-empty** leaves the records MULTRA does not author empty.
   - **v2-filled** gives them neutral pretimed values. It is probe-only.
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
- *(2026-11-03: Vistro's writer gives Links `Lanes` 2, full lanes only; the owner chose to count the
  pocket, 3, per D440, pending row 17.)* `StLanes` with `Storage`/`Taper` make a pocket. The Links `Lanes` value is the number of full
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
| 6 | `u0-handwritten.csv` into Vistro 2026: first error or difference | 2026-11-03, via *File → Import → Synchro…*, from screenshots. **Came through:** four legs with names and directions; speeds 50/60; every lane 3.25 m; pockets NB/SB left 1 × 40 m and EB/WB right 1 × 60 m; shared lanes as drawn; every volume; PHF 0.92 (analysis volume = volume ÷ 0.92). **Not shown:** the intersection *Median* is unchecked (Links `Median` 2.0, row 14). *Analysis Method* reads HCM 2010, not set by the file. **Timing refused:** "Could not build RBC Basic Timing. Reason: CellNotFound: Row not found: Row = 3, Column = 1, NodeID = 1", then "Could not populate signal group data for controller 1, signal group 1". Our `[Timeplans]` and `[Phases]` lack rows that both real writers emit; see row 13. Screenshot, same day: after the refusal the junction has no cycle, empty Ring 1–4 sequences and an *ICA error* label, and every delay is 0. HCM 2010 is Vistro's default; UTDF has no record for the method, and the owner can set HCM 7th after import. |
| 7 | Left-hand: setting in each program, carried by the file, far-turn column | 2026-11-03: Synchro right-hand only (owner); Vistro 2026 has the setting (`LEFTHANDTRAFFIC` = 1 in its `.vistromerge`) but its UTDF drops it; UTDF carries the side in neither ([utdf-real-exports](../evidence/utdf-real-exports.md)). The far-turn column is now the export mode's (D437: geometric `R`, mirrored `L`); how each program imports it is still open. |
| 8 | May `u0-synchro.csv` be committed (GPL)? | 2026-11-03: no Synchro file (D438); the real-study exports supplied are not committed, only their field names are recorded. See row 12. |
| 9 | Vistro 2026: left-hand on, metric; HCM 7 delay and LOS per approach and junction | 2026-11-03, draft 1: drawn with metric units and left-hand traffic on (owner). Not yet to the sheet (row 16); the HCM report will come with the redo. |
| 10 | `u0-vistro.csv` exported; far turn (EB right) in `EBR` or `EBL`; with left-hand off, does it change? | 2026-11-03, draft 1 ([`u0-vistro-draft1.csv`](../../tests/data/utdf/u0-vistro-draft1.csv)): with left-hand on, EB right (the far turn) is under **`EBR`**, with its 60 m pocket (196 ft). Vistro writes UTDF in **feet** (`Metric,0`) even for a metric project. Left-hand off was not exported. |
| 11 | Synchro: does it import UTDF? If yes, first error or difference for `u0-vistro.csv` and `u0-handwritten.csv` | |
| 12 | May `u0-vistro.csv` be committed (GPL)? | 2026-11-03: yes (owner); draft 1 is committed as `u0-vistro-draft1.csv`. |
| 13 | Import [`u0-handwritten-v2-empty.csv`](../../tests/data/utdf/u0-handwritten-v2-empty.csv) and [`u0-handwritten-v2-filled.csv`](../../tests/data/utdf/u0-handwritten-v2-filled.csv) into Vistro: first error, or does the timing appear (cycle 100; phases 1/2/4/5/6/8; splits 14/44/42/14/44/42)? | 2026-11-03, screenshots. **v2-empty:** refused, `Phases Table, VehExt Cell for controller (INTID) 1, Column D1 is invalid`, then `Could not populate signal group data`. The rows were now found, but empty cells are invalid; Vistro showed its own defaults (cycle 90, controller 2) and no rings. **v2-filled:** accepted. Controller 1, cycle 100, Ring 1 = 1·2|4, Ring 2 = 5·6|8, SG 1/2/4/5/6/8 = 14/44/42/14/44/42 s, all as §3. Vistro's HCM 2010 then gives delay 186.72 s, LOS F, v/c 0.988. Owner choice: D439. |
| 14 | Median: after importing, does Vistro show Main St's 2.0 m median anywhere (approach, link or cross-section)? | 2026-11-03 (owner): no median is shown after import. Links `Median` 2.0 is not carried into Vistro. |
| 15 | With the network's left-hand setting on, is the imported junction drawn left-hand (NB vehicles approach on the west side)? | 2026-11-03, screenshots (v2-empty, v2-filled): the junction looks drawn **right-hand** (NB on the east side, SB west, EB south). Whether that project had left-hand on is not yet confirmed. **Owner, 2026-11-03:** the project was set left-hand before import; after import it is right-hand, because **Vistro switched the network to right-hand itself**. Importing UTDF resets Vistro's driving side; the user must set left-hand again after import. |
| 16 | Redo `u0-vistro.csv` to §1–§3. Draft 1 differs: cycle 60 with phases 2/4/6/8 only (no protected 1/5), PHF 1.00, HV 2 %, and NB/SB drawn as left+through shared with an exclusive right, where the sheet has a left pocket and a through+right shared lane. | 2026-11-03: owner will redo. A redo sent the same day was empty after the first column in every section (spreadsheet re-save or broken export), so it was not committed; a re-export is pending. Draft 2, same day ([`u0-vistro-draft2.csv`](../../tests/data/utdf/u0-vistro-draft2.csv)): matches §1–§2 (names, lanes, pockets, volumes, PHF 0.92, HV 5 %). Its timing is still 60 s with phases 2/4/6/8 and permitted far turns; the owner will set §3 and export again. **Golden, same day: [`u0-vistro.csv`](../../tests/data/utdf/u0-vistro.csv)** (owner accepted it). It matches §1–§2 with a 100 s cycle and protected-only far turns (EBR phase 3, WBR phase 7, no permitted phase). The phase numbers, order and splits are Vistro's own, not §3's: NB/SB 6/2 at 0→33, far turns lagging 33→55/53, EB/WB through 8/4 at 55/53→100. UT0 compares fields and codes, not timing values. |
| 17 | Import [`u0-handwritten-v3.csv`](../../tests/data/utdf/u0-handwritten-v3.csv) (v2-filled with `[Links] Lanes` 3 on the junction legs, D440): are the lanes drawn as §1 (2 full + 1 pocket of 40/60 m), and is the timing still exact? | 2026-11-03, screenshots: imported with the pockets drawn as pockets. The west and east legs widen with a taper before the stop line; there are no 3 full lanes and no doubled pocket. Results are identical to v2-filled (`Lanes` 2): junction 186.72 s, LOS F, v/c 0.988; per approach (HCM 2010) EB 277.65 F, WB 229.19 F, SB 24.46 C, NB 24.72 C. Phases are as §3: EB 6 + far turn 1, WB 2 + 5, SB 4, NB 8. **D440 holds.** |
| 18 | Patch for what UTDF cannot carry (Vistro has *File → Import → Merge Data*, owner 2026-11-03). Import `u0-handwritten-v3.csv`, then merge [`u0-patch-v1.vistromerge`](../../tests/data/utdf/u0-patch-v1.vistromerge): `$NETWORK` `LEFTHANDTRAFFIC` 1 and `$NODE` 1 `ICAMETHODSIGNALIZED` HCM 7th Edition. Is the project left-hand and node 1 HCM 7th afterwards, is the drawing correct (NB on the west side, pockets intact, far turns 1/5), and do delay and LOS change from 186.72 s / F? | 2026-11-03 (owner): merged after the v3 import; **the project stayed right-hand**. HCM method not reported. v1 lacked `NETVERSIONID` and `VISUMVERSIONNO`; row 20 retries in the export's own shape. |
| 19 | After importing v3 (before any patch), export *File → Export → Merge Data…* and send it. It shows Vistro's node, link and leg numbers after a UTDF import, which a `$LEG` median patch (`HASCISLAND`, `CISLANDWIDTH`, `CISLANDLEN`) needs. | 2026-11-03: sent, not committed (owner). Structure recorded in [utdf-real-exports](../evidence/utdf-real-exports.md): junction 1 keeps its number; end nodes become 100100 W, 100109 S, 100118 E, 100127 N; links 227 N, 318 E, 409 S, 500 W. |
| 20 | Import `u0-handwritten-v3.csv`, then merge [`u0-patch-v2.vistromerge`](../../tests/data/utdf/u0-patch-v2.vistromerge) (`$NETWORK` with `NETVERSIONID`, node 1 HCM 7th, a 2 m median on legs 318/500). Did (a) left-hand, (b) HCM 7th and (c) the Main St median take effect? If still right-hand: does setting left-hand by hand redraw the junction correctly (NB on the west side, pockets and phases 1/5 kept)? | 2026-11-03 (owner): **median applied**; still right-hand; still HCM 2010. Vistro **refuses to switch to left-hand while a right-hand junction exists**, so a UTDF import can never become left-hand in Vistro. |
| 21 | Whole network as Vistro's own format. In a **new empty project set to left-hand**, *Merge Data* [`u0-full-lh-a.vistromerge`](../../tests/data/utdf/u0-full-lh-a.vistromerge); then, in another new left-hand project, [`u0-full-lh-b.vistromerge`](../../tests/data/utdf/u0-full-lh-b.vistromerge) (lane indices mirrored). For each: does it merge, is it left-hand, HCM 7th, a 2 m Main St median; are NB/SB left pockets on the kerb and EB/WB right pockets on the far side; is the timing kept (SG 1/2/4/5/6/8 = 14/44/42/14/44/42); delay and LOS? | 2026-11-03 (owner): both variants **create a second, overlapping junction** and **stay right-hand**; only `a` gets the median. Merge Data ignores `LEFTHANDTRAFFIC`. **Probing stopped by the owner; decision D441.** |
