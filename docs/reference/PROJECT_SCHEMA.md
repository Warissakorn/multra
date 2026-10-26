# Project schema inventory

This is the codec ownership and version map for an editor-project schema (D355), with
generated partial envelope/background (D356) and Network/Link/Lane contracts (D357). It describes implemented
readers/writers, rather than inferring a contract from one document. It is **not a complete JSON Schema**, a new validator, or a change to file
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
Connector, control and demand field metadata remain separate follow-ups; their semantic
validation stays in the codec.

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
is `envelope-background-network-link-lane` and complete remains false. Desktop observed
schema discovery is unchanged.

The generated schema chooses rules from the document's root `schemaVersion` (1–57).
Identical structures share `$defs`, and identical network profiles share conditional version
groups. Reference names are implementation details; consumers follow `$ref`. The same
metadata drives native known-key checks and Link/Lane field reads. Link decoding has its
own file; shared Connector/reference decoding and model geometry algorithms are unchanged.

Covered structure:

- Network requires id, drivingSide (left/right), links, connectors and signalHeads. Links
  have typed items; Connector and signal-head arrays deliberately have unconstrained items.
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
