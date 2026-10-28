# src/project

Persistence and runs, without Qt: the `ProjectDocument` codec (reads schemas 1–59), catalogs from
`data/`, compilation for each level, and every run's output formats. The document persists
authoring data and the optional background; no compiled runtime network is saved, and an empty
draft network is valid. Schema inventory: [PROJECT_SCHEMA](../../docs/reference/PROJECT_SCHEMA.md).

## Dependencies

Reads `src/model/`, `src/core/`, `src/eval/`, `src/runner/`, `src/analysis/`, `src/macro/`,
`src/meso/` and nlohmann/json. Never `src/commands/`, `src/editor/`, `src/shell/` or Qt
(`multra-check-architecture`): commands depend on this module, never the reverse. The shell owns
atomic file replacement. A new file feature is written only when used, so a document without it
keeps its schema and bytes.

## Document and codec

| File | Holds | Contract |
|---|---|---|
| `document.*` | `ProjectDocument` (authoring network, definition, optional background) and `parseDocument`/`documentJson`; implemented with `parse.cpp`, `definition.cpp`, `signal_migration.cpp` (legacy programs as colour runs) and route migration to Link and Connector ids. `allocateId` and `documentIds` share one traversal; no id index survives an edit | [PROJECT_SCHEMA](../../docs/reference/PROJECT_SCHEMA.md) |
| `document_schema.*` | `documentSchemaVersion`: the writer version chosen from features alone, the rule `documentJson` uses; input versions are not retained | [PROJECT_SCHEMA](../../docs/reference/PROJECT_SCHEMA.md) |
| `document_envelope.*` | Envelope and background field metadata, read and validation stages, the envelope-only partial draft-2020-12 schema | [PROJECT_SCHEMA](../../docs/reference/PROJECT_SCHEMA.md) |
| `network_schema.*` | `projectDocumentSchema`: the composed partial schema for schemas 1 to the latest, printed by `multra-project-schema` | [PROJECT_SCHEMA](../../docs/reference/PROJECT_SCHEMA.md) |
| `network_fields.*` | Versioned known-key and type metadata shared by the row codecs (`network_link_codec.cpp`, `network_connector_codec.cpp`, `network_signal_head_codec.cpp`, `signal_control_codec.cpp`, `right_of_way_codec.cpp`, `measurement_codec.cpp`, `demand_codec.cpp`, `routing_codec.cpp`); rules below | [PROJECT_SCHEMA](../../docs/reference/PROJECT_SCHEMA.md) |
| `json.*` | Section accessors that fail with a named, translatable code; `parseNetwork`; `output.cpp` writes the run's state and event JSON | [PROJECT_SCHEMA](../../docs/reference/PROJECT_SCHEMA.md) |
| `observed_schema.*` | Bounded structural JSON hints for desktop automation (50,000 values, depth 32); no required fields, ranges or version rules | [DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md) |
| `node_codec.*` | `network.nodes` and each Node's `analysis` (an approach `speedLimit` only below 60, D390) | [NODES §2](../../docs/reference/NODES.md), [NODE_ANALYSIS §4](../../docs/reference/NODE_ANALYSIS.md) |
| `street_segment_codec.*` | `network.streetSegments`, a Link's `streetSegmentId`, owned `definition.crossSections` | [STREET_SEGMENTS §3, §4](../../docs/reference/STREET_SEGMENTS.md) |
| `zone_codec.*` | The zone layer: zones, zone connectors (with direct Street segment targets), OD matrices, parking lots, and their structural checks | [ZONES_AND_PARKING](../../docs/reference/ZONES_AND_PARKING.md) |
| `study_area_codec.*` | `studyArea` at the root | [STUDY_AREA §3, §5](../../docs/reference/STUDY_AREA.md) |
| `study_period_codec.*` | `studyPeriods`, `definition.clockStart`, a matrix's `periodId` | [STUDY_PERIODS §3-§5](../../docs/reference/STUDY_PERIODS.md) |
| `mode_codec.*` | Owned `definition.modes`, a matrix's `modeId`, a vehicle type's `pcu` | [STUDY_PERIODS §5-§7](../../docs/reference/STUDY_PERIODS.md) |
| `link_type_codec.*` | A Link's `linkType`, owned `definition.linkTypes`, installed `data/link-types/` and `data/vdf/` | [MACRO_NETWORK §4, §5](../../docs/reference/MACRO_NETWORK.md) |
| `link_speed_codec.*` | A Link's posted `speedLimit` and its schema-60 gates; moves a 58–59 approach row's speed to its Link | [JUNCTION_TEMPLATES §9](../../docs/reference/JUNCTION_TEMPLATES.md) |
| `macro_settings.*` | `definition.macroSettings` | [MACRO_NETWORK §7](../../docs/reference/MACRO_NETWORK.md) |
| `macro_run_definition.*` | Saved named Macro run definitions, apart from the latest result | [MACRO_NETWORK §21](../../docs/reference/MACRO_NETWORK.md) |
| `meso_settings.*` | `definition.mesoSettings` and a link type's `jamDensity` | [MESO_NETWORK §8](../../docs/reference/MESO_NETWORK.md) |
| `evaluation_period.*` | The evaluation period and cool-down; `newProjectDocument` | [BATCH](../../docs/reference/BATCH.md) |
| `behaviour_library.*` | The project-owned behaviour library: names and model tags, vehicle classes, link behaviour types, assignment | [DRIVING_BEHAVIOUR §7](../../docs/reference/DRIVING_BEHAVIOUR.md) |
| `clamp_rules.*` | `definition.clampRules`, three booleans true when absent | [MERGE_CLAMPS §3](../../docs/reference/MERGE_CLAMPS.md) |
| `lane_preference.*` | A vehicle type's `kerbLanes` | [LANE_PREFERENCE](../../docs/reference/LANE_PREFERENCE.md) |
| `speed_curve.*` | A vehicle type's desired-speed range and cumulative curve | [SPEED_DISTRIBUTION](../../docs/reference/SPEED_DISTRIBUTION.md) |
| `speed_decisions.*` | Desired-speed decisions in the file and compiled per lane | [SPEED_DECISIONS](../../docs/reference/SPEED_DECISIONS.md) |
| `speed_areas.*` | Reduced-speed areas in the file and compiled per lane section | [SPEED_DECISIONS §7](../../docs/reference/SPEED_DECISIONS.md) |
| `route_destination.*` | Editor-only destination markers; compilation ignores them | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `map_settings.*` | The optional background reference and settings, without Qt or PROJ (`map_codec.cpp`) | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `counted_volumes.*` | Inputs whose volume is their entry decision's turning counts | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md) |
| `demand_time_types.*` | Time-dependent composition and type routing rules; the compiler emits ordinary Poisson streams | [DEMAND_TIME_TYPES](../../docs/reference/DEMAND_TIME_TYPES.md) |

## Catalogs, compilation and checks

| File | Holds | Contract |
|---|---|---|
| `load.*` | `loadScenario`, `findDataDirectory`, `parseSeedList`; load errors carry the file and a translatable code | [SIMULATION](../../docs/reference/SIMULATION.md) |
| `run.*` | `resolveCatalogs` and compilation for a run: compositions expanded to one input per type | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md) |
| `demand_catalog.*` | Resolving authored catalog ownership; the type a new input starts from | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md) |
| `demand_paths.*` | Glue between authored demand and the routeless walk: placed decisions, `expandRouteless` | [POSITIONED_ROUTING](../../docs/reference/POSITIONED_ROUTING.md) |
| `demand_preview.*` | Expected counts from the same compiled snapshot as Run, without arrivals | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md) |
| `station_routing.*` | Physical stations to route distances; clips awareness-dependent lateral spans | [POSITIONED_ROUTING](../../docs/reference/POSITIONED_ROUTING.md) |
| `od_demand.*` | `withOdDemand`: OD matrices added as ordinary routes and inputs before compilation; their Run-tier checks | [ZONES_AND_PARKING §3](../../docs/reference/ZONES_AND_PARKING.md) |
| `meso_street_segment_demand.*` | Meso's compiled-graph OD paths for a matrix with a direct Street segment target; other matrices keep Micro's paths | [MESO_STREET_SEGMENT_DEMAND](../../docs/reference/MESO_STREET_SEGMENT_DEMAND.md) |
| `parking_compile.*` | Each authored ParkingLot as the engine's lot, after `compileScenario` | [ZONES_AND_PARKING §4](../../docs/reference/ZONES_AND_PARKING.md) |
| `display.*` | `loadDisplayCatalog` | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md) |
| `diagnostics.*` | Draft issues, then runtime issues, as rows; never throws | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `readiness.*` | What would stop or weaken a run, per level, from the functions the runs call | [READINESS §3](../../docs/reference/READINESS.md) |
| `node_proposals.*` | The Nodes a Macro or Meso run points at; listed, never refined | [READINESS §5, §12](../../docs/reference/READINESS.md) |
| `input_manifest.*` | SHA-256 of the exact parsed bytes (`input_sha256.cpp`, no external dependency) for discharge studies | [DISCHARGE](../../docs/reference/DISCHARGE.md) |

## Runs and outputs

| File | Holds | Contract |
|---|---|---|
| `evaluation.*` | The queue-counter definition and `evaluationSpec`; `discharge_json.cpp` writes discharge JSON | [BATCH](../../docs/reference/BATCH.md), [DISCHARGE](../../docs/reference/DISCHARGE.md) |
| `csv_format.*` | Cell formats every results CSV shares | [BATCH](../../docs/reference/BATCH.md) |
| `batch_output.*` | Batch JSON and CSV | [BATCH](../../docs/reference/BATCH.md) |
| `comparison_output.*` | Scenario comparison JSON and CSV | [BATCH §7](../../docs/reference/BATCH.md) |
| `parallel_seeds.*` | `runSeedsParallel`: the runner's seeds on worker threads; the runner stays thread-free | [BATCH](../../docs/reference/BATCH.md) |
| `los_output.*` | The LOS pack loader and LOS cells | [LOS](../../docs/reference/LOS.md) |
| `node_output.*` | Each Node's turns, legs and conflicts as evaluation specs, and their rows | [NODES §4](../../docs/reference/NODES.md) |
| `macro_input.*` | What a Macro run reads from a document | [MACRO_NETWORK §3-§6](../../docs/reference/MACRO_NETWORK.md) |
| `macro_run.*` | One Macro run of a named period, shared by the CLI and the editor; `runNamedMacro` runs a saved definition without changing the document | [MACRO_NETWORK §7, §9](../../docs/reference/MACRO_NETWORK.md) |
| `macro_output.*` | A Macro run's fields and one row per arc | [MACRO_NETWORK §9](../../docs/reference/MACRO_NETWORK.md) |
| `meso_input.*` | What a Meso run reads: resolved link types, settings, `mesoDemand` | [MESO_NETWORK §3, §8](../../docs/reference/MESO_NETWORK.md) |
| `meso_run.*` | `runMeso` over the whole Micro run | [MESO_NETWORK §7](../../docs/reference/MESO_NETWORK.md) |
| `meso_output.*` | A Meso run's outputs by slice, in PCU | [MESO_NETWORK §7](../../docs/reference/MESO_NETWORK.md) |
| `study_area_run.*` | The document a Micro run over a study area compiles; never saved | [STUDY_AREA §4, §5](../../docs/reference/STUDY_AREA.md) |
| `study_area_macro.*` | Gate flows cut from Macro's assignment | [STUDY_AREA §6](../../docs/reference/STUDY_AREA.md) |
| `study_area_meso.*` | Gate flows cut from Meso's loading, with composition and single-type classes per slice | [STUDY_AREA §7](../../docs/reference/STUDY_AREA.md) |
| `side_files.*` | The `<project>.macro.json` and `<project>.meso.json` side files | [STUDY_AREA §8, §9](../../docs/reference/STUDY_AREA.md) |
| `export_marking.*` | Program-drawn objects a study-area run rests on, and the marking every export carries | [STUDY_AREA §8, §10, §11](../../docs/reference/STUDY_AREA.md) |
| `node_analysis_run.*` | M13: HCM on every Node that carries `analysis`; `node_analysis_output.cpp` writes the JSON and CSV | [NODE_ANALYSIS](../../docs/reference/NODE_ANALYSIS.md) |
| `node_analysis_permitted.*` | M13 A3: one permitted or protected-permitted far-turn lane group (`analysePermitted`), the shared lane's Eq. 31-59 share for lane flows, and a protected lane group's queue service time g_s | [NODE_ANALYSIS_A3](../../docs/reference/NODE_ANALYSIS_A3.md) |
| `object_csv.*` | Five authored object tables as CSV (UTF-8 BOM, CRLF, full precision, nested JSON cells); no compilation | [OBJECT_TABLE_CSV](../../docs/reference/OBJECT_TABLE_CSV.md) |

## Codec rules

`network_fields` owns versioned known-key/type metadata for Network, Link, Lane, Connector, SignalHead, signal controller/group/program/phase rows, geometry points and attachment references. `network_link_codec` reads Links in the original order, leaving linkType/
streetSegmentId to their existing document stages. `projectDocumentSchema` (`network_schema.hpp`)
composes envelope/background with these structural rules for schemas 1–59; shared `$defs`
and conditional version groups keep it compact. Model invariants and nested features remain
codec-owned, and `projectEnvelopeSchema` retains its original envelope-only API.

`network_connector_codec` reads Connectors and the LaneReference shared with native signal
heads, preserving versioned fraction/station keys and diagnostics. Whole-network attachment
migration stays in `parse.cpp`. The composed structural schema includes Connector/reference
fields; other control/demand rows and geometric/physical feasibility remain codec-owned.

`network_signal_head_codec` reads SignalHead rows through the same field metadata, including
nullable optional text and the conditional controller/group read. A non-null controllerId
activates groupNumber even when empty; inactive group values remain ignored. The composed
schema includes this condition and shared LaneReferences. ID resolution and legacy program migration remain in their original codec stages.

`signal_control_codec` reads controllers/groups and legacy programs/phases through shared
metadata. Controller/group fields are always closed; legacy program/phase fields stay open.
The composed schema includes nullable definition presence and signal arrays. Document-stage
generated-flag guards, cyclic timing validation and pre-13 migration keep their original order.

`right_of_way_codec` reads schema-14 waiting lines, conflict areas and priority rules, plus
schema-15 Stop/Yield controls, through shared field metadata. The composed schema covers
these rows and their control points/paths. Nullable collections, legacy non-object handling,
version gates, generated-flag diagnostics and queue-counter path decoding are preserved.
Geometry, owner/line/area resolution, priority cycles and run-readiness remain model checks.


`measurement_codec` reads schema-16 queue counters/measurement lines and schema-23 travel-time
sections, with optional controlType from 24. Shared field metadata preserves nullable sources,
ControlPoint/ControlPathRef reads and diagnostic order. The composed partial schema includes
these rows; exactly-one-source rules, references, section bounds and LOS meaning remain model
and evaluation concerns. Empty/non-object measurement lines still decode to empty sources.


`demand_codec` reads base Routes and VehicleInputs/volume intervals through shared field
metadata; definition duration/timeStep use the same accessor. These rows retain open extra
keys and contains-based optional reads (explicit null fails). Interval-derived totals stay
at the original decode stage. The counted-volume version guard remains in parseDocument;
route migration and counted synchronization still follow complete definition decoding.


`routing_codec` reads RoutingDecision rows, route shares, time intervals and type-rule flow
matrices through shared metadata. Decision/share/type rows remain strict at every file version;
period rows stay open. Nullable optionals, contains-based typeRules and distinct matrix errors
retain their read order. The document-only pre-20 position guard stays before parsing; counted
volume synchronization follows the complete definition. Schema validation leaves matrix sizes,
weights, time windows and destination reachability to existing model validators.

## Tests

`tests/project_tests.cpp`, the `*_schema_tests.cpp` and `*_codec_tests.cpp` files, `tests/batch_tests.cpp`,
`tests/macro_*_tests.cpp`, `tests/meso_*_tests.cpp`, `tests/study_area_*_tests.cpp`,
`tests/node_analysis_*tests.cpp`; the `project-schema-cli` and `cli-*` ctests run `multra-cli`.
