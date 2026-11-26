# NEXT — what a session does first

The single live to-do, **open items only**: what shipped is in [PROGRESS](PROGRESS.md), the
[archive](archive/README.md) and the [decision record](decisions/RECORD.md); gates are in
[ROADMAP](ROADMAP.md); owner items only in [OWNER_SITTING](plans/OWNER_SITTING.md) and its
[Windows sheet](plans/OWNER_SITTING_WINDOWS.md). Rewrite this file each session
([AGENTS](../AGENTS.md#session-end)); never append a finished slice here (D319). Text cut from it is archived
(newest: [2026-11-26](archive/NEXT-state-2026-11-26.md); the last whole copy:
[2026-11-19](archive/NEXT-state-2026-11-19.md); all in the [archive index](archive/README.md)).

## Standing facts

- **Windows CI builds without precompiled headers.** After adding an include of a header that uses a type whole
  (e.g. `Json` past `json_fwd.hpp`), run `python3 tools/check_no_pch.py build/desktop` (about 9 min on 8 jobs).
- **Books:** five owner copies (Sheffi, HCM 7, Ortúzar & Willumsen, Planning Handbook 4th ed.,
  Yperman); repositories, page rules, caveats and the chapter map by milestone are in
  [BOOKS](reference/BOOKS.md). Attach with `add_repo`, never vendor; cite the book, read the image
  before using a number. Checks of 2026-10-23: [book-copies-check](evidence/book-copies-check.md).
- **Before an engine slice:** keep `src/core/` files at 500 lines or fewer (2026-11-22: largest `types.hpp` 471,
  `validate.cpp` 438, `lanes.cpp` 435); split along the responsibility a change touches first (D319). `types.hpp` reached 497
  with D618, so its W74 and perception value types moved to `w74_types.hpp` (2026-11-22, line limit only; no rebuild
  measured). The owner deferred splitting `types.hpp` for rebuild cost on its
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

## Left from closed work (D602–D635)

- **M3.2, closed by the owner (D625, 2026-11-23).** Still open on their own gates:
  [OWNER_LOOKS_CARVED](plans/OWNER_LOOKS_CARVED.md) M3.2.10 and M3.2.13 (now with item 4, M3.2.4e/M3.2.12 on
  Windows), M3.2.8a.1, M3.4, M6. Branching stays read-only (D624); a visibility key only for a study with a flared
  approach and a cited default.
- **Parallel sessions of 2026-11-23** ([PARALLEL_SESSIONS](plans/PARALLEL_SESSIONS.md)): all four merged (A
  D626–D627, B D630, C D634, D no ID). C's owner look is under *Tables like Excel* below.
- **Priority rules like Vissim** ([PRIORITY_RULES](reference/PRIORITY_RULES.md), D604–D617; item 51 passed): a
  class picker in place of typed class ids if the owner asks.
- **G7 waiting acceptance** ([YIELD_IMPATIENCE](reference/YIELD_IMPATIENCE.md), D618–D620; item 52 passed). The
  other realism gaps ([evidence](evidence/realism-gaps-vissim-sumo-aimsun.md)): G8 needs nothing; G4, G5, G6 are
  named in ROADMAP M3.4; G9 is M3.3.3b.

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

## Owner checks on Windows — triaged, one sitting (D602, 2026-11-20)

The Windows sheet was cut from about 70 items to 17 (D602). **Answered 2026-11-20:** C14, C15, E33, E34,
E35, E36, B45 (words in PROGRESS). **Answered 2026-11-26 on #152:** F7, F9, F10, F11, all passed. **Left, in order** ([OWNER_SITTING_WINDOWS](plans/OWNER_SITTING_WINDOWS.md); A2 passed 2026-11-21, D612):
E32, B46 + item 42, B48, item 50, C16, C17/C18, I.5, F8, G1, H1, C12. Thirty-one
older looks are carved out to [OWNER_LOOKS_CARVED](plans/OWNER_LOOKS_CARVED.md) (open, closing nothing).
A session prepares the build and records the owner's words; Linux tests close none of them (rule 3).

**Owner findings of 2026-11-20, decided the same day:** A2's lines outside the tool → D603 (controlled
lines hidden outside the right-of-way tools); E35's block selection → D605 (one outline per selected
block). Both are Linux-verified; the owner's look comes with A2 attempt 2 and at the next table use.

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
  - ARCHITECTURE has the dependency table; its *Editor boundary* is a 22-line summary with one link
    per contract (slice D, 2026-11-23).
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
  - The owner's look at D497–D504 (B45) passed, 2026-11-20.
  - The Node analysis example uses the pocket template (D513).
  - Not wanted (D505): palette names at 1366 px. No start page (D505): File already has recent files and examples. Reopen only if a real
    user cannot find them.
- **Process:**
  - **Owner, on GitHub:** require `Native C++` before merging to main (PRs 210 and 213 merged
    seconds after opening).
  - Left from D502: milestone status statements ("M10.2 implemented") are still not checked against code;
    the user guide has no Thai edition and no pictures.

## Tables like Excel (D490) — done; E35 passed on paste (2026-11-20), one selection defect above

- Contract: [EDITOR_WORKFLOW](reference/EDITOR_WORKFLOW.md).
- **Left from the compactness review:** done on Linux (D634): every Results level's strip fits at 1024 × 768 in
  Thai with no label changed, and the clamp note opens with its not-validated statement. **Owner:** the same
  strip in Thai at 1024 on Windows (its Thai font may measure wider); if it scrolls, shorten labels first.
- **Not chosen, ask before doing:** sort by header click, and cell selection in one-object lists
  (each list's `selectRow()` callers would need rewriting first).

## Optimisation passes (2026-11-07 D487–D489; 2026-11-19 D597–D600; 2026-11-22 D621) — follow-ups only

- **2026-11-22:** GCC/Clang Debug builds compile with `-Og` (D621): the desktop suite 331 → ~140 s, results
  byte-identical; `-DMULTRA_DEBUG_OG=OFF` restores `-O0`. The schema-label dump streams case by case (peak memory
  2.2 GB → 0.6 GB). The root `CMakeLists.txt` is split by responsibility into `cmake/` includes (D630, [BUILDING](BUILDING.md#where-the-build-rules-live)): add a test or tool to the include that owns it.
- C14 passed (2026-11-20, "นิ่ง"). A `run-presentation-ui` failure on Windows CI only is a Windows LayoutRequest
  source, not a flaky test.
- The next CI run: read `linux (desktop)`'s `ccache -s`; hits should rise after D489. If not, the
  cause is keys or sloppiness, not size.
- **Owner check (D597, D598):** MSVC Release wall time, now part of OWNER_SITTING_WINDOWS C12.
- **D599:** the build is warning-free on GCC 13; the next GCC 14+ or MSVC `/W4` build may show new ones:
  fix them in the code, never widen the GCC < 14 exemption.
- **Engine, the lever left** ([ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md), 2026-11-23): `closestVehicle`
  is 15.7 % of the oversaturated study. A by-rear order rebuilt every tick (2026-11-19) and one seeded from the fleet's
  order and repaired by insertion (D627: study −8.8 %, benchmark +0.6 to +0.8 %) were measured and not shipped. Left:
  an order stored in `SimState` (with the `types.hpp` split). Insertion's per-arrival rebuild is gone (D635); an arrival
  tick still resolves refs, spans and buckets twice, and reusing insertion's would need them permuted into id order.
  Measure study, M2.6 and the benchmark with callgrind, hash events (seeds 42–43).

## Canvas style — slices A–C done; stop for owner review

- [CANVAS_STYLE](plans/CANVAS_STYLE.md) holds the grill answers and slices A–E; A–C and their
  fixes are D423–D426 and D460–D466. The junction median spikes stay (Q5).
- **Owner review answered 2026-11-20:** C15 (slice C) "ตรง", E33 "ตรง", E34 "กดถึงได้ตลอด". Slice D (lane
  arrows, white stop line) still starts only on the owner's go.
- **Stop here; do not start another slice.** A9 (D422, NA24–NA30) awaits the owner's final review;
  B40/B41 are answered and no contract question is pending. Windows/high-DPI appearance is
  [OWNER_SITTING E32](plans/OWNER_SITTING.md).
- **If the owner requests changes:** keep them in A9 and follow the reading list, the behaviour to
  preserve and the verification steps in [the archived NEXT's M13 section](archive/NEXT-state-2026-11-19.md#m13--a9-implemented-stop-for-owner-review)
  (NODE_ANALYSIS_MODEL, NODE_ANALYSIS_EDITOR, NODE_ANALYSIS §4–§6/§12, D383/D418–D422). Schema 65 is unused.
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
- Windows appearance: E24–E31 are carved out to ROADMAP M14 (D602), E32 stays on the sheet; Linux tests do not close those items.
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
- **Mapped so far (D558–D573, D636):** compositions, modes, vehicle types, behaviours (item 49 answered,
  D562), vehicle classes and link behaviour types, the zone layer, study periods and area, the map,
  Macro settings and runs, trip rates, deterrence sets and cost matrices, period factors, mode
  choice. Rows: [PROJECT_SCHEMA_DEMAND](reference/PROJECT_SCHEMA_DEMAND.md) and
  [PROJECT_SCHEMA_FEATURES](reference/PROJECT_SCHEMA_FEATURES.md).
- **`procedures` (73, feedback 75) is mapped (D636, PRS1–PRS3)** after the owner's "ok" to D573–D574; the owner accepted
  D636 and its review fixes (2026-11-23, "ok"). The Macro sets' and procedures' tables are in `macro_set_fields.cpp`. No further
  schema slice is briefed: the next is the owner's pick (an uncovered root feature, or the proposal below).
- **Proposed, not booked (review finding 7, deferred by D574):** the per-row "nonempty array of
  Object" kinds (`tripRateSets`, `deterrencePurposes`, `modeChoiceNests`, …; about 15 since D566)
  could become one kind that reads its Object from the Field. It touches every slice's table and
  rule switch, so it is its own slice, measured by the emitted schema staying byte-identical.
- Never promote sample-derived hints to an authoritative contract. Explicit batch/comparison/
  Macro/Meso identities/exports remain separate from single-run APIs; reuse existing validation,
  History and GUI run timer; no generic job/session subsystem is booked.

## Order of level work (D378)

Micro, Meso and Macro carry equal weight; open level work follows the study chain, Macro → Meso →
Micro ([PROBLEM](PROBLEM.md) §3). M10.2–M10.4 are done; when the owner asks for an engine
slice, M9 and M11.5 are done to their owner gates; M12 R1 and R2 are done (D553, D554) and wait for the owner (C18); the M11 gate waits on the book. Each starts with its contract. M13 is
a Node method serving all three; A4 waits for the owner. OWNER_SITTING B26 (hybrid meso–micro; the
user roles) is open.

## M9, M10, M11.5, M12 — implemented; each waits for the owner

What shipped is in PROGRESS, the decision record and [the archived NEXT](archive/NEXT-state-2026-11-19.md).
Contracts: [SUBAREA_CONSISTENCY](reference/SUBAREA_CONSISTENCY.md), [MESO_ASSIGNMENT](reference/MESO_ASSIGNMENT.md),
[DYNAMIC_ASSIGNMENT](reference/DYNAMIC_ASSIGNMENT.md), and for M10 [TRIP_GENERATION](reference/TRIP_GENERATION.md) §14,
[TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md) §14–§16, [MODE_CHOICE](reference/MODE_CHOICE.md) §13,
[NESTED_LOGIT](reference/NESTED_LOGIT.md), [PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md),
[PROCEDURE_FEEDBACK](reference/PROCEDURE_FEEDBACK.md), [MODE_CHOICE_LOGSUM](reference/MODE_CHOICE_LOGSUM.md).

- **M12 (R1, R2; D553–D557):** **Owner:** C18 on Windows. On the corridor fixture Micro reads 240 against
  Macro 600 and Meso 593: the movement gives way in Micro, while Meso ignores priority inside a Node (§12).
- **M11.5 (S1, D550–D552):** **Owner:** C17. **Deferred (D551):** Meso following positioned weights (a loading
  change) waits for a study that needs it; the M11 gate's benchmark waits on the owner's copy of Boyles et al.
  2025 (D315).
- **M9 (A1–A4, D549–D552):** **Owner:** whether the M9 gate's rows (DYNAMIC_ASSIGNMENT §14) close it (rule 8).
- **M10 (D470–D485, D547, D548):** the gate stays open (rule 8). **Owner:** the feedback dialog and page, and the
  *Nests* dialog, on Windows; OWNER_SITTING item 42. **Later (M10.3):** whether the Furness target and cap
  become a document setting (open from D1). **Next, the owner's pick:** G/MS/D/A (Ortúzar Eq. 6.12–6.13, mode
  split by origin then per-mode gravity), if a study needs it; otherwise M10 waits for the owner's checks.

## Next session — the owner's pick

- **Open from S5:** restoring the full Macro or Meso table on reopen would need a new owner
  choice: the owner chose the cut source only (D331). The stored source and freshness line remain
  the reopen contract.
- **Lateral: closed by owner choice (2026-11-04)** after D444–D448 ([audit](evidence/lateral-run-audit.md)).
  The long m2.6 waits are signal-queue congestion that exists switched off too
  ([m26-long-waits](evidence/m26-long-waits.md)).
  - **Gap seeking ([GAP_SEEKING](reference/GAP_SEEKING.md), D455–D458), deferred:** speeding up to slip in
    ahead, the lane-change state machine, a no-progress timer, and abandoning a change.
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
- **M3.3.3b W99 runs, unvalidated** ([W99](reference/W99.md); history D637–D654 in PROGRESS and
  [the archived NEXT](archive/NEXT-state-2026-11-26.md)). The unvalidated marker stays. **Stopping at lines:**
  [LINE_STOP](reference/LINE_STOP.md) L3F is in as `clampRules.lineRest` (schema 87, on by default for projects);
  queues rest ([LINE_QUEUE](reference/LINE_QUEUE.md), [PROTOTYPE_QUEUE](reference/PROTOTYPE_QUEUE.md),
  [REACTION_STOP](reference/REACTION_STOP.md)); B is closed ([STOP_PROFILE](reference/STOP_PROFILE.md), D663–D667:
  B3P at lines, today's law toward vehicles at rest).
  **Next:**
  - **W99 behind a moving leader.** Most of W99's remaining clamps are behind leaders still creeping when they
    stop. Capping those needs the leader's braking, which `Leader` does not carry. It is a contract question
    before code. A leader braking to a stop is outside Q2 as well.
  - **Startup lost time below HCM's 2.0 s** in every model and in SUMO is the absence of a reaction at green, not
    a line question. PERCEPTION's `reactionTime` is the existing mechanism; any value needs a citation (D178).
    Not proposed until the owner asks.
  - **The M6 U record and sweep (D543, D546)** run with the switch off; re-measure them under D652/D653 if M6
    work resumes.
  - **A cited W99 preset** only if one exists (D178).
  - **Left from B:** the staged approach and the higher final braking toward a standing queue (D666; a second
    site would reopen them); the last step to rest (Akçelik & Biggs 1987, Deligianni 2017, both unread, owner's
    copy); W74's and W99's profile, which D258's floor sets. A headway-arm fixture under `lineRest` would be new
    work.
  **Owner (Windows), 2026-11-26 on package #152:** F7, F9, F10 and F11 passed. Left: [F8](plans/OWNER_SITTING_WINDOWS.md),
  the CC1 table, on the same package.
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
