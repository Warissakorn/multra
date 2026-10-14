C++ accumulators fed by engine states and events. `summary` is the completed-trip diagnostic; `movement` is the M2.5 per-movement delay/travel-time and approach-queue report for one run (see `docs/reference/SIMULATION.md`); `lane_changes`, `segment_times`, `stop_lines`, `arrival_phases` and `dead_end_waits` are M3.2.8c diagnostics (CLI flags). `lots` is the M8.5 parking-lot report (occupancy, flows, gate wait), owned by `MovementAccumulator` (D160). HCM control delay, LOS letters, multi-seed aggregation and confidence intervals remain planned (M5).

`discharge` adds independent lane/cycle queue headways and startup estimates (M3.3.1a), with raw crossings and explicit unavailable reasons. See [contract](../../docs/reference/DISCHARGE.md); vehicle-type follower filters and CLI windows/ranks are available; the project layer emits captured input hashes, and shared-prefix route recognition is supported. `discharge_passage` reconstructs proven post-remap/source motion from prior positions, unique engine spans and terminal evidence; it never reads display traces. Only remaps of vehicles queued at Go, while ranks are still open, invalidate estimates; same-tick source-sink types come from core's pure `upcomingArrivals` replay with an exact fingerprint (D125).

Section observers bind provisional starts independently and measure actual longitudinal metres
through proven event/span remaps. Sections and stop lines reconstruct source/terminal trips absent
from fleet snapshots. Arrival-phase widths are finite/positive; derived histograms are bounded to 16 MiB across all rows.
Stop-line passages also map the prior front through unique engine spans before testing motion;
a lateral coordinate jump cannot count as a longitudinal crossing.
