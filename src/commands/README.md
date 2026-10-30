# src/commands

Named document transactions and bounded snapshot Undo/Redo. Fill transactions explicitly preserve generated flags; manual edits refine them. Every edit runs inside
`History::execute`, which validates the whole candidate document before publishing it, so one
committed gesture is one atomic, undoable change, and a failed change leaves the model, history
and save state as they were. History lives in memory; it is not a persisted journal or a plugin
registry. Contract: [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md),
[NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md).

## Dependencies

Qt-free. Reads `src/model/`, `src/project/` and `src/core/` headers. `src/project/` never includes
this module (`multra-check-architecture`).

## Files

| File | Holds | Contract |
|---|---|---|
| `history.*` | `History`: execute, Undo, Redo over bounded document snapshots | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) |
| `detail.hpp` | Internal: the route and input clean-up both removal paths share | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md) |
| `network_commands.*` | Link and lane edits: geometry and widths (`link_geometry_commands.cpp`), reverse, splitting (`split_link.cpp`), pockets, opposite carriageways, reference-safe deletion (`delete_objects.cpp`) | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `pocket_commands.hpp` | `addTurnPocket` (`split_link.cpp`): kerb/far pockets with a separate widening taper, inside one History transaction | [JUNCTION_TEMPLATES §6](../../docs/reference/JUNCTION_TEMPLATES.md) |
| `connector_commands.*` | Connector creation, interior points, retargeting, curve/straight reset, the added or dropped lane's side, deletion; Link and lane edits share its reanchoring | [NETWORK_EDITOR_CONNECTORS](../../docs/reference/NETWORK_EDITOR_CONNECTORS.md) |
| `conflict_follow.*` | Re-derives each authored conflict area's extents after the drawing changes (an area is a setting on an overlap, not a place) | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md) |
| `right_of_way_commands.*` | Right-of-way controls, conflict areas (`conflict_authoring.cpp`) and conflict groups (`conflict_group_commands.cpp`) | [M3_CONTRACT](../../docs/reference/M3_CONTRACT.md), [GIVE_WAY_STOP](../../docs/reference/GIVE_WAY_STOP.md) |
| `appearance_commands.*` | Names on Links, Connectors and signal heads: free text, never a key | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `demand_commands.*` | Routes, inputs, compositions, routing decisions, desired-speed decisions, Signal Controllers; empty id allocates, same id replaces | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md), [SPEED_DECISIONS §6](../../docs/reference/SPEED_DECISIONS.md) |
| `catalog_commands.*` | One atomic capture of installed catalog entries into the project | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md), [STUDY_PERIODS](../../docs/reference/STUDY_PERIODS.md) |
| `behaviour_commands.*` | The behaviour library: put, duplicate, road assignment, user lists, delete with replacement | [DRIVING_BEHAVIOUR §1](../../docs/reference/DRIVING_BEHAVIOUR.md) |
| `zone_commands.*` | Zones, zone connectors, OD matrices and parking lots; a Zone a matrix names cannot be deleted; Link deletion and splitting carry zone connectors | [ZONES_AND_PARKING](../../docs/reference/ZONES_AND_PARKING.md) |
| `street_segment_commands.*` | Street segments: put, *Apply template* (remapping direct Street segment zone targets to the generated Links in one step), deletion and splitting that carry those targets | [STREET_SEGMENTS §3, §4](../../docs/reference/STREET_SEGMENTS.md), [STREET_SEGMENTS_IMPLEMENTED §22](../../docs/reference/STREET_SEGMENTS_IMPLEMENTED.md#22-street-segment-only-macro-demand-d340) |
| `coarse_commands.*` | A Node's or Street segment's coarse values | [STREET_SEGMENTS §6](../../docs/reference/STREET_SEGMENTS.md), [NODES §5](../../docs/reference/NODES.md) |
| `node_commands.*` | Nodes: put (replacing clears `generated`), rename, delete, merge, *Suggest nodes*; the study area's Nodes, gate flows, source and settling | [NODES §2](../../docs/reference/NODES.md), [STUDY_AREA](../../docs/reference/STUDY_AREA.md) |
| `node_detail_commands.*` | F4: a coarse Node's Micro detail | [READINESS §13](../../docs/reference/READINESS.md) |
| `junction_fill.*` | Template fill planning, site refusals, physical pockets, template signals and generated-only replacement | [JUNCTION_TEMPLATES §5](../../docs/reference/JUNCTION_TEMPLATES.md) |
| `transition_fill.cpp` | Upstream site taper, aligned lane mapping, generated polygon enlargement with named capture/overlap preview refusals, generated merges and atomic replacement through the junction fill | [JUNCTION_TEMPLATES §7](../../docs/reference/JUNCTION_TEMPLATES.md) |
| `junction_fill_detail.hpp` | Private per-call geometry/restore/reference/signal plans; no persisted layout | [JUNCTION_TEMPLATES §5](../../docs/reference/JUNCTION_TEMPLATES.md) |
| `node_detail_priority.*` | F4/J3b shared HCM ranks and generated right-of-way objects | [READINESS §13.4](../../docs/reference/READINESS.md) |
| `junction_commands.*` | Recipe choice/detach preserving coarse, B36 site-only ownership preservation without resurrection, catalog ownership, independent directions and end-following renames; each inside History | [JUNCTION_TEMPLATES §4](../../docs/reference/JUNCTION_TEMPLATES.md) |
| `junction_edit_commands.*` | Read-only replacement/affected-corridor previews and atomic explicit seam repair/cross-section rebuild; ordinary Fill guards remain | [JUNCTION_TEMPLATE_EDITOR](../../docs/reference/JUNCTION_TEMPLATE_EDITOR.md) |
| `node_analysis_commands.*` | A Node's analytical inputs (`analysis`; all-default stored as none) | [NODE_ANALYSIS §4](../../docs/reference/NODE_ANALYSIS.md) |
| `fill_commands.*` | *Fill defaults*: each fill writes a field from a value the project already cites, listed with its source, applied only when chosen | [READINESS §4](../../docs/reference/READINESS.md) |
| `map_commands.*` | Sets or removes the background reference atomically; never moves network geometry | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `macro_run_commands.*` | Replaces the ordered named Macro run list, allocating missing ids | [MACRO_NETWORK_IMPLEMENTED](../../docs/reference/MACRO_NETWORK_IMPLEMENTED.md) |

## Tests

`tests/history_tests.cpp`, `tests/network_lifecycle_tests.cpp`, `tests/connector_*_tests.cpp`,
`tests/right_of_way_*_tests.cpp`, `tests/demand_*_tests.cpp`, `tests/zones_tests.cpp`,
`tests/road_*_tests.cpp`, `tests/node_*_tests.cpp`, `tests/fill_defaults_tests.cpp`.
