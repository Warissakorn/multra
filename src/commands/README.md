# src/commands

Named document transactions and bounded snapshot Undo/Redo. Every edit runs inside
`History::execute`, which validates the whole candidate document before publishing it, so one
committed gesture is one atomic, undoable change, and a failed change leaves the model, history
and save state as they were. History lives in memory; it is not a persisted journal or a plugin
registry. Contract: [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md),
[NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md).

## Dependencies

Qt-free. Reads `src/model/`, `src/project/` and `src/core/` headers. `src/project/` never includes
this module (`multra-check-architecture`).

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `history.*` | `History`: execute, Undo, Redo over bounded document snapshots | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) | shell |
| `detail.hpp` | Internal: the route and input clean-up both removal paths share | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md) | this folder |
| `network_commands.*` | Link and lane edits: geometry and widths (`link_geometry_commands.cpp`), reverse, splitting (`split_link.cpp`), pockets, opposite carriageways, reference-safe deletion (`delete_objects.cpp`) | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) | lab, shell, tools |
| `connector_commands.*` | Connector creation, interior points, retargeting, curve/straight reset, the added or dropped lane's side, deletion; Link and lane edits share its reanchoring | [NETWORK_EDITOR_CONNECTORS](../../docs/reference/NETWORK_EDITOR_CONNECTORS.md) | editor, lab, shell, tools |
| `conflict_follow.*` | Re-derives each authored conflict area's extents after the drawing changes (an area is a setting on an overlap, not a place) | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md) | this folder |
| `right_of_way_commands.*` | Right-of-way controls, conflict areas (`conflict_authoring.cpp`) and conflict groups (`conflict_group_commands.cpp`) | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md), [GIVE_WAY_STOP](../../docs/reference/GIVE_WAY_STOP.md) | shell, tools |
| `appearance_commands.*` | Names on Links, Connectors and signal heads: free text, never a key | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) | shell |
| `demand_commands.*` | Routes, inputs, compositions, routing decisions, desired-speed decisions, Signal Controllers; empty id allocates, same id replaces | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md), [SPEED_DECISIONS §6](../../docs/reference/SPEED_DECISIONS.md) | lab, shell, tools |
| `catalog_commands.*` | One atomic capture of installed catalog entries into the project | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md), [STUDY_PERIODS](../../docs/reference/STUDY_PERIODS.md) | shell, tools |
| `behaviour_commands.*` | The behaviour library: put, duplicate, road assignment, user lists, delete with replacement | [DRIVING_BEHAVIOUR §1](../../docs/reference/DRIVING_BEHAVIOUR.md) | shell, tools |
| `zone_commands.*` | Zones, zone connectors, OD matrices and parking lots; a Zone a matrix names cannot be deleted; Link deletion and splitting carry zone connectors | [ZONES_AND_PARKING](../../docs/reference/ZONES_AND_PARKING.md) | shell |
| `road_commands.*` | Roads: put, *Apply template* (remapping direct Road zone targets to the generated Links in one step), deletion and splitting that carry those targets | [ROADS §3, §4](../../docs/reference/ROADS.md), [ROADS_IMPLEMENTED §22](../../docs/reference/ROADS_IMPLEMENTED.md#22-road-only-macro-demand-d340) | shell, tools |
| `coarse_commands.*` | A Node's or Road's coarse values | [ROADS §6](../../docs/reference/ROADS.md), [NODES §5](../../docs/reference/NODES.md) | shell, tools |
| `node_commands.*` | Nodes: put (replacing clears `generated`), rename, delete, merge, *Suggest nodes*; the study area's Nodes, gate flows, source and settling | [NODES §2](../../docs/reference/NODES.md), [STUDY_AREA](../../docs/reference/STUDY_AREA.md) | shell, tools |
| `node_detail_commands.*` | F4: a coarse Node's Micro detail | [READINESS §13](../../docs/reference/READINESS.md) | this folder (`fill_commands`), tools |
| `node_analysis_commands.*` | A Node's analytical inputs (`analysis`; all-default stored as none) | [NODE_ANALYSIS §4](../../docs/reference/NODE_ANALYSIS.md) | tests only (no UI yet) |
| `fill_commands.*` | *Fill defaults*: each fill writes a field from a value the project already cites, listed with its source, applied only when chosen | [READINESS §4](../../docs/reference/READINESS.md) | shell |
| `map_commands.*` | Sets or removes the background reference atomically; never moves network geometry | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) | shell |
| `macro_run_commands.*` | Replaces the ordered named Macro run list, allocating missing ids | [MACRO_NETWORK_IMPLEMENTED](../../docs/reference/MACRO_NETWORK_IMPLEMENTED.md) | shell |

## Tests

`tests/history_tests.cpp`, `tests/network_lifecycle_tests.cpp`, `tests/connector_*_tests.cpp`,
`tests/right_of_way_*_tests.cpp`, `tests/demand_*_tests.cpp`, `tests/zones_tests.cpp`,
`tests/road_*_tests.cpp`, `tests/node_*_tests.cpp`, `tests/fill_defaults_tests.cpp`.
