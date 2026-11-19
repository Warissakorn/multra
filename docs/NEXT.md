# NEXT — what a session does first

The single live to-do, **open items only**: what shipped is in [PROGRESS](PROGRESS.md), the
[archive](archive/README.md) and the [decision record](decisions/RECORD.md); gates are in
[ROADMAP](ROADMAP.md); owner items only in [OWNER_SITTING](plans/OWNER_SITTING.md). Rewrite this file
each session ([AGENTS](../AGENTS.md#session-end)); never append a finished slice here (D319). The
previous full list is [archived](archive/NEXT-state-2026-10-23.md).

## Standing facts

- **Windows CI builds without precompiled headers.** After adding an include of a header that uses a type whole
  (e.g. `Json` past `json_fwd.hpp`), run `python3 tools/check_no_pch.py build/desktop` (about 9 min on 8 jobs).
- **Books:** five owner copies (Sheffi, HCM 7, Ortúzar & Willumsen, Planning Handbook 4th ed.,
  Yperman); repositories, page rules, caveats and the chapter map by milestone are in
  [BOOKS](reference/BOOKS.md). Attach with `add_repo`, never vendor; cite the book, read the image
  before using a number. Checks of 2026-10-23: [book-copies-check](evidence/book-copies-check.md).
- **Before an engine slice:** keep `src/core/` files at 500 lines or fewer (2026-11-13: largest `types.hpp` 469,
  `lanes.cpp` 422, `validate.cpp` 418); split along the responsibility a change touches first (D319). The owner deferred splitting `types.hpp` on its
  own: estimated 466 → 420 objects rebuilt after a runtime-state edit, 338 with `eventJson`,
  `checkpointJson` and `summaryJson` moved out of `project/json.hpp`. Do it with the first engine
  slice that edits `Vehicle` or `SimState`, and measure the rebuild before and after.
- If `map-ui` fails in a parallel `check`, capture its output before re-running.
- **M0 is closed** by owner ruling (D369, [evidence](evidence/m0-plausibility.md)). Bare M0
  scenarios still hold amber as red (D147); a change that moves the M0 baseline reopens it.
- **Shipped projects (D337, D492):** all stay; [their README](../data/projects/README.md) names each
  one's purpose and builder. `macro-four-step` (D492) is the Macro example: run
  `--procedure four-step`; regenerate with `multra-macro-example data <file>` after any change to
  the four-step codecs' written form (owner's look: OWNER_SITTING B46). `four-leg-signalised`
  carries a 300 s cool-down (schema 27): a test that needs the bare drawing clears it with
  `changeEvaluationPeriod(d, 0, std::nullopt, 0)`.

## M3.2.7d attempt 1 failed at step 1 (2026-11-18) — M3.2.4e comes before attempt 2

- **2026-11-19, STOP_LINES done** (D594–D595): default lines, `frontStopDistance`, schema 79, the Stop
  line tool (`W`), Waits at and Use default lines in the tab.
- **2026-11-19, line approach done** (D596, [LINE_APPROACH](reference/LINE_APPROACH.md) §9): a held vehicle
  rests with its front at a zone's line, no crawl, no go-then-brake. **Next: the owner's attempt 2 of
  M3.2.7d on Windows** (lines, the Stop line tool, the stop at the line).
- **2026-11-19, later:** owner: areas 100 % opaque (D592) and start M3.2.4e. Done: [MERGE_MOUTH](reference/MERGE_MOUTH.md)
  (D593), the T-junction regenerated with its merge lines 1 m short of the measured overlap. **Next: the owner's
  attempt 2 of M3.2.7d on Windows** (step 1 look); then STOP_LINES slice 1b (derived lines, `frontStopDistance`)
  and the line-approach contract.
- **2026-11-19:** the owner chose derived stop positions and a Stop line tool; the draft is
  [STOP_LINES](reference/STOP_LINES.md) (D591, rows SL1–SL8). **Owner reviews §2–§6, then slice 1 =
  M3.2.4e + derived lines.** Done the same day: D587 (row dialogs once), D588 (no white Connector
  edge), D589 (intermediate points: red border, rounding, Connect dialog), D590 (areas only under
  their tool).

- The owner found the merge waiting lines inside the lane being joined ([OWNER_SITTING](plans/OWNER_SITTING.md)
  A2, with the measured positions). The fix is M3.2.4e (ROADMAP): its own admission contract and
  failure-first clearance rows ([ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md)) before the D50/D59
  solver changes, then regenerate the fixture and its archived sweep metadata deliberately.
- **Steps 3–4 (2026-11-19):** gap/headway response plausible; three findings with measurements in OWNER_SITTING A2:
  - **Ask:** conflict washes too faint (D435 alpha 80/95 of 255). Shown: 180/200 and 255 (100 % hides the
    road and the waiting lines under a band). Amending D435 needs the owner's choice.
  - **Line approach (engine, its own contract like LINE_APPROACH):** a held minor crawls ~20 s and stops 2–3 m
    short; release is 2.5 m/s² at once and re-blocking brakes it again. Options to measure: a kinematic stop
    at the line in `approaching`/`following` (scoped like F1a), zero standstill distance to a line, and a
    go-decision hysteresis. Measure before choosing (D254's pattern).
  - **Double click:** fixed by D587 (a row's dialog opened twice); the white Connector edge by D588. The
    owner's look on Windows confirms both.
- **Ask, before attempt 2:** the owner asked to test only what needs the owner. Proposed: keep steps 1
  (look), 3–4 (gap/headway, Stop/Yield, engineer's plausibility) and the verdict; steps 2 and 6–7 are
  covered by `rightofway` round-trip and same-seed replay tests. Changing §3 needs the owner's yes and a D-row.

## Counted study Ratchadaphisek × Lat Phrao (D575–D578) — a failing case; wait for B50

- [studies/ratchada-latphrao](../studies/ratchada-latphrao/README.md): counts from the owner's sheet,
  lanes and right-turn lanes from the owner's drawing (D577), a 155 s Webster plan. Regenerate with
  `multra-count-study` after any change to `counts.csv` or `site.csv`.
- **Purpose review (owner, 2026-11-17):** the study is not tuned further for throughput. That would
  be calibration in all but name (D544), and the M2.6/M5.7 purpose is the owner building a study in
  the editor and recording friction, which this builder-made project is not evidence of. It stays
  as the failing case of the lane-lock defect.
- **Lane locks done (D578–D582, ROADMAP M3.2.11; [LANE_LOCKS](reference/LANE_LOCKS.md)); last clamp
  fixed (D583).** C (zipper) goes with M3.4. **Owner:** the two new editor fields (`entryLanes`,
  `missedExitAfter`) were compiled but never opened.
- Gates: none closed by this study. A3/B4 stay the owner's.

## Owner checks waiting from recent sessions

All in [OWNER_SITTING](plans/OWNER_SITTING.md) (rule 3; Linux tests close none of them): B45 (UI
review), B46 (Macro example), C14 (Run view after D463/D487), C15 (run vehicles, slice C), C16
(random streams), E35 (tables like Excel), E36 (Route tool and marks), item 42 (M10 pages).

**Review waiting (code, not a gate):** D570–D574 on branch `claude/optimistic-bohr-lpfu98`: the
Macro-set schema rows ([PROJECT_SCHEMA_FEATURES](reference/PROJECT_SCHEMA_FEATURES.md)) and D574,
the code-review fixes to them (named floors, `onlyKnownKeys`, `citedSource`, `atLeastOne`, ctest
`schema-labels` and `python3-jsonschema` on Linux CI). Verified on Linux headless (`check` 240/240)
and the changed units without the PCH; the desktop preset and Windows were not run, and the CI
change has not run on GitHub yet. Nothing is merged. Stop for owner review before `procedures`.

## Review of 2026-11-10 — queued proposals (D493)

The first review's proposals are done (D493, [PROGRESS](PROGRESS.md)). The rest wait here in review
order; none is started. **Ask** marks one the owner chooses before any code.

- **Owner only, from the purpose review:** the M5.7 rehearsal (OWNER_SITTING A3) and a second user
  on the Micro sentence. Answered (D505): the M6 benchmark sources ([ROADMAP](ROADMAP.md) M6.0);
  Micro's M6 gate may close before M10 and M11.
- **Architecture** (includes checked per module since D494):
  - `EditorWindow` split done (D514–D524). The metadata-driven object table (D525,
    [OBJECT_TABLE](plans/OBJECT_TABLE.md)) is piloted on Study periods; the other tabs wait for the
    owner's verdict (OWNER_SITTING B48). `src/project/` (codecs, runs, outputs; `src/report/` is
    reserved) waits until report work starts.
  - ARCHITECTURE has the dependency table; its *Editor boundary* prose (about 130 lines of
    per-decision notes) still belongs in the contracts.
  - `shell` is not checked: it may include everything, so there is nothing to refuse.
- **Engine:**
  - Random streams per input (D506–D512, D526): slice 4's cost was not measured with callgrind
    (D512). Shipped projects stay `shared` until a recorded decision regenerates them.
  - **Left from D495:** phase 1's per-vehicle loop in `stepSimulation`, about 150 lines, is still
    one body; split it only with a callgrind run before and after (it is the hot path).
  - **M6 analytic rows against HCM:** every kind has its first run (D535–D543, evidence linked from
    [m6-benchmark-options](evidence/m6-benchmark-options.md)); the misses stand as known deviations
    without calibration (D544, which also settles D537's question of changing `L_hj`, `t_x`, `v_S`); both sweeps are done and nothing was adopted (D545, D546).
    **Next on M6:** nothing until independent data (R7 inD, Q4 (d)); the gate stays open.
    - **Ask** (D535): is S1's gate S1-a alone, or S1-f and S1-d too?
    - D530 (the S1 diagnostic) and D531 (median and skewness) each need a measurement contract before use.
- **UI** (seen on Linux at 1440 × 900, [the review's steps](PROGRESS.md)):
  - The owner's look at D497–D504 is OWNER_SITTING B45.
  - The Node analysis example uses the pocket template (D513).
  - Not wanted (D505): palette names at 1366 px. No start page (D505): File already has recent files and examples. Reopen only if a real
    user cannot find them.
- **Process:**
  - **Owner, on GitHub:** require `Native C++` before merging to main (PRs 210 and 213 merged
    seconds after opening).
  - Left from D502: milestone status statements ("M10.2 implemented") are still not checked against code;
    the user guide has no Thai edition and no pictures.

## Tables like Excel (D490) — done; owner's look is E35

- Contract: [EDITOR_WORKFLOW](reference/EDITOR_WORKFLOW.md).
- **Left from the compactness review:** the Results tab strip still scrolls at 1024 in Thai, and
  the clamp note has no not-validated statement of its own (both predate D490).
- **Not chosen, ask before doing:** sort by header click, and cell selection in one-object lists
  (each list's `selectRow()` callers would need rewriting first).

## Optimisation pass of 2026-11-07 (D487–D489) — follow-ups only

- Owner: C14. A `run-presentation-ui` failure on Windows CI only is a Windows LayoutRequest
  source, not a flaky test.
- The next CI run: read `linux (desktop)`'s `ccache -s`; hits should rise after D489. If not, the
  cause is keys or sloppiness, not size.
- The engine's next lever is the per-tick containers (spans, refs, events), not single copies
  ([ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md)); measure with callgrind first. D584 reused
  insertion's snapshot (oversaturated study −6.6 % Ir); what is left there is listed in the notes
  (older spread costs, not booked: `closestVehicle` 8 %, per-tick `Vehicle` moves 6 %, D283).
  Owner check: MSVC Release wall time of the study.

## Canvas style — slices A–C done; stop for owner review

- [CANVAS_STYLE](plans/CANVAS_STYLE.md) holds the grill answers and slices A–E; A–C and their
  fixes are D423–D426 and D460–D466. The junction median spikes stay (Q5).
- **Stop for owner review:** C15 (slice C) and E33/E34 (D427–D432,
  [remediation evidence](evidence/ui-audit-fixes.md)). Slice D (lane arrows, white stop line)
  starts only on the owner's go.

## M13 — A9 implemented; stop for owner review

- **Stop here; do not start another slice.** The owner accepted checkpoints `a6d0a25` and
  `d663bf5` so far, including shipped demand totals, schema-64 reopen, exact Undo/source
  clearing and unchanged old outputs. B41 is answered: explicit per-Node session-only manual
  count window, matching batch evaluation start/end and Node T, labelled manual counts; no
  stored field. D422 finishes that path and NA24–NA30; A9 now awaits the owner's final review.
- **If the owner requests changes:** keep them in A9. Read [NODE_ANALYSIS_MODEL](reference/NODE_ANALYSIS_MODEL.md),
  [NODE_ANALYSIS_EDITOR](reference/NODE_ANALYSIS_EDITOR.md), NODE_ANALYSIS §4–§6/§12,
  D383/D418–D422 by anchor, and BATCH/NODES/STUDY_PERIODS for reused evaluation/clock rules.
  All B40/B41 choices are answered; no further contract question is pending.
- **Preserve:** current saved/Save-representation capture and exact analytical CLI JSON/CSV;
  staged explicit model window/HV choices, T=window hours/PHF=1, whole-set preview and one Undo;
  historical source fields at feature-selected 64 with old-file/version-path handling. Demand
  edits clear the claim and Undo restores it; site-only edits retain it. Schema 65 is unused.
- **Manual comparison:** Confirm manual count window… selects one Node and shows its effective
  T/batch window before Confirm. Validate through the same comparison function; missing,
  non-finite/reversed/mismatched windows or T refuse. No inference from duration, name matching,
  History edit, stored confirmation or implicit run. Re-Analyse, batch replacement/cancel,
  edits/Undo and New/reopen clear confirmations. Node/approach source labels and CSV
  countsSource distinguish manual counts/model demand; analytical export format stays unchanged.
- **Verification after any correction:** desktop build/ctest/check, conditional headless, real
  CLI parity and old-output byte checks; both driving sides/locales, then commit/push the same
  branch and stop again. Keep Window header ≤500 lines. Windows/high-DPI/manual appearance is
  [OWNER_SITTING E32](plans/OWNER_SITTING.md), with E31's cleared-approach carry-over fixed.
- After A9 approval, wait for the owner's next pick. A4/timing/coarse HCM are unselected. No
  M13/M14 milestone gate closes merely from these Linux tests or implementation review.

## M14 — junction slices complete; UTDF U1, U2a and U2b shipped

- SR, J1–J7, J6a and J4T are approved; the milestone stays open for UTDF's gates. Preserve
  [JUNCTION_TEMPLATES](reference/JUNCTION_TEMPLATES.md), D388–D418 and their passing rows,
  including natural polygon fitting, ownership, site drafts and corridor/reference guards.
- **UTDF export:** U1, U2a and U2b are done (D442, D443; `--export-utdf` and *File → UTDF for
  Synchro/Vistro…*); contracts and rows UT0–UT9, UT11 in [UTDF_EXPORT §7](reference/UTDF_EXPORT.md#7-slices).
- **Owner (OWNER_SITTING I.5, UT10):** import `tests/data/utdf/u2-u0-junction-geometric.csv` into
  Vistro 2026 and report:
  - whether geometry, volumes and timing match UTDF_PROBE §1–§3;
  - whether the empty `[Network]` values are accepted.
- **Later:** the Vistro median patch, once Vistro's post-import link numbering is confirmed. Until
  then it is a report step (D441, D443).
- All-way-stop generation stays listed but disabled until A5/U1 needs it (D399); StopControl
  is unchanged. No timing defaults: all site timing is required. Posted speed is never a cap
  (D381). Macro/Meso freeSpeed and the drawing's HCM inputs remain independent of recipes.
- Windows appearance remains OWNER_SITTING E24–E32; Linux tests do not close those items.
  Watch B29's turn keys, HCM storage and partial-total questions; do not silently settle them.

## Object interchange follow-up (M1.23.1–M1.23.2, D365–D367)

- Owner selected native object-table CSV export over the schema queue; packaged Windows checks are
  [OWNER_SITTING G](plans/OWNER_SITTING_WINDOWS.md#g--object-interchange). Read [OBJECT_TABLE_CSV](reference/OBJECT_TABLE_CSV.md)
  for columns/units and nested JSON conventions before extending interchange. GeoJSON,
  imports/CRS mapping, recent-file/document workflow and measured rendering retain their scope.
- Read [NETWORK_IMAGE](reference/NETWORK_IMAGE.md) before changing bounds or rendering. The
  remaining M1.23 formats/workflow/performance work still needs its own bounded slices.

## AI automation follow-up (D344–D350, D558–D573)

- An authoritative formal schema covering every optional/versioned field remains separate from D350
  observed-shape discovery plus codec dry validation. Root presence rules, codec owners and save
  floors: [PROJECT_SCHEMA](reference/PROJECT_SCHEMA.md); definition demand rows: [PROJECT_SCHEMA_DEMAND](reference/PROJECT_SCHEMA_DEMAND.md);
  optional feature rows: [PROJECT_SCHEMA_FEATURES](reference/PROJECT_SCHEMA_FEATURES.md).
- **Mapped so far (D558–D573):** compositions, modes, vehicle types, behaviours (item 49 answered,
  D562), vehicle classes and link behaviour types, the zone layer, study periods and area, the map,
  Macro settings and runs, trip rates, deterrence sets and cost matrices, period factors, mode
  choice. Rows: [PROJECT_SCHEMA_DEMAND](reference/PROJECT_SCHEMA_DEMAND.md) and
  [PROJECT_SCHEMA_FEATURES](reference/PROJECT_SCHEMA_FEATURES.md).
- **Following slice (`procedures`, schema 73, feedback from 75), after owner review of D573–D574.**
  Stop before starting it. Read D571–D574's diffs and copy the pins-first pattern as D574 left it:
  `networkCodec::onlyKnownKeys` for a reader with no version, named floor constants (add
  `procedureVersion=73`, `feedbackVersion=75` and use them in `document.cpp`'s guards and save
  chain), `tests/schema_refusal.hpp` for refusal pins, and the shared `citedSource` row if a source
  appears. Contracts: PROCEDURE_SEQUENCE §2 and
  PROCEDURE_FEEDBACK §2; inventory the implemented reader rather than inferring from the writer.
  1. **Inventory (read, change nothing):** procedure_codec.cpp's parseProcedures, step,
     feedback, feedbackIssues and procedureIssues; document.cpp's guards and save selection.
     Root procedures is refused below 73 at its path, even null. Feedback's pre-75 guard runs
     AFTER parseProcedures: malformed feedback can fail INVALID_PROCEDURE before the guard;
     valid feedback below 75 gives EDIT_UNSUPPORTED_FIELD at procedures[i].feedback. Keep this
     order. Other codes are INVALID_ID and DUPLICATE_ID. Inspect kKinds/procedureStepKindName.
  2. **Pins first (PRS1):** tests/procedure_schema_tests.cpp and _cases.hpp, registered in CMake,
     built/passing on unchanged readers before metadata/schema/codec edits. Pin kind/type/key
     paths, active checked before keys, missing ids, assignment's exclusive presence, feedback
     methods/bounds/native integer tokens and complete issue order, including post-parse guards.
  3. **Rows as read:** root false below 73, then a nonempty array of closed rows requiring text
     id/name and nonempty steps; feedback false below 75. Steps are four closed rows tagged by
     required kind; reuse the existing kind formatter for tag strings. Optional active is boolean
     (true is accepted even though only false is written). Generation requires text rateSet;
     distribution requires text deterrenceSet, with optional text skimMode/costMatrix; modeChoice
     requires text modeChoiceSet, with optional nonempty text periods; assignment needs exactly
     one of text period or macroRun (exclusive presence, unlike D573's inclusive sources).
     Feedback is closed {method, weight?, maxIterations, tolerance}: method msa or constantWeight;
     constantWeight requires numeric weight in [0, 1), msa forbids weight; maxIterations is a
     native integer token in [2, 1000], tolerance numeric ≥ 0. Blank/unique ids remain codec
     checks; dependency/known-reference/run rules remain with the existing run. No step-order
     restriction or uniqueness inferred from example procedures.
  4. **One key source:** replace only onlyKeys lists with shared field tables through
     `onlyKnownKeys` (reader has no version; it checks at kLatestSchema). Keep kind-before-active, active-before-keys, field order,
     int64 conversion/clamping, feedbackIssues and the document's post-parse feedback guard.
     Do not change writer bytes, save floors, step formatting, catalogs or runs.
  5. **Coverage:** add -procedures to the value and update $comment, C++/CLI pins, help, tools
     README and DESKTOP_AUTOMATION's sentence; keep x-multra-complete false.
  6. **Labels in `check` (D574):** add the new cases namespace to
     `tests/schema_labels_dump_tests.cpp`; ctest `schema-labels` then checks every label and every
     shipped project against the emitted schema. Still by hand (temporary, then deleted): writer
     output at 73 (all step kinds, both assignment forms, active false) and 75 (MSA and
     constantWeight, numeric boundaries), and about 20 probes outside cases(). Native-token floats
     such as 2.0 may be labelled schema-accepted/codec-refused; schema-refused/codec-accepted must
     be empty.
  - **Pitfalls:** filter by group (multra-tests project), not test name; optional-only closed rows
    emit required: []; parseDocument calls validateDocument. The feedback floor is post-parse;
    do not move it earlier. Use PROJECT_SCHEMA_FEATURES for the new section and fix moved-section
    links if splitting again. Keep every file ≤500 lines; network_fields.cpp is 433 at D573.
    Headless is sufficient; report desktop/no-PCH/Windows honestly. The local linker-only
    -Wl,--strip-debug cache flag fits the small disk; repository presets remain unchanged.
  - **Session end:** headless build and cmake --build build/headless --target check green;
    append the next decision ID and index entry; PROJECT_SCHEMA_FEATURES PRS1–PRS3,
    PROJECT_SCHEMA/reference README updated; PROGRESS entry with the oldest moved whole to
    archive and its links rewritten; rewrite this brief and Review waiting; commit/push the
    same branch, without PR or merge, then stop for owner review.
- **Proposed, not booked (review finding 7, deferred by D574):** the per-row "nonempty array of
  Object" kinds (`tripRateSets`, `deterrencePurposes`, `modeChoiceNests`, …; about 15 since D566)
  could become one kind that reads its Object from the Field. It touches every slice's table and
  rule switch, so it is its own slice, measured by the emitted schema staying byte-identical.
- Never promote sample-derived hints to an authoritative contract. Explicit batch/comparison/
  Macro/Meso identities/exports remain separate from single-run APIs; reuse existing validation,
  History and GUI run timer; no generic job/session subsystem is booked.

## Order of level work (D378)

Micro, Meso and Macro carry equal weight; open level work follows the study chain, Macro → Meso →
Micro ([PROBLEM](PROBLEM.md) §3). M10.2–M10.4 are done (below); when the owner asks for an engine
slice, M9 and M11.5 are done to their owner gates; M12 R1 and R2 are done (D553, D554) and wait for the owner (C18); the M11 gate waits on the book. Each starts with its contract. M13 is
a Node method serving all three; A4 waits for the owner. OWNER_SITTING B26 (hybrid meso–micro; the
user roles) is open.

## M12 subarea workflow — R1 and R2 done (D553, D554); stop for the owner

- [SUBAREA_CONSISTENCY](reference/SUBAREA_CONSISTENCY.md), rows SC1–SC7 pass first time. `runMesoArea` runs Meso on the
  study area's run-time copy; `--consistency-report OUT --project P [--seeds] [--csv]` writes Macro, Meso and Micro
  veh/h per gate and kept Link with Δ, % and GEH (no pass/fail). On the corridor fixture Micro reads 240 against
  Macro 600 and Meso 593: the movement gives way in Micro, while Meso ignores priority inside a Node (§12).
- **R2 done (D554, §13–§14, SC8–SC10):** *Results → Cross-level → Consistency* (seeds, Run/Cancel, the table, CSV).
  **Owner:** C18 on Windows. **O2 answered (D555, §15):** a gate counts the inside zones' trips on its Link (SC11). **O1 answered and read (D556, D557):**
  TAG M3.1, May 2026, §3.3.10, p. 31; the formula matches.

## M11.5 Meso assignment — S1 done (D550); stop for the owner

- [MESO_ASSIGNMENT](reference/MESO_ASSIGNMENT.md), rows MA1–MA8 pass: M9's loop with a Meso provider (FIFO cumulative-curve
  costs), `--dynamic-assignment --meso`, `"level": "meso"` in the route-costs file. The bottleneck fixture converged at
  n = 6 with short share 0.5833 against the hand x* = 0.5909 (§9).
- Done since: A2b (D551, DYNAMIC_ASSIGNMENT §15–§16) and A4, the Results page (D552). **Owner:** C17.
- **Deferred (D551):** Meso following positioned weights (a loading change) waits for a study that needs it; the M11
  gate's benchmark waits on the owner's copy of Boyles et al. 2025 (D315).

## M9 dynamic assignment — A1–A3 done (D549); stop for the owner

- [DYNAMIC_ASSIGNMENT](reference/DYNAMIC_ASSIGNMENT.md), rows DA1–DA9. Owner chose every recommended option:
  MSA with all-or-nothing auxiliary shares per decision × interval cell, cost from the decision to arrival,
  δ (Eq. 10.22) ≤ 0.05 within 20 runs on one seed, a separate `MultraRouteCosts` file read back by `--route-costs`.
- A1–A3 done (D549); A2b and A4 since (D551, D552). Rows DA1–DA17.
- **Owner:** whether the M9 gate's rows (DYNAMIC_ASSIGNMENT §14) close it (rule 8).

## M10 four-step — G1–G2, D1–D4, E1–E2, N1, P1–P2, F1–F2, L1–L2 done (D470–D485, D547, D548); stop for the owner

- Code is complete and chains end to end: generation → distribution → mode choice → written
  `odMatrices` → Run Macro, or one stored procedure (`--procedure`). Contracts and their rows:
  [TRIP_GENERATION](reference/TRIP_GENERATION.md) §14, [TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md)
  §14–§16, [MODE_CHOICE](reference/MODE_CHOICE.md) §13, [NESTED_LOGIT](reference/NESTED_LOGIT.md),
  [PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md), [PROCEDURE_FEEDBACK](reference/PROCEDURE_FEEDBACK.md). The M10
  gate stays open (rule 8).
- Feedback (D547, schema 75) and logsum (D548, schema 76, [MODE_CHOICE_LOGSUM](reference/MODE_CHOICE_LOGSUM.md))
  are done. **Owner:** the feedback dialog and page, and the *Nests* dialog, on Windows.
- **Owner:** OWNER_SITTING item 42.
- **Later (M10.3):** whether the Furness target and cap become a document setting (open from D1).
- **Next, the owner's pick:** G/MS/D/A (Ortúzar Eq. 6.12–6.13, mode split by origin then per-mode gravity), if a
  study needs it; otherwise M10 waits for the owner's checks.

## Next session — the owner's pick

- **Open from S5:** restoring the full Macro or Meso table on reopen would need a new owner
  choice: the owner chose the cut source only (D331). The stored source and freshness line remain
  the reopen contract.
- **Lateral: closed by owner choice (2026-11-04)** after D444–D448 ([audit](evidence/lateral-run-audit.md)).
  The long m2.6 waits are signal-queue congestion that exists switched off too
  ([m26-long-waits](evidence/m26-long-waits.md)).
  - **Gap seeking ([GAP_SEEKING](reference/GAP_SEEKING.md)):** slices 1–2 done (D455, D458; slice 2
    only switched on), D455's clamp cost fixed (D456).
    - **Deferred:** speeding up to slip in ahead, the lane-change state machine, a no-progress
      timer, and abandoning a change.
  - **Not shipped:** no `data/` behaviour carries the lateral keys; D178 needs a cited preset first.
    The one exception is the [lateral lab](reference/LATERAL_LAB.md) (D457), a shipped project with
    four scenes carrying lab placeholder values. **The owner's check:** open it in the editor and
    watch the crossings, the straddling and keep-left (the D203 display).
    The +10–28 % wall cost switched on needs a profile before any work.
  - **Later, mixed-traffic discharge:** if the engine's saturation flow is ever measured with
    motorcycles filtering (no lane discipline), Wani et al. (2019, *IJTRE* 6(11), 5769–5774;
    owner's PDF) measure discharge headway per strip one motorcycle wide, keeping the smallest
    headway when a vehicle spans strips. Method only: it prints no model coefficients, only bar
    charts, so it is no benchmark (not in M6).
- **Weaving (M3.4, D449–D459): the next step is the owner's choice.** The lab is
  [WEAVING_FIXTURE](reference/WEAVING_FIXTURE.md) (rows BA179–BA195; two-entry weaves are scene 4,
  D459). A routing decision on a weave is still not supported.
  - Results (D458): [evidence](evidence/gap-seeking-changer.md).
  - **Left:**
    - the 500 m weave alone at ×1.0 on is slower than before (24 → 28.5 s);
    - ×1.2 on is over capacity (150 m: 91 % served).
    - one clamp at ×1.0 on in the full lab since D459's draw: seed 48, the 150 m weave, a car standing
      mid-crossing ([evidence](evidence/weaving-two-entries.md)).
  - **Next, the owner's choice:**
    - the pending owner checks (UT10, A9, canvas);
    - NGSIM at M6.
- **Meso (M11, in through M11.4b):** the gate needs the benchmark: waiting on the owner's copy of
  Boyles et al. 2025 (D315), then the tolerance and rows. Priority at a Node waits on Brilon and Wu
  (2001) and a cited `t_ij`; HCM 7 Ch. 20's headways are a citable candidate (BOOKS §4). M11.5
  waits on M9, whose two-route fixture has a published candidate (Ortúzar Example 11.1).
- **Macro (S2):** B21, Sheffi Fig 5.1's capacities, for the owner. §19 (c), the procedure
  sequence, is done (D484, D485).
- **M3.3.3b — the W99 contract before code** (ROADMAP M3.3, D177): read the following-model
  requirements in [DRIVING_BEHAVIOUR](reference/DRIVING_BEHAVIOUR.md), then write a sourced state,
  equation and parameter contract with hand-checkable acceptance cases. Keep the unvalidated
  marker (D13, D369) and the owner gates; this item does not authorize closing those gates.
- **Clamps (2026-11-18, seeds 42–51, summary `safetyClamps`):** M2.6 1, M6-S1 1, lane lab 0; the lane
  lab's seed-74 clamp (D252) is outside that range. Source clearance (D108) remains;
  `joinAnticipation` ships off (D255); D95 stays off.
- **Node detail:** no optimization booked (target met, D353; measurements in
  [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md)). Re-profile before proposing more work;
  scoping `automaticConflicts` below network-wide is the owner's choice.
- **Analytical junction capacity (M13):** open for the owner after A3
  ([NODE_ANALYSIS_A3](reference/NODE_ANALYSIS_A3.md) §7): a permitted far turn in l+sl, lr or
  one-lane layouts (refused, narrows D375 1a) and Q1 for permitted lane groups. A4 only when asked.
- **Owner checks** (Windows, packaged builds, appearance) are only in
  [OWNER_SITTING](plans/OWNER_SITTING.md); a session prepares the build and sheet, never a
  verdict. M2.8 has only owner checks left.
