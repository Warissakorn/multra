# src/eval

Accumulators fed by the engine's `SimState` snapshots and events, so the engine does not know they
exist. They measure simulated delay, travel time, queues and diagnostics. None of them computes HCM
control delay, and none is validated (rule 4). HCM analysis lives in `src/analysis/`; multi-seed
aggregation and confidence intervals in `src/runner/`.

## Dependencies

May read `src/core/types.hpp` only; no model, project, Qt or I/O (`multra-check-architecture`).

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `summary.*` | The incremental completed-trip diagnostic | [SIMULATION](../../docs/reference/SIMULATION.md) | project, runner, shell |
| `movement.*` | `MovementAccumulator`: one run's per-movement delay and travel time, and approach queue counters (Vissim's queue-counter conditions); owns the lots report | [SIMULATION](../../docs/reference/SIMULATION.md), [BATCH](../../docs/reference/BATCH.md) | project, runner |
| `sections.*` | Travel-time sections: each line as places on runtime segments; delay per section | [TRAVEL_TIME_SECTIONS](../../docs/reference/TRAVEL_TIME_SECTIONS.md) | this folder |
| `los.*` | Level-of-service packs as content (rule 5): per control type, the A-E delay bounds, applied to simulated section delay only | [LOS](../../docs/reference/LOS.md) | project |
| `nodes.*` | Micro node evaluation: groups a Node's turn sections and leg queue counters | [NODES §4](../../docs/reference/NODES.md) | model/network |
| `lots.*` | Each parking lot's occupancy, flows and gate waits, from snapshots alone | [ZONES_AND_PARKING §5](../../docs/reference/ZONES_AND_PARKING.md) | this folder |
| `discharge.*` | Per-head, per-lane queue discharge headways and start-up estimates, tick-quantised, with raw crossings and explicit unavailable reasons; not capacity, PCU or LOS | [DISCHARGE](../../docs/reference/DISCHARGE.md) | project, shell, tools |
| `discharge_passage.*` | `dischargeMotions`: reconstructs proven motion across remaps from the observer's own prior positions, unique engine spans and terminal evidence | [DISCHARGE](../../docs/reference/DISCHARGE.md) | this folder |
| `lane_changes.*` | Diagnostic: where mandatory lane changes happen and how long stub vehicles wait, per movement | [M3_8_CONTRACT §2](../../docs/reference/M3_8_CONTRACT.md) | project |
| `segment_times.*` | Diagnostic: mean time from departure to each runtime segment, per movement | [ARCHITECTURE](../../docs/ARCHITECTURE.md), `multra-cli` flag | project |
| `stop_lines.*` | Diagnostic: how each signal head's stop line discharges (standing time in red and green, greens waited) | [ARCHITECTURE](../../docs/ARCHITECTURE.md), `multra-cli` flag | project |
| `arrival_phases.*` | Diagnostic: histograms of when in the cycle vehicles reach and first stop on a segment, bounded to 16 MiB in all | [ARCHITECTURE](../../docs/ARCHITECTURE.md), `multra-cli` flag | project |
| `dead_end_waits.*` | Diagnostic: why a stub vehicle waits at its dead end, decided once per wait | [ARCHITECTURE](../../docs/ARCHITECTURE.md), `multra-cli` flag | project |

## Rules every observer keeps

- Motion is reconstructed only from the observer's own previous positions and unique engine span
  mappings, never from display traces. A lateral jump never counts as a longitudinal crossing.
- Sections and stop lines also count source and terminal trips missing from fleet snapshots.
- Discharge estimates are invalidated only by a remap of a vehicle queued at Go while ranks are
  still open. A same-tick source or sink takes its type from core's pure `upcomingArrivals` replay
  with an exact fingerprint.

## Tests

`tests/movement_tests.cpp`, `tests/queue_counter_tests.cpp`, `tests/travel_time_section_tests.cpp`,
`tests/los_tests.cpp`, `tests/node_evaluation_tests.cpp`, `tests/discharge_tests.cpp`,
`tests/discharge_passage_tests.cpp`, `tests/discharge_identity_tests.cpp`, `tests/arrival_phase_tests.cpp`,
`tests/wait_cause_tests.cpp`, `tests/lane_change_tests.cpp`, `tests/parking_output_tests.cpp`.
