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
