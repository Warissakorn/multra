# Diverge acceptance fixture — rows A62–A66, 2026-11-22 (D622)

M3.2's gate names "controlled merges, diverges and crossing conflicts" and a no-overlap regression;
the diverge row was the last one unwritten (ROADMAP M3.2, review O6/R7). This note records what the
row measured. It is Linux evidence of the engine as built, not a validation and not an owner verdict.

## Fixture

`tests/diverge_tests.cpp` (`diverge.*`), built through the editor's commands and compiled with
`compileDocument`, as the editor runs it:

- Link A, one 3.5 m lane, 200 m; two Connectors from its end, each 30 m long and 5 m to the side, to
  Links B and C (170 m each). Authored routes A→B and A→C, 450 veh/h each, cars, 0–300 s, run 420 s,
  step 0.1 s.
- The editor's own measure (`automaticConflicts`) finds one branching area between the two Connectors,
  11.76 m long on each, plus two 0.09 m mouth pieces against A. Nothing is stored and nothing compiles:
  no conflict zone, no priority rule, no lane-change span.
- The bodies are checked in the plane: each car is a 4.5 × 1.8 m rectangle centred half a length
  behind its front, on its route's centre line (Links' geometry, each Connector's centre between its two
  painted rails, D114), and two rectangles touch when every one of the four separating axes overlaps.
  On this drawing the two Connector centre lines are 1.56 m apart 7.6 m past the split and 5.0 m apart at
  15 m, so two car bodies can touch for roughly the first 8 m.

## Rows and results

| Row | Case | Result |
|---|---|---|
| A62 | The plain diverge, compiled | **Passes.** Only branching areas; one between the two branches; none stored; no zone, rule or span compiled |
| A63 | Free flow, seeds 42–51 | **Passes.** Vehicles leave A in the order they departed; no clamp; no overlap along either path; no footprints touch across the split |
| A64 | B held red 120 s by a head 30 m into B, so its queue reaches back over the split; seeds 42–51 | **Fails on the cross-branch half.** Order, clamps (0) and the along-path check pass; the forcing holds in 9 of 10 seeds (seed 42's queue stops short of the split). Footprints of a car to B and a car to C overlap in 8 seeds (table below) |
| A65 | B held red 90 s by a head 0.5 m into its Connector | **Passes.** The car to B stands at the line (over 30 s); nobody who departed after it gets past it or onto C: the shared lane is first-in, first-out, as Meso's diverge (ME21). It then drains in order, no clamp |
| A66 | A64's run copied at tick 1,500 with a queue across the split | **Passes.** Both copies emit identical events to the end and drain |

A64, footprint overlaps per seed (`MULTRA_DIVERGE_REPORT=1 multra-tests diverge`; ticks are 0.1 s):

| Seed | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | 50 | 51 |
|---|---|---|---|---|---|---|---|---|---|---|
| Ticks with an overlap | 0 | 46 | 23 | 67 | 0 | 45 | 57 | 5 | 42 | 12 |
| Deepest (m) | 0 | 1.69 | 1.60 | 1.59 | 0 | 1.61 | 1.60 | 0.93 | 1.63 | 1.55 |

No run reported a safety clamp: the engine does not see these overlaps at all.

## Cause

Seed 43, t = 44.4–45.1 s: car 8 (to B) creeps towards B's queue at 1.3 m/s with its front 8.4 m past the
split; car 9 (to C) passes it at 9–10 m/s, overlapping by up to 0.95 m. Car 9 followed car 8 on A. Once
car 8's rear left A, its only span is on B's Connector, which is not on car 9's route, so
`closestVehicle` (`src/core/leaders.hpp`) stops returning it, and car 9 accelerates through the part of
the split where the two lanes still overlap. A pure diverge was treated as having no competing arrival
(D109: "original order", no reservation); that holds while the leader moves away, not while it stands or
crawls inside the split.

## Remedy options (owner item 53; nothing is built)

- **(a) Sibling-branch leaders, recommended.** The compiler gives each pair of Connectors that leave the
  same lane at the same station the length past the split within which their bodies can touch (from the
  same rails as the editor's branching area). A follower then also takes as its leader a span on the
  sibling branch whose rear is inside that length. No reservation, no new state, order kept; runs change
  only where a queue or a slow car stands in a split. Cost: one more bucket scan per vehicle near a split,
  measured before and after with callgrind (it is the hot path).
- **(b) A branching zone in the core:** reserve the split like a conflict zone. Heavier, and it would hold
  a car to C behind a car to B that is already clear of it.
- **(c) Leave it, documented:** M3.2's no-overlap gate is then not met at diverges, so M3.2 stays open or
  is carved (rule 8).

Under (a) every shipped project with a diverge (four-leg, M2.6, T-junction, the labs) may change where a
branch queue reaches the split; the four TS baselines (single-lane, no diverge) should not. The change
is a recorded decision with re-published reports, not a silent regeneration.
