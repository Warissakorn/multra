# Project schema — feature extensions

Optional feature rows of the composed partial schema (`multra-project-schema`), split from
[PROJECT_SCHEMA_DEMAND](PROJECT_SCHEMA_DEMAND.md) at D573 along whole extension sections.
[PROJECT_SCHEMA](PROJECT_SCHEMA.md) supplies envelope, presence and save-floor rules;
`parseDocument` and `validateDocument` stay authoritative and `x-multra-complete` stays false.

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

## Owned trip-rate set extension (D570)

The composed CLI coverage adds `-trip-rates`; complete remains false. The root `tripRateSets`
(M10.2 G2, `trip_rate_codec.cpp`) is refused below 66 when present, even null
(`EDIT_UNSUPPORTED_FIELD` at `tripRateSets`); the schema gives it `false`. Rows are those of
[TRIP_GENERATION §6.1](TRIP_GENERATION.md#61-a-trip-rate-set), every one closed:

- `tripRateSets` is a nonempty array of sets requiring text `id`, `name` and `unit`, a `source`
  object and a nonempty `purposes` array. `source` holds optional text `citation` and `table`.
- A purpose requires text `id` and the sides `productions` and `attractions`. A side is chosen by its
  `method`: `regression` requires a number `constant` and a `terms` array (empty allowed) of
  `{attribute, coefficient}`; any other value takes the cross-classification row, whose `method` must
  be `crossClassification`, with a nonempty `categories` array of `{attribute, rate}`, `rate` ≥ 0.
  A key of the other method is refused.
- Every refusal is `INVALID_TRIP_RATE_SET` at the row or key's path; an id is `INVALID_ID` or
  `DUPLICATE_ID`.

The reader asks `networkCodec::knownKey` against the same tables. Left to the codec: blank ids,
attributes and purpose ids, unique ids per list and unique attributes per side, and a catalog file's
required citation.

| Row | Check |
|---|---|
| TR1 | On the unchanged reader: the base parses from 66; the 66 floor even null; `INVALID_TRIP_RATE_SET` for null, `[]`, a number row, an unknown key at the set, source, purpose, category and term, a side with the other method's key, a negative rate and an unknown method; `DUPLICATE_ID` at `tripRateSets[1].id` |
| TR2 | `tripRateSets` `false` below 66; from it the nonempty array, the closed rows with their required keys, one row for both sides, the `method` branch and `rate` ≥ 0 |
| TR3 | Every case labelled refused is refused by `parseDocument`; the 1,559 labels, writer output (the two Ortúzar example sets), earlier labels and all 18 projects agree with the emitted schema (`jsonschema`, one-off) |

## Owned deterrence-set and cost-matrix extension (D571)

The composed CLI coverage adds `-deterrence-cost`; complete remains false. The root
`deterrenceSets` and `costMatrices` ([TRIP_DISTRIBUTION §5–§6](TRIP_DISTRIBUTION.md),
`deterrence_codec.cpp`, `cost_matrix_codec.cpp`) are refused below 67 when present, even null
(`EDIT_UNSUPPORTED_FIELD` at the root key); the schema gives each `false`.

- `deterrenceSets` is a nonempty array of closed sets requiring text `id` and `name`, a `source`,
  `function: "exponential"`, `costUnit: "min"` and a nonempty `purposes` array. Its closed `source`
  reuses the shared `citedSource` row (D570, renamed D574): optional text `citation` and `table`.
- Each closed purpose requires text `id` and numeric `beta` > 0 (`exclusiveMinimum: 0`). Optional
  `intrazonalShare` is numeric with 0 ≤ share < 1. `constraint` is `false` below 68; from 68 an
  enum of `double` and `origin`, adding `destination` from 72. The document's existing guards
  keep `EDIT_UNSUPPORTED_FIELD` at `deterrenceSets[i].purposes[k].constraint` before row decoding.
- `costMatrices` is a nonempty array of closed matrices requiring text `id` and `name`,
  `unit: "min"`, a nonempty `zoneIds` array of text and a nonempty `rows` array of arrays whose
  cells are numbers ≥ 0 or null. Optional `source` is closed with optional text `citation` only;
  unlike a deterrence source it refuses `table`.

Both readers now use `networkCodec::knownKey` against the same field tables. The deterrence
reader has no file version, so key membership uses the widest active constraint version, 72;
`document.cpp` retains its original version guards. Codes (`INVALID_DETERRENCE_SET`,
`INVALID_COST_MATRIX`, `INVALID_ID`, `DUPLICATE_ID`), paths and check order are unchanged.
Blank and unique ids, blank and unique zone ids, n×n matrix agreement and a catalog deterrence
set's required citation stay with the codecs. Known zone references remain a run check.

| Row | Check |
|---|---|
| DS1 | On the unchanged reader: the base parses from 67; both root floors even null, in their original order; constraint presence below 68 and destination below 72 precede row checks; allowed constraint values parse; unknown keys at set/source/purpose, beta 0, share 1 and duplicate set/purpose ids keep their codes and paths; simultaneous faults retain the complete issue order |
| DS2 | `deterrenceSets` `false` below 67; nonempty arrays, closed rows and required keys, source shared with trip rates, function/unit consts, exclusive beta/share bounds and the versioned constraint enum |
| DS3 | Every case labelled refused is refused by `parseDocument`; all 984 deterrence labels, 854 matrix labels, D570's 1,559 labels, three writer outputs (67, 68 and destination at 72, each with a null matrix cell) and all 18 TrafficSim projects under data/ and tests/ agree with the emitted schema (`jsonschema` 4.26.0, one-off: 3,418 labels, zero mismatches) |
| CM1 | On the unchanged reader: the base and its null cell parse from 67; the 67 floor even null; null/empty lists, unknown keys before missing fields, source table or non-text citation, blank/duplicate zone ids, row-count/row-width faults, a negative cell and duplicate matrix ids retain their codes and paths; simultaneous faults retain the complete issue order |
| CM2 | `costMatrices` `false` below 67; nonempty closed matrix rows, optional closed citation-only source, minute const, nonempty text zone ids and nonempty rows of arrays with nonnegative number-or-null cells; no uniqueItems or row-width constraint |
| CM3 | Every case labelled refused is refused by `parseDocument`; all 854 matrix labels and the writer/project coverage in DS3 agree with the emitted schema, including codec-only n×n, blank-id and duplicate-id refusals labelled schema-accepted |

## Period-factor extension (D572)

The composed CLI coverage adds `-period-factors`; complete remains false. Root `periodFactors`
([MODE_CHOICE §5–§6](MODE_CHOICE.md), `period_factor_codec.cpp`) is `false` below 69, preserving
`EDIT_UNSUPPORTED_FIELD` at `periodFactors` when present, even null. From 69 it is a nonempty
array of closed rows requiring text `purposeId` and `periodId` and numeric
`productionToAttraction` and `attractionToProduction`, each in [0, 1].

The reader replaces only its exact-key expression with `networkCodec::exactFields`, using the
shared four-required-field table at 69 (D567's pattern). `PERIOD_FACTOR_INVALID` stays at the row
before text checks, then each share, then the sum check. Blank text, sum ≤ 1, known periods and
unique purpose/period pairs remain codec checks. `parseDocument` calls `validateDocument` at its
end: `periodFactorIssues` retains `UNKNOWN_PERIOD` before `DUPLICATE_ID` in list order. Purpose
resolution remains a run check. Writer, feature-selected versions and serialized forms are unchanged.

| Row | Check |
|---|---|
| PF1 | On the unchanged reader: valid periods/factors from 69; presence floor even null; non-array/empty lists, non-object rows, missing/extra/replaced keys at the row first; blank text, bad share types/ranges, valid 0/1 and exact-sum-one boundaries, sum > 1, complete text/share/sum issue order and UNKNOWN_PERIOD/DUPLICATE_ID order |
| PF2 | Root false below 69, then a nonempty array of closed rows with all four keys required; plain strings and numbers in [0, 1], with no uniqueness, sum or reference-resolution schema rule |
| PF3 | All labelled structural refusals fail parseDocument; jsonschema 4.26.0 agrees on all 4,540 labels (1,104 PF cases, D570/D571 regressions, writer with 0/1 and exact-sum-one shares, all 18 TrafficSim projects, 20 independent probes). Probes: 13 both accept, 4 codec-only refusals, 3 both refuse; schema-refused/codec-accepted is empty across all labels. Temporary tests/registration deleted |

## Owned mode-choice set extension (D573)

The composed CLI coverage adds `-mode-choice`; complete remains false. Root `modeChoiceSets`
([MODE_CHOICE §6](MODE_CHOICE.md), `mode_choice_codec.cpp`) is false below 70, even null
(`EDIT_UNSUPPORTED_FIELD` at `modeChoiceSets`), then a nonempty array of closed set rows.

- A set requires text `id` and `name`, `source`, `costUnit: "min"` and nonempty `purposes`.
  Its closed source reuses the shared `citedSource` row: optional text `citation` and `table`.
- A closed purpose requires text `id` and nonempty `alternatives`; optional `nests` is false
  below 71, and optional `distributionCost` is false below 76, then const `logsum`.
  The document retains its presence guards at each purpose's field path, even null.
- A closed alternative requires text `id`, numeric `constant` and `timeCoefficient` ≤ 0.
  Optional `modeId` and `costMatrix` are text when present, and at least one must be present:
  an inclusive `anyOf` of required keys permits both. Optional `occupancy` is numeric and ≥ 1.
- Nests ([NESTED_LOGIT §3](NESTED_LOGIT.md)) are a nonempty array of closed rows requiring text
  `id`, numeric `phi` with 0 < phi ≤ 1, and a nonempty text array `alternatives`.

Only key membership moves to the shared tables through `knownKey` at 76: the reader has no
file version. Codes (`MODE_CHOICE_INVALID_SET`, `INVALID_ID`, `DUPLICATE_ID`), paths and issue
order stay unchanged, including the zero-reference check before nests and logsum after nests.
Blank and unique ids, a zero reference constant, nest members naming their purpose's own
alternatives, a member occurring at most once across nests, and one shared strictly negative
logsum coefficient ([MODE_CHOICE_LOGSUM §2](MODE_CHOICE_LOGSUM.md)) remain codec checks;
those cases are labelled schema-accepted. Catalog citation and run-time mode/matrix resolution
remain outside the schema. Writer bytes, save floors, catalog resolution and runs are unchanged.

| Row | Check |
|---|---|
| MCS1 | On unchanged readers: MNL from 70, nests from 71, logsum from 76; presence guards even null, with nests before logsum and guards before row checks; closed set/source/purpose/alternative/nest rows; either cost source or both; missing/invalid sources, numeric bounds, zero reference, duplicate ids, nest membership and logsum semantic refusals; a complete 17-issue sequence preserves all codes/paths/order |
| MCS2 | Root false below 70; closed rows and required keys, one shared source, nonempty lists and numeric bounds; inclusive anyOf rather than mutual exclusion; nests false below 71 and logsum false below 76; no blank/unique-id, zero-reference, membership or coefficient-agreement schema rule |
| MCS3 | Labelled structural refusals fail parseDocument; jsonschema 4.26.0 agrees on all 6,151 labels: 1,609 mode-choice cases, 4,501 D570–D572 cases, writer output at 70/71/76, all 18 TrafficSim projects under data/ and tests/, and 20 independent probes. Probes: 9 both accept, 7 codec-only refusals, 4 both refuse; schema-refused/codec-accepted is zero across all labels. Temporary tests/entries deleted |

## Review fixes (D574)

A code review of D570–D573 changed how the rows above are kept, not what they accept. The emitted
schema is byte-identical except that the shared `{citation, table}` row is named `citedSource`.

- The trip-rate, deterrence, cost-matrix and mode-choice readers check keys through one helper,
  `networkCodec::onlyKnownKeys`, at the latest schema; `document.cpp` keeps every floor and now
  names it with the same constants the field tables use.
- A Field attribute `atLeastOne` marks fields of which one must be present; the schema emits the
  `anyOf` in table order and `mode_choice_codec.cpp` asks `hasAtLeastOne` at the same point as before.
- ctest `schema-labels` (`tools/check_schema_labels.py`, registered when Python's `jsonschema` is
  installed, not under sanitizers) checks every slice's labelled cases and every shipped project
  against the emitted schema, so the schema half of the TR3, DS3/CM3, PF3 and MCS3 rows now runs
  in `check`. The envelope slice is excluded: its labels are for `projectEnvelopeSchema()` alone.

