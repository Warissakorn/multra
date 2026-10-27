# Project schema inventory

This is the codec ownership and version map for an editor-project schema (D355), with
generated partial envelope/background (D356), Network/Link/Lane (D357),
Connector/attachment-reference (D358), SignalHead (D359), signal control (D360),
right-of-way (D361), measurement (D362), base demand (D363) and routing-decision contracts (D364). It describes
implemented readers/writers, rather than inferring a contract from one document. It is
**not a complete JSON Schema**, a new validator, or a change to file
semantics. `parseDocument` and `validateDocument` remain authoritative. Desktop
`document.schema` still returns non-authoritative observed shapes; `document.validate`
uses those existing codecs and validators.

## Envelope and presence rules

The reader accepts editor schemas 1–57 (`kLatestSchema` in `document.hpp`). A missing
`schemaVersion` selects the separate legacy scenario path, including pre-5 attachments
and legacy signal/amber behavior. A formal editor schema must not accidentally accept
that path as a canonical project.

| Root field | Reader requirement / writer presence | Codec owner |
|---|---|---|
| `format` | Required for projects, exactly `TrafficSim`; emitted always | `document_envelope.cpp` |
| `schemaVersion` | Required integer 1–57 for projects; writer chooses from features | `document_envelope.cpp`, `document_schema.hpp` |
| `nextId` | Required unsigned JSON integer; >0 and below uint64 maximum | `document_envelope.cpp` |
| `revision` | Required unsigned JSON integer; below uint64 maximum | `document_envelope.cpp` |
| `network` | Required non-null object; empty draft network is valid | `parse.cpp`, network validators |
| `definition` | Absent/null means no authored demand; writer emits null when absent | `definition.cpp`, feature codecs below |
| `background` | Required object, including an empty image; writer always emits it | `document_envelope.cpp` |
| `map` | Optional from 29; null is invalid; omitted without settings | `map_codec.cpp` |
| `zones`, `zoneConnectors`, `odMatrices`, `parkingLots` | Optional from 31; present arrays must be nonempty; null invalid | `zone_codec.cpp` |
| `studyPeriods` | Optional from 49; present array must be nonempty; null invalid | `study_period_codec.cpp` |
| `studyArea` | Optional from 55; present object needs nonempty nodes; null invalid | `study_area_codec.cpp` |
| `macroRuns` | Optional from 56; present array must be nonempty; null invalid | `macro_run_definition.cpp` |

`background` requires `pngBase64` (string), numeric `x`, `y`, `metresPerPixel`,
`rotation`, `opacity`. Validation requires finite numbers, positive metresPerPixel,
opacity in [0,1], and at most 32 MiB of encoded bytes. The codec does not decode PNG
content. Desktop patch protection is a separate rule: format/revision/image bytes
cannot change, and schemaVersion must remain present.

Do not apply a blanket `additionalProperties:false` or a blanket null-as-absent rule.
The envelope/background readers do not reject all unknown keys; network known-field
checks apply from schema 7, while map and zone shapes use exact keys. Definition
feature/catalog readers have their own rules. JSON Schema integers also cannot express
the distinction between a native signed integer and an unsigned parser value; preserve
codec checks for the native representation and exact uint64 bounds. Qt JSON transport
has its own exact-integer limits, separate from the stored format.

## Feature-selected save version

`documentSchemaVersion(document)` is the writer's single feature-selection function.
It performs no serialization or validation and does not retain an input file version.
The highest applicable rule wins; absence of every newer feature selects 17. Opening
an empty valid schema-57 project therefore still saves schema 17. Introducing a high
version feature does not require populating unrelated lower-version features.

The following rows map the writer's feature families to their codec owners. A family
row is not an exhaustive list of its nested keys or semantic constraints.

| Save floor | Feature trigger | Codec owner / field family |
|---|---|---|
| 17 | Default; includes earlier implemented authoring features | `document.cpp`, `parse.cpp`, `definition.cpp` |
| 18 | Owned compositions or embedded vehicle names | `definition.cpp`, `demand_catalog.cpp` |
| 19 | Time/type demand rules | `demand_time_types.cpp` |
| 20 | Routing decision position | `station_routing.cpp` |
| 21 | Behaviour library or road behaviour assignment | `behaviour_library.cpp`, `parse.cpp` |
| 22 | Evaluation period | `evaluation_period.cpp` |
| 23 | Travel-time sections | `parse.cpp` |
| 24 | Section control type | `parse.cpp` |
| 25 | Owned W74 behaviour | `behaviour_library.cpp`, `parse.cpp` |
| 26 | Count-derived input volumes | `counted_volumes.cpp` |
| 27 | Positive evaluation cool-down | `evaluation_period.cpp` |
| 28 | Owned amberDeceleration | `behaviour_library.cpp` |
| 29 | Map settings | `map_codec.cpp` |
| 30 | Route destination markers | `route_destination.cpp` |
| 31 | Zone layer | `zone_codec.cpp` |
| 32 | Owned perception parameters | `behaviour_library.cpp`, `parse.cpp` |
| 33 | Owned lateral parameters | `behaviour_library.cpp`, `parse.cpp` |
| 34 | Owned curve lateral acceleration | `behaviour_library.cpp` |
| 35 | Owned kerb bias parameters | `behaviour_library.cpp` |
| 36 | Owned commitment deceleration | `behaviour_library.cpp` |
| 37 | Owned mandatory-change parameters | `behaviour_library.cpp` |
| 38 | Desired-speed decisions | `speed_decisions.cpp` |
| 39 | Reduced-speed areas | `speed_areas.cpp` |
| 40 | Owned vehicle speed curve | `speed_curve.cpp` |
| 41 | Owned stop dwell time | `behaviour_library.cpp` |
| 42 | Owned vehicle kerbLanes | `lane_preference.cpp` |
| 43 | Owned driver-factor spread | `behaviour_library.cpp` |
| 44 | Owned red-running parameters | `behaviour_library.cpp` |
| 45 | Nondefault clamp rules | `clamp_rules.cpp` |
| 46 | joinAnticipation enabled | `clamp_rules.cpp` |
| 47 | standingStopFloor disabled | `clamp_rules.cpp` |
| 48 | Nodes | `node_codec.cpp` |
| 49 | Study periods, nonzero clockStart, period-based OD, modes or PCU | `study_period_codec.cpp`, `mode_codec.cpp` |
| 50 | Link type assignment or owned link types | `link_type_codec.cpp` |
| 51 | Macro settings | `macro_settings.cpp` |
| 52 | Meso settings or owned jam density | `meso_settings.cpp`, `link_type_codec.cpp` |
| 53 | Roads, roadId, owned cross-sections, overpasses or capacity-basis/rank fields | `road_codec.cpp`, `link_type_codec.cpp` |
| 54 | Generated flags | `node_codec.cpp`, `parse.cpp`, `definition.cpp` |
| 55 | Study area with selected Nodes | `study_area_codec.cpp` |
| 56 | Named Macro run definitions | `macro_run_definition.cpp` |
| 57 | Road-target zone connectors | `zone_codec.cpp` |

Schemas 1–16 are read compatibility, not possible save floors. In particular, 1–4 use
fraction attachments converted to metre stations on read, pre-8 routes migrate from
lanes/paths to owning objects, and pre-13 editor signal programs migrate to controllers.
Below 7, network unknown-field handling remains legacy/additive. Other early defaults
and nested keys must be inventoried from the relevant readers before claiming complete
historical-schema coverage.

## Nested coverage that a formal schema must preserve

- Network: Link/Lane/Connector/reference/geometry keys and version-specific defaults in
  `parse.cpp`; Node/generated keys in `node_codec.cpp`; Roads/overpasses/cross-sections
  in `road_codec.cpp`. Geometry point and polygon representations differ by object.
- Controls: right-of-way paths/sides, waiting lines, conflict areas, rules, Stop/Yield,
  queue counters and travel-time sections in `parse.cpp`; signal controller/group/program
  shapes in `model/demand/signal_control.cpp` and `definition.cpp`.
- Demand: routes, inputs, intervals, routing decisions and counts in `definition.cpp`
  and `demand_*.cpp`; optional owned catalogs distinguish absence (installed catalog)
  from a present empty override. Owned behaviours need explicit model tags from 21;
  prototype and W74 parameter sets are distinct.
- Zones: exact row shapes; an OD matrix takes periodId **or** startTime/endTime, and
  modeId **or** compositionId. A zone connector takes linkId **or** roadId/roadDirection;
  Road targets require 57. See [ZONES_AND_PARKING](ZONES_AND_PARKING.md) and
  [STUDY_PERIODS](STUDY_PERIODS.md).
- Settings: exact map/reference keys and UTM agreement, Macro/Meso settings and numeric
  bounds, named Macro definitions and their settings, study-area nodes/flow source/gate
  flows. References, ID uniqueness, simple polygons and run-readiness remain codec/model
  checks even if structural JSON Schema validation succeeds.

A generated schema needs codec-owned field metadata for requiredness, null handling,
unknown-key policy, version gates and tagged alternatives. The inventory above does not
supply that metadata for every nested field. Network, Link, Lane and Link geometry field metadata are implemented as described below.
Connector and attachment-reference metadata are described below. Other control/demand
field metadata remain separate follow-ups; their semantic validation stays in the codec.

## Generated envelope/background schema (D356)

`projectEnvelopeSchema()` in `document_envelope.hpp` generates a draft-2020-12 schema
from the field metadata used by the native project envelope reader and typed validation.
The CLI prints the composed document schema (including the network extension below)
without a document, Qt, external catalogs or a running editor:

```sh
./build/headless/bin/multra-project-schema > project.schema.json
```

The tool is installed beside `multra-cli`; it accepts only `--help` or no arguments.
It prints JSON directly, preserving the exact integer maximum `18446744073709551614`.
Use an exact-integer JSON parser for those bounds; a binary64-only parser rounds them.
No generated schema file is maintained as another source of truth.

The envelope-only C++ API retains `x-multra-coverage: envelope-background` and
`x-multra-complete:false`; the composed CLI coverage is described below.
The schema covers required root envelope fields, counter limits, the root shape of network
and optional definition, and background fields/ranges. Root/background unknown keys stay
open, as in the codec. Nested network/demand fields, optional features and their version
rules, ID references and run readiness remain unchecked by this partial schema. A valid
schema result alone never establishes that the document can be loaded or run.

Two constraints need the codec even within the covered fields:

- `x-multra-nativeType` records integer-token/unsigned-integer-token requirements. JSON
  Schema treats `17.0` as an integer, whereas the native reader rejects its floating token.
- `pngBase64.maxLength` supplies a necessary character-count bound; the stricter
  `x-multra-maxUtf8Bytes` bound is checked by the codec. A non-ASCII string can fit the
  character count and exceed the byte limit. This validates encoded size, not PNG contents.

Standard validators may ignore these annotations. The native read/validation stages still
check finite doubles and preserve existing error codes and legacy scenario handling.
Desktop `document.schema` continues to return observed shapes; its Qt JSON transport is
not used to carry these exact uint64 schema bounds. `document.validate` remains the way to
check complete documents/patches against the codec in the live desktop workflow.

## Network, Link and Lane extension (D357)

`projectDocumentSchema()` in `network_schema.hpp` composes the stable envelope-only API
with Network, Link, Lane and Link geometry-point metadata from `network_fields`.
`multra-project-schema` now prints this composed schema with no new arguments. Coverage
now also includes the Connector/reference extension below; complete remains false and
desktop observed schema discovery is unchanged.

The generated schema chooses rules from the document's root `schemaVersion` (1–57).
Identical structures share `$defs`, and identical network profiles share conditional version
groups. Reference names are implementation details; consumers follow `$ref`. The same
metadata drives native known-key checks and Link/Lane field reads. Link decoding has its
own file; the later Connector/reference extraction below preserves model geometry algorithms.

Covered structure:

- Network requires id, drivingSide (left/right), links, connectors and signalHeads. Links
  have typed items; Connector items use the extension below, while signal-head items use the extension below.
  Other known Network properties are allowed at their existing versions, with their contents
  unchecked. Roads/overpasses retain the document's pre-53 refusal even on legacy versions.
- Links require id, geometry and lanes. Geometry uses object points with required numeric x/y;
  lanes have required string id and numeric width. Optional level, displayType, laneOffset,
  name, boundaryMarkings and versioned behaviourType/linkType/roadId get their read shapes.
- Level is an integer token within [-1000,1000]; `x-multra-nativeType` records the native-token
  restriction. Marking names come from the existing model formatter. Model defaults supply
  schema default annotations, which do not fill or mutate input documents.
- name may be absent or null; either leaves the model's empty name. Other optional Link
  scalar/array fields reject null when read. Before 7 unknown keys remain additive on
  Network/Link/Lane/points; from 7 their known-key lists are closed.
- behaviourType is read from 21; earlier legacy unknown-field behavior remains. linkType
  is known by the standalone network reader from 21 but rejected by document prechecks
  before 50 and read in the existing later stage. roadId stays gated at 53. Those stage
  boundaries and diagnostic paths are preserved, rather than normalized by the refactor.

This is structural coverage. Nonblank/unique IDs, positive lane widths, lane counts,
geometry validity, marking-count relationships and behaviour/type/Road reference resolution
still require model/document validation. The schema can accept a structurally valid empty
geometry or zero width that the codec refuses. Unmapped Node/control/Road/demand objects
can likewise pass the partial schema and fail the codec; schema success never establishes
full authoring validity, run readiness or scientific validation.

## Connector and attachment-reference extension (D358)

D358 added Connector/reference coverage; D359 below extends the current composed coverage
with SignalHead rows. Complete remains false.
They add versioned Connector rows, their geometry points and from/to LaneReferences.
The reference decoder is also shared by native signal-head reads; their row schema is
added by D359 below. The envelope-only API, CLI arguments and desktop observed
schema response keep their existing contracts.

Connector metadata supplies required id/from/to/geometry, optional lane counts, level,
displayType, name, laneBlend, laneWidths, laneMarkings, laneChangeSide, behaviourType and
generated. The separate Connector reader preserves original decode order and error codes.
Geometry-point decoding is shared with Links, without changing geometry calculations.

Version/presence rules:

- References require string linkId/laneId. Before 5 the optional position is fraction of
  lane arclength; from 5 it is station in metres along the Link reference polyline. The
  wrong position key is forbidden even when null or legacy unknown fields are additive.
  Correct position keys reject null. Both keys absent preserve source-end/target-start
  attachment semantics. Legacy conversion/clamping and endpoint normalization still occur
  at the original whole-network migration stage, after Connector and signal-head decoding.
- Counts default to the model's one lane per end; level, displayType and name use their
  model defaults. Native integer tokens and parser bounds [-1000,1000] apply to counts and
  level. Feasible lane counts/ranges are stricter model checks, not inferred schema limits.
- Optional laneBlend/widths/markings are arrays; absent widths/markings stay empty, meaning
  derive from the Links. Element shapes are numeric or the existing marking-name enum.
  Their lengths, physical ranges and geometric relationships remain codec/model checks.
- name may be null and then stays empty. laneChangeSide is known/read from 17, accepts
  left/right or null (null/absence means the existing kerb-side rule). behaviourType is
  known/read from 21 and requires a string when present.
- generated is known from 54 and, when present, must be boolean true; false is represented
  by omission. Before 7 unknown keys remain additive/ignored, including later side/behaviour/
  generated keys. From 7 the version's Connector/reference/point known-key lists are closed.

The schema does not resolve lane IDs, enforce attachment bounds, check endpoint distances,
validate a drivable shape, choose lane ranges or prove traffic/Vissim fidelity. In particular,
no [0,1] limit is inferred for legacy fraction values: existing native migration remains
responsible for their interpretation. Standard integer-valued floats can pass JSON Schema
and fail native integer-token checks, as with the previously covered envelope/Link fields.

## SignalHead extension (D359)

The composed schema/CLI coverage is
`envelope-background-network-link-lane-connector-reference-signal-head`; complete remains
false. SignalHead rows require id, lane (the shared versioned LaneReference), numeric position
and string programId. programId is required even when a controller is supplied; its empty
string is permitted structurally. name and connectorId may be absent/null and then stay empty.
Their schema default annotations do not fill input documents. Unknown-key and generated flag
rules follow the same pre-7 additive/from-54 true-only policy as the other covered objects.

Controller/group reads are conditional, with shared metadata supplying both native decoding
and generated JSON Schema if/then rules:

- Absent/null controllerId leaves controller/group at their model defaults; groupNumber is
  ignored regardless of its type, nullness or presence.
- A non-null controllerId must be a string and requires groupNumber, including when the string
  is empty. No default is inserted for either field. An integer-valued number in native int
  range is required for the group; `1.0` is accepted, unlike integer-token-only level/count
  fields. Non-numbers, fractional/nonfinite values and out-of-range conversion retain the
  original errors. The schema describes mathematical integers and native int bounds.
- These keys were known/read without an explicit schema-13 gate in the existing reader;
  the schema preserves that compatibility instead of inventing a new refusal.

The separate SignalHead reader preserves field/error ordering. Whole-network attachment
migration and pre-13 project signal-program migration still happen in their original stages.
The schema covers pre-migration input structure, not the resulting controller/program data.
Controllers/groups/program references, the semantic program-versus-controller choice, valid
head positions and geometric attachment constraints still require the model/document codec.
Other control/demand row schemas remain separate follow-ups.


## Signal control extension (D360)

The composed CLI coverage is now
`envelope-background-network-link-lane-connector-reference-signal-head-signal-control`;
complete remains false. An absent/null definition remains valid. A present definition object
requires signalPrograms; signalControllers is optional, but explicit null is invalid for either
array. Other definition fields remain open and require their existing codecs.

- Controller rows require text id, numeric cycle/offset and a groups array. Group rows require
  number, greenStart, greenEnd and amber. Both accept optional nullable text name. Their keys
  are closed at every input version, including schemas before 7 and before 13: the existing
  standalone reader never had a schema-13 read gate.
- A group number accepts mathematically integral numbers, including 1.0, in native int range.
  Zero/negative values and duplicate groups remain semantic validation concerns. Empty group
  arrays, cyclic timing, time-step alignment and head-to-group resolution also require the
  existing validators; this structural schema adds no timing or reference claims.
- Controller generated is forbidden before schema 54, even if null; from 54 it must be true.
  The standalone controller reader still accepts generated:true independently of file version;
  parseDocument retains the earlier pre-54 guard and its diagnostic path.
- Legacy programs require text id, numeric offset and a phases array. Each phase requires
  numeric duration and color exactly red/amber/green. Program/phase keys remain open at every
  version. Positive durations and valid cycles remain outside this structural contract.

Shared metadata supplies native type/key reads and schema rules. Controller groups retain
integer conversion and diagnostic order, with generated validation after groups. The program
reader retains duration-before-color diagnostics. Pre-13 project migration stays after definition
and network decoding; bare M0 scenarios retain their programs. The envelope-only schema API,
writer version selection and desktop observed-shape response remain unchanged.


## Right-of-way extension (D361)

The composed CLI coverage adds `-right-of-way` to the D360 coverage identifier; complete
remains false. Network rightOfWay is known/read from schema 14; Stop/Yield rows from 15.
Before schema 7 the whole unknown property is ignored, while schemas 7–13 reject it even
when null. In schema 14 a stopControls property is unknown even when null.

- All four collections (waitingLines, conflictAreas, priorityRules, stopControls) are optional;
  absent/null collections decode as empty. Active non-null collections require arrays. The
  legacy rightOfWay reader itself ignores non-object containers, yielding empty controls;
  schema generation preserves that behavior instead of inventing an object-only rule.
- Waiting lines require id and point; a point requires path and numeric station. Conflict
  areas require id, kind (crossing/merge), first, second and priority
  (firstYields/secondYields/undetermined). Each side requires path, numeric entryStation and
  exitStation, and text waitingLineId. Branching remains a derived display kind.
- Priority rules require id, conflictAreaId and numeric gapTime/headway. Stop controls require
  id, waitingLineId, mode (stop/yield) and a text array conflictAreaIds. These four row types
  accept optional nullable text name. From schema 54 generated may only be true; earlier
  active readers reject the key. Nested objects reject unsupported keys.
- A ControlPathRef reads five optional nullable strings (linkId, laneId, connectorId,
  fromLaneId, toLaneId); the codec checks their meaning later. Its legacy decoder also maps
  non-objects to empty references. Required path fields reject null before that decoder;
  schema rules retain this distinction. Queue counters reuse this same native path reader.

Native reads and generated schemas share the field metadata. Point-before-id errors for
waiting lines and generated-value validation after each row retain their original order.
There is no new geometry or runtime algorithm. Owner/path/line/area resolution, station and
extent validity, positive gap/headway, priority cycles and run-readiness remain existing
model checks. Queue-counter and travel-time-section row schemas remain follow-up work.


## Measurement extension (D362)

The composed CLI coverage adds `-measurements` to the D361 identifier; complete remains
false. Queue counters are known/read from schema 16; travel-time sections from 23; a section's
controlType from 24. Before schema 7 unknown network properties remain ignored; later files
predating these keys reject them even when null. Active network arrays may be absent/null.

- Queue-counter rows require text id and a lines array; optional name accepts text/null.
  Measurement lines read optional nullable referenceId and point. A non-null point requires
  path and numeric station, using the existing shared ControlPoint/ControlPathRef metadata.
  Unsupported keys fail. Non-object measurement lines retain the legacy empty-source decode;
  exactly one usable source, valid references and valid positions remain model checks.
- Travel-time sections require text id and start/end line objects, each with text linkId and
  numeric station. Optional name accepts text/null. From 24, optional controlType accepts
  signalised/unsignalised/null; schema 23 rejects the key even if null. No generated flag is
  accepted on counters, measurement lines, section rows or section lines.
- Measurement-line referenceId is read before point. Section id/name and start/end decode
  before controlType. Native diagnostics and paths remain in that order. The parser delegates
  these readers at the same stages after right-of-way decoding and attachment migration.

Requiredness, null policy, type and known-key rules use shared metadata. Schema definitions
reuse the same control point/path entries for waiting lines and queue counters. No measurement
algorithm, LOS threshold, delay definition, writer version rule or serialized output changes.
Negative/out-of-bounds stations, reversed sections, empty or ambiguous measurement sources,
reference resolution and runtime readiness remain the existing model/evaluation validators.


## Route and VehicleInput extension (D363)

The composed CLI coverage adds `-routes-inputs` to the D362 identifier; complete remains
false. A present definition object now also requires numeric duration/timeStep and routes/
inputs arrays, as the native ScenarioDefinition reader does. Definition absence/null remains
valid for a draft project; other unmapped definition fields remain codec-owned.

- Route rows require text id and a segmentIds array of text. VehicleInputs require text id,
  routeId and vehicleTypeId, plus numeric vehiclesPerHour, startTime and endTime. Optional
  laneShares is a numeric array; compositionId/routingDecisionId/linkId are optional text.
  These optionals use contains, so explicit null fails instead of meaning absent.
- Optional intervals requires an array of rows with numeric startTime/endTime/vehiclesPerHour.
  Rows decode before deriveInputTotals replaces scalar totals. Scalars are still required and
  type-checked first; an empty interval array leaves them unchanged. Ordered/gapped intervals,
  nonnegative rates and valid time windows remain model concerns.
- Route, input and interval keys stay open at every version, preserving the standalone reader.
  volumeFromCounts requires a boolean and is forbidden in project files before schema 26 even
  when false/null. This shared version floor remains enforced before definition decoding;
  the standalone scenario reader still accepts the boolean independently of project version.
- Legacy route lane/path-to-object migration and counted-volume synchronization retain their
  original stages. Counted volumes still derive from routing-decision counts after the full
  definition is read; the writer omits those derived intervals as before.

Shared metadata supplies requiredness, native types and schema shapes. File version selection,
serialized bytes, catalog ownership, routing algorithms and simulation dynamics are unchanged.
Topology/reachability, catalog/reference resolution, valid demand rates/windows, lane weights
and counted-decision semantics remain existing validators. Routing decisions and catalog row
schemas remain separate follow-ups.


## RoutingDecision extension (D364)

The composed CLI coverage adds `-routing-decisions` to the D363 identifier; complete remains
false. Definition routingDecisions is optional; explicit null fails. Decision rows require text
id and a routes array. Their optional name/linkId/position/intervals use present, so null is
ignored. TypeRules uses contains, so explicit null fails. Decision, route-share and type-rule
keys remain strict even for files before schema 7; period rows keep open extra keys.

- Route shares require text routeId and numeric relativeFlow. Optional destinationLinkId and
  intervalFlows accept text/null and numeric-array/null respectively. Decision periods require
  numeric startTime/endTime. Type rules require text vehicleTypeId, a numeric relativeFlows
  array and an intervalFlows array of numeric arrays. Empty arrays/negative weights are left
  to existing model validators, along with matrix dimensions, windows and destination topology.
- Project files before schema 20 reject position even if null. The document guard still runs
  before normal decoding; standalone routing reads and bare project reads keep their existing
  position support. Schema generation and the document guard share the same version floor.
  Type rules retain their historical reader behavior without an artificial schema-19 read gate.
- Native error ordering and paths remain unchanged. Relative-flow cells report
  `Expected numbers: relativeFlows`; malformed matrix rows report ROUTING_DECISION_INTERVALS;
  nonnumeric share/matrix cells report INVALID_SHARE. Nullable periods/flows remain distinct
  from required non-null type-rule arrays.

The dedicated reader uses shared field metadata. Counted-volume synchronization remains after
complete definition decoding, and the writer still omits derived counted intervals. No routing
or simulation algorithm, catalog ownership, writer version rule, serialized output or desktop
observed-schema response changes. Catalog row schemas remain separate follow-ups.
