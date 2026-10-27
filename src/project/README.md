Qt-free M0 loading/output and authoring ProjectDocument codec (reads schemas 1–57). The document persists authoring data and optional background; no compiled runtime network is saved. Empty draft networks are valid. Commands depend on this module, never the reverse. The shell owns QSaveFile atomic replacement.

`input_manifest` optionally records hashes of exact parsed project/catalog/evaluation bytes for CLI discharge studies. Collection is passed explicitly through compilation; loaders keep their legacy path when absent. `input_sha256` has no external dependency. See [measurement/provenance contract](../../docs/reference/DISCHARGE.md).

`evaluation_period` reads, writes and validates the M5.3 evaluation period (schema 22) and its M5.9 cool-down (schema 27, D146), and builds `newProjectDocument`.

`batch_output` formats M5.2 batch reports as JSON and CSV, with the cell formats it shares with `movementCsv` in `csv_format.hpp`. `parseSeedList` (in `load`) reads `--seeds` lists. `parallel_seeds` runs the runner's `runSeed` on worker threads (D152); the runner itself stays thread-free. See [BATCH](../../docs/reference/BATCH.md).

`behaviour_library` reads, writes and validates the M3.3.2a project-owned behaviour library (names/model tags, vehicle classes, link behaviour types, Link/Connector assignment) and the Run refusal of assigned roads until M3.3.2b. See [contract](../../docs/reference/DRIVING_BEHAVIOUR.md#7-implemented-library-and-codec-m332a-d126).

`map_settings`/`map_codec` persist optional schema-29 reference/settings without Qt/PROJ; old map-free bytes stand.

`route_destination` validates and persists editor-only schema-30 destination markers; compilation ignores them.
`zone_codec` persists the optional schema-31 zone layer (zones, zone connectors, OD matrices; M8.2, D154) and its structural checks; compilation does not read it until M8.3.
`od_demand` adds the OD matrices to a copy of the definition as ordinary routes and inputs before compilation, and holds their Run-tier checks (M8.3, D155).
Direct Road zone targets use schema 57 and Macro's compiled direction endpoints; Micro requires *Apply template* to remap them to Links. See [ROADS_IMPLEMENTED §22](../../docs/reference/ROADS_IMPLEMENTED.md#22-road-only-macro-demand-d340). `meso_road_demand` adds Meso's compiled-graph OD paths, resolving shares, modes, periods and PCU from the same catalogs; a matrix uses them only when one of its zones has a Road target, the rest keep Micro's paths (D342). See [MESO_ROAD_DEMAND](../../docs/reference/MESO_ROAD_DEMAND.md).
`parking_compile` turns each authored ParkingLot into the engine's lot after `compileScenario` (M8.4b, D157).

`study_area_run` builds the run-time Micro cut; `study_area_meso` retains composition and single-type classes per Meso slice. Run-time-only type fields pass through `od_demand` to ordinary inputs without changing catalog ownership or stored codecs. See [STUDY_AREA_IMPLEMENTED §22](../../docs/reference/STUDY_AREA_IMPLEMENTED.md#22-single-vehicle-type-meso-cuts-and-cli-coverage-d338).

`macro_run_definition` persists named Macro period/settings definitions at schema 56; `runNamedMacro` executes them without changing the document or its global settings. Results identify the chosen definition; the side file still keeps the latest source for each period. See [MACRO_NETWORK §21](../../docs/reference/MACRO_NETWORK.md#named-macro-runs).

`allocateId` checks each candidate directly against current object IDs; it shares the network/definition
traversal with `documentIds` and additionally checks zone/mode IDs. No ID index survives an edit.

`observed_schema` generates bounded, non-authoritative structural JSON hints for desktop
automation without Qt. Exhausting the visit budget leaves unvisited array item types open;
it does not infer required fields, codec version rules or semantic validity.

`documentSchemaVersion` (`document_schema.hpp`) selects the writer version directly from
features, without serializing or validating the document. `documentJson` uses this same rule;
input schema versions are not retained. See the [schema inventory](../../docs/reference/PROJECT_SCHEMA.md).

`document_envelope` owns shared envelope/background field metadata, native read/validation
stages and partial draft-2020-12 schema generation. `multra-project-schema` prints that
schema directly, retaining exact uint64 limits. Nested feature validation remains in the
existing codecs; native integer-token and UTF-8 byte checks are annotated constraints.

`network_fields` owns versioned known-key/type metadata for Network, Link, Lane, Connector, SignalHead, signal controller/group/program/phase rows, geometry points and attachment references. `network_link_codec` reads Links in the original order, leaving linkType/
roadId to their existing document stages. `projectDocumentSchema` (`network_schema.hpp`)
composes envelope/background with these structural rules for schemas 1–57; shared `$defs`
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
