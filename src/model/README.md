# src/model

The authoring model and its compilation into the engine's `Scenario`: the drawn network in
`network/`, demand and signal control in `demand/` ([its map](demand/README.md)). Loading and saving
live in `src/project/`; the commands that edit the model in `src/commands/`.

## Dependencies

May read `src/core/`, `src/analysis/`, `src/macro/` and `src/meso/` headers. No
Qt, JSON or file I/O: the codecs are in `src/project/`. Derived data (runtime sections, Connector
paths, Node membership, conflicts) is computed on every read and never stored.

## network/

| File | Holds | Contract | Used by |
|---|---|---|---|
| `network.hpp` | The network value types (Links, lanes, Connectors, heads, sections) and the functions on them, implemented in `geometry.cpp`, `connector_geometry.cpp`, `connector_equation.cpp` (arc length by Gauss-Legendre), `connector_paths.cpp`, `road_boundaries.cpp`, `markings.cpp`, `head_slots.cpp`, `sections.cpp`, `lane_family.cpp`, `routing.cpp` (routes name Links and Connectors, never lanes or paths), `validate.cpp`, and `compile.cpp`: `buildScenario` and `compileScenario` | [SIMULATION](../../docs/reference/SIMULATION.md), [NETWORK_EDITOR_CONNECTORS](../../docs/reference/NETWORK_EDITOR_CONNECTORS.md), [LANE_PREFERENCE](../../docs/reference/LANE_PREFERENCE.md) | editor, lab, project |
| `connector_lane_mapping.*` | Lane-range topology shared by width derivation and runtime paths; never reads geometry | [CONNECTOR_FOUR_POINT_MOUTH](../../docs/reference/CONNECTOR_FOUR_POINT_MOUTH.md) | this folder |
| `connector_surface.*` | The Connector's four-point mouth and painted lane surfaces; `ConnectorSurfaceCache` for one immutable calculation, destroyed before any edit | [CONNECTOR_FOUR_POINT_MOUTH](../../docs/reference/CONNECTOR_FOUR_POINT_MOUTH.md) | editor |
| `connector_attachment.*` | The 0.01 m lane-attachment measurement shared by validation and shell diagnostics | [NETWORK_EDITOR_CONNECTORS](../../docs/reference/NETWORK_EDITOR_CONNECTORS.md) | shell |
| `control.hpp` | Authored right-of-way controls as values | [M3_CONTRACT §1](../../docs/reference/M3_CONTRACT.md) | this folder |
| `right_of_way.*` | What those controls mean: structural issues (reject the edit) apart from runtime issues; automatic conflicts (`automatic_conflicts.cpp`), coverage (`conflict_coverage.cpp`), crossings and groups | [M3_CONTRACT §2](../../docs/reference/M3_CONTRACT.md) | commands, editor, project, shell |
| `conflict_surface.*` | Conflict geometry on the same painted lane rails as display and paths; stations in authored metres | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md) | editor |
| `conflict_display.*` | Conflict and mouth polygons and their station spans for display (`conflict_polygons.cpp`, `conflict_bands.cpp`, `conflict_mouth_bands.cpp`) | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md) | editor |
| `conflict_polygon_math.*` | The inverse bilinear cross-section map shared by overlap and mouth measurement | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md) | this folder |
| `diagnostics.*` | Draft issues (reject an edit) apart from runtime issues (cannot run yet); an empty network is a legal draft | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) | project |
| `display.*` | Display types and vehicle colours by type | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md) | editor, project, shell |
| `rotation.*` | Rotating a selection: what carries geometry and the centre it turns about | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) | commands, editor |
| `routeless.*` | Vehicles with no route, as Vissim runs them: follow the lane, equal shares at splits | [SIMULATION](../../docs/reference/SIMULATION.md) | project |
| `travel_time.*` | What an authored travel-time section means, and its checks | [TRAVEL_TIME_SECTIONS](../../docs/reference/TRAVEL_TIME_SECTIONS.md) | project |
| `polygon.*` | Plane polygon tests shared by zones and Nodes | [NODES §2](../../docs/reference/NODES.md) | commands, model/demand, project |
| `nodes.*` | What an authored Node means: id checks; legs, turns and control are derived | [NODES §2](../../docs/reference/NODES.md) | this folder |
| `node_membership.*` | A Node's legs, turns and conflicts, derived from the Micro detail inside its polygon on every read; refusals, never exceptions | [NODES §3](../../docs/reference/NODES.md) | commands, editor, model/demand, project, shell |
| `node_suggestions.*` | *Suggest nodes*: one generated polygon per uncovered cluster of conflicts | [NODES §6](../../docs/reference/NODES.md) | commands, shell |
| `node_analysis.hpp` | The analytical inputs a Micro detail does not give (schema 58): per Node, approach and turn | [NODE_ANALYSIS §4](../../docs/reference/NODE_ANALYSIS.md) | this folder, via `network.hpp` |
| `node_detail.*` | F4's plan for a coarse Node's Micro detail | [READINESS §13](../../docs/reference/READINESS.md) | commands |
| `coarse_nodes.*` | What a Node without Micro detail reads from the Roads around it | [ROADS §6](../../docs/reference/ROADS.md), [NODES §5](../../docs/reference/NODES.md) | commands, model/demand, project, shell |
| `roads.*` | What an authored Road means on its own | [ROADS §3](../../docs/reference/ROADS.md) | commands |
| `road_crossings.*` | *Find crossings*: where Roads on one level cross or touch | [ROADS §5](../../docs/reference/ROADS.md) | commands |
| `study_area.*` | The part of the network a Micro run over a study area keeps | [STUDY_AREA §4](../../docs/reference/STUDY_AREA.md) | project, shell |

## Tests

Most `multra-tests` groups: `tests/network_lifecycle_tests.cpp`, `tests/connector_*_tests.cpp`,
`tests/conflict_*_tests.cpp`, `tests/right_of_way_*_tests.cpp`, `tests/node_*_tests.cpp`,
`tests/road_*_tests.cpp`, `tests/study_area_tests.cpp`.
