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
- **`desiredSpeed`** requires numeric `min` and `max` (`value()` sentences since D562). It and
  its curve points close with the row from 21 (D562): an unknown key is `EDIT_UNSUPPORTED_FIELD` at
  `vehicleTypes[i].desiredSpeed.<key>` or `vehicleTypes[i].desiredSpeed.curve[k].<key>`; below 21
  they stay additive, and the installed catalog and bare definitions stay open. Its `curve` (from
  40) is an array of points with numeric `speed` and `share`; a bad curve or point is
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
diagnostics, and the from-21 key checks use the same field table. The curve, `kerbLanes` and
`pcu` keep their own readers and codes; for those the table drives only the schema. The two
reader limits D560 found were answered by the owner ([OWNER_SITTING](../plans/OWNER_SITTING.md)
item 49) and fixed by D562: the bounds now report `Missing field: min`, `Expected number: max`
or `Field is null: min` instead of the JSON library's message, and `desiredSpeed` and curve
points no longer drop unknown keys from 21. The installed catalog
(`resolveDemandCatalog`) uses the same `parseVehicleType`. Ranges, unknown behaviours and duplicate ids
remain the catalog validators.

| Row | Check |
|---|---|
| VT1 | On the unchanged reader: an unknown key is kept out of the save below 21 and refused at `vehicleTypes[0].extra` from 21; `vehicleType.axles.extra` refused; `desiredSpeed` missing is reported before `id`; field sentences; a missing, null or non-numeric bound reports its field sentence (D562); unknown keys in `desiredSpeed` and a curve point are refused at their paths from 21 and kept out of the save below it (D562); float `kerbLanes`, partial `pcu` and a non-array curve keep their codes; `name: null` refused in a project and ignored bare; the three guards refuse null at their paths |
| VT2 | At every version the row, `desiredSpeed` and curve points are open exactly below 21 (D562), the eight keys are required, `axles` and `pcu` are closed, and `curve`/`kerbLanes`/`pcu` are `false` below 40/42/49 |
| VT3 | The base row parses at every version, and every case labelled refused is refused by `parseDocument`; the 5,700 labels were checked once against the emitted schema with `jsonschema` (0 disagree) |
| VT4 | One-off `jsonschema`: the 9 shipped and test projects owning types, the 7 installed `data/vehicle-types/` files, and a writer round-trip carrying `curve`, `kerbLanes` and `pcu` all validate |

## Behaviour extension (D561 inventory, D563 B1, D564 B2)

Read 2026-11-14 from `parseBehaviour` (`parse.cpp`), `parseBehaviourLibrary` and the `reject*`
guards (`behaviour_library.cpp`), and the key lists in `src/core/`. B1 (D563) and B2 (D564) are mapped,
below; B3 needed nothing (B1 already typed perception and lateral).

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

**Generator gap.** `network_schema.cpp` has no per-model branch; B1 adds one (the `whenNonNull`
`if`/`then` is the precedent). Requiredness from a version already works: a required field with
`rejectBefore` is `false` and absent from `required` below its floor, required from it.

**Slices** (what the schema newly refuses in each):
- **B1:** `model` and `name` `false` below 21; from 21 `model` required, a prototype-only enum
  21–24 and both values from 25; `if model == w74` selects the W74 branch, else the prototype.
  Prototype branch: its five required numbers, its optional numbers, every W74 key `false`. W74
  branch: the 18 W74 keys required, the eight prototype-only keys `false`. Keys come from the core
  lists, not copies in `network_fields.cpp` (floors stay project-side); `common[]` moves to the
  same table. Both branches stay open (`additionalProperties` true); a completeness test checks
  every core and common key has a property in the latest branches.
- **B2:** the ten floors (`false` below each) and both branches closed from 21.
- **B3:** perception's integer token and the lateral numbers' types, if B2 left them unchecked. Each slice pins the
reader first; the reader stays schema-only unless a pin shows identical diagnostics and values.

**B1 as built (D563).** Coverage adds `-behaviours`; complete remains false. `definition.behaviours`
is optional, never required. Below 25 every row is the prototype row; from 25 the item is
`if model == "w74"` then the W74 row, else the prototype row. `model` and `name` are `false`
below 21; from 21 `model` is required (prototype only at 21–24, both values from 25; the W74 row
takes `const "w74"`). The prototype row requires `id` and its five numbers, and gives every W74
key `false`; the W74 row requires `id` and the 18 W74 keys and gives the eight prototype-only keys
`false`. Shared optional numbers, perception (`observedVehiclesAhead` an `integer-token`) and
lateral keys are numbers in both. Both rows stay open, and the ten floors are not mapped (B2).
The rows are built once from `w74ParameterKeys`, `perceptionParameterKeys` and
`lateralParameterKeys` plus a project-side list (`network_fields.cpp`), and
`parseBehaviourLibrary`'s from-21 closure now asks that table (`behaviourKey`); its old `common[]`
held exactly the table's non-core keys (checked before removal). The reader is otherwise
unchanged in B1; `readOptionals`' second key list was folded into the table by B2 (D564).

| Row | Check |
|---|---|
| BH1 | Per version: branch from 25 only, prototype `required` (`model` from 21), `model`/`name` `false` below 21, the enum size, W74 keys `false` on the prototype, prototype keys `false` on the W74 row, the 18 W74 keys required there |
| BH2 | Every core key has a property in both latest rows and is a `behaviourKey`; both rows list the same names; `cc0` is not a key |
| BH3 | The prototype base parses at every version and the W74 base from 25; every case labelled refused is refused by `parseDocument`; the 7,036 version- and model-dependent labels agree with the emitted schema under `jsonschema` (one-off), as do a W74 writer round-trip and all 18 projects in the repository |

**B2 as built (D564).** Both rows are closed from 21 (`additionalProperties` false; open and
additive below), and each floored key is `false` below its floor in both rows: `amberDeceleration`
28, the perception keys 32, the lateral keys 33, `curveLateralAcceleration` 34, `kerbLaneBias` 35,
`commitmentDeceleration` 36, both mandatory keys 37, `stopDwellTime` 41, `driverFactorSpread` 43,
both red-running keys 44. The floors are named constants in `network_fields.hpp`
(`amberVersion` … `redRunningVersion`) that `document.cpp`'s `reject*` calls now use. The shared
optional numbers are one table (`behaviourOptionalNumbers`: name, `DriverBehaviour` member, floor,
whether a bad value is `INVALID_BEHAVIOUR_PARAMETER`); `readOptionals` reads its nine keys from it
in the old order, so the duplicate D563 noted is gone.

| Row | Check |
|---|---|
| BH4 | On the unchanged reader: each of the 18 floored keys, null, is refused at `behaviours[0].<key>` one version below its floor; an unknown key is refused from 21 and kept out of the save below |
| BH5 | Both rows closed exactly from 21; every floored key `false` exactly below its floor in both rows |
| BH6 | 14,076 version- and model-dependent labels, a W74 writer round-trip and all 18 projects in the repository agree with the emitted schema (`jsonschema`, one-off) |

## Vehicle-class and link-behaviour-type extension (D565)

The composed CLI coverage adds `-behaviour-classes`; complete remains false. Both catalogs are read
by `parseBehaviourLibrary` from 21 and refused below it, even null, at `definition.vehicleClasses`
and `definition.linkBehaviourTypes` (`rejectBehaviourLibraryBefore21`); the schema gives each
`false` below 21. From 21 a present value must be an array (null or another type is
`EDIT_CATALOG_READ`).

- A vehicle class requires text `id` and a `vehicleTypeIds` array of text; optional text `name`.
- A link behaviour type requires text `id`; optional text `name` and `defaultBehaviourId`, and an
  `overrides` array of rows that each require text `classId` and `behaviourId`.
- All three rows are closed: an unknown key is `EDIT_UNSUPPORTED_FIELD` at `vehicleClasses[i].<key>`,
  `linkBehaviourTypes[i].<key>` or `linkBehaviourTypes[i].overrides[k].<key>`.
- Optional texts are contains-based: a null `name` is `Expected text: name` (the reader keeps its
  own `text()` helper, whose sentence differs from `value()`'s `Field is null`).

The key checks now use `networkCodec::knownFields` with the same tables, so `requireKnownFields`
(its only callers) is removed. Reference resolution (known classes, behaviours, vehicle types)
stays with the library validators.

| Row | Check |
|---|---|
| BC1 | On the unchanged reader: both gates refuse null at their paths below 21; the three unknown-key paths; `Expected text: name` and `: vehicleTypeIds`; `EDIT_CATALOG_READ` for a null catalog; `Missing field: id` |
| BC2 | Both catalogs `false` below 21; from 21 closed rows with `id` required, `vehicleTypeIds` required on a class, `classId`/`behaviourId` on an override |
| BC3 | Every case labelled refused is refused by `parseDocument`; the 3,040 labels, a writer round-trip and all 18 projects agree with the emitted schema (`jsonschema`, one-off) |

## Zone-layer extension (D566)

The composed CLI coverage adds `-zones`; complete remains false. The four root arrays `zones`,
`zoneConnectors`, `odMatrices` and `parkingLots` sit in each version branch beside `network` and
`definition` (`zone_codec.cpp`). Below 31 each is refused when present, even null
(`UNSUPPORTED_FIELD: <key>`); the schema gives each `false`. From 31 a present value is a
nonempty array (`minItems` 1); null, `[]` or another type is `EDIT_ZONE_INVALID`. Every row is
closed, and every row refusal is the bare `EDIT_ZONE_INVALID` unless named below.

- A zone requires text `id` and `name`, a `polygon` of `[x, y]` number pairs and an `attributes`
  object of numbers.
- A Link connector requires text `id`, `zoneId`, `linkId`, a `direction` of `origin` or
  `destination`, and a number `share`. From 57 a connector carrying the street-segment id key is
  the street row instead: `roadId` and `roadDirection` (57–58) or `streetSegmentId` and
  `streetSegmentDirection` (59 on), a nonempty id and `forward` or `backward`, with the same
  `id`, `zoneId`, `direction` and `share` (D389). The schema selects the row with `if required
  [<id key>]`. The other version's names are `EDIT_UNSUPPORTED_FIELD` at
  `zoneConnectors[i].<key>`, and `roadId`/`roadDirection` are refused so below 57.
- An OD matrix requires text `id`, a `zoneIds` array of text and a `cells` array of number arrays.
  It takes `compositionId` or, from 49, `modeId`; and `startTime` with `endTime` or, from 49,
  `periodId`. Both or neither is `OD_TYPES_AMBIGUOUS` or `OD_PERIOD_AMBIGUOUS` at `odMatrices[i]`,
  checked before the keys. `modeId` and `periodId` below 49 are `EDIT_UNSUPPORTED_FIELD` at their
  paths (`rejectModesBefore49`, `rejectStudyPeriodsBefore49`). An optional `generatedBy` from 70 is
  only `"mode-choice"` (`OD_GENERATED_INVALID`); below 70 it is `EDIT_UNSUPPORTED_FIELD`.
- A parking lot requires text `id`, `zoneId` and `initialCompositionId`, unsigned-integer tokens
  `capacity` and `initialOccupancy` (at most 4,294,967,295), a number `gateServiceTime` and a `dwell`
  of `[minutes, cumulative]` pairs.

The either-or keys use a field attribute, `unless`: such a field is required unless the named
alternative is present, and refused when it is. The schema writes it as `if required [modeId] then
compositionId false else compositionId required` (a `oneOf` of required lists would accept
`periodId` with only `startTime`). Below the alternative's floor the field is plainly required. The
codec's `exactKeys` reads the same tables: an inactive or excluded key may not appear, a required
one must. The two ambiguity checks and the `generatedBy` checks keep their own codes and order.

Not in the schema: a `5.0` count (a native unsigned token), the model checks of
`zoneLayerIssues` (simple polygons, known references, square cells, the dwell curve, unique ids),
and the root `studyPeriods` and `studyArea`, which have their own codecs.

| Row | Check |
|---|---|
| ZL1 | On the unchanged reader: both bases parse (Link connectors from 31, street connectors from 57); the 31 gate even null; `EDIT_ZONE_INVALID` for null, `[]` and an unknown key; the two ambiguity codes before an unknown key; `modeId`/`periodId` below 49, `generatedBy` below 70, `OD_GENERATED_INVALID`; `roadId`/`roadDirection` below 57 and the rename on each side of 59; an empty street id; a signed `capacity` token |
| ZL2 | The four keys `false` below 31 and nonempty arrays from it; the connector branch from 57 with the version's id key; closed rows; the matrix's required list and conditions from 49; `generatedBy` `false` below 70; the lot's unsigned-token marker |
| ZL3 | Every case labelled refused is refused by `parseDocument`; the 9,668 labels, writer output (a period and mode matrix, a generated matrix, the street fixture at 57, 58 and 59) and all 18 projects, 8 with zones, agree with the emitted schema (`jsonschema`, one-off) |

## Study-period and study-area extension (D567)

The composed CLI coverage adds `-studies`; complete remains false. Three more keys join the
zone-layer table: the root `studyPeriods` and `definition.clockStart` (S1.1, from 49) and the root
`studyArea` (S5.2a, from 55). Each is refused below its floor when present, even null
(`EDIT_UNSUPPORTED_FIELD` at `studyPeriods`, `definition.clockStart` or `studyArea`); the schema
gives each `false`.

- `studyPeriods` from 49 is a nonempty array of closed rows requiring text `id` and `name` and
  number `start` and `end`; anything else is `EDIT_ZONE_INVALID` (`study_period_codec.cpp`).
- `definition.clockStart` from 49 is an optional number; null or another type is `INVALID_NUMBER`
  at `definition.clockStart`. Its range (0 to two days) is the model's check.
- `studyArea` from 55 is a closed object requiring a nonempty `nodes` array of text and a
  `flowSource` of `typed`, `macro` or `meso`, with an optional nonempty `gateFlows` array. A gate
  flow is a closed row requiring text `periodId`, `from` and `to` and a number `vehiclesPerHour`,
  and either text `compositionId` or text `modeId` (`unless`, as in D566). Anything else is
  `EDIT_STUDY_AREA_INVALID` (`study_area_codec.cpp`).

Both codecs now check keys with `networkCodec::exactFields` (D566's `exactKeys`, moved beside the
tables) and take the file version. Known Nodes and periods, nonnegative flows and the clock range
stay with `studyAreaIssues` and `validateDocument`.

| Row | Check |
|---|---|
| SP1 | On the unchanged reader: both bases parse (periods and clockStart from 49, the SA2 corridor from 55); the three floors even null, at their paths; `INVALID_NUMBER` for a null, text or negative clockStart; `EDIT_ZONE_INVALID` for null, `[]`, a number or an unknown key; `EDIT_STUDY_AREA_INVALID` for a null area, an unknown key and `modeId` beside `compositionId`; a mode gate flow parses |
| SP2 | The three keys `false` below their floors; from them a number clockStart, nonempty closed period rows, a closed area with `nodes`/`flowSource` required and a three-value enum, and closed gate flows with the `modeId` condition |
| SP3 | Every case labelled refused is refused by `parseDocument`; the 2,888 labels, writer output (periods with clockStart at 49; the corridor at 55, 58 and 59; a mode gate flow), D566's 9,668 labels and all 18 projects agree with the emitted schema (`jsonschema`, one-off) |

## Map extension (D568)

The composed CLI coverage adds `-map`; complete remains false. The root `map` (M1.21,
`map_codec.cpp`) joins the root feature table (`rootFeatures`, renamed from D566's zone-layer
table). Below 29 it is refused when present, even null (`UNSUPPORTED_FIELD: map`); the schema gives
it `false`. From 29 it is a closed object; anything else is `EDIT_MAP_INVALID`.

- `providerId` is text of 1–80 characters from `a`–`z`, `0`–`9` and `-` (`^[a-z0-9-]{1,80}$`);
  `opacity` a number from 0 to 1; `reference` a closed object.
- The reference requires `projectedCrs`, a WGS 84 UTM zone (`EPSG:32601`–`32660` or
  `32701`–`32760`), `anchorLongitude` −180 to 180, `anchorLatitude` −80 to 84, `anchorWorldX` and
  `anchorWorldY` within ±`mapWorldAnchorLimit` (10⁷ m) and `rotationDegrees` within ±180.

These ranges are the codec's own (`validateMapSettings`, `utmCrs`), written as `minimum`,
`maximum` and `pattern`. Left to the codec: the CRS must be the anchor's own UTM zone, including
the Norway and Svalbard exceptions.

| Row | Check |
|---|---|
| MP1 | On the unchanged reader: the base parses from 29; the 29 gate even null; `EDIT_MAP_INVALID` for null, a number, `{}` and an unknown key in either object; a well-formed zone that is not the anchor's is refused; the limits (80 characters, opacity 1, rotation −180, anchor 10⁷) parse |
| MP2 | `map` `false` below 29; from 29 both objects closed with every key required, and the opacity, latitude and anchor bounds |
| MP3 | Every case labelled refused is refused by `parseDocument`; the 4,636 labels, writer output (a southern zone at the range limits), D566's and D567's labels and all 18 projects agree with the emitted schema (`jsonschema`, one-off) |

## Macro settings and run extension (D569)

The composed CLI coverage adds `-macro-runs`; complete remains false. `definition.macroSettings`
(S2.3, `macro_settings.cpp`) from 51 and the root `macroRuns` (`macro_run_definition.cpp`) from 56
are each refused below their floor when present, even null (`EDIT_UNSUPPORTED_FIELD` at
`definition.macroSettings` or `macroRuns`); the schema gives each `false`.

- A settings row is a closed object requiring a number `targetGap` with 0 < gap < 1 and an
  integer `maxIterations` from 1 to 100,000 (a native integer token: `2.5` and `5.0` are refused).
  Anything else is `INVALID_MACRO_SETTING` at the settings path or its key.
- `macroRuns` is a nonempty array of closed rows requiring text `id`, `name` and `period` and a
  settings row. A non-array, `[]` or a non-object row is `INVALID_MACRO_RUN` at `macroRuns` or
  `macroRuns[i]`; an unknown key, checked first, and a missing or non-text field are
  `INVALID_MACRO_RUN` at `macroRuns[i].<key>`; a missing `settings` is `INVALID_MACRO_SETTING`.

The ranges are the codec's own (`macroSettingsIssues` runs inside the read). Both readers now ask
`networkCodec::knownKey` against the same tables. Left to the model: a blank `period` or `id`,
and unique ids.

| Row | Check |
|---|---|
| MR1 | On the unchanged readers: the base parses from 51 (with runs from 56); both floors even null; `INVALID_MACRO_RUN` for null, `[]`, a number row, an unknown key before a missing id, a null name and a blank period; `INVALID_MACRO_SETTING` for a missing or null row, an unknown key, `targetGap` 1, `maxIterations` 2.5 and −4,294,967,295 |
| MR2 | Both keys `false` below their floors; one shared closed settings row (exclusive gap bounds, iteration bounds) and closed run rows with every key required |
| MR3 | Every case labelled refused is refused by `parseDocument`; the 1,557 labels, writer output with a second run at the limits, D566–D568's labels and all 18 projects (2 with Macro runs or settings) agree with the emitted schema (`jsonschema`, one-off) |
