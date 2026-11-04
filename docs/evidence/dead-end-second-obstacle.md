# The dead end as a second obstacle (D456)

2026-11-04. Not validated; development evidence (rule 4). Linux, headless build.

## Cause

Gap seeking slice 1 (D455) left one cost: in the weaving lab, switched on at ×1.0, safety
clamps rose from 7 to 23 ([gap-seeking-follower](gap-seeking-follower.md)).

A scratch probe was run on the full lab at ×1.0, switched on, seeds 42–51. It logged each clamped
vehicle's last 6 s, and set its actual acceleration against what `follow` gives for its dead end
alone.

- **13 of the 23 clamps are on a stub route.** In all 13, a moving vehicle ahead hid the dead end.
  - **Vehicle 125, seed 47:** the dead end alone asked for −1.5 m/s² at 66 m, yet the vehicle held
    about 0 m/s² at 12.5 m/s. It was following a vehicle about 16 m ahead whose route went on past
    the dead end.
  - When the dead end became nearer than that leader, the vehicle braked at −8 m/s² and was
    clamped.
  - Its own change was under way, stalled at a lateral of 0.8 m.
- **The other 10 are not on a stub route.** Most are vehicles behind such a hard stop.
- **Why:** the dead end was merged into one leader, and the nearest obstacle wins. So a moving
  vehicle nearer than the dead end hid it. The courtesy hold already avoided this by being a second
  obstacle.

## Change

`stepSimulation` (`src/core/simulation.cpp`): when the leader is nearer than the dead end,
car-following runs once more against the dead end as a standing obstacle, and the lower
acceleration is kept (a strict `<`, as for the courtesy hold).
- The hold cap and the stop at the line are unchanged.
- Routes without a dead end, or without lane changes, are byte-identical.
- No new key.

## Rows (failure-first)

- **BA196** (`laneswap`):
  - **Setup:** a stub car at 12 m/s, 66 m short of its dead end, behind a 12 m/s car whose route
    goes on. A car alongside its target place refuses its change.
  - **Before** (a probe linked against the previous `simulation.cpp`): 0 m/s² on the first step,
    then −8 m/s².
  - **After:** −1.39 m/s² on the first step, never harder than −1.39 m/s², and no clamp.
- **BA197** (`weavinglab`): the 500 m weave alone, ×1.0, switched on, seed 42, 450 s.
  - **Before:** 2 clamps, 381 served.
  - **After:** 0 clamps, 387 served.

## Weave scenes alone, ×1.0, switched on (450 s, seeds 42–51)

| Scene | Clamps, before → after | Served, sum before → after | Mean delay over seeds, before → after (s) |
|---|---|---|---|
| 150 m | 1 → 0 | 2761 → 2973 | 72.2 → 67.5 |
| 300 m | 2 → 0 | 3027 → 3335 | 54.1 → 44.1 |
| 500 m | 9 → 0 | 3717 → 3868 | 28.9 → 24.2 |

Every seed of every scene loses all of its clamps, and fewer vehicles are left unfinished.

**Full lab, ×1.0, switched on, seeds 42–51:** clamps 23 → 0.

## Product runs (seeds 42–51, against D455's batches)

| Project | Off: delay (s), completed | On: delay (s), completed |
|---|---|---|
| Four-leg | 47.49 → 47.48; 504.7 → 504.7 | 47.64 → 47.60; 504.7 → 504.7 |
| Lane-change lab | 9.03 → 9.03; 873.3 → 873.3 | 9.07 → 9.06; 873.3 → 873.3 |
| M2.6 | 46.40 → 46.37; 1959.5 → 1959.1 | 46.60 → 46.57; 1958.5 → 1958.9 |
| T-junction | identical | identical |

Clamps are unchanged everywhere (M2.6: 0.1 per seed, off and on).

## Weaving sweep (seeds 42–51, the full lab, against D455)

`multra-weaving-sweep . --seeds 42-51 --factors 0.3,0.4,0.5,0.6,0.8,1.0,1.2`.

**Locked seeds:**

| Run | D455 | D456 |
|---|---|---|
| Off at ×1.0 | 0/1/0 | 0/0/0 |
| On at ×1.0 | 0/1/0 | 0/0/0 |

No seed locks at any factor, off or on.

**Clamps** (summed over the seeds and scenes):

| Factor | Off | On |
|---|---|---|
| ×0.3 | 0 → 0 | 2 → 0 |
| ×0.4 | 4 → 0 | 2 → 0 |
| ×0.5 | 5 → 0 | 4 → 0 |
| ×0.6 | 9 → 0 | 15 → 0 |
| ×0.8 | 12 → 0 | 12 → 0 |
| ×1.0 | 21 → 0 | 23 → 0 |
| ×1.2 | 37 → 0 | 14 → 1 |

**The D450 (a) overrun clears:** at ×0.4 on, 500 m, D455 left 4.5 vehicles per seed unfinished.
Every vehicle is now served.

**Mean delay on:**

| Factor | 150 m | 300 m | 500 m |
|---|---|---|---|
| ×0.8 | 64.1 → 43.4 s | 24.2 → 19.7 s | 15.1 → 13.1 s |
| ×1.0 | 150.6 → 137.3 s | 123.8 → 97.7 s | 64.1 → 40.5 s |

Waiting at a dead end falls at every factor.

**Still open:**
- Switched on at ×1.0, the 150 m weave leaves 25 of about 640 vehicles per seed unfinished, though
  arrivals continue to the end.
- ×1.2 switched on is over capacity.
- The 150 m weave switched on at ×0.8 still averages 43 s of delay. That is the dense-stream case
  gap seeking slice 2 targets.


