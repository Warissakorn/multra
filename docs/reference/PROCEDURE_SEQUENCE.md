# Procedure sequence (MACRO_NETWORK §19 c)

Written 2026-11-06 as a contract (D483). The owner answered its four choices in the same session
(§6). P1 is implemented (D484, §8); P2 (the editor) follows (§7).

MACRO_NETWORK §19 booked option (c), "a general procedure sequence like Visum's", as an M10 seam
until generation, distribution and mode choice existed. They now exist:
- [TRIP_GENERATION](TRIP_GENERATION.md);
- [TRIP_DISTRIBUTION](TRIP_DISTRIBUTION.md);
- [MODE_CHOICE](MODE_CHOICE.md), [NESTED_LOGIT](NESTED_LOGIT.md).

Evidence: [macro-run-procedures](../evidence/macro-run-procedures.md) §1, from the 2026-10-19 web
study, where the help pages could only be read through the search summary.
- Visum keeps an ordered list of operations in the version file. Its generated four-step order is
  generation → distribution → mode choice → skim → assignment.
- A 2022 fault let a step that raised errors count as executed.

Rule 4 applies:
- Every output keeps the not-validated marker.
- A procedure adds no number of its own; it only chains existing runs.

## 1. Scope

- **In:**
  - named procedures stored in the project;
  - each holds an ordered list of steps of four kinds;
  - a run that chains the steps in memory;
  - a combined JSON output, the CLI, and an editor dialog and page.
- **Out:**
  - feedback loops, i.e. congested costs back into distribution (Ortúzar Ch. 11). This is the next
    slice, on top of this one.
  - logsum feedback (G/MS/D/A, Ortúzar §6.5.2);
  - skim or matrix operations as steps;
  - writing any result into the document.

## 2. Stored form (schema 73)

Root `procedures`, written only when non-empty:

```json
[ { "id": "base-am", "name": "Base, AM",
    "steps": [ { "kind": "tripGeneration", "rateSet": "…" },
               { "kind": "tripDistribution", "deterrenceSet": "…", "skimMode": "car" },
               { "kind": "modeChoice", "modeChoiceSet": "…", "periods": ["period-am"] },
               { "kind": "assignment", "period": "period-am" } ] } ]
```

| `kind` | Keys | Input |
|---|---|---|
| `tripGeneration` | `rateSet` | — |
| `tripDistribution` | `deterrenceSet`; optional `skimMode`, `costMatrix` | the nearest earlier active `tripGeneration` |
| `modeChoice` | `modeChoiceSet`; optional `periods` (non-empty list) | the nearest earlier active `tripDistribution` |
| `assignment` | exactly one of `period` (project settings) and `macroRun` (a D339 definition) | the document's `odMatrices`; the nearest earlier active `modeChoice`, if any, replaces the copy's generated matrices (§3) |

- **Every step** may carry `active: false`, written only when false.
- **Steps** may repeat in any order.
- **Shape faults** are refused at their path:

| Fault | Code |
|---|---|
| an unknown kind or key, a missing parameter, both `period` and `macroRun`, empty `steps` or `periods`, a non-string id | `INVALID_PROCEDURE` |
| a procedure id blank | `INVALID_ID` |
| a procedure id repeated | `DUPLICATE_ID` |

- **Run-time checks.** Set, matrix, mode, period and run ids are checked by a run, as D474 checks
  `costMatrices`, so a catalog can be edited.
- **Schema.** Below 73, `procedures` is `EDIT_UNSUPPORTED_FIELD`. A file without procedures keeps
  its bytes and schema.

## 3. Run

`runProcedure(document, data, id)`:

1. **Unknown id:** `PROCEDURE_UNKNOWN` at `procedures`.
2. **Each step** in order:
   - An inactive step is *skipped*.
   - An active step without its input (§2) is refused with `PROCEDURE_STEP_INPUT_MISSING` at
     `procedures[i].steps[k]`.
   - Otherwise it runs the existing unit:
     - `runTripGeneration`;
     - `runTripDistribution` on its input's result;
     - `runModeChoice` on its input's result;
     - `runMacro` or `runNamedMacro`.
   - Their refusal codes and paths are unchanged.
3. **A refused step stops every later step,** which is marked *not run*. Earlier steps stay *done*
   with their results. A step is never *done* with an issue (the Visum fault).
4. **The assignment demand.** The run copies the document, removes the matrices carrying
   `generatedBy: "mode-choice"`, and adds the generated vehicle matrices of the nearest earlier
   mode-choice step (`mc-<period>-<mode>`, as E2's *Write OD matrices* would). The assignment runs
   on that copy, and the document itself is never changed. With no earlier mode-choice step, the
   document's own matrices are used.
5. **Cancel** stops between steps and inside an assignment, as MACRO_NETWORK §19 a does. The
   cancelled step and the later ones are *not run*.

A run is no History step. Step statuses are `done`, `skipped`, `refused` and `notRun`.

## 4. Outputs

- **JSON:** the marker, `procedure {id, name}`, the revision, then
  `steps[{index, kind, status, issues, result}]`. Each `result` is the step's existing writer
  (`tripGenerationJson`, `tripDistributionJson`, `modeChoiceJson`, `macroJson`), or null.
- **CLI:** `--procedure ID` with `--project`. It combines with none of `--macro`, `--macro-run`,
  `--trip-generation`, `--trip-distribution` or `--mode-choice`. An unknown id is refused.
- **Editor (P2):**
  - *Procedures…*: a list of procedures, and for the selected one a steps table (kind, active,
    parameters chosen from the project and catalog), with Add/Remove/Up/Down. OK is one History
    step and Cancel changes nothing.
  - A *Procedure* Results page: choose, *Run* on a worker with Cancel, a table of step, kind,
    status and summary, the JSON export, and *Stale* after an edit.
  - The Macro page is not filled, since it shows one run per period.

## 5. Seams

- Feedback (next): an assignment's congested times as the next distribution's costs, iterated to a
  stopping rule.
- Writing a procedure's matrices stays E2's *Write OD matrices* on the Mode choice page.

## 6. Owner choices (answered 2026-11-06, D483)

1. **Shape.**
   - (a) A fixed generation → distribution → mode choice → assignment chain (recommended).
   - **(b) A general Visum-like list of steps (chosen).**
2. **Generated matrices.**
   - **(a) Used in memory; the document is unchanged (recommended, chosen).**
   - (b) Written into the document at each run.
3. **Storage.**
   - **(a) Named procedures in the project, run from the editor and the CLI (recommended,
     chosen).**
   - (b) Chosen each time in a dialog.
4. **Feedback.**
   - **(a) Not now, the next slice (recommended, chosen).**
   - (b) Now.

## 7. Slices and rows

| Slice | Content | Rows |
|---|---|---|
| P0 | This contract. **Done (D483)** | — |
| P1 | `procedures` at schema 73; `runProcedure`; `procedureJson`; `--procedure`. **Implemented (D484)** | PS1–PS9, PS11 |
| P2 | *Procedures…* and the *Procedure* page | PS10 |

Each row first asserts that its forcing worked.

| Row | Asserts |
|---|---|
| PS1 | Codec and schema 73: round trip; each fault of §2 at its path; no procedures keeps the old bytes and schema; < 73 refuses |
| PS2 | Chaining: the generation, distribution and mode-choice steps' JSON equal the units run alone with the same sets |
| PS3 | Assignment after mode choice equals *Write OD matrices* then `runMacro`, link by link; the document is unchanged |
| PS4 | Assignment with no earlier mode choice equals `runMacro` on the document; `macroRun` equals `runNamedMacro` |
| PS5 | A missing input is `PROCEDURE_STEP_INPUT_MISSING` at its path; a refused step stops the later ones; earlier ones stay *done* |
| PS6 | An inactive step is skipped, and its dependants take the nearest earlier active input or refuse |
| PS7 | Repeats: two assignments for two periods; with two mode-choice steps, an assignment uses the nearest |
| PS8 | The observer reports each step in order; Cancel inside an assignment stops every later step |
| PS9 | CLI: the marker, an unknown id, flag conflicts |
| PS10 | Editor: the dialog is one Undo step and Cancel keeps the bytes; the page runs with no History step and goes stale after an edit; saving writes 73 |
| PS11 | Shipped projects and every existing output byte-identical |

## 8. As implemented (P1, D484)

- **Files:**
  - `src/project/procedure_codec.*`: `Procedure`, `ProcedureStep`, parse, write and issues;
  - `src/project/procedure_run.*`: `runProcedure`, `ProcedureObserver` (a per-step callback that
    can cancel, plus the Macro observer);
  - `src/project/procedure_output.*`: `procedureJson`, with `completed` and `cancelled`;
  - `tools/four_step_cli.*` (`runProcedureProject`) and `--procedure` in `tools/run_simulation.cpp`.
    The CLI exits 1 when a step is refused and prints each issue on stderr.
- **Chaining.** A step runs its unit with its inputs' parameters: a distribution step uses the rate
  set of its generation step, and a mode-choice step uses those of its distribution step and that
  step's generation step. The units are deterministic, so each result equals the unit run alone
  (PS2).
  - This replaces the plan's chaining overloads. Those would have split the input manifests
    between steps and broken that equality.
  - Generation is computed again inside each later step; that costs nothing at study sizes.
- **Moved.** `generatedOdMatrices` moved from `src/commands/mode_choice_commands.*` to
  `src/project/mode_choice_run.*`, because the architecture check forbids project → commands. The
  in-memory copy needs no `ownModesWhenNamed`: PS3 equals the E2 write path, which does call it.
- **Manifests.** Each step gets its own copy of the CLI's manifest (the project file already
  recorded), so a step's `inputManifest` equals that unit's CLI run.
- **Fixture:** `tests/data/trip-distribution/procedure.traffic.json`: the mode-choice fixture at
  schema 73, A→P 0, and the procedure `four-step`.
- **Rows:**
  - `tests/procedure_tests.cpp` (group `procedure`) covers PS1–PS8;
  - `cli-procedure`, `-unknown` and `-conflict` cover PS9;
  - PS11: the existing suites pass unchanged.
- **Mutations:** each of the following failed a row:
  - continuing after a refusal;
  - taking the farthest input;
  - assigning the document instead of the copy;
  - ignoring `active`.
