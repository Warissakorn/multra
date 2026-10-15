# M3.5 slice 4: reaction-time action delay — 2026-10-15

Base `ff19ac5` plus D184. Contract: [PERCEPTION](../reference/PERCEPTION.md) §5–§8;
rows BA30, BA35 and BA36 of [the delivery plan](../plans/DRIVING_BEHAVIOUR.md).
Local verification is **Linux headless only**: GCC 14.2.0, CMake 4.4.4, Ninja 1.13.2,
nlohmann/json 3.11.3. Engine/libraries Release `-O3`, tests `-O1`, strict floating point.
Qt and Windows were not verified here. No milestone gate or calibration claim follows.

## Implementation

`FollowingResult` is the decision/FIFO-entry value in `core/types.hpp`; `Vehicle` owns an
oldest-first `reactionHistory`. Its size is `floor(reactionTime / dt + 0.5)`, zero when absent.
`perception.cpp` reads the applicable oldest entry without touching the shared motion snapshot.
The motion loop stages the undelayed decision after anticipation and courtesy, applies the old
entry through current type bounds and D105, and leaves integration and all physical caps in place.
At publish, in vehicle order, it resizes the FIFO, consumes the oldest and appends the decision.
Zero-delay motion allocates no decision list and carries no FIFO.

Source insertion and Lab placement initialise zero/free/no-state entries without RNG draws.
On a changed delay at a road boundary, shrinking drops oldest entries and growing prepends copies
of the oldest. The pre-motion front segment owns the crossing tick. Retunes compare quantised
delay counts for every behaviour, unused ones included, and refuse a change before mutating state;
values that quantise to the same count remain editable. Hypothetical `follow` calls are unchanged.

## Acceptance evidence

`tests/perception_delay_tests.cpp` covers:

- BA35: a two-car braking fixture, first forced on the undelayed engine, at dt 0.1/0.25/0.5;
  the follower's first braking is exactly d ticks after the leader's first braking.
- Source and Lab startup wait d ticks, on the prototype and W74; a full FIFO copied mid-run
  replays exactly through both const-copy and move stepping.
- BA30: absent, zero and a positive value rounded to zero give identical vehicles, events,
  queues and RNG at every tick on both models. The codec's retired unread-key assertion now
  compares the four-leg project tick by tick with zero reaction time.
- BA36: road-boundary trimming, padding and clearing; atomic retune refusal and same-count
  acceptance; current type bounds after a retune; delayed W74 state and mode.
- BA36 safety: an unseen physical leader, D105 restart suppression, red and dead-end caps,
  an occupied conflict area and a same-tick swept conflict, with late clamps counted.
- D105 also holds a stopped courtesy helper at its virtual standstill boundary, even when
  its physical leader is farther away. Stop service requires a full tick of rest. D108's source test still checks its ordinary first
  step. Courtesy and lane-change judgements are immediate, while the courtesy action is queued.

Checked on Linux headless: full Release build and `check` **97/97**, including all 16
new delay cases. The CLI seed-42 pin remains **45 completed / 31.75870154816726 s / 1 clamp**.
Qt desktop and Windows were not verified here.

**Mutation check:** a separately compiled copy of `perception.cpp` makes `reactionTicks` return
zero, with the production sources untouched. Eleven of the sixteen tests fail; five physical
safety/control tests still pass, as expected when instantaneous actions retain the old caps.

## Performance (§8)

Six interleaved pairs of `trafficsim-engine-benchmark 40 1800`, before and after, on the same
build environment, after the build and test suite finished. This is the unassigned scenario with no perception keys: 12,065 trips,
peak 626 vehicles. Wall times are machine-specific.

| Binary | Runs (ms) | Median (ms) |
|---|---|---|
| Before | 5124.24, 3807.93, 3865.19, 3845.17, 4651.21, 3799.76 | 3855.18 |
| After | 4108.86, 4147.22, 4437.30, 3857.52, 3850.85, 4499.44 | 4128.04 |

Median change: **+7.08 %**, below §8's 10 % unassigned-scenario budget. The timing spread
on this shared Linux host is large; these numbers compare this pair of binaries only. The
added vehicle-owned vector and zero-delay dispatch are not claimed free of cost.

## Reproduce

```
cmake --preset headless -DCMAKE_BUILD_TYPE=Release -DTRAFFICSIM_JSON_INCLUDE_DIR=<json-include>
cmake --build --preset headless -j 4
ctest --preset headless
cmake --build build/headless --target check
build/headless/bin/trafficsim-tests perceptiondelay
build/headless/bin/trafficsim-cli 42
build/headless/bin/trafficsim-engine-benchmark 40 1800
```

BA38's multi-seed discharge measurement is the next slice. Nothing here establishes that a
reaction-time value reproduces field data or Vissim.
