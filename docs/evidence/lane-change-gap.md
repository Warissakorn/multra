# Mid-change gap loss: waiting at the line — 2026-10-16

Branch `claude/m3.4-lane-change-duration-slice1` on `dc6b207` plus this change (D208). Local host:
Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Contract:
[LANE_CHANGE_DURATION](../reference/LANE_CHANGE_DURATION.md) §4; decision D208 (owner's choice).

## Rows (`tests/lane_change_duration_tests.cpp`, ctest `lanechangeduration`, BA54, 5 tests)

The `stub()` fixture: two lanes, the left a stub. Each row starts the change under way (a set
target) and asserts its forcing first.

| Row | Case | Asserts |
|---|---|---|
| BA54 | A 5 m/s changer, a 10 m/s car on the target lane 6 m behind it that passes during the move | The body never covers the lane with the car alongside; the car passed first; it then changes; no clamp, no braking beyond comfortable, no 2D overlap |
| BA54 | Body edge at the line, a car 1 m ahead on the target lane at the same speed (inside the 2 m standstill distance) | It waits (`lateralSpeed` 0) and never covers the lane; no clamp |
| BA54 | Body edge at the line, a 10 m/s car 1 m behind on the target lane (keys 0.5/1.0, so the car takes an overhang as its leader) | The car never brakes beyond comfortable; no clamp; the car goes by |
| BA54 | Body 0.25 m over the line, a slower car 1 m ahead there (keys 0.5/1.0) | It steps back until it no longer covers the lane; no clamp; no overlap |
| BA54 | A leader 60 m ahead on the target lane | Control: the move runs at 1 m/s every tick |

Mutation checks (each reverted): without the alongside test, rows 1 and 4 fail; without the
leader test, row 2; without the follower test, row 3; without the step back, row 4. Row 3 survived
its mutation with keys 0/0 (dLat 0: a 0.1 m overhang is no car's leader) and was moved to 0.5/1.0.

`check` **163/163**; `trafficsim-cli 42` byte-identical; switched-off `lane-change-lab`,
`four-leg-signalised` and `m2.6-study-template` (seeds 42–51) identical to `dc6b207`.

## How the rule was found (switched-on runs, keys 0.5/1.0/1, seeds 42–51)

| Version | lane-change-lab clamps | four-leg-signalised clamps |
|---|---|---|
| `dc6b207` (D207) | 2 (seed 49) | 0 |
| Leader ahead and alongside only, per segment | 29 | 2 |
| + step back over the line | 20 | — |
| + follower behind | 0 | 2 |
| **+ route distances instead of per segment (D208)** | **0** | **0** |

The intermediate causes, each probed: a changer waiting with its body over the line overtook the
lane's car, which then found it as a leader at a negative gap; a waiting changer pulled 0.06 m
clear ahead of a follower and stepped over; a changer leaving a Connector stepped over while the
follower was still on the Connector segment. Completed trips and active counts are unchanged
against `dc6b207` in both projects; lab mean delay moves by ≤ 0.05 s.

## Performance

Release, both binaries from the same build directory, five interleaved pairs, `--seeds 42-51
--threads 1`, switched on, process wall medians: `lane-change-lab` **1963 → 2091 ms** (+6.5 %),
`four-leg-signalised` **1294 → 1222 ms** (noise). The test scans every vehicle for each outward move
that covers a lane; it adds to D206's owed switched-on cost.

## Not covered here

- A straddler still does not follow the other lane's leaders (LATERAL §6 says "the follower's
  carriageway"; the code searches its own lane). The step back keeps it from waiting over the line,
  but a body crossing at speed is still guided only by this start-like test.
- A changer that waits past its span window keeps its target (no abandon, §9).
