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
existing catalog and time-type validators. Owned modes are D559 and owned vehicle types D560 (below);
behaviours remain codec-owned and outside the coverage claim.

Acceptance (`tests/composition_schema_tests.cpp`, group `project`):

| Row | Check |
|---|---|
| CS1 | The reader's unknown-key paths, null/array/number/missing diagnostics and the `EDIT_CATALOG_READ` container refusal are unchanged |
| CS2 | At every schema version the four rows are closed, `id`/`types`, `vehicleTypeId`/`share` and `startTime`/`endTime`/`types` are required, `name` is not nullable and `compositions` is not required |
| CS3 | The base document parses at every version, and every case labelled refused is refused by `parseDocument`; the 4,408 labels were checked once against the emitted schema with `jsonschema` (0 disagree) |
| CS4 | The shipped projects with owned compositions validate against the emitted schema (checked once with Python `jsonschema` when this was built; not in `check`) |

## Owned mode extension (D559)

The composed CLI coverage adds `-modes` to the D558 identifier; complete remains false. Contract:
[STUDY_PERIODS](STUDY_PERIODS.md) §6 (D269).

- `definition.modes` is optional and never required. Below schema 49 it is refused even when
  null: the schema gives it `false`, and `rejectModesBefore49` refuses it at `definition.modes`
  (`EDIT_UNSUPPORTED_FIELD`) before the definition is read. Both use `networkCodec::modeVersion`.
- From 49 a present value must be an array (null or another type is `EDIT_CATALOG_READ`). A mode
  row has exactly `id`, `name` and `compositionId`, each required text; it is closed at every
  version. Every structural refusal of a row is the bare `EDIT_CATALOG_READ`, with no path, for
  an owned entry and an installed `data/modes/` file alike.
- `parseMode` takes its key list and count from the same metadata (`Object::mode`), so the
  schema and reader cannot disagree on keys; its diagnostic is unchanged.

Unique ids, nonblank ids and a known composition (`MODE_COMPOSITION_UNKNOWN`) remain the catalog
validators. The same schema-49 guard also refuses `odMatrices[i].modeId` (codec-owned; zones are not in the
coverage claim) and a vehicle type's `pcu` (mapped by D560).

| Row | Check |
|---|---|
| MS1 | `parseMode`'s what() is exactly `EDIT_CATALOG_READ` for two or four keys, a swapped key, a number, a null and a non-object; `modes: null` from 49 gives the same code; at 48 an array, null or empty array is `EDIT_UNSUPPORTED_FIELD` at `definition.modes` |
| MS2 | Below 49 `modes` is `false`; from 49 it is an array of closed rows with three required strings; it is never required |
| MS3 | The base document is refused at `definition.modes` below 49 and parses from 49; every case labelled refused is refused by `parseDocument`; the 1,672 labels were checked once against the emitted schema with `jsonschema` (0 disagree) |
| MS4 | A project owning the two installed modes validates at its own version 50 and is refused at `definition` at 48 (one-off, `jsonschema`; no shipped or test file owns modes) |

## Owned vehicle-type extension (D560)

The composed CLI coverage adds `-vehicle-types` to the D559 identifier; complete remains false.
`definition.vehicleTypes` is optional and never required; absence means the installed catalog.
A present value must be an array (`parseDefinition`'s `Expected array` / `Field is null`).

- **The row** requires text `id` and `behaviourId`, numeric `length`, `width`, `maxAcceleration`,
  `comfortableDeceleration`, `maxDeceleration`, and a `desiredSpeed` object. `desiredSpeed` is
  read before `id`. Optional `name` (text, contains-based; a null is `EDIT_CATALOG_READ`, read by
  `parseAuthoringDefinition`, ignored by a bare definition) and `axles`.
- **Open below 21, closed from 21:** before `behaviourVersion` an unknown key is read and not
  written back (the legacy additive rule, as the network below 7); from 21
  `parseBehaviourLibrary` refuses it at `vehicleTypes[i].<key>` after the definition is read.
- **`axles`** is closed at every version (`vehicleType.axles.<key>`) and requires numeric
  `wheelbase`, `frontOverhang` and `rearOverhang`.
- **`desiredSpeed`** stays open and requires numeric `min` and `max`. Its `curve` (from 40) is an
  array of open points with numeric `speed` and `share`; a bad curve or point is
  `INVALID_SPEED_CURVE` at `vehicleType.desiredSpeed.curve`.
- **`kerbLanes`** (from 42) is a JSON integer token: `2.0` is `INVALID_KERB_LANES` in the reader
  although JSON Schema's `integer` admits it, so the rule carries `x-multra-nativeType:
  integer-token`; 1–16 is the validator's.
- **`pcu`** (from 49) is closed `{value: number, source: text}`; anything else is `INVALID_PCU`.
- **Gates:** `curve` below 40, `kerbLanes` below 42 and `pcu` below 49 are refused even when null,
  at `vehicleTypes[i].desiredSpeed.curve`, `.kerbLanes` and `.pcu`. The schema gives each `false`,
  and the document guards use the same constants (`speedCurveVersion`, `kerbLanesVersion`,
  `modeVersion`).

`parseVehicleType` reads the scalars and axles through `networkCodec::value()` with unchanged
diagnostics, and the from-21 key check uses the same field table. The speed range and curve,
`kerbLanes` and `pcu` keep their own readers and codes; for those the table drives only the
schema. Two reader limits are recorded, not changed: `desiredSpeed.min`/`max` are read with the
JSON library's `.at()`, so a missing or non-numeric bound reports that library's message
and the open `desiredSpeed` object and curve points accept an unknown key at every version and
drop it on save (checked by a round-trip at the latest schema), as the below-21 type row does
(both: [OWNER_SITTING](../plans/OWNER_SITTING.md) item 49). The installed catalog
(`resolveDemandCatalog`) uses the same `parseVehicleType`. Ranges, unknown behaviours and duplicate ids
remain the catalog validators.

| Row | Check |
|---|---|
| VT1 | On the unchanged reader: an unknown key is kept out of the save below 21 and refused at `vehicleTypes[0].extra` from 21; `vehicleType.axles.extra` refused; `desiredSpeed` missing is reported before `id`; field sentences; a missing bound throws; float `kerbLanes`, partial `pcu` and a non-array curve keep their codes; `name: null` refused in a project and ignored bare; the three guards refuse null at their paths |
| VT2 | At every version the row is open exactly below 21, the eight keys are required, `axles` and `pcu` are closed, `desiredSpeed` and curve points are open, and `curve`/`kerbLanes`/`pcu` are `false` below 40/42/49 |
| VT3 | The base row parses at every version, and every case labelled refused is refused by `parseDocument`; the 5,700 labels were checked once against the emitted schema with `jsonschema` (0 disagree) |
| VT4 | One-off `jsonschema`: the 9 shipped and test projects owning types, the 7 installed `data/vehicle-types/` files, and a writer round-trip carrying `curve`, `kerbLanes` and `pcu` all validate |

## Behaviour inventory (D561, before code)

Read 2026-11-14 from `parseBehaviour` (`parse.cpp`), `parseBehaviourLibrary` and the `reject*`
guards (`behaviour_library.cpp`), and the key lists in `src/core/`. Nothing below is mapped yet.

- **Model tag.** Below 21 `model` and `name` are refused at `behaviours[i].<key>` even when null
  (`rejectBehaviourLibraryBefore21`), so every row is the prototype. From 21 `model` is required:
  missing, non-text or unknown is `UNSUPPORTED_BEHAVIOUR_MODEL` at `behaviours[i].model`
  (`parseBehaviourLibrary`). `"w74"` below 25 is the same code (`rejectW74Before25`). Values:
  `kPrototypeBehaviourModel`, `kW74BehaviourModel`.
- **Prototype row.** Requires text `id` and numeric `standstillDistance`, `additiveSafetyDistance`,
  `multiplicativeSafetyDistance`, `followingTime`, `speedThreshold` (field sentences). Optional
  numbers, contains-based: `maxDecelerationCooperativeBraking`, `discretionaryLaneChangeThreshold`,
  `acceptedDecelerationTrailingVehicle`, `discretionaryLaneChangeHoldTime`, `amberDeceleration`.
  Any W74 key is `EDIT_UNSUPPORTED_FIELD` at `<path>.<key>` at every version.
- **W74 row.** Requires text `id` and every `w74ParameterKeys()` entry (18) as a number
  (`INVALID_BEHAVIOUR_PARAMETER` at the key). The eight prototype-only keys (the five required
  above plus the three lane-change ones) are `EDIT_UNSUPPORTED_FIELD`. Optional
  `maxDecelerationCooperativeBraking` and `amberDeceleration`.
- **Both models.** `perceptionParameterKeys()` (5; `observedVehiclesAhead` and the integer flag
  make it a JSON integer token), `lateralParameterKeys()` (3) and the `readOptionals` numbers
  (`curveLateralAcceleration`, `kerbLaneBias`, `commitmentDeceleration`, `mandatoryDecelerationOwn`,
  `mandatoryDecelerationTrailingVehicle`, `stopDwellTime`, `driverFactorSpread`, `redRunningShare`,
  `redRunningTime`); a non-number (or a non-integer perception integer) is
  `INVALID_BEHAVIOUR_PARAMETER`; null is refused. Ranges and lateral completeness are validators.
- **Closure.** Below 21 the row is open and additive. From 21 `parseBehaviourLibrary` refuses an
  unknown key against a hand-kept `common[]` list plus the three core lists, after the definition
  is read and after the model branch has refused the other model's keys.
- **Floors (presence, even null, `EDIT_UNSUPPORTED_FIELD` at `behaviours[i].<key>`):**
  `amberDeceleration` 28, perception 32, lateral 33, `curveLateralAcceleration` 34, `kerbLaneBias`
  35, `commitmentDeceleration` 36, both mandatory keys 37, `stopDwellTime` 41, `driverFactorSpread`
  43, both red-running keys 44. Owned W74 (25) is the read guard above plus a save floor.
- **Census.** One installed file (`data/driver-behaviour/`), prototype; nine shipped/test projects
  own behaviours, all prototype; no file in the repository carries a W74 row.
- **Out of scope:** `vehicleClasses` and `linkBehaviourTypes` (refused below 21; separate rows).

**Slices.** B1: the common row and the model tag as a per-model branch (`if model` then the W74
row, else the prototype), keys generated from the core lists rather than copied into
`network_fields.cpp` (they stay `core/`'s; floors stay project-side), the `common[]` list replaced
by the same table, a completeness test that every core and common key has a property in the
latest row; the row open until B2 maps the rest. B2: the W74 row and the closed from-21 row with
all floors. B3: perception's integer token and lateral, if not already in B2. Each slice pins the
reader first; the reader stays schema-only unless a pin shows identical diagnostics and values.
