# Study periods, modes and PCU (S1)

Written 2026-10-19 (D266) as a contract with five owner choices. The owner accepted every
recommendation the same day (D267, §9: 1a–5a). Slice S1.1 is implemented (D268, §13); S1.2 is not.
This is stage S1 of the multi-level workflow
([MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) §2, §10; D175): "StudyPeriod +
mode/PCU, a schema change with byte-identical schema-31 saves". It fills what §2 leaves open: the
study clock's form, how a Micro run meets a period, `profileId` and `macroFlowBasis`.

Rule 4 applies: nothing here is validated.

## 1. Scope

- **In S1:**
  - the StudyPeriod object;
  - an OD matrix naming a period;
  - the clock, and how a Micro run places a period on it;
  - the Mode catalog, and an OD matrix naming a mode;
  - the vehicle type's `pcu`, and a mode's derived PCU.
- **Out:**
  - every Macro use of these, which starts in M10.1. That includes refusing `MODE_PCU_MISSING`;
  - trip generation, CountSet and calibration (M10);
  - profiles within a period, unless owner choice 3 says otherwise;
  - zone kinds (`internal`/`gate`), which come with M10's boundary flows.

## 2. Today (measured from the code on D265's commit)

- **OD matrices.** `OdMatrix{id, startTime, endTime, compositionId, zoneIds, cells}` lives in
  `src/model/demand/zones.hpp`, with its codec at schema 31 in `src/project/zone_codec.cpp`.
  - `odDemandInputs` (`src/project/od_demand.cpp`) turns each cell into Poisson inputs over
    `[startTime, endTime]` in **run seconds**, at
    `cell / (end − start) · 3600` veh/h per origin–destination connector share.
  - `OD_PERIOD_INVALID` refuses a matrix that ends after the run's `duration`.
- **Clock.** The model has no clock time. Run time starts at 0 and ends at `duration`, plus any
  cool-down.
- **Catalogs.**
  - A composition (`data/compositions/`, or the project's own) is `{id, types[{vehicleTypeId,
    share}]}`.
  - A vehicle type (`data/vehicle-types/`) has no `pcu`.
  - `data/modes/` does not exist.

## 3. StudyPeriod (schema 49)

```json
"studyPeriods": [ { "id": "period-3", "name": "AM peak", "start": 25200, "end": 28800 } ]
```

- **Where it lives.** A new top-level array, written only when one exists. Every other file keeps
  its bytes and schema.
  - Below schema 49 the key is refused with `EDIT_UNSUPPORTED_FIELD studyPeriods`.
  - An unknown field, or an empty array, is refused on load (D19a).
- **Fields:**
  - `id` comes from `allocateId(d, "period")` and is in `documentIds`;
  - `name` is free text and never a key;
  - `start` and `end` are **clock seconds since midnight** (owner choice 1), finite, with
    `0 ≤ start < end ≤ 172800`. The upper bound of two days lets a period run past midnight.
- **Refused:** `PERIOD_INVALID` at the field's path, when a value is not finite or the order is
  wrong.
- **Periods may overlap.** Two matrices in overlapping periods add, as overlapping inputs do today.
- `profileId` and `macroFlowBasis` are not stored in S1 (owner choices 3, 4).

## 4. The clock and the Micro run

- **`definition.clockStart`** is the clock time of run time 0, in seconds since midnight (owner
  choice 2). It is absent = 0, and written only when not 0.
- **A matrix naming a period** expands over run times
  `[period.start − clockStart, period.end − clockStart]`.
  - A matrix that names no period keeps today's run-second `startTime`/`endTime`. That keeps
    schema-31 files meaning what they meant.
  - A placed period that starts before 0 or ends after `duration` is `OD_PERIOD_INVALID`, the same
    code as today. The editor's run diagnostics name the matrix.
- **Results** stay in run seconds. The clock appears only where the editor labels a time; nothing
  in an output changes.

## 5. The OD matrix (changed)

| Field | Rule |
|---|---|
| `startTime`, `endTime` | As in ZONES_AND_PARKING §2.3, **or** absent when `periodId` is set |
| `periodId` (new) | An existing StudyPeriod. Exactly one of `periodId` or the pair `startTime` + `endTime` |
| `compositionId` | As today, **or** absent when `modeId` is set |
| `modeId` (new) | An existing Mode. Exactly one of `compositionId` or `modeId` |

- **Refused on load:**
  - `OD_PERIOD_AMBIGUOUS` when both forms of the period are given, or neither;
  - `OD_TYPES_AMBIGUOUS` when both type sources are given, or neither;
  - `UNKNOWN_PERIOD` and `UNKNOWN_MODE` for an id that does not exist.

  Each is reported at the field's path.
- **Byte-identical schema-31 saves.** A schema-31 matrix has times and a composition. It loads into
  the same fields and saves to the same bytes at schema 31. A file reaches schema 49 only when it
  holds a period, a `periodId`, a `modeId` or a `clockStart`. Nothing is migrated, so no
  "implicit period" object is ever made or written.
- **Deleting.** A period or mode that a matrix names cannot be deleted (`EDIT_PERIOD_IN_USE`,
  `EDIT_MODE_IN_USE`), as zones already work (`EDIT_ZONE_IN_USE`).

## 6. Mode (catalog, `data/modes/`)

```json
{ "id": "car", "name": "Car", "compositionId": "car-only" }
```

- **A catalog,** like compositions: shipped files plus the project's own, if M8's project-catalog
  rule extends to it. The S1.2 contract row decides that against `resolveCatalogs`.
- **Micro meaning.** A matrix with `modeId` draws its vehicle types from the mode's composition. It
  therefore runs **byte-identical** to the same matrix naming that composition.
- **Refused:**
  - `UNKNOWN_MODE`;
  - `MODE_COMPOSITION_UNKNOWN` at the mode file;
  - `EDIT_CATALOG_READ` for a malformed file.
- **Shipped.** S1.2 ships `car` (`car-only`) and `mixed` (`urban-mixed`), so the existing
  compositions get mode names. No PCU comes with them (§7).

## 7. PCU

- **The vehicle type** gets an optional field `"pcu": {"value": 1.0, "source": "…"}`.
  - The value must be finite and > 0, and the source a non-empty citation.
  - It is refused with `INVALID_PCU` at its path otherwise.
  - Absent means unknown; there is **no default** (§2 of the workflow).
- **A mode's PCU** is derived and never stored: the share-weighted mean over its composition's
  types, Σ shareᵢ · pcuᵢ / Σ shareᵢ.
  - It is absent when any type of the composition has no `pcu`.
  - The workflow's "time-weighted PCU over the matrix period" is this same number while demand is
    uniform within a period (choice 3a).
- **Values** (owner choice 5). Rule 5 forbids an uncited number. WORKFLOW §7 leaves the Thai source
  "to be chosen by an expert", and motorcycles are flagged uncertain (Powell 2000: 0 in the first
  6 s of green, about 0.53–0.65 later). S1 ships **no value** unless the owner names a source.
- **Readers.** Micro never reads PCU. The editor's mode list shows the derived value, or "—".
  Macro's `MODE_PCU_MISSING` is M10.1's row.

## 8. Editor

- **Study periods table** (Demand group):
  - id, name, start, end, shown as HH:MM:SS, with add, edit, rename and delete, each one Undo step;
  - deleting a period in use is refused, and the matrix is named.
- **Run settings** gains a `clockStart` field (HH:MM:SS).
- **The OD matrix dialog:**
  - a Period choice, where "own times" keeps today's two fields;
  - a Types choice between Composition and Mode, each a list.
- **Mode list** (S1.2): read-only, showing id, name, composition and derived PCU.

## 9. Owner choices (answered 2026-10-19, D267: every recommendation, 1a–5a)

1. **Clock form in the file.**
   - **(a) Seconds since midnight, shown as HH:MM:SS (recommended).** Every time in the file is
     already a number of seconds, and no parser or time-zone rule is added.
   - (b) "HH:MM:SS" strings in the file.
2. **How a Micro run meets a period.**
   - **(a) `clockStart` on the run; each matrix placed by its own period (recommended).** One run
     can hold several periods, such as a shoulder and a peak, and warm-up stays the evaluation
     period's job (M5.3).
   - (b) The run names one `periodId` and starts at the period's start minus the warm-up. That is
     simpler to explain, but it allows one period per run.
3. **Profile within a period (`profileId`).**
   - **(a) Not in S1 (recommended).** Demand stays uniform within a period, as matrices are today,
     until a consumer and a cited profile exist. Vehicle inputs already carry their own volume intervals (DEMAND_TIME_TYPES).
   - (b) 15-minute shares on the period now.
4. **`macroFlowBasis`.**
   - **(a) Not stored until M10.1 reads it (recommended).** Absent means `mean`, D175's answer,
     and no field exists before its reader (as NODES §5).
   - (b) Stored on the period now.
5. **PCU values.**
   - **(a) The field and the derivation now, with no shipped value until the owner or an expert
     names the Thai source (recommended).**
   - (b) Ship values now from a source the owner names (title, edition, table).

## 10. Slices

| Slice | Content | Rows |
|---|---|---|
| S1.1 | StudyPeriod, `clockStart`, the matrix `periodId`, codec at 49, validation, Study periods table, OD dialog Period choice. **Implemented (D268)** | SP1–SP8 |
| S1.2 | Mode catalog, the matrix `modeId`, `pcu` on vehicle types, derived mode PCU, mode list | SP9–SP14 |

## 11. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| SP1 | Shipped projects and a schema-31 OD file | Saved bytes and schema unchanged; no new key |
| SP2 | One period | Schema 49 round-trips; 48 carrying `studyPeriods` refused at its path; `PERIOD_INVALID` at each bad field |
| SP3 | A matrix with `periodId` | Expands to inputs equal, byte for byte, to the same matrix with run-second times `[start − clockStart, end − clockStart]` |
| SP4 | `clockStart` | Shifts the expansion by its value; a placed period outside `[0, duration]` is `OD_PERIOD_INVALID` |
| SP5 | Both or neither period forms | `OD_PERIOD_AMBIGUOUS` at the matrix path; `UNKNOWN_PERIOD` at `periodId` |
| SP6 | Deleting a period in use | `EDIT_PERIOD_IN_USE`; the document unchanged |
| SP7 | Study periods table | Add, edit, rename and delete, each one Undo step; HH:MM:SS shown; `clockStart` in Run settings as one step |
| SP8 | Single-run and batch outputs of a period project | Equal to the run-second twin's outputs |
| SP9 | A matrix with `modeId` | Runs byte-identical to the same matrix with the mode's composition |
| SP10 | Both or neither type sources; an unknown mode | `OD_TYPES_AMBIGUOUS`; `UNKNOWN_MODE` |
| SP11 | A mode file naming an unknown composition | `MODE_COMPOSITION_UNKNOWN` at the file |
| SP12 | `pcu` | Round-trips; non-finite, ≤ 0 or no source is `INVALID_PCU` at its path |
| SP13 | Mode PCU | Equals the hand-computed share-weighted mean; absent when any type lacks `pcu` |
| SP14 | Deleting a mode in use; the mode list | `EDIT_MODE_IN_USE`; the list shows the derived value or "—" |

## 12. Seams

- **ZONES_AND_PARKING §2.3** stays the matrix contract for times and compositions. This document
  adds the period and mode forms.
- **M10.1** is the first reader of a mode's PCU, and of `macroFlowBasis` if choice 4a stands.
- **The run-time cut (S5)** places boundary flows by the same clock.

## 13. S1.1 as implemented (D268)

- **Model.**
  - `StudyPeriod{id, name, start, end}` and `ZoneLayer::studyPeriods` are in
    `src/model/demand/zones.hpp`, together with `OdMatrix::periodId` (empty = own times).
  - `AuthoringDefinition::clockStart` is in `definition.hpp`.
- **File.** `src/project/study_period_codec.*` holds `usesStudyPeriods`, `rejectStudyPeriodsBefore49`
  and the array and `clockStart` codecs.
  - `parseMatrix` (`zone_codec.cpp`) takes either key set, and refuses both forms or neither as
    `OD_PERIOD_AMBIGUOUS` at `odMatrices[i]`.
  - A matrix with a period is written without times.
  - A file without any of these keeps schema 31 (or lower) and its bytes.
- **Checks:**
  - `zoneLayerIssues` adds `PERIOD_INVALID` (at `.id`, `.start`, `.end`) and `UNKNOWN_PERIOD`.
  - The document check refuses a `clockStart` outside [0, 172800] with `INVALID_NUMBER` at
    `definition.clockStart`.
  - Run refuses a placed window outside `[0, duration]` with `OD_PERIOD_INVALID`.
- **Expansion.** `odMatrixWindow` (`od_demand.*`) gives the matrix's own times, or its period less
  `clockStart`. `withOdDemand` and `odDemandIssues` read it, so a placed matrix expands exactly
  like its run-second twin.
- **Commands.** `putStudyPeriod` (prefix `period`) and `deleteStudyPeriod`, which refuses with
  `EDIT_PERIOD_IN_USE`.
- **Editor:**
  - **Study periods tab** (`editor_periods.cpp`, Demand group): add, edit and delete, each one
    Undo step.
    - The dialog takes the clock in `HH:mm:ss`, and an end at or before the start is the next day.
    - A start after midnight of the next day cannot be typed in the dialog. The file allows it.
  - **Run settings:** a `clockStart` field, `editorClockStart`. Left untouched, it keeps a stored
    value past midnight.
  - **OD dialog:** a Study period choice, `editorOdStudyPeriod`. Choosing a period disables the
    own times, and the OD table shows the period's name.
- **Rows.** SP1–SP6 and SP8 are in `tests/study_period_tests.cpp` (ctest `studyperiods`), and SP7 is
  in `tests/study_period_ui_tests.cpp` (ctest `studyperiod-ui`). Thirteen codec rows' "future
  schema" example moves to 50.
- **Mutations caught:**
  - the window ignoring `clockStart`;
  - no schema gate;
  - periods written when empty;
  - an in-use delete allowed;
  - ambiguity allowed;
  - an unknown period accepted.
