# src/core

The deterministic C++20 microsimulation engine: a compiled `Scenario` and a seed in, `SimState`
snapshots and `SimEvent`s out. Units are metres, seconds, m/s and m/s². Contract:
[SIMULATION](../../docs/reference/SIMULATION.md), [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md)
and the behaviour contracts linked per file below.

## Dependencies

Imports nothing outside this folder and a reviewed set of standard headers: no Qt, JSON, model
types, file I/O or wall clock; one seeded RNG in `SimState` (`multra-check-architecture`).
Everything the tick decides reads the immutable pre-step snapshot, so no decision depends on the
order vehicles are visited. Each optional behaviour below is off unless its key or switch is set,
and then the step is unchanged to the bit.

## Files

| File | Holds | Contract |
|---|---|---|
| `types.hpp` | The runtime contract: `Scenario`, `SimState`, `Vehicle`, `SimEvent`, behaviours (`effectiveBehaviour` by front segment), `upcomingArrivals` | [SIMULATION](../../docs/reference/SIMULATION.md) |
| `simulation.*` | Public API: `createSimulation`, `stepSimulation`, `runSimulation`, `totalTicks`, `canonicalScenario`; `retuneSimulation` and `placeVehicle` (in `retune.cpp`, for the Behaviour Lab only); `signalColorAt` (in `signals.cpp`). `demand.cpp` draws arrivals | [SIMULATION](../../docs/reference/SIMULATION.md), [BEHAVIOUR_LAB §1](../../docs/reference/BEHAVIOUR_LAB.md) |
| `validate.*` | `validateScenario`, `assertValidScenario`, `onTimeGrid` | [SIMULATION](../../docs/reference/SIMULATION.md) |
| `detail.hpp` | splitmix64's finaliser and the canonical id order shared by create and retune | [SIMULATION](../../docs/reference/SIMULATION.md) |
| `routes.*` | The run index: routes as segment indexes, `appendVehicleSpans` (the occupied spans every leader search reads) | [SIMULATION](../../docs/reference/SIMULATION.md) |
| `routing.*` | Station route decisions: `routingIssues`, `nextRouteDecision`, `applyRouteDecision` (one seeded draw per selection) | [POSITIONED_ROUTING](../../docs/reference/POSITIONED_ROUTING.md) |
| `following.*` | `followingAcceleration` and `follow`, the one car-following entry every model runs through | [SIMULATION](../../docs/reference/SIMULATION.md), [W74 §6](../../docs/reference/W74.md) |
| `w74.*` | Wiedemann-74-structured car-following | [W74](../../docs/reference/W74.md) |
| `leaders.*` | Leader searches: `forCoveredSpans` over every lane a straddling body covers | [LATERAL §6](../../docs/reference/LATERAL.md) |
| `lanes.*` | Lane changes: mandatory changes off stub routes, discretionary changes, keep-to-kerb (`kerbParameterIssues`), `decideLaneChanges`, `recordLaneChange` | [M3_8_CONTRACT §2](../../docs/reference/M3_8_CONTRACT.md), [OVERTAKING](../../docs/reference/OVERTAKING.md), [MERGE_DECELERATION](../../docs/reference/MERGE_DECELERATION.md) |
| `conflicts.*` | Crossing admission: zones by route distance, right of way, Stop and give-way service, commitment; `tickCap` and `relievedCap` (in `caps.cpp`) | [M3_CONTRACT §4](../../docs/reference/M3_CONTRACT.md), [COMMITMENT](../../docs/reference/COMMITMENT.md), [MERGE_CLAMPS](../../docs/reference/MERGE_CLAMPS.md), [STOP_DWELL](../../docs/reference/STOP_DWELL.md) |
| `line_state_types.hpp` | Per-vehicle state at a line kept between ticks: `StopService`, the `RedRelease` pairs | [M3_CONTRACT §5](../../docs/reference/M3_CONTRACT.md), [RED_RUNNING §3](../../docs/reference/RED_RUNNING.md) |
| `red_running.*` | The red-runner draw, `redElapsed`, `letThrough` | [RED_RUNNING](../../docs/reference/RED_RUNNING.md) |
| `join_anticipation.*` | `indexJoins`, `joinEntries`, `anticipateJoin`: a second following law across a join, only with `clampRules.joinAnticipation` | [JOIN_ANTICIPATION](../../docs/reference/JOIN_ANTICIPATION.md) |
| `perception.*` | Perception keys and ranges, the reaction-time action FIFO, `perceivedLeaders` | [PERCEPTION](../../docs/reference/PERCEPTION.md) |
| `lateral_types.hpp` | `DrivingSide`, the compiled `SegmentCrossSection` | [LATERAL](../../docs/reference/LATERAL.md) |
| `lateral.*` | The lateral key table (one list for codec, validator and serializer), lateral spans and filters | [LATERAL §3, §9](../../docs/reference/LATERAL.md) |
| `lateral_motion.*` | Lateral motion toward a `lateralTarget` after the longitudinal step, admitted in vehicle-id order without 2D overlap | [LATERAL §7](../../docs/reference/LATERAL.md) |
| `speed_control.*` | Curve speed: the switch, key checks and `speedTarget` from `Segment::curvature` | [SPEED_CONTROL](../../docs/reference/SPEED_CONTROL.md) |
| `speed_decision_types.hpp` | A type's desired-speed range and curve; a desired-speed decision as run | [SPEED_DISTRIBUTION](../../docs/reference/SPEED_DISTRIBUTION.md), [SPEED_DECISIONS](../../docs/reference/SPEED_DECISIONS.md) |
| `speed_distribution.*` | Speed of a cumulative share and its inverse; moving a curve onto an edited range | [SPEED_DISTRIBUTION §3](../../docs/reference/SPEED_DISTRIBUTION.md) |
| `speed_decision.*` | Desired-speed decisions crossed this tick, by route distance | [SPEED_DECISIONS §3](../../docs/reference/SPEED_DECISIONS.md) |
| `speed_area.*` | Reduced-speed areas bounding the speed target | [SPEED_DECISIONS §7](../../docs/reference/SPEED_DECISIONS.md) |
| `parking_types.hpp` | A parking lot as the engine runs it (ids, resolved once by the index) | [ZONES_AND_PARKING §4](../../docs/reference/ZONES_AND_PARKING.md) |
| `parking.*` | The lot runtime: gate hold, admission, dwell, departures that keep their space until inserted | [ZONES_AND_PARKING §4](../../docs/reference/ZONES_AND_PARKING.md) |

## Entry points

`src/model/` and `src/project/` compile a document into a `Scenario`; `src/runner/`, `src/shell/`
and `multra-cli` step it; `src/eval/` observes the snapshots.

## Tests

`tests/core_tests.cpp` and most of the `multra-tests` groups (behaviour, conflicts, lane changes,
lateral, perception, speed, parking); `tests/seed_mixing_tests.cpp` for seeding.
