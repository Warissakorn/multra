# VISIBILITY — how far a yielding driver judges the gap (D607)

The owner's choice at OWNER_SITTING A2 (M3.2.7d attempt 2), 2026-11-20, after
[line-approach-other-programs](../evidence/line-approach-other-programs.md) (verbatim): "V ทำเหมือน vissim",
then "1ก 2ก" (100 m by default; an unseen stretch counts as possibly occupied) and "ทุกไฟล์" (the default
applies to every file). Not validated (rule 4): Vissim's attribute is copied, not calibrated.

## 1. Rule

- A conflict area's priority rule carries `visibility`, metres short of the **waiting line** within
  which the yielding driver judges the gap. Absent: 100 m.
- Beyond it the area counts as blocked for that driver, so it plans to stop at its line by the same
  hold a red head uses (Vissim: far from the area a yielding vehicle "plans to stop as close as
  possible" to it). Within it, the gap time and headway test decides as before.
- A driver who can no longer stop at its line goes unless the area is physically taken (commitment,
  M3.2.8a), unseen stretch or not.
- Measured from the waiting line, not from the area as Vissim does: Vissim's stop line stands at the
  area, while a MULTRA line may be placed upstream (D595), and its own driver must be able to see.
- Range (1, 1000] m; under 1 m a driver at its line could never see (Vissim's help warns the same).
- Derived merge rules (`PriorityRule`), signal heads and the major side are unchanged.

## 2. File and editor

- `rightOfWay.priorityRules[i].visibility`, schema 80, written only when set; refused below 80.
- The Conflict area dialog's `visibility` field (1.1–1000 m) edits every rule of the area's group in
  one Undo step; 100 stores nothing.

## 3. Rows

| Row | Statement | Test |
|---|---|---|
| VS1 | The key round-trips at 80 and is refused at 79; without it a file keeps its version and compiles to 100 m | `visibility.vs1_…` |
| VS2 | 1, 0.5, 1000.5 and NaN are refused at `….visibility` | `visibility.vs2_…` |
| VS3 | 40 m out with a clear area, unlimited keeps its speed and 20 m slows (the forcing) | `visibility.vs3_…` |
| VS4 | 60 m out, inside 100 m, the motion equals unlimited tick for tick | `visibility.vs4_…` |
| VS5 | With 2 m, a driver at 6 m and 14 m/s that cannot stop goes; one standing there is held | `visibility.vs5_…` |
| VS6 | Shipped files keep their bytes; their events change only where §4 says | §4 |
| VS7 | The dialog shows 100 without the key, edits it in one Undo step, writes it only when not 100 | `priority_ui_tests` |
| VS8 | The same seed gives the same run | `visibility.vs8_…` |

## 4. Measured (seeds 42–51, unlimited against 100 m)

**Every shipped project and the counted study are unchanged at the 100 m default:** the batch means
(completed, mean delay, pending, clamps; seeds 42–51) and the full single-seed reports (movements,
segment times, lane changes, priority lines; seeds 42 and 43) match unlimited exactly. The prototype law
does not brake for a held line 100 m ahead at these approach speeds, so a driver is already within
visibility before it would slow. The M0 baseline therefore does not move, and M0 stays closed.

Where it starts to act (T-junction, seeds 42–51, minor mean delay crossing / near turn, s; temporary
switch, reverted):

| Visibility | Unlimited | 70 m | 50 m | 40 m | 30 m | 20 m | 10 m |
|---|---|---|---|---|---|---|---|
| Crossing turn | 19.15 | 19.15 | 19.27 | 19.34 | 19.61 | 19.86 | 20.70 |
| Near turn | 9.93 | 9.94 | 10.15 | 10.48 | 10.98 | 11.64 | 13.14 |

Completed vehicles (272.3) and clamps (0) are the same in every column. A study that wants the approach
slowdown at a junction with poor sight lines enters a shorter visibility for that area (Vissim's own
example values are not given in the help read; SUMO's default is 4.5 m).
