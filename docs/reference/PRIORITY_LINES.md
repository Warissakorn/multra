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
- The critical headway `t_c` is not estimated here; U-c's effective `t_c` needs its own method
  and contract.

## 3. Report

Per area: `zone`, `control` (`stop` or `yield`), `window` (s), `minorCrossed`, `minorPerHour`,
`majorEntered`, `majorPerHour`, `followUps` (n), `followUpMean` and `followUpSd` (s; absent with
n < 1 and n < 2), `excluded`. `multra-cli --project F --priority-lines` adds them as JSON
`priorityLines`; single runs only.

## 4. Acceptance rows

| Row | Statement | Group |
|---|---|---|
| PL1 | A hand-built crossing (Stop): `minorCrossed` and `majorEntered` equal the passages a test counts independently from the snapshots in the window, and each interpolated time lies in `(t − Δt, t]` | `prioritylines` |
| PL2 | Saturated minor demand, no major traffic: every consecutive pair after the first queued vehicle is a follow-up, and `followUpMean` equals the mean of those differences computed independently | `prioritylines` |
| PL3 | The same with major traffic: a pair with a major entry between its crossings is not a follow-up, so `followUps` is less than in PL2's count of consecutive queued pairs (the forcing) | `prioritylines` |
| PL4 | Sparse minor demand (60 veh/h, Yield): a vehicle released at rest far from any queue is not queued, so only pairs the arrivals bunched within `maxGap` follow up (the independent count, fewer than half the crossings); the area reports `control: yield` | `prioritylines` |
| PL5 | A run with the same seed gives the same report, bit for bit | `prioritylines` |
| PL6 | `--priority-lines` needs `--project`; the JSON carries one row per conflict area in `Scenario` order | `cli-priority-lines`, `cli-priority-lines-refused` |

Return to [the reference index](README.md).
