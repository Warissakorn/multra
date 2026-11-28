# NEXT — what a session does first

The single live to-do, **open items only**:
- what shipped is in [PROGRESS](PROGRESS.md), the [archive](archive/README.md) and the
  [decision record](decisions/RECORD.md);
- gates are in [ROADMAP](ROADMAP.md);
- owner items are only in [OWNER_SITTING](plans/OWNER_SITTING.md) and its [Windows sheet](plans/OWNER_SITTING_WINDOWS.md).

Rewrite this file each session ([AGENTS](../AGENTS.md#session-end)), and keep it under about 10,000 characters (D682).
Detail lives in the linked contracts; the whole file before the last cut is
[NEXT-state-2026-11-27](archive/NEXT-state-2026-11-27.md).

## Standing facts

- **Windows CI builds without precompiled headers.** After adding an include of a header that uses a type whole,
  run `python3 tools/check_no_pch.py build/desktop` (about 9 min).
- **Books:** the owner's five copies and their page rules are in [BOOKS](reference/BOOKS.md). Attach them with
  `add_repo`, and cite only what you have read.
- **`src/core/` files stay at 500 lines or fewer.** Split `types.hpp` with the first engine slice that edits
  `Vehicle` or `SimState`, and measure the rebuild before and after (see the archived NEXT).
- **M0 is closed** (D369). A change that moves `multra-cli 42` reopens it.
- **Shipped projects (D337, D492):** their purposes and builders are in [their README](../data/projects/README.md).
  A test that needs `four-leg-signalised` without its cool-down clears it with
  `changeEvaluationPeriod(d, 0, std::nullopt, 0)`.
- **Pipeline ([DEV_PIPELINE](plans/DEV_PIPELINE.md), D678–D679):**
  - run the sanitizer on the touched test groups;
  - one package per owner sitting, at most five owner items a round;
  - one pull request per topic;
  - a small fix is recorded as in AGENTS' working rules (D682).
- **PR [#232](https://github.com/Warissakorn/multra/pull/232)** carries this branch (D635–D682) and waits for the
  owner to merge.

## Owner (Windows sheet, in order)

E32, B46 with item 42, B48, item 50, C16, C17/C18, I.5, G1, H1, then C12 (optional).
- Section F is closed: F7–F12 passed on 2026-11-26 and 2026-11-27.
- About thirty older looks are carved out to [OWNER_LOOKS_CARVED](plans/OWNER_LOOKS_CARVED.md).
- Chat decisions still open: B4, B21, B26, B29 (B22 is a host question only).
- A session prepares the build and records the owner's words. Linux tests close none of these (rule 3).

## Editor and tests

- **UI_GEOMETRY ([§5](reference/UI_GEOMETRY.md), D680–D683).** The sweep now selects before it triggers (124 dialog
  views per scale) and checks widgets set on cells; 0 findings. Next: the 34 actions the sweep's "Not reached" line
  lists (Edit conflict area is among them on a project that has one: diagnose why a table's first row does not enable
  it), then dialogs opened from panels.
- **The owner's Windows look (D683):** fields in tables now fill their cells, square, and a narrow column widens to its
  field (decision flows, OD grid, junction template roles, controller, parking lot). Add it to the next package.
- **Not chosen, ask first:** sorting by header click; cell selection in one-object lists (D490).
- **Object table:** the other tabs wait for the owner's B48 verdict on the Study periods pilot (D525).

## Engine (Micro)

- **Stopping at lines ([LINE_STOP](reference/LINE_STOP.md) §11, [LINE_SETBACK](reference/LINE_SETBACK.md) §9).**
  `lineRest` is always on, and an absent `lineSetback` is 1 m (D677). Open:
  - W99 behind a moving leader: most of W99's remaining clamps. `Leader` carries no braking, so this is a contract
    question before code.
  - Startup lost time below HCM's 2.0 s: a missing reaction at green, not a line question. Any `reactionTime`
    value needs a citation (D178).
  - The M6 U record and sweep (D543, D546) ran on the earlier law; re-measure them if M6 resumes.
  - A priority rule sees only its conflict segment: a known limit (D675; G1R kept as a patch).
- **The lever left for speed:** `closestVehicle`, at 15.7 % of the oversaturated study. An order stored in
  `SimState` comes with the `types.hpp` split. Measure with callgrind and hash events
  ([ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md)).
- **Phase 1 of `stepSimulation`** is still one body (D495). Split it only with callgrind before and after.
- **M6:** nothing until independent data (R7 inD, Q4 (d)). Ask (D535): is S1's gate S1-a alone?
- **Weaving (M3.4, [WEAVING_FIXTURE](reference/WEAVING_FIXTURE.md)):** the owner chooses the next step. A routing
  decision on a weave is not supported.
- **Lateral and gap seeking** are closed by the owner (D444–D458). There is no shipped preset without a citation.
- **Counted study Ratchadaphisek × Lat Phrao** ([README](../studies/ratchada-latphrao/README.md)): the failing case of
  the lane-lock defect, not tuned further (D544). The owner has never opened `entryLanes` or `missedExitAfter` in
  the editor (item 50).

## Levels (Macro → Meso → Micro, D378)

- **M9, M10, M11.5 and M12 are implemented and each waits for the owner:** C17, C18, item 42, and whether M9's rows
  close its gate. The contracts are linked from the archived NEXT.
- **Meso (M11):** the gate needs the benchmark, which waits on the owner's copy of Boyles et al. 2025 (D315).
  Priority at a Node waits on Brilon and Wu (2001).
- **Macro:** B21 (Sheffi Fig. 5.1). Whether the Furness target and cap become a setting is still open from D1.
- **M13/M14:** the owner looks at A9, the canvas slices and UT10 (I.5) on Windows. Slice D of the canvas starts
  only on the owner's go. The Vistro median patch waits until Vistro's link numbering is confirmed.
- **Project schema (D558–D636):** every root feature is mapped up to `procedures`. The next slice is the owner's
  pick. A single "nonempty array of Object" kind is proposed but not booked (D574).

## Process

- **Owner, on GitHub:** require `Native C++` before merging to main.
- Milestone status statements are not checked against code, and the user guide has no Thai edition (D502).
