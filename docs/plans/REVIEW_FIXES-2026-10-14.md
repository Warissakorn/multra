# Code-review correction plan — 2026-10-14 (D167)

Owner-approved plan for the supplied TrafficSim review of main `c26487c744dcff64b0fc86ed00d69dbe1d602f8a`.
The current request authorizes every item together; separate commits keep each system reviewable.
Contracts are the maintained semantics; NEXT owns follow-up and PROGRESS records verification.

| Review | Interface / behavior | Regression |
|---|---|---|
| R4 | Section and stop-line observers process terminal events and source-sink trips inside one tick | `traveltime`, `movement` |
| R3 | Record shared-prefix start even before a positioned choice; require actual end passage | `traveltime`, TRAVEL_TIME_SECTIONS TT13 |
| R2 | Display trace distinguishes logical remap from lateral blend; selected suffix and rear rolling stay continuous | `lane-change-pose`, `lane-change-display-ui` |
| R1 | CLI/accumulator refuse non-finite/non-positive widths; at least one bin, bounded histogram memory | `movement`, `cli-phase-bin` |
| R5 | `resizeVehicleLength` scales explicit wheelbase/overhangs; UI and project Apply use it, validator unchanged | `lab`, `behaviour-lab-ui` |
| R6 | Graph distances decide reachability; expand shortest alternatives with explicit budgets | `odexpand` |
| R7 | Split moves downstream decision/station; continuity-span station refuses atomically | `stationrouting` |
| R8 | Identities include initial occupancy; whole-run trips minus cumulative admissions conserve identities | `parkingoutputs` |
| GUI risk | Tick cancellation, owned retiring workers, generation-gated progress/results; no join at restart | `batch`, `batch-run-ui` |
| OD observation | Report per-cell skipped vehicle volume; keep existing normalization policy | `odexpand`, `OD_SKIPPED_DEMAND` advisory |

Verification sequence: clean baseline; regression failures against old code; focused fixed tests;
headless full suite and repository checks; desktop CI on Linux and Windows; owner appearance
review remains separate. Preserve frozen fixtures, unvalidated markers and every milestone gate.
