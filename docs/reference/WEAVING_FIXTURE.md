# Weaving fixture contract (M3.4)

Written 2026-11-04 (D449). The owner asked whether MULTRA simulates weaving and how other tools
test it, and agreed to step 1: a synthetic fixture and a measurement with the lateral switch off and
on. It is **mechanism evidence, not validation** (rule 4): the marker stays, and calibration and field
data stay with M6. Rows BA179–BA183 (BA184–BA193 for the lock, BA195 for gap seeking) are in [the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows).
Lane changing itself is [M3_8_CONTRACT](M3_8_CONTRACT.md) §2 and
[LANE_CHANGE_DURATION](LANE_CHANGE_DURATION.md); this page changes neither.

## 1. Scope and sources

How other tools handle weaving (surveyed 2026-11-04, from public sources; no product was run):

- **SUMO** keeps synthetic weaving networks as regression tests
  ([sumo-user, `lc_model/ramps/onoff_ramp`](https://www.eclipse.org/lists/sumo-user/msg13146.html)).
  Its default lane-change model LC2013 changes lanes instantly; sublane SL2015 is the option
  ([benchmark](https://arxiv.org/pdf/2512.18537)).
- **Vissim:**
  - A necessary change starts at the *lane change distance*. It ends at the *emergency stop
    position*.
  - After the *waiting time before diffusion* (default 60 s), the vehicle is removed from the
    network.
  - Agencies lengthen the lane change distance well beyond its 200 m default
    ([TxDOT](https://www.txdot.gov/manuals/des/tsp/chapter-13-microsimulation-analysis/13-5-calibration/13-5-3-calibration-parameters-and-best-practices/13-5-3-1-vissim-calibration/13-5-3-1-1-lane-change-parameters.html),
    [Louisiana DOTD](https://dotd.la.gov/media/ggmgprf1/vissim-calibration.pdf)).
- **Aimsun** (from a paper, not the manual) has three zones before a turn. In the nearest zone a
  vehicle must reach its lane, slowing or stopping, and its neighbours open a gap.
- **Field data:** FHWA NGSIM US-101 and I-80 are the usual sets of trajectories in weaving and
  merging sections ([FHWA](https://www.fhwa.dot.gov/publications/research/operations/07030/07030.pdf)).
- **No tool** claims weaving accuracy from its defaults. Calibration is per site.

MULTRA follows the same split: a synthetic fixture now, field data later (§7).

## 2. Geometry and routes

`data/projects/weaving-lab.traffic.json`, built by `tools/weaving_network.hpp` through the editor's
commands. `weavinglab` pins the file to the builder; regenerate it with `multra-weaving-fixture`.

- **Layout:** left-hand traffic and three separate scenes, 60 m apart. Each is a Type A weave of
  **150, 300 or 500 m**:
  - a 3-lane weave Link that starts at the network edge;
  - lane 0, the auxiliary lane on the kerb side, leads to a 1-lane exit Link;
  - lanes 1–2 lead to a 2-lane mainline Link, through 20 m Connectors.
- **Origin by entry lane:**
  - "ramp" vehicles enter on lane 0 and "mainline" vehicles on lanes 1–2, set by input lane shares;
  - two authored routes per scene, to the mainline and to the exit;
  - mainline → exit and ramp → mainline each cross the lane 0/1 line once inside the weave. From
    lane 2 to the exit is two changes.
- **Why the weave is the entry Link:**
  - A change is compiled only on a family's entry Link (M3_8_CONTRACT §2, D71/D93).
  - A weave drawn as a mainline and a ramp Link joining a 3-lane Link therefore cannot carry the
    crossing movements: those routes have no single-lane path (`UNSUPPORTED_ROUTE_TOPOLOGY`).
  - A routing decision on the weave does not help. No walk from the ramp reaches lanes 1–2, so its
    stubs are not kept.
  - This is the D96 limitation in another shape. BA179 pins it (`weavingTwoEntries()`).
  - The cost: the lab has no merge at the gore and no upstream approach. Vehicles are inserted at
    the gore.
- **Compiled:** every stub's dead end is the weave's end, so every change is made inside the weave.
  No conflict zone is compiled.

## 3. Demand

Plausible round numbers, not counts.

- **Volumes:**
  - mainline 3000 veh/h: 2100 to the mainline and 900 to the exit;
  - ramp 800 veh/h: 240 to the mainline and 560 to the exit.
- **Shares:** 30 % of each origin crosses, so the volume ratio is 0.3 (1140 of 3800 veh/h weave).
- **Vehicles:** cars only.
- **Timing:** inputs run 0–600 s, and the run lasts 900 s so the demand drains.

## 4. Variants

`multra-weaving-sweep <root> [--seeds 42-51] [--factors …]`, sharing `tools/weaving_sweep.hpp` with
the rows.

- **Demand factor:** scales every compiled input's volume.
- **Lateral off:** the catalog as shipped; changes are instantaneous.
- **Lateral on:** `lateralMinDistanceStanding` 0.5 m, `lateralMinDistanceAt50` 1.0 m and
  `maxLateralSpeed` 1 m/s on every behaviour of an owned catalog copy. These are the audit's keys;
  they are not shipped (D178).
- **Discretionary changes:** stay off (D102). No other parameter changes.

## 5. Measures

All are from existing accumulators. The engine does not know the fixture exists.

- **Per scene, from `MovementRow` and `LaneChangeRow`:**
  - served `vehicles` and `unfinished` (active or pending at the end);
  - mean delay, weighted by vehicles;
  - mandatory `changes`;
  - `waitingVehicles` and `longestWait` at the dead end.
- **Last arrival:** the latest arrival time of the scene's routes in a run. It is far before the end
  while vehicles remain, the sign that the scene stopped serving.
- **Per run:** safety clamps and a 2D body overlap after any tick, both as the lateral rows test them.
  Also `removed`, vehicles that left without arriving, which must be 0 (M3 §4: no diffusion).

A "capacity" read is the highest factor at which a scene still serves its demand. It is a property
of this fixture and this build, not of a road.

## 6. Rows

BA179–BA183 in [DRIVING_BEHAVIOUR](../plans/DRIVING_BEHAVIOUR.md) §3, in `weavinglab`. BA180–BA183
run the 300 m scene alone for 450 s, so they stay quick.

## 7. Limits and what comes next

- **Not validated** (rule 4). Nothing here is a weaving capacity of a real road.
- **HCM 7 Ch. 13** ([BOOKS](BOOKS.md#32-hcm-7-2022--capacities-los-m6-benchmarks-node-evaluation) §3.2) is a later comparison candidate, cited from the owner's
  copy when it is used. Field studies report it differs from simulation in lane-change rates, so it
  is not the truth either.
- **NGSIM US-101 and I-80** are the validation candidates for M6, once the owner approves a source
  entry.
- **The weave-end lock (D450–D454):**
  - A swapping pair is now kept while the front one closes in.
  - No change lands where a partner waits or will wait.
  - A change under way counts as on its target route.
  - No courtesy goes to a change that may not land, and such a change under way claims nothing.
  - The yielding partner brakes up to `maxDecelerationCooperativeBraking` once the front one closes in.
  - Locks remain switched off at ×1.0 (0–2 seeds of 10). Switched on, one lock is left from ×0.5 to ×1.0 ([evidence](../evidence/weaving-yield-cap.md)).
- **Gap seeking, slice 1 (D455):** a follower opens a gap for a refused stub anywhere in its span. One seed of 10 locks at ×1.0, off and on ([evidence](../evidence/gap-seeking-follower.md)).
- **No diffusion:** MULTRA never removes a stuck vehicle (M3 §4). A lock therefore shows as
  `unfinished` and a frozen last arrival, where Vissim would delete the pair after 60 s. Compare
  with Vissim only with that difference stated.
- **The gore:** a merge at the gore and a weave with two entries are not modelled (§2).
