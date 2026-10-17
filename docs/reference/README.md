# Reference and contracts

Current behaviour, numerical contracts and file semantics. Plans and dated
acceptance records live in [plans](../plans/README.md); milestone status lives in
[ROADMAP](../ROADMAP.md). Read the applicable contract and decision, not every file.

| Document | Scope |
|---|---|
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
