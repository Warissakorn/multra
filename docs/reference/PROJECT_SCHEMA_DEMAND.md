# Project schema — demand extensions

The demand rows of the composed partial project schema (`multra-project-schema`), moved from
[PROJECT_SCHEMA](PROJECT_SCHEMA.md) when that file reached its size limit. Its envelope, presence
and save-floor rules apply here unchanged; `parseDocument` and `validateDocument` stay
authoritative, and `x-multra-complete` stays false.

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
and counted-decision semantics remain existing validators. Routing decisions are D364 and owned
compositions D558 (below).

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
observed-schema response changes. Owned composition rows are D558.

## Owned composition extension (D558)

The composed CLI coverage adds `-compositions` to the D364 identifier; complete remains false.
`definition.compositions` is optional and never required; a present value must be an array
(null or another type is `EDIT_CATALOG_READ` in `parseAuthoringDefinition`). Absence still
means the installed catalog and a present empty array still owns an empty one. No read gate:
every editor schema 1 to the latest reads owned compositions, their intervals included. The
save floors 18 (owned compositions) and 19 (composition intervals) stay writer-only rules.

- Composition rows require text `id` and a `types` array. Optional `name` (text) and
  `intervals` (array) use contains, so explicit null fails (`Field is null: name`).
- Type-share rows require text `vehicleTypeId` and numeric `share`. Interval rows require
  numeric `startTime`/`endTime` and a `types` array of the same type-share rows.
- All three rows are closed at every file version, schema 1–6 included, as `parseComposition`
  always was: an unknown key is `EDIT_UNSUPPORTED_FIELD` at `composition.<key>`,
  `composition.types.<key>`, `composition.intervals.<key>` or `composition.intervals.types.<key>`,
  reported before a missing field of the same row.

`parseComposition` reads through the shared field metadata (`Object::composition`,
`compositionShare`, `compositionInterval` in `network_fields.cpp`), so the schema and reader
cannot disagree on keys, requiredness or types. The installed catalog (`loadCompositions`) uses
the same reader and keeps its diagnostics. Positive shares, a positive sum, known vehicle types,
unique ids, ordered and non-overlapping intervals and an input's `compositionId` remain the
existing catalog and time-type validators. Owned modes, vehicle types and behaviours remain
codec-owned and outside the coverage claim.

Acceptance (`tests/composition_schema_tests.cpp`, group `project`):

| Row | Check |
|---|---|
| CS1 | The reader's unknown-key paths, null/array/number/missing diagnostics and the `EDIT_CATALOG_READ` container refusal are unchanged |
| CS2 | At every schema version the four rows are closed, `id`/`types`, `vehicleTypeId`/`share` and `startTime`/`endTime`/`types` are required, `name` is not nullable and `compositions` is not required |
| CS3 | The base document parses at every version, and every case the schema refuses is refused by `parseDocument` |
| CS4 | The shipped projects with owned compositions validate against the emitted schema (checked once with Python `jsonschema` when this was built; not in `check`) |
