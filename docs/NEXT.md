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
- **Shipped projects (D337):** all four stay; [their README](../data/projects/README.md) names each
  one's purpose and builder. `four-leg-signalised` now carries a 300 s cool-down (schema 27): a test
  that needs the bare drawing clears it with `changeEvaluationPeriod(d, 0, std::nullopt, 0)`.

## Canvas style — slices A–C done; stop for owner review

- Owner-directed and separate from M13/M14: [CANVAS_STYLE](plans/CANVAS_STYLE.md) holds the grill
  answers and slices A–E. Slice A (road surface, kerb, junction layers) is D423; slice B (label
  halo and screen-space placement) is D424; slice B2 (Vistro-like boxed input/decision labels
  with leaders) is D425; B3 (nearby labels stacked in columns, kept off roads) is D426.
- D460 (owner review), D461 (display fixes), D462 (owner choices) and D463 (UI round 2:
  Properties fits its panel, clipped summary ends on a row, short labels, one-line scope banner,
  "—" in empty result cells) and D464 (a PNG export shows every rate and signal head; the LOD
  threshold test measures real decoration again) are done. The owner checks them on Windows through
  [the images](images/); the junction median spikes stay (Q5).
- **Slice C (run vehicles) is done (D465), with the head/casing follow-up D466; stop for owner
  review** of `docs/images/canvas-run-d465/` and `docs/images/canvas-heads-d466/`. Slice D (lane arrows, white stop line) starts only on the owner's go.
- Owner-requested editor fixes D427–D431 and all 34 UI/data/sizing audit corrections D432
  are implemented. Review the [remediation evidence](evidence/ui-audit-fixes.md) and check
  Drawing/Tables/Results, density, modal retries and Lab on Windows through OWNER_SITTING E33/E34.
  Linux regression evidence does not close those appearance items or authorize slice C.

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
- **UTDF export:** U1 (D442), U2a (D443) and U2b are done. Contracts are in
  [UTDF_EXPORT §7](reference/UTDF_EXPORT.md#7-slices).
  - There are two ways to export:
    - the CLI, `multra-cli --export-utdf`;
    - the desktop, *File → UTDF for Synchro/Vistro…*, which shows the report first and writes UTM
      when the project has a map reference.
  - Nodes are numbered by template Apply or *Number Nodes*.
  - Passing rows: UT0–UT9 and UT11.
  - **Compass/NEMA:** mirrored mode keeps the group numbers, which are already right-hand NEMA
    numbers (D443).
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
  [OWNER_SITTING G](plans/OWNER_SITTING.md#g--object-interchange). Read [OBJECT_TABLE_CSV](reference/OBJECT_TABLE_CSV.md)
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
Micro ([PROBLEM](PROBLEM.md) §3). When the owner asks for an engine slice, the next one is
**M10.2 and M10.3 (done, below)**, then M10.4, M9, M11.5 and the M11 gate, M12. Each starts with
its contract. M13 is a Node method serving all three; A4 waits for the owner. OWNER_SITTING B26
(hybrid meso–micro; the user roles) is open.

## M10.2 trip generation — G1 and G2 done (D470, D471); stop for the owner's look

- [TRIP_GENERATION](reference/TRIP_GENERATION.md) §14 records both slices. The editor has project-owned
  rate sets (schema 66, *Trip-rate sets…* with Import/Export JSON), zone attributes in the Edit
  Zone dialog, and a *Trip generation* Results page. Rows TG1–TG14 pass on Linux (desktop and
  headless). M10.2's code is complete; the M10 gate stays open (rule 8).
- **Owner:** look at the two dialogs and the page on Windows ([OWNER_SITTING](plans/OWNER_SITTING.md)
  item 42). Linux offscreen tests do not close that look.
- **Build note:** a Debug desktop build with debug info filled this container's disk (23 GB);
  configure with `-DCMAKE_CXX_FLAGS_DEBUG="-O0 -g0"` here. Qt needs `qt6-base-dev libproj-dev
  libxkbcommon-dev nlohmann-json3-dev` from apt.

## M10.3 trip distribution — D1 and D2 done (D473, D474); D3 contracted (D475); stop for the owner

- [TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md) §14 records both slices:
  - the pure gravity/Furness engine and the free-flow skim;
  - deterrence sets (a catalog with none shipped, plus owned at schema 67);
  - owned cost matrices that override skim cells and are the only source of diagonal costs;
  - `--trip-distribution … [--cost-matrix]`;
  - the *Trip distribution* Results page with *Deterrence sets…* and *Cost matrices…*.
- Rows TD1–TD14 pass on Linux (desktop and headless). M10.3's code is complete; the M10 gate stays
  open (rule 8).
- **Owner:** the Windows look at the page and both dialogs
  ([OWNER_SITTING](plans/OWNER_SITTING.md) item 42).
- **D3 contract written (D475):** singly constrained purposes, TRIP_DISTRIBUTION §15. **Owner:**
  answer §15.2's five choices (recommended 1a–5a); slice D3 (rows TD15–TD20) starts on the answer.
- **Then, the owner's pick:**
  - M10.4 (P-A to OD, persons to vehicles, the period factor, mode choice; starts with its
    contract and Ortúzar Ch. 6–7 read from the page images).
  - Open from D1: whether the Furness target and cap become a document setting.

## Next session — the owner's pick

- **Open from S5:** restoring the full Macro or Meso table on reopen would need a new owner
  choice: the owner chose the cut source only (D331). The stored source and freshness line remain
  the reopen contract.
- **Lateral: closed by owner choice (2026-11-04).** The audit's findings and the review's copies
  are fixed (D444–D448; [audit](evidence/lateral-run-audit.md),
  [crossing-window](evidence/lateral-crossing-window.md), [covered-sections](evidence/lateral-covered-sections.md),
  [swap-lock](evidence/lateral-swap-lock.md), [refused-step-back](evidence/lateral-refused-step-back.md),
  [start-filter](evidence/lateral-start-filter.md)). The long m2.6 waits are signal-queue congestion
  that exists switched off too ([m26-long-waits](evidence/m26-long-waits.md)).
  - **Gap seeking ([GAP_SEEKING](reference/GAP_SEEKING.md)):** slices 1 and 2 are done.
    - **Slice 1 (D455):** a follower opens a gap for a refused stub anywhere in its span.
    - **Slice 2 (D458, [evidence](evidence/gap-seeking-changer.md)):** switched on only (the
      owner's choice), a refused changer falls back behind its blocker.
    - **Deferred:** speeding up to slip in ahead, the lane-change state machine, a no-progress
      timer, and abandoning a change.
    - **D455's clamp cost is fixed (D456, [evidence](evidence/dead-end-second-obstacle.md)):** a
      stub's dead end is a second obstacle, so a nearer leader no longer hides it. Clamps on at ×1.0
      23 → 0.
  - **Not shipped:** no `data/` behaviour carries the lateral keys; D178 needs a cited preset first.
    The one exception is the [lateral lab](reference/LATERAL_LAB.md) (D457), a shipped project with
    four scenes carrying lab placeholder values. **The owner's check:** open it in the editor and
    watch the crossings, the straddling and keep-left (the D203 display).
    The +10–28 % wall cost switched on needs a profile before any work.
- **Weaving (M3.4, D449–D455): the next step is the owner's choice.** The weaving lab
  ([WEAVING_FIXTURE](reference/WEAVING_FIXTURE.md), rows BA179–BA195), the weave-end lock fixes
  D450–D454 and D456 ([weaving-lock](evidence/weaving-lock.md), [held-change](evidence/weaving-held-change.md),
  [yield-cap](evidence/weaving-yield-cap.md)) and gap seeking slice 1 (D455,
  [evidence](evidence/gap-seeking-follower.md)).
  - **Two-entry weaves run (D459, [evidence](evidence/weaving-two-entries.md)):** joined chains
    start on the weave. They are scene 4 of the lab; a routing decision on a weave is still not
    supported.
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
- **Macro (S2):** B21, Sheffi Fig 5.1's capacities, for the owner. §19 (c), a procedure sequence,
  remains booked until M10.2–M10.4 provide its stages (D277).
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
