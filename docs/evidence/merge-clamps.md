# M3.2.8a.1: the shipped clamps traced and the options measured — 2026-10-18

Branch `claude/red-running-owner-choices`, on `a8cf8b9` (D248). Linux, GCC 13.3.0, Ninja. The
contract and the owner's choice are in [MERGE_CLAMPS](../reference/MERGE_CLAMPS.md) (D249). **No
engine code is committed:** every probe and option below was a temporary patch of
`src/core/simulation.cpp` (and, for S, `src/model/network/sections.cpp`), built in a scratch
Release `-g` directory and reverted. It follows D232's and D241's method.

## Where the clamps are

Shipped projects at seeds 42–81 (the no-key output, the same as `ef9ccb2` and D247):

| Project | Safety clamps in 40 runs |
|---|---|
| `four-leg-signalised` | 0 |
| `lane-change-lab` | 1 |
| `m2.6-study-template` | 41 |
| `t-junction-priority` | 0 (D240) |

This note traces M2.6's 41. The lane-change lab's one clamp is not traced here.

## The probe

For every clamp, a temporary stderr line recorded:
- what set `allowedDistance`, by the source line of each cap;
- the vehicle leader (gap, speed, id, its segment and position);
- the vehicle leader on the tick before;
- when a priority rule last held the vehicle, and when one last held its leader;
- both vehicles' segments.

A second probe printed one vehicle tick by tick. A third printed the derived rules' geometry.

## M2.6's merge geometry

M2.6's eight derived rules (D35, D50) are all of one shape. The minor connector's line is 1 m
before its end (`kYieldClearance`, `src/model/network/sections.cpp`). The conflict point is the
**end** of the major connector. Both connectors feed one downstream lane, the join. Gap time 3 s,
headway 7 m.

- **What the rule sees.** Its scan reads only the major connector. While a major vehicle's front
  is past the join, its span there is clipped at the end, so the reach reads 0 and it holds the
  minor. Once its rear leaves the connector, nothing on the connector holds the minor.
- **What car-following sees.** A vehicle whose front has entered the shared lane, but whose rear is
  still on its own connector, occupies that lane from the join to its front. A follower on the
  other connector sees its rear **at the join, standing still**, until the real rear crosses. The
  gap shrinks by the follower's own travel, whatever the leader's speed (seed 42, vehicle 619: 6.81,
  5.31, 3.85 then 2.91 m behind a leader at 14.4 m/s).
- **The standstill cap** is `gap − standstillDistance` (2 m). A minor stopped at its line is 1 m
  from the join, so it is inside the buffer of any vehicle that has just entered the shared lane.

## The 41 clamps

37 are at a join, 4 at a red head:

| Class | Count | What happens |
|---|---|---|
| **M** | 20 | A minor held at (or just released from) its line, creeping at 0–2.8 m/s, when a major vehicle's front enters the shared lane 0.2–2.2 m ahead at 11–16 m/s. Its rear reads at the join, the cap allows 0 m, and the creep is clamped. D105 removed this case at exactly zero speed; `core.moving_merge_follower_still_reports_the_impossible_stop` pins the moving case as a clamp |
| **J1** | 9 | A major vehicle at 5.5–15.7 m/s meets a minor that its rule released 0.6–3.4 s earlier, entering the shared lane 0.1–4.0 m ahead at 3.3–6.9 m/s. The reading: the minor was creeping at its line, so `committed` (it cannot stop before a line about 0 m away) let it pass the rule's headway and gap tests, and only occupancy held it. The J prototype, which removes just that, removes 8 of the 9 |
| **J2** | 8 | A major vehicle meets a minor released 7–33 s earlier that is only now entering the shared lane ahead of it. The released minor accelerates from rest along a 20–32 m connector, and the major closes in during that time |
| **R** | 4 | A vehicle creeping at 0.45–1.14 m/s at a red head. Stopping at the line needs 2.9–7.7 m/s², within the car's 8, but GIVE_WAY_STOP §2 (D240) brakes only to give-way lines |

| Seed | t (s) | Vehicle | Its segment | Speed | Bound by | Leader: gap, speed, where | Released | Class |
|---|---|---|---|---|---|---|---|---|
| 42 | 1089.2 | 619 | `connector-50` 18.9 m | 13.89 | vehicle leader | 2.91 m, 14.4 m/s, `lane-45` +4.98 m | leader 15.8 s ago | J2 |
| 42 | 1883.7 | 1039 | `connector-58` 30.5 m | 0.81 | vehicle leader | 1.76 m, 12.1 m/s, `lane-33` +0.41 m | itself 0.0 s ago | M |
| 42 | 1885.6 | 1039 | `connector-58` 30.5 m | 0.00 | vehicle leader | 2.00 m, 12.0 m/s, `lane-33` +4.75 m | itself 0.1 s ago | M |
| 43 | 2327.8 | 1355 | `connector-56` 20.1 m | 1.55 | vehicle leader | 1.25 m, 16.4 m/s, `lane-9` +1.22 m | itself 0.0 s ago | M |
| 44 | 2313.4 | 1347 | `connector-56` 19.2 m | 1.90 | vehicle leader | 2.17 m, 13.0 m/s, `lane-9` +0.96 m | itself 0.0 s ago | M |
| 45 | 1349.5 | 739 | `lane-29/sec-2` 5.9 m | 1.05 | red head | — | — | R |
| 48 | 1576.3 | 887 | `connector-50` 18.0 m | 14.78 | vehicle leader | 3.35 m, 13.7 m/s, `lane-45` +4.10 m | leader 32.7 s ago | J2 |
| 50 | 2291.6 | 1245 | `connector-50` 18.4 m | 13.23 | vehicle leader | 3.25 m, 11.5 m/s, `lane-45` +4.74 m | leader 29.6 s ago | J2 |
| 51 | 249.3 | 126 | `connector-50` 18.8 m | 9.50 | vehicle leader | 2.57 m, 9.3 m/s, `lane-45` +7.81 m | leader 15.4 s ago | J2 |
| 51 | 673.2 | 303 | `connector-58` 30.3 m | 1.03 | vehicle leader | 1.92 m, 11.5 m/s, `lane-33` +0.22 m | itself 0.0 s ago | M |
| 51 | 1692.4 | 957 | `connector-55` 31.2 m | 0.90 | vehicle leader | 1.04 m, 11.4 m/s, `lane-45` +1.01 m | itself 0.1 s ago | M |
| 52 | 3006.4 | 1691 | `connector-55` 30.2 m | 0.34 | vehicle leader | 2.00 m, 13.3 m/s, `lane-45` +4.42 m | itself 0.0 s ago | M |
| 53 | 1146.4 | 539 | `connector-58` 30.5 m | 0.45 | vehicle leader | 1.72 m, 11.3 m/s, `lane-33` +0.32 m | itself 0.0 s ago | M |
| 53 | 2165.9 | 1196 | `connector-50` 19.3 m | 14.07 | vehicle leader | 3.35 m, 12.0 m/s, `lane-45` +5.76 m | leader 30.2 s ago | J2 |
| 54 | 722.6 | 414 | `lane-5/sec-2` 6.0 m | 0.45 | red head | — | — | R |
| 55 | 3246.5 | 1847 | `connector-55` 30.9 m | 0.16 | vehicle leader | 1.30 m, 12.3 m/s, `lane-45` +1.17 m | itself 0.0 s ago | M |
| 56 | 2692.8 | 1526 | `connector-52` 30.1 m | 14.80 | vehicle leader | 2.11 m, 5.7 m/s, `lane-9` +1.45 m | leader 0.8 s ago | J1 |
| 58 | 2706.0 | 1460 | `connector-58` 30.4 m | 1.27 | vehicle leader | 1.77 m, 14.4 m/s, `lane-33` +1.42 m | itself 0.0 s ago | M |
| 59 | 184.0 | 67 | `connector-58` 31.0 m | 1.72 | vehicle leader | 1.22 m, 16.3 m/s, `lane-33` +0.77 m | itself 0.0 s ago | M |
| 59 | 536.5 | 297 | `connector-52` 29.9 m | 5.48 | vehicle leader | 2.26 m, 3.3 m/s, `lane-9` +3.98 m | leader 3.4 s ago | J1 |
| 60 | 2814.0 | 1527 | `connector-52` 30.3 m | 15.72 | vehicle leader | 1.89 m, 3.4 m/s, `lane-9` +0.14 m | leader 0.6 s ago | J1 |
| 60 | 3360.8 | 1835 | `lane-5/sec-2` 5.9 m | 1.14 | red head | — | — | R |
| 61 | 133.4 | 38 | `connector-50` 18.6 m | 8.88 | vehicle leader | 2.80 m, 10.1 m/s, `lane-45` +9.59 m | leader 7.4 s ago | J2 |
| 61 | 1864.1 | 958 | `connector-58` 30.6 m | 0.80 | vehicle leader | 1.64 m, 13.6 m/s, `lane-33` +1.12 m | itself 0.0 s ago | M |
| 62 | 1498.7 | 839 | `connector-52` 30.2 m | 6.91 | vehicle leader | 2.01 m, 4.6 m/s, `lane-9` +2.90 m | leader 1.5 s ago | J1 |
| 62 | 3125.9 | 1726 | `connector-55` 30.6 m | 1.80 | vehicle leader | 1.65 m, 14.9 m/s, `lane-45` +0.34 m | itself 0.0 s ago | M |
| 63 | 328.4 | 168 | `connector-59` 19.9 m | 1.17 | vehicle leader | 1.50 m, 12.0 m/s, `lane-21` +0.25 m | itself 0.0 s ago | M |
| 67 | 2673.3 | 1381 | `connector-52` 29.6 m | 9.64 | vehicle leader | 2.63 m, 3.5 m/s, `lane-9` +0.47 m | leader 0.7 s ago | J1 |
| 68 | 427.3 | 234 | `connector-53` 19.1 m | 10.78 | vehicle leader | 2.28 m, 9.3 m/s, `lane-33` +7.91 m | leader 10.5 s ago | J2 |
| 68 | 2384.1 | 1321 | `connector-49` 30.0 m | 8.89 | vehicle leader | 2.21 m, 4.2 m/s, `lane-21` +3.57 m | leader 1.6 s ago | J1 |
| 69 | 3128.4 | 1721 | `connector-55` 30.9 m | 0.38 | vehicle leader | 1.32 m, 12.1 m/s, `lane-45` +0.71 m | itself 0.0 s ago | M |
| 70 | 667.3 | 346 | `connector-53` 19.3 m | 10.23 | vehicle leader | 2.07 m, 9.3 m/s, `lane-33` +7.69 m | leader 8.5 s ago | J2 |
| 71 | 726.4 | 310 | `connector-55` 32.0 m | 2.84 | vehicle leader | 0.20 m, 13.3 m/s, `lane-45` +1.24 m | itself 0.5 s ago | M |
| 72 | 857.2 | 465 | `connector-50` 19.8 m | 11.36 | vehicle leader | 1.62 m, 5.6 m/s, `lane-45` +0.31 m | leader 0.6 s ago | J1 |
| 72 | 1233.2 | 642 | `connector-52` 30.5 m | 12.64 | vehicle leader | 1.68 m, 3.6 m/s, `lane-9` +0.18 m | leader 0.6 s ago | J1 |
| 76 | 600.8 | 337 | `lane-5/sec-2` 6.0 m | 0.53 | red head | — | — | R |
| 77 | 1690.1 | 946 | `connector-55` 31.7 m | 2.33 | vehicle leader | 0.53 m, 11.5 m/s, `lane-45` +0.25 m | itself 0.4 s ago | M |
| 77 | 1746.1 | 987 | `connector-53` 19.3 m | 9.02 | vehicle leader | 2.11 m, 6.9 m/s, `lane-33` +2.24 m | leader 0.9 s ago | J1 |
| 79 | 3544.4 | 1930 | `connector-58` 31.1 m | 0.39 | vehicle leader | 1.06 m, 13.2 m/s, `lane-33` +1.18 m | itself 0.0 s ago | M |
| 81 | 1335.6 | 726 | `connector-55` 30.7 m | 1.07 | vehicle leader | 1.55 m, 12.4 m/s, `lane-45` +0.79 m | itself 0.0 s ago | M |
| 81 | 2824.4 | 1494 | `connector-58` 31.2 m | 0.38 | vehicle leader | 1.06 m, 11.3 m/s, `lane-33` +0.55 m | itself 0.0 s ago | M |

## The options measured

Each option was a temporary patch:
- **A, merge receiving space.** At a derived merge, the rule also holds the minor while a vehicle on
  the shared lane is closer than `standstillDistance` plus the minor's largest move this tick
  (from its line distance and setback).
- **B, an anticipating cap.** On a tick that would clamp, the vehicle-leader cap adds the distance
  the leader's rear must still travel along the follower's route this tick, even at its type's
  maximum braking. That is `vL·dt − ½·bL·dt²` (or `vL²/2bL` if it stops within the tick), less the
  part of its body that is not yet on the follower's route.
- **M, a side entry at a held line.** On a tick that would clamp, a vehicle held by a give-way line
  nearer than its vehicle leader is capped by the line, not by a leader that is partly off its
  route. GIVE_WAY_STOP's stop then applies at the line.
- **R, a stop at a red head.** GIVE_WAY_STOP §2 applies to a binding red-head line too.
- **Q** is R, only for a behaviour with `amberDeceleration`; bare scenarios keep today's rule.
- **J, held means not committed.** A vehicle that the rule held on the previous tick is not
  `committed` to that rule.
- **S, the derived setback.** `kYieldClearance` is 3 m instead of 1 m.

Seeds 42–81, all four projects. "Seeds changed" counts seeds whose output differs at all from the
no-change run; the pin is `trafficsim-cli 42`.

| Option | M2.6 clamps | M2.6 mean delay, s (±95 %) | M2.6 seeds changed | Other projects | Pin | TS reference rows |
|---|---|---|---|---|---|---|
| none | 41 | 46.013 (±0.383) | — | — | — | 5/5 |
| A | 40 | 46.013 | 1 | identical | same | — |
| B | 35 | 46.012 | 5 | lane lab 1 seed differs (still 1 clamp) | same | — |
| M | 31 | 46.012 | 14 | identical | same | — |
| R | 37 | 46.006 | 4 | identical | **differs** | **3/5** |
| J | **49** | 46.012 | 14 | identical | same | — |
| S | 35 | 46.011 | 40 | four-leg 1 seed differs | same | — |
| S+R+B | 23 | 46.004 | 40 | four-leg, lane lab | differs | — |
| B+M | 21 | 46.010 | 17 | lane lab 1 seed | same | — |
| **B+M+Q** | **17** | **46.003 (±0.381)** | 21 | lane lab 1 seed | same | **5/5** |
| B+M+Q+J | 27 | 46.003 | 30 | lane lab 1 seed | same | — |

- **No body overlap in any run.** An overlap counter checked every pair of spans on a segment at
  every tick, in every option, and found none (0 in the no-change run too).
- **Completed trips** are 1980.80 a run in every option.
- **What each option leaves:**
  - **A:** the release margin matters little, because the clamp comes from the *next* major's front
    entering, not from the one that just left.
  - **M alone:** 3 M clamps are left, and 7 come back a few ticks later for the same vehicles. Once
    the major's whole body is on the shared lane, the 1 m setback still sits inside the 2 m buffer.
    B removes those.
  - **S** clears every M and J1 clamp but adds 24 new ones. A minor that has crossed its line
    (now 3 m back) is still on the 3 m stretch when the next major's front reaches the join, and it
    no longer checks the rule. S also moves every M2.6 seed and one four-leg seed.
  - **J** holds a creeping minor at its line, which it then cannot stop at in about 0 m, so the
    minor clamps itself. That is the line approach's softness (D227, D232), not this rule.
  - **R** changes the M0 crossing demo, which has no `amberDeceleration`, and fails 2 of the 5
    TypeScript reference rows. Q keeps them.
- **B+M+Q leaves 17:** J1 9, J2 4, M 3, and 1 new.
- **One existing row changes under M:** `core.moving_merge_follower_still_reports_the_impossible_stop`
  (D105's pinned moving case) brakes to its line instead of clamping. Every other row of
  `trafficsim-tests` passes under B+M+Q (938 of 939).

**Cost** (callgrind Ir, Release `-g`, seed 42, the prototype without its probes, against the same
tree without it):

| Project | Without | B+M+Q prototype |
|---|---|---|
| `four-leg-signalised` | 888.80 M | 893.92 M (+0.58 %) |
| `lane-change-lab` | 1253.26 M | 1269.50 M (+1.30 %) |
| `m2.6-study-template` | 3765.92 M | 3787.85 M (+0.58 %) |

The prototype passes a leader id to every leader search and tracks one more minimum per cap. An
implementation would measure again.

## D251: the package implemented

On `7d133cb` (D250) plus this change, the same host. Contract:
[MERGE_CLAMPS](../reference/MERGE_CLAMPS.md) §3.

**One fix against the D249 prototype.** At a signal head, the prototype passed the *give-way* line
distance to `stopAtLine` instead of the head's own distance. That distance was often infinite,
which gave a braking of about 0, so a creeping vehicle passed its red line.

The implementation brakes to the binding distance. It removes the same 4 red-head clamps. M2.6's
mean delay is 46.010 s, against the prototype's 46.003 s. The prototype figures above are kept as
they were measured.

**Rows** (`tests/merge_clamp_tests.cpp`, ctest `mergeclamps`, 10 passing). Each engine row first
runs with its switch off and asserts today's clamp.

| Row | Asserts |
|---|---|
| BA158 | The crossing demo: every tick's events and the final vehicles are equal with the rules on and off |
| BA159 | 2.5 m behind a 12 m/s leader: clamped with `leaderTravel` off, not with it on. The move fits the gap the leader would leave under its maximum braking |
| BA160 | A standing leader: still clamped |
| BA161 | A committed minor and a leader with 4 m of its body still on its own connector: still clamped. With the body on the route: clamped off, free on |
| BA162 | D105's fixture: clamped with `sideEntryAtLine` off. On, it has no clamp and stays at its line |
| BA163 | A committed minor that its rule does not hold: clamped. A held minor whose standing leader is wholly on its route: clamped |
| BA164 | 1 m/s, 0.08 m short of a red head with `amberDeceleration` 3: clamped with `redLineStop` off, brakes to the line on. Beyond the maximum it goes as committed, the same either way |
| BA165 | The same without `amberDeceleration`: clamped |
| BA166 | `tickCap` with the rules off, or with no leader to find, is `min(holds, leader cap)` |
| BA167 | Schema 45 round-trips through `compileDocument`. All on writes no key and keeps the old schema bytes. 44 carrying the key, a non-boolean, an unknown key and a non-object are refused at their paths |

- **Run settings** (`signal-ui`): the three boxes are ticked by default. Unticking one writes it
  `false` at schema 45, and Undo restores it.
- **D105's row** (`core.moving_merge_follower_still_reports_the_impossible_stop`) now sets
  `sideEntryAtLine` off and keeps its clamp.
- **Fifteen codec rows** used 45 as "a future schema". They use 46 now, as D238 did for 44.

**Mutations,** each applied in turn. All 14 are caught:

| Mutation | Caught by |
|---|---|
| No B | BA159, BA161 |
| B ignores the hidden body | BA161–BA163, D105's row |
| No M | BA162 |
| M ignores the hidden body | BA163 |
| No Q | BA164 |
| Q without the `amberDeceleration` scope | BA165 |
| Each switch ignored (three mutations) | BA159/BA161; BA162 and D105's row; BA164 |
| No parse, no writer, no schema 45, no older-schema refusal | BA167 (writer and schema also `signal-ui`) |
| No command | `signal-ui` |

**Outputs,** seeds 42–81, Release. "Seeds changed" is against `fcb6d3a`:

| Project | Rules | Clamps | Mean delay, s (±95 %) | Seeds changed |
|---|---|---|---|---|
| `m2.6-study-template` | all on (default) | **17** | 46.010 (±0.382) | 21 |
| | `leaderTravel` off | 27 | 46.012 (±0.383) | 18 |
| | `sideEntryAtLine` off | 31 | 46.012 (±0.383) | 9 |
| | `redLineStop` off | 21 | 46.010 (±0.382) | 17 |
| | all off | 41 | 46.013 (±0.383) | 0 (byte-identical) |
| `lane-change-lab` | all on | 1 | 8.915 (±0.092) | 1 |
| | all off | 1 | 8.916 (±0.092) | 0 (byte-identical) |
| `four-leg-signalised`, `t-junction-priority` | every setting | 0 | unchanged | 0 (byte-identical) |

- **Unchanged:** completed trips are 1980.80 per M2.6 run in every row, and `trafficsim-cli 42` is
  unchanged.
- **The TS reference rows** pass, 5 of 5.
- **The 17 left on M2.6** (a temporary probe, never committed): pull-out (J1) 9, late catch-up (J2)
  4, merge creep (M) 3, and 1 new (seed 59, vehicle 67, 0.1 s after its traced M clamp).
- **No body overlap** on any project, by the same temporary span-pair counter as D249.

**Performance** (callgrind Ir, Release `-g`, seed 42; against the same engine without the change):

| Project | Without | This change |
|---|---|---|
| `four-leg-signalised` | 888.80 M | 893.26 M (+0.50 %) |
| `lane-change-lab` | 1253.26 M | 1262.45 M (+0.73 %) |
| `m2.6-study-template` | 3765.92 M | 3791.58 M (+0.68 %) |

A first version cost 2.4–3.4 %. Two changes brought it down:
- **Inlining.** `tickCap` is now inline, so the common tick never leaves it.
- **The leader id is found only on the relief path.** Passing an id to every `closestVehicle` call
  had cost 23 M Ir in that function alone on the lane lab.

## D252: the lane-change lab's clamp

Traced on 2026-10-19, on `e926bb6` (D251), with no code change. The method is D249's: a temporary
stderr line in the clamp branch, then a tick-by-tick print of the two vehicles, never committed.

**Where it is.** The only clamp in `lane-change-lab` over seeds 42–81 is at seed 74, t = 91.9 s:
- vehicle 59 at 3.55 m/s;
- its leader, 51, 2.14 m ahead at 0.63 m/s;
- both on `lane-18` on route `route-29/lane-3`, so the same lane, and neither has ever changed lanes;
- the binding cap is the vehicle leader's, 0.14 m. Its next hold is 9.7 m away.

**What happens:**

| t (s) | 51: speed, distance to its next hold | 59: speed, gap to 51 | 59's braking |
|---|---|---|---|
| 89.5 | 12.2 m/s, 15.0 m | — | — |
| 91.0 | 2.9 m/s, 4.5 m | 7.8 m/s, 5.8 m | — |
| 91.5 | 1.4 m/s, 3.4 m | 5.4 m/s, 3.5 m | ≈ 4.8 m/s² |
| 91.9 | 0.6 m/s, 3.0 m | 3.6 m/s, 2.1 m | ≈ 4.4 m/s² — clamped |

- **The leader** brakes from 12.2 m/s to a creep in about 2.4 s, at up to about 8 m/s², toward the
  hold at the end of its stub lane. It is waiting there to change lanes.
- **The follower** brakes at only about 4.4–4.8 m/s². Its car's maximum is 8. The gap closes inside
  the standstill buffer at 3.6 m/s.

**What it means:**
- **Not a merge clamp.** It is none of D249's classes. It is the follower class D241 found at seed 73:
  the prototype's braking is too soft behind a leader that stops hard. ENGINEERING_NOTES records
  D227's kinematic braking floor, measured and not adopted.
- **The D251 switches barely touch it.** `leaderTravel` credits 0.025 m, because the leader is
  slow, and the clamp stays. The tick moves the follower 0.025 m further, which shifts seed 74's
  mean delay by 0.0006 s. That is the one lane-lab seed D251 changes.
- **A fix would change the prototype's braking law,** with D227's own rows and the owner's choice.
  None is proposed here.

## Not covered here

- **Anticipation across a join was not prototyped.** In it, a vehicle approaching a join would see
  the other stream before that stream's front enters the shared lane. It is the only option aimed
  at J1 and J2's cause. It would change trajectories at every join, including in clamp-free runs.
- The lane-change lab's one clamp; source-insertion clearance (M3.2.8a.1a, D108).
- Calibration, and whether gap time 3 s and headway 7 m suit a merge from rest. Those are study
  inputs (rule 4).
