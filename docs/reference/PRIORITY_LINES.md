# PRIORITY_LINES — the counter at a priority line (D528)

Contract for the observer M6's U rows need ([m6-benchmark-options](../evidence/m6-benchmark-options.md)
§5–§6): at each conflict area's waiting line, the minor vehicles that leave it, the major
vehicles that enter the area, and the follow-up headway between queued minor vehicles. Every
figure is a count or a time measured in the simulation; it is **not validated** and not an HCM
capacity (rule 4). Written before code; rows PL1–PL6 fail first.

## 1. What is counted

Per conflict area (`Scenario::conflictZones`, Stop and Yield alike), in `Scenario` order:

- **A minor crossing:** a vehicle on a route whose minor side meets the area has its front pass
  the area's waiting line (`RouteZone::waitAt`) between two snapshots. Its time is interpolated
  linearly between the two snapshots' distances, so it lies in `(t − Δt, t]`.
- **A major entry:** a vehicle on a route whose major side meets the area has its front pass the
  area's entry on that route (`RouteZone::entryAt`), timed the same way.
- A crossing counts when its time lies in the evaluation window `[warmup, end]` (the run's end
  without one); the window's length turns counts into hourly rates.
- **Queued:** a minor vehicle is queued from the first snapshot at which it stood (speed below
  the queue definition's `beginSpeed`) with its front short of the waiting line, **and** within
  the queue definition's `maxGap` of the line, or of the rear of the next minor vehicle ahead of
  it short of the same line (distances to the line, so several minor routes compare). This is
  Vissim's queue condition; a vehicle released from its source at rest, far from any queue, is
  not queued.
- **A follow-up headway** (HCM 7 Ch. 20's `t_f`; [Kyte96]'s continuous queuing): consecutive
  minor crossings of one area, leader then follower, both in the window, where the follower was
  queued at or before the leader's crossing time and no major entry lies between the two
  crossing times. Its value is the follower's time minus the leader's.

## 2. What is not counted

- A vehicle whose route changed between the two snapshots (a lane change or a routing remap) is
  not timed at that snapshot: its passage cannot be proved, as DISCHARGE refuses it. Such
  skipped passages are counted per area as `excluded`, so none is silent.
- A vehicle that leaves the network between two snapshots after passing a line is not counted
  (a waiting line or entry is never at a route's last metres in the U projects). Priority rules
  (`PriorityRule`, the older merge form) have no row; the U projects use conflict areas.
- The critical headway `t_c` is estimated only by Siegloch's regression (§5), which needs a
  continuous minor queue; maximum likelihood over accepted and rejected gaps is left to U-e (R7).

## 3. Report

Per area: `zone`, `control` (`stop` or `yield`), `window` (s), `minorCrossed`, `minorPerHour`,
`majorEntered`, `majorPerHour`, `followUps` (n), `followUpMean` and `followUpSd` (s; absent with
n < 1 and n < 2), `excluded`, and §5's `siegloch` object. `multra-cli --project F --priority-lines` adds them as JSON
`priorityLines`; single runs only.

## 4. Acceptance rows

| Row | Statement | Group |
|---|---|---|
| PL1 | A hand-built crossing (Stop): `minorCrossed` and `majorEntered` equal the passages a test counts independently from the snapshots in the window, and each interpolated time lies in `(t − Δt, t]` | `prioritylines` |
| PL2 | Saturated minor demand, no major traffic: every consecutive pair after the first queued vehicle is a follow-up, and `followUpMean` equals the mean of those differences computed independently | `prioritylines` |
| PL3 | The same with major traffic: a pair with a major entry between its crossings is not a follow-up, so `followUps` is less than in PL2's count of consecutive queued pairs (the forcing) | `prioritylines` |
| PL4 | Sparse minor demand (60 veh/h, Yield): a vehicle released at rest far from any queue is not queued, so only pairs the arrivals bunched within `maxGap` follow up (the independent count, fewer than half the crossings); the area reports `control: yield` | `prioritylines` |
| PL5 | A run with the same seed gives the same report, bit for bit | `prioritylines` |
| PL7 | `sieglochFit` on points that lie exactly on a line returns its slope as `t_f`, its intercept as `t_0` and `t_0 + t_f/2` as `t_c`; with fewer than two distinct `n` it returns nothing | `prioritylines` |
| PL8 | Saturated minor demand with major traffic: the saturated gaps and their counts `n` equal those a test derives independently from the passages by §5's rule, and `t_f` lies within 0.5 s of `followUpMean` | `prioritylines` |
| PL9 | Sparse minor demand (PL4's run): a gap whose next minor vehicle was not queued at the gap's end is not saturated, so fewer gaps qualify than there are major gaps in the window (the forcing), and none without a waiting vehicle | `prioritylines` |
| PL6 | `--priority-lines` needs `--project`; the JSON carries one row per conflict area in `Scenario` order | `cli-priority-lines`, `cli-priority-lines-refused` |

## 5. Siegloch's regression for the effective `t_c` (D534)

[TFT8] §8.2.2 (FHWA monograph chapter 8, PDF pp. 5–6 = pages 8-3 and 8-4, read from the page
images): with a continuous minor queue, record each major gap and the number of minor vehicles
that enter in it; for each number `n` of entering vehicles take the mean gap `E(t)`; regress `E(t)`
(dependent) on `n` by least squares; the slope is `t_f`, the intercept `t_0`, and
`t_c = t_0 + t_f/2` (Eq. 8.1).

- **A major gap** is the time between two consecutive major entries of one area, both in the
  window. Its `n` is the number of minor crossings strictly after its start and at or before its
  end.
- **Saturated** (the continuous queue): with the gap's crossings `c_1 … c_n` and `c_{n+1}` the
  first minor crossing after the gap's end, every `c_j` was queued at or before the event before
  it (the gap's start for `c_1`, `c_{j−1}`'s crossing after), and `c_{n+1}` was queued at or before
  the gap's end. A gap without a following minor crossing is not saturated. Only saturated gaps
  with `n ≥ 1` (gaps "accepted by n drivers") enter the regression; saturated gaps with `n = 0`
  are counted, not regressed.
- **Report** (`siegloch` per area): `gaps` (saturated, all `n`), `points` (`n`, `gaps`, `meanGap`
  for each `n ≥ 1`), and `t0`, `tf`, `tc` (s), absent with fewer than two distinct `n`. The fit is
  unweighted over the points, as the regression on the average values in [TFT8].
- An effective `t_c` of the engine, measured, not validated (rule 4). It feeds U-c's capacity
  forms (M6 §6) and is not the `gapTime` input.

[TFT8]: Troutbeck, R.J., Brilon, W. Unsignalized intersection theory. Chapter 8 of the FHWA
*Revised Monograph on Traffic Flow Theory*,
<https://www.fhwa.dot.gov/publications/research/operations/tft/chap8.pdf>; not vendored.

Return to [the reference index](README.md).
