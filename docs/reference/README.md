# Reference and contracts

Current behaviour, numerical contracts and file semantics. Plans and dated
acceptance records live in [plans](../plans/README.md); milestone status lives in
[ROADMAP](../ROADMAP.md). Read the applicable contract and decision, not every file.

| Document | Scope |
|---|---|
| [DESKTOP_AUTOMATION.md](DESKTOP_AUTOMATION.md) | Local JSON commands for AI control of the visible desktop, live enable/disable and reconnect (D344), guarded New and intentional Quit (D345), deferred recovery (D346), map commands/tile readiness (D347), Connector proposals/attachment diagnostics (D348), single-run identity/explicit export (D349), codec-backed discovery/dry validation and run-until (D350), Windows PowerShell workflow, document guards and atomic edits |
| [ZONES_AND_PARKING.md](ZONES_AND_PARKING.md) | Zones, zone connectors, OD matrices and parking lots: schema 31, OD → Micro expansion, lot runtime, outputs, editor and rows (M8) |
| [MAP_BACKGROUND.md](MAP_BACKGROUND.md) | Schema 29, UTM reference, OSM tiles, caches, paint and acceptance rows |
| [BATCH.md](BATCH.md) | Multi-seed batches: runs, aggregate, 95 % CI, overloaded seeds and output (M5.2); scenario comparison `--compare` (M5.8, §7) |
| [LOS.md](LOS.md) | Level of service from section delay: the HCM pack as data, control types, approach/intersection rows (M5.5) |
| [TRAVEL_TIME_SECTIONS.md](TRAVEL_TIME_SECTIONS.md) | Travel-time sections: lines, crossings, section delay, output and acceptance rows (M5.4) |
| [DISCHARGE.md](DISCHARGE.md) | Lane/cycle discharge and startup diagnostics, quantization and unavailable results |
| [DRIVING_BEHAVIOUR.md](DRIVING_BEHAVIOUR.md) | Class/road behaviour assignment: library storage, runtime selection and editor (M3.3.2a–c) and the target capability map |
| [BEHAVIOUR_LAB.md](BEHAVIOUR_LAB.md) | Behaviour Lab: circuit routes, live retune, vehicle placement, the lab ring and window, rows BL1–BL12 (M3.3.4) |
| [LATERAL.md](LATERAL.md) | Two-dimensional occupancy: lateral frame, cross-sections, spans on every overlapped lane, lateral admission, zones and display (M3.7.0 contract, D180; slices 1–4 implemented, D187–D189, D202, D203) |
| [BEHAVIOUR_PARAMETERS.md](BEHAVIOUR_PARAMETERS.md) | Every value that shapes driving behaviour: settable now, should become settable, kept fixed (D213, D214), and each setting's scope, project-wide or local (D216) |
| [COMMITMENT.md](COMMITMENT.md) | Commitment deceleration at give-way lines: `commitmentDeceleration`, the lower of it and the type maximum, rows BA72–BA78 (B-2 contract, D218; implemented, D219) |
| [MERGE_DECELERATION.md](MERGE_DECELERATION.md) | Accepted deceleration for mandatory lane changes: own and trailing keys, rows BA79–BA85 (B-3 contract, D220; implemented, D221) |
| [SPEED_DECISIONS.md](SPEED_DECISIONS.md) | Desired-speed decisions (per-type ranges at a Link station, quantile kept, delay against the speed in force) and reduced-speed areas (temporary per-type speed over a Link stretch, by lane); rows BA86–BA108 (B-6, D222, D224; implemented, D223, D225) |
| [SPEED_DISTRIBUTION.md](SPEED_DISTRIBUTION.md) | Desired-speed distribution curve on a vehicle type: piecewise-linear cumulative points, the same draw, the share as quantile, schema 40, rows BA110–BA116 (B-5 contract, D228; implemented, D229) |
| [LANE_PREFERENCE.md](LANE_PREFERENCE.md) | Lane preference by vehicle type: `kerbLanes` on the type, entry only on the kerb-most N lanes, absent = today, schema 42, rows BA124–BA130 (B-7 contract, D233; implemented, D234) |
| [JOIN_ANTICIPATION.md](JOIN_ANTICIPATION.md) | Anticipation across a join (M3.2.8a.1b): prototypes measured (D253); A4 implemented as the off-by-default `clampRules.joinAnticipation`, schema 46, rows BA168–BA172 (D254, D255) |
| [LINE_APPROACH.md](LINE_APPROACH.md) | Stopping cleanly at a line and behind a hard stop (M3.2.8a.1c): floors measured (D256); F1a implemented as `clampRules.standingStopFloor`, on by default, schema 47, rows BA173–BA178 (D257, D258) |
| [NODES.md](NODES.md) | Nodes (S0, M5.1): a polygon over a junction, derived legs, turns and control, Micro per-turn delay, queue and letters; owner choices answered (D260); S0.1 object, codec, tool and tab (D261) S0.2 membership (D262, D263) S0.3 evaluation (D264) and S0.5 Suggest nodes (D265) implemented; S0.6 Node rows in Results (D278) |
| [MACRO_NETWORK.md](MACRO_NETWORK.md) | Macro network and static assignment (S2, M10.1): the drawn network as a Macro graph, link types and VDF data, one study period's demand in PCU, Frank–Wolfe UE; contract written (D271), owner choices answered (D272); S2.1 link types and the Macro graph implemented (D273); S2.2 assignment implemented (D274); S2.3 run settings, outputs and CLI implemented (D275); run procedure studied (D276); S2.4 editor surface implemented (D277) |
| [MESO_NETWORK.md](MESO_NETWORK.md) | Meso network and link-transmission loading (M11): the Meso graph from the Macro graph and Nodes, jam density per link type, LTM links (Yperman), the Tampère node model, signals as averaged green, spill-back, outputs by time slice, `--meso` and the editor surface; the contract with its owner choices (D291, D292, amended D297–D300) |
| [MESO_NETWORK_IMPLEMENTED.md](MESO_NETWORK_IMPLEMENTED.md) | What M11.2–M11.4b built (MESO_NETWORK §14–§21): files, functions, rows ME1–ME28 and the mutations each caught |
| [MESO_ROAD_DEMAND.md](MESO_ROAD_DEMAND.md) | Schema-57 Road targets in Meso: minimum free-flow-time OD paths, connector shares, PCU/windows, refusal rules, editor/CLI and rows MR1–MR6 |
| [MACRO_NETWORK_IMPLEMENTED.md](MACRO_NETWORK_IMPLEMENTED.md) | What S2.1–S2.4 built (MACRO_NETWORK §16–§18, §20): files, functions, rows and the mutations each sweep caught |
| [BOOKS.md](BOOKS.md) | The owner's five textbook copies (Sheffi, HCM 7, Ortúzar & Willumsen, the Planning Handbook, Yperman): attaching, citing, conversion caveats and the chapters that serve each milestone |
| [STUDY_PERIODS.md](STUDY_PERIODS.md) | StudyPeriod (S1): study-clock periods for OD matrices, the run's `clockStart`, the Mode catalog and vehicle-type PCU; contract written (D266), owner choices answered (D267); S1.1 study periods implemented (D268); S1.2 modes and PCU implemented (D269, D270) |
| [ROADS.md](ROADS.md) | Roads (S3): the Road and its generated Links, cross-section templates, Nodes from crossings, S0.4's coarse Node values, seam checks and HCM-cited link types; contract written (D303), owner choices answered (D304); S3.2–S3.7 implemented, S3 complete (D304, D305, D307–D312), described in ROADS_IMPLEMENTED |
| [ROADS_IMPLEMENTED.md](ROADS_IMPLEMENTED.md) | S3.2–S3.7 and Road-only Macro demand (ROADS §14–§22): files, functions, lifecycle and rows RD1–RD34 |
| [READINESS.md](READINESS.md) | Readiness, *Fill defaults* and Node proposals (S4): the Problems tab by level with seam checks, cited fills, proposals from results; contract written and answered (D313); S4.2–S4.4 implemented (D314, D316, D317); S4 complete |
| [STUDY_AREA.md](STUDY_AREA.md) | Study area, run-time cut and stale marking (S5): the study area as a set of Nodes, Micro on its kept set with gates, typed, Macro and Meso gate flows, side files with SHA-256 staleness, export marking; contract written and answered (D326); S5 implemented (D327–D332), described in STUDY_AREA_IMPLEMENTED |
| [STUDY_AREA_IMPLEMENTED.md](STUDY_AREA_IMPLEMENTED.md) | What S5.2a–S5.5b built (STUDY_AREA §16–§21): files, functions, owner answers and rows SA1–SA16 |
| [MERGE_CLAMPS.md](MERGE_CLAMPS.md) | M2.6's 41 remaining clamps by cause (merge creep, pull-out, late catch-up, red-head creep) and the measured options; B+M+Q chosen as three project switches, `clampRules`, on by default (D250); implemented (D251): M2.6 41 → 17; rows BA158–BA167 |
| [GIVE_WAY_STOP.md](GIVE_WAY_STOP.md) | A feasible stop at a give-way line: on a tick that clamps today, a stop within the type's maximum brakes at `v²/(2·gap)` instead; rows BA146–BA151 (M3.2.8a.1 slice, D239; implemented, D240) |
| [RED_RUNNING.md](RED_RUNNING.md) | Red-light running share: `redRunningShare` and `redRunningTime` on the driver behaviour, runners pass red only in its first seconds while moving, a counted `red-running` event, absent = today, schema 44, rows BA138–BA145 (B-9 contract, D237; implemented, D238; M2.6 clamps traced, D241) |
| [DRIVER_FACTOR.md](DRIVER_FACTOR.md) | Driver-factor spread: `driverFactorSpread` on the driver behaviour, the standard deviation of the factor drawn when a vehicle is made, absent = 0.15, schema 43, rows BA131–BA137 (B-4 contract, D235; implemented, D236) |
| [STOP_DWELL.md](STOP_DWELL.md) | Stop dwell time: `stopDwellTime` on the driver behaviour, rest in whole ticks of at least that time, absent = today, schema 41, rows BA117–BA123 (B-8 contract, D230; implemented, D231) |
| [OVERTAKING.md](OVERTAKING.md) | Keep to the kerb lane: `kerbLaneBias`, return/pass/undertake gains, rows BA64–BA71 (M3.4 contract, D213; slice 1 implemented, D215) |
| [SPEED_CONTROL.md](SPEED_CONTROL.md) | Curve speed on Connectors: the behaviour key, the radius, the anticipated bound, rows BA56–BA63 (M3.6 contract, D211; slice 1 implemented, D212) |
| [LANE_CHANGE_DURATION.md](LANE_CHANGE_DURATION.md) | Lane changes as lateral moves: start rule before conflict areas, crossing, end, display, rows BA48–BA53 (M3.4 contract, D204; slice 1 implemented, D206) |
| [PERCEPTION.md](PERCEPTION.md) | Look-ahead, observed vehicles, look-back, rear time-to-collision and reaction time (M3.5.0 contract, D179; not implemented) |
| [W74.md](W74.md) | W74 car-following equations, parameters, traits, state and model switch (runs since D138; not validated) |
| [POSITIONED_ROUTING.md](POSITIONED_ROUTING.md) | Positioned Route recognition and schema 20 |
| [SIMULATION.md](SIMULATION.md) | Simulation core and network model |
| [ENGINEERING_NOTES.md](ENGINEERING_NOTES.md) | Dated measurements, recipes, labs, test notes and the do-not-retry list (moved from NEXT 2026-10-15) |
| [AMBER.md](AMBER.md) | Amber stop-or-go: the continuous check, `amberDeceleration`, M0 legacy and acceptance rows (M4.2) |
| [VEHICLE_POSE.md](VEHICLE_POSE.md) | Vehicle pose — rear axle reference and continuous lane changes |
| [PROJECT_SCHEMA.md](PROJECT_SCHEMA.md) | Project schema inventory and partial formal envelope/background/Network/Link/Lane/Connector/reference/SignalHead and signal/right-of-way control measurement and base Route/VehicleInput/RoutingDecision coverage, with version rules |
| [NETWORK_EDITOR.md](NETWORK_EDITOR.md) | Native network editor |
| [NETWORK_EDITOR_CONNECTORS.md](NETWORK_EDITOR_CONNECTORS.md) | Native network editor — Connectors |
| [CONNECTOR_FOUR_POINT_MOUTH.md](CONNECTOR_FOUR_POINT_MOUTH.md) | Centred Connector axis and four-point mouths |
| [EDITOR_WORKFLOW.md](EDITOR_WORKFLOW.md) | History, keyboard editing and rotation |
| [AUTHORING_EXTENSIONS.md](AUTHORING_EXTENSIONS.md) | Link geometry actions, positional-reference safety and shared markings |
| [MIGRATION.md](MIGRATION.md) | Native entry points, frozen migration baselines and replay compatibility |
| [DEMAND_TIME_TYPES.md](DEMAND_TIME_TYPES.md) | Time and type Demand — slice 6 |
| [DEMAND_CATALOGS.md](DEMAND_CATALOGS.md) | Project Demand catalogs — slice 5 |
| [M3_CONTRACT.md](M3_CONTRACT.md) | M3 right-of-way contract |
| [M3_8_CONTRACT.md](M3_8_CONTRACT.md) | M3.2.8 behaviour contract |

Return to [the task reading map](../README.md#read-by-task).
