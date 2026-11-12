# NEXT — what a session does first

The single live to-do, **open items only**: what shipped is in [PROGRESS](PROGRESS.md), the
[archive](archive/README.md) and the [decision record](decisions/RECORD.md); gates are in
[ROADMAP](ROADMAP.md); owner items only in [OWNER_SITTING](plans/OWNER_SITTING.md). Rewrite this file
each session ([AGENTS](../AGENTS.md#session-end)); never append a finished slice here (D319). The
previous full list is [archived](archive/NEXT-state-2026-10-23.md).

## Standing facts

- **Books:** five owner copies (Sheffi, HCM 7, Ortúzar & Willumsen, Planning Handbook 4th ed.,
  Yperman); repositories, page rules, caveats and the chapter map by milestone are in
  [BOOKS](reference/BOOKS.md). Attach with `add_repo`, never vendor; cite the book, read the image
  before using a number. Checks of 2026-10-23: [book-copies-check](evidence/book-copies-check.md).
- **Before an engine slice:** `src/core/` files are at the 500-line limit; split along the
  responsibility a change touches first (D319). The owner deferred splitting `types.hpp` on its
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

## Owner checks waiting from recent sessions

All in [OWNER_SITTING](plans/OWNER_SITTING.md) (rule 3; Linux tests close none of them): B45 (UI
review), B46 (Macro example), C14 (Run view after D463/D487), C15 (run vehicles, slice C), C16
(random streams), E35 (tables like Excel), E36 (Route tool and marks), item 42 (M10 pages).

## Review of 2026-11-10 — queued proposals (D493)

The first review's proposals are done (D493, [PROGRESS](PROGRESS.md)). The rest wait here in review
order; none is started. **Ask** marks one the owner chooses before any code.

- **Owner only, from the purpose review:** the M5.7 rehearsal (OWNER_SITTING A3) and a second user
  on the Micro sentence. Answered (D505): the M6 benchmark sources ([ROADMAP](ROADMAP.md) M6.0);
  Micro's M6 gate may close before M10 and M11.
- **Architecture** (includes checked per module since D494):
  - **Split `EditorWindow` (D505): steps 1–5 done (D514 `BatchSession`, D517 `RunSession`, D518
    `RecoverySession`, D519 `AutomationHost`, D520 `ObjectTab`), and all 13 object tabs are panels
    (D521–D523), and the Macro/Meso runs' functions are on their panels (D524; header 396 lines).**
    D505's listed steps are done. The metadata-driven object table (D525,
    [OBJECT_TABLE](plans/OBJECT_TABLE.md)) is piloted on Study periods; the other tabs wait for the
    owner's verdict (OWNER_SITTING B48). `src/project/`
    (codecs, runs, outputs; `src/report/` is reserved) waits until report work starts.
  - ARCHITECTURE has the dependency table; its *Editor boundary* prose (about 130 lines of
    per-decision notes) still belongs in the contracts.
  - `shell` is not checked: it may include everything, so there is nothing to refuse.
- **Engine:**
  - **Random streams per input: done** (slices 1–4, D506, D510, D512; choice 3, the paired
    comparison under `perInput`, D526, BATCH §7 rows CMP11–CMP14).
    Slice 4's cost was not measured with callgrind (D512). A new project is `perInput` at schema 74; shipped projects stay
    `shared` until a recorded decision regenerates them.
  - **Left from D495:** phase 1's per-vehicle loop in `stepSimulation`, about 150 lines, is still
    one body; split it only with a callgrind run before and after (it is the hot path).
  - **Analytic rows against HCM: §6 written (D515), tolerance set (D516: S1 ±5 %, S2/U ±15 %).**
    HCM 7 values read (D527: `s0` 1,900; `g` = displayed green; d1/d2 and Eq. 20-18 forms; U-a's
    `t_c` 6.2 s, `t_f` 3.3 s; references computed in [§6](evidence/m6-benchmark-options.md)).
    The priority-line counter is built (D528, [PRIORITY_LINES](reference/PRIORITY_LINES.md),
    `--priority-lines`). D529: S1-f added (Akçelik & Besley 2002); the S2-c script is done
    (`multra-fctl-reference`, [FCTL_REFERENCE](reference/FCTL_REFERENCE.md) FC1–FC6) and its
    references are in §6 (10.98, 13.49, 19.57 s at `x` = 0.5, 0.7, 0.85). S1-d's references are read (Bonneson 1992, D532; Lee & Chen 1986, D533).
    U-c's effective `t_c`: Siegloch's regression in `--priority-lines` (D534, PRIORITY_LINES §5,
    PL7–PL9). Every source the first comparable run needs is read and computed.
    **D535: the first comparable run, kind S1, made** ([evidence](evidence/m6-s1-first-run.md)):
    S1-a 1,944 veh/h passes (1,900 ±5 %); S1-f misses (−6.7 % against 2,083); S1-d passes ranks 3, 7, 8
    and misses 2, 4, 5, 6, 9 against Lee & Chen. **Ask** the owner whether S1's gate is S1-a alone
    or needs S1-f and S1-d too. D536: the D530 diagnostic contract and observer; its rerun measured
    nothing (the engine's queue creeps at ~0.1 m/s). D537 (owner): read at Go, at 5 km/h. **Done**
    ([evidence](evidence/m6-s1-diagnostic.md)): `L_hj` 7.95 m against 6.9 (+15 %, about two thirds of
    S1-f's gap), `t_x` 1.195 s against 1.17, `v_S` 43.5 against 45.1 km/h; the queue creeps through the red.
    **Ask** the owner whether any of these is to change (it would move S1-a too).
    **D538: S2-c passes** ([evidence](evidence/m6-s2c-first-run.md)): 11.83, 14.13 and 19.79 s against
    10.98, 13.49 and 19.57 s (+7.7, +4.7, +1.1 %). **D539: S2-b and S2-a run** ([evidence](evidence/m6-s2ab-first-run.md)): pass only at
    `x` = 0.5; −16 % to −53 % below HCM `d1 + d2` from 0.7 to 1.05, the gap in `d2`. **Ask** the owner:
    measure the arrival process at the line, move the source closer, or accept. Then U-c and U-a. Then the S2
    runs (S2-c first, `multra-fctl-reference`) and the U runs (U-c, U-a).
    D530: the S1 diagnostic (`t_x`, `L_hj`, `v_S`; Murat & Cetin 2019) needs a measurement contract
    before it is used.
    D531: S1 keeps the mean headway; the median and skewness beside it need a `--discharge`
    contract.
- **UI** (seen on Linux at 1440 × 900, [the review's steps](PROGRESS.md)):
  - D497–D504 are done; the owner's look is OWNER_SITTING B45.
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
  ([ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md)); measure with callgrind first.

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

## AI automation follow-up (D344–D350)

- Open automation work: an authoritative formal schema covering every optional/versioned
  field remains separate from D350 observed-shape discovery plus codec dry validation.
  Use [PROJECT_SCHEMA](reference/PROJECT_SCHEMA.md) for root presence rules, codec owners
  and save-version floors. Next: map project-owned Composition/type-share/interval rows,
  preserving catalog presence/ownership, contains-based optionals and feature-selected save
  floors 18/19 without inventing new read gates. Extend the composed partial schema only with
  codec parity cases; keep type references, unique positive weights, valid time windows and
  run-readiness in existing validators. Vehicle/behaviour catalogs and other optional-feature
  fields remain codec-owned until mapped.
  Never promote sample-derived hints to an authoritative contract.
  Explicit batch/comparison/Macro/Meso identities/exports remain separate from single-run APIs.
  Reuse existing validation,
  History and GUI run timer; no generic job/session subsystem is booked yet.

## Order of level work (D378)

Micro, Meso and Macro carry equal weight; open level work follows the study chain, Macro → Meso →
Micro ([PROBLEM](PROBLEM.md) §3). M10.2–M10.4 are done (below); when the owner asks for an engine
slice, the next one is M9, then M11.5 and the M11 gate, M12. Each starts with its contract. M13 is
a Node method serving all three; A4 waits for the owner. OWNER_SITTING B26 (hybrid meso–micro; the
user roles) is open.

## M10 four-step — G1–G2, D1–D4, E1–E2, N1, P1–P2 done (D470–D485); stop for the owner

- Code is complete and chains end to end: generation → distribution → mode choice → written
  `odMatrices` → Run Macro, or one stored procedure (`--procedure`). Contracts and their rows:
  [TRIP_GENERATION](reference/TRIP_GENERATION.md) §14, [TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md)
  §14–§16, [MODE_CHOICE](reference/MODE_CHOICE.md) §13, [NESTED_LOGIT](reference/NESTED_LOGIT.md),
  [PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md). The M10 gate stays open (rule 8).
- **Owner:** OWNER_SITTING item 42.
- **Later (M10.3):** whether the Furness target and cap become a document setting (open from D1).
- **Next, the owner's pick:**
  - congested-cost feedback on top of procedures (the next slice the owner booked, D483 4a);
  - congested-cost feedback (Ortúzar Ch. 11);
  - logsum feedback from mode choice into distribution (G/MS/D/A, §6.5.2).

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
  - **Results (D458, [evidence](evidence/gap-seeking-changer.md)):**
    - No seed of 10 locks at any factor (×0.3–×1.2), off or on.
    - Clamps are 0 at every factor, off and on.
    - Switched on, every vehicle is served through ×1.0. Mean delay is 15/12/14 s at ×0.8 and
      87/44/35 s at ×1.0.
    - Switched off, results are byte-identical to D456.
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
- **Clamps:** 5 left on M2.6 (pull-out 1, late catch-up 4), the lane lab's 1 (D252) and source
  clearance (D108); `joinAnticipation` ships off (D255); D95 stays off.
- **Node detail:** no optimization booked (target met, D353; measurements in
  [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md)). Re-profile before proposing more work;
  scoping `automaticConflicts` below network-wide is the owner's choice.
- **Analytical junction capacity (M13):** open for the owner after A3
  ([NODE_ANALYSIS_A3](reference/NODE_ANALYSIS_A3.md) §7): a permitted far turn in l+sl, lr or
  one-lane layouts (refused, narrows D375 1a) and Q1 for permitted lane groups. A4 only when asked.
- **Engine spread costs, not booked:** `closestVehicle` 8 %, per-tick `Vehicle` moves 6 % (D283).
- **Owner checks** (Windows, packaged builds, appearance) are only in
  [OWNER_SITTING](plans/OWNER_SITTING.md); a session prepares the build and sheet, never a
  verdict. M2.8 has only owner checks left.
