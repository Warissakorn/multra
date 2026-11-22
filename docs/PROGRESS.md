# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-22 — M3.2's diverge rows written: A64 fails, bodies overlap inside a split (D622)

- **Owner:** "พัฒนาต่อ"; the one NEXT item needing no owner was M3.2's diverge acceptance-fixture row (review O6/R7).
- **D622:** `tests/diverge_tests.cpp` (`diverge.*`), rows A62–A66 in M3_ACCEPTANCE: one lane splitting by two Connectors.
  A62 (branching only, nothing compiled), A63 (free flow: departure order kept, no clamp, no overlap), A65 (a branch
  blocked at the split holds everyone behind it, first-in first-out) and A66 (copied-state replay) pass. **A64 fails:**
  when a branch queue reaches back into the split, a car to the other branch loses its leader once the leader's rear
  leaves the shared lane and drives through it where the lanes still overlap (8 of 10 seeds, up to 1.69 m, no clamp).
  Bodies are checked as rectangles in the plane; station overlap alone over-reports (lanes 11.76 m, bodies about 8 m).
- **Not done:** the engine is unchanged; the remedy changes runs at every diverge, so it is the owner's choice
  (OWNER_SITTING item 53; recommended (a), sibling-branch leaders). [Evidence](evidence/diverge-acceptance.md).
- **Verified (Linux, offscreen):** desktop ctest 368/368 (the new `diverge` group included); docs, file sizes pass.

## 2026-11-22 — Three optimisation passes: the test loop, the Debug build, the live docs (D621)

- **Owner:** /optimize-program three times (the third with "เอกสาร"); each plan answered "แก้ตามแนะนำ". The owner did not
  revert the W74 split of `types.hpp` (line limit only; NEXT keeps the deferred rebuild split).
- **Pass 1 (after G7):** the schema-label dump streams case by case, output byte-identical (dump 38.3 → 25.3 s, peak memory
  2,217 → 603 MB, `schema-labels` 84.2 → 71.5 s); merge groups out of `right_of_way.cpp` (500 → 447 lines); comments and the
  D592/D610 amendment notes brought in line with D620; one misleading-indentation warning fixed. A pickling change for the
  Python half was measured and dropped (1.1 s of pickling; fork-shared validation was slower, 40.2 → 66.1 s).
- **Pass 2, D621:** GCC/Clang Debug builds compile with `-Og`: the desktop suite 331 s (313–336) → 138–143 s, `check`
  336–356 → 148 s, `multra-cli 42` byte-identical; `-DMULTRA_DEBUG_OG=OFF` restores `-O0`. The engine was not touched:
  Release runs 0.25 µs per vehicle-tick. Two more warnings fixed; `types.hpp` 497 → 471 (`w74_types.hpp`).
- **Pass 3 (docs):** NEXT's four stale statements corrected; finished-slice history moved verbatim to
  [NEXT-state-2026-11-22](archive/NEXT-state-2026-11-22.md) and the `procedures` brief to
  [PROCEDURE_SCHEMA_SLICE](plans/PROCEDURE_SCHEMA_SLICE.md) (NEXT ~7,100 → ~5,400 tokens); D400–D599 frozen in
  [RECORD-D400-D599](decisions/RECORD-D400-D599.md) as D486 splits every 200 IDs (RECORD.md ~61,700 → ~5,900 tokens).
- **Verified (Linux, offscreen):** desktop `check` 367/367 after each pass; docs and sizes pass. Nothing visible changed,
  so there is no Windows look.

## 2026-11-22 — Windows items 51 and 52 passed; conflict-area display as Vissim (D620)

- **Owner (verbatim):** "4. 51 ok"; on item 52: "5. สมจริง แต่สามารถใช้ตาม Aimsun next ได้อย่างถูกต้องตามกฎหมายใช่หรือไม่".
  Findings 1–3 with two pictures: arrows off their lanes at multi-lane crossings; new junctions already green and red;
  "3. อธิบาย conflict area ที่ยังไม่สามารถปรับได้". Answers: "พิมผิด ต้องการให้แสดงเส้นลูกษรตรงกลางเฉพาะตอนเลือก Conflict area นั้น";
  "เหลืองเหมือนกันหมด".
- **D620:** centre arrows on the lane's own centre and only on the selected area; unauthored areas yellow (automatic merges
  included), authored undecided red on both sides. Explained in chat: the dark brown areas are branching (a diverge from one
  lane), read-only because vehicles keep their order there; the licensing answer (methods from public manuals, reimplemented;
  no code or text copied; not legal advice).
- **Verified (Linux, offscreen):** desktop 367/367 (priority-canvas checks red-red and no arrows off the selected area); the
  new geometry row puts two lanes' centres 3.5 m apart; docs and sizes pass.

## 2026-11-21 — G7 slice 2: the gap reduction in the dialogs, and the waiting-acceptance lab (D619)

- **Owner:** "ทำต่อ".
- **D619:** the area dialog and each marker in the rule dialog have *Gap reduction while waiting (Aimsun)*. The
  [lab](../data/projects/README.md) `yield-impatience-lab` ships for OWNER_SITTING_WINDOWS item 52. Seed 42: the minor
  delays fall from 60.9 to 24.7 s and 38.7 to 12.8 s, the major ones rise 0.14–0.29 s; no clamp, no overlap.
- **Verified (Linux, offscreen):** desktop 367/367, headless 250/250 (YI1–YI12 pass), docs and sizes pass; `multra-cli 42`
  keeps 45 / 31.75870154816726 / 1. The no-PCH check and Windows wait for the package build.

## 2026-11-21 — G7 slice 1: a waiting driver accepts less, Aimsun's gap reduction and SUMO's impatience (D618)

- **Owner (verbatim):** "G7 ทำแบบ Aimsun"; on the follow-on, "ในสถานะการที่รถทางรองออกแล้วรถทางหลักหยุด แล้วรถทางรองคันถัดไป
  ออกต่อกันกับคนแรก เนื่องจากมีโอกาสจากรถทางหลักหยุด"; "ทบทวนความเต็มใจให้รถทางหลักชะรอของ sumo"; "ทำทั้งสองส่วนตามแนะนำ".
- **Read first** (hosts allowed for this): Vissim 2025's stop-sign and conflict-area help, SUMO's docs and `MSLink.cpp`, Aimsun
  Next's manual. G8 needs nothing (all three hold a regular stop for a step or stop fully); Vissim has no G7 key.
- **D618, [YIELD_IMPATIENCE](reference/YIELD_IMPATIENCE.md):** schema 84; Aimsun's linear gap reduction over a per-vehicle
  hashed yield time, set per rule or marker; SUMO's impatience at conflict areas only, so a standing major vehicle lets the
  next minor vehicle follow. Off by default; `multra-cli 42` keeps 45 / 31.75870154816726 / 1.
- **Verified (Linux):** `yield_impatience` YI1–YI9 pass; `ctest --preset headless` 250/250; desktop: all but `results-tabs-ui`
  passed in the parallel run, which timed out under load and passes alone (34 s); docs and sizes pass.
- **Next:** slice 2, the place keys in the dialogs and a lab scene for the owner's look.

## 2026-11-21 — PRIORITY_RULES slice 3: display, table rows and the unreached-marker row (D617)

- **Owner:** "ทำส่วนที่ 3 เลย".
- **D617:** each marker's headway shows as a faint green band upstream; a picked rule (line, marker or row) links its
  line to each marker. The Conflict areas tab lists the rules after the areas; the row's dialog edits the name, the
  line's Stop/Yield and classes, and each marker's gapTime, headway, maxSpeed (0 = no limit) and classes in one Undo
  step (`setPriorityRule`); Delete keeps the line (`removePriorityRule`). A marker no route reaches is an advisory
  (`PRIORITY_MARKER_UNREACHED`). Classes are typed as ids for now.
- **Fixed on the way:** the table's group loop ran to the new row count and read past the groups (caught by
  `stop-line-ui` and `priority-ui` as a SEGFAULT before commit).
- **Rows:** `priority_marker` (dialog command, refusals, Delete, advisory), `priority-ui` (rows, canvas pick, dialog,
  Undo, Delete on the lab), `priority-canvas` unchanged.
- **Checks:** `multra-cli 42` keeps 45 / 31.75870154816726 / 1; `check` desktop 366/366, headless 249/249; 679 units
  build without the PCH. Windows: Package binaries run started for the owner's item 51.

## 2026-11-21 — The priority-rule lab for the owner's Windows look (item 51)

- **Owner (verbatim):** "สร้าง Project สำหรับทดสอบเช็ค บน Window", then "สร้างเสร็จแล้ว pack binary เลย".
- **`data/projects/priority-rule-lab.traffic.json`** (builder `tools/priority_rule_network.hpp`, regenerate with
  `multra-priority-rule-fixture`; *Open example → Priority rule lab*): the T-junction's roads and demand, left-hand,
  with two priority rules placed by the Stop line tool's commands and no conflict area. The right turn yields at its
  line to markers on both streams; the left turn has a Stop with no area. Owner steps: OWNER_SITTING_WINDOWS item 51.
- **Rows:** `priority_marker` (file is the builder's output; rules compile, Stop on the left turn, every derived
  merge rule has the turn yielding; a seed-42 run holds right-turners and serves every left-turner's Stop; same events).
- **Reading, no verdict (rule 4):** seeds 42–44, mean delay right turn 29.6 s, left turn (Stop) 44.4 s, major 4.0 s;
  the Yield T-junction with conflict areas gives 17.5 s and 9.5 s.
- **Checks:** `check` desktop 366/366, headless 249/249 (`studyperiods` SP1 counts ten shipped projects).

## 2026-11-21 — PRIORITY_RULES slice 2: the Stop line tool adds conflict markers (D616)

- **Owner:** "ทำงานถัดไป".
- **D616:** the Stop line tool places a line with no area downstream (amends D595's refusal); with a line picked,
  Ctrl+right-click on another lane adds a marker at 3.0 s / 5.0 m, starting the rule; a marker drags along its path
  and Delete removes it, the rule with its last marker; one Undo step each (`priority_marker_commands.cpp`).
  Markers follow their roads (split, delete, copy, study-area cut) and show as green bars (`canvas_markers.cpp`).
- **Rows:** `priority_marker` PR7, PR8 (5 tests); `stop-line-ui` extended (place alone, add, drag, Delete, Undo);
  `stoplines` SL4 updated for the lone line.
- **Checks:** `multra-cli 42` keeps 45 / 31.75870154816726 / 1; `check` desktop 366/366, headless 249/249; 677 units
  build without the PCH. Windows not run.

## 2026-11-21 — PRIORITY_RULES slice 1b: a Stop on a priority rule's line (D615)

- **Owner:** "ทำงานถัดไป" (the slice-1 remainder, D606 Q4 (a)).
- **D615:** a Stop control on a marker rule's waiting line makes every marker's runtime rule a Stop
  (`PriorityRule::stop`); the engine holds the vehicle until its stop service records a full stop at that line,
  then the gap test applies, as a Stop zone does. Such a control may name no area (no schema step). Yield is the
  rule alone.
- **Rows:** `priority_rule` PR14–PR16 and PR10 with a Stop (13 tests). Without the hold PR14 fails.
- **Checks:** `multra-cli 42` keeps 45 / 31.75870154816726 / 1; `check` desktop 365/365, headless 248/248. Windows not run.

## 2026-11-21 — PRIORITY_RULES slice 1: a stop line with conflict markers, no area (D614)

- **Owner:** "ทำงานถัดไป" (D609's order: PRIORITY_RULES slice 1, choices D606).
- **D614, [PRIORITY_RULES](reference/PRIORITY_RULES.md):** schema 83 adds a priority rule's marker form
  (`waitingLineId`, `vehicleClassIds`, `markers` with `gapTime`, `headway`, optional `maxSpeed` in km/h and
  `vehicleClassIds`). Each marker compiles to one core `PriorityRule` at the line (`priority_markers.cpp`);
  classes become type ids in the project layer (`priority_rule_classes.cpp`). The engine's rule loop skips a
  type outside the line's classes, a major vehicle outside the marker's, and a major vehicle above `maxSpeed` in
  the headway test. No UI.
- **Left from §3:** the Stop at the line (a Stop control names only areas; next, before slice 2) and the Problems
  row for a marker no route reaches (with the table).
- **Rows:** `priority_rule` PR1–PR6, PR10–PR13 (10 tests). Without the engine change PR11 and PR12 fail. PR9:
  `multra-cli 42` keeps 45 / 31.75870154816726 / 1; no shipped file uses the form.
- **Checks:** `check` passes on desktop 365/365 and headless 248/248. Windows not run.

## 2026-11-21 — G3 as Vissim: a vehicle stuck at its dead end is removed and counted (D613)

- **Owner, in chat on G3 (as recommended):** as Vissim, remove and count separately, off by default per behaviour.
- **D613, [DIFFUSION](reference/DIFFUSION.md):** a behaviour's `diffusionTime` (schema 82) removes a vehicle waiting
  that long at its stub's dead end (`DiffusedEvent`); runs with the key report `removed` (summary, evaluation, batch).
  Editor row beside `missedExitAfter`. Also PROJECT_SCHEMA's missing rows 80 and 81.
- **Unchanged without the key:** `multra-cli 42` keeps 45 / 31.75870154816726 / 1.
- **Measured** on the study's spread probe, seeds 42–45: at 60 s, 165 removed, completed 4,501 → 5,010.
- **Checks:** `diffusion` 6/6; headless `check` 247/247. Desktop `check` alone failed `behaviour-lab-ui`
  (SIGSEGV in `~DriverBehaviour`) and `cli-consistency`: both binaries were never `check` dependencies and ran
  stale. `cmake/check_builds_all.cmake` now makes `check` depend on every executable. Verified: after touching
  `types.hpp`, `check` alone gives desktop 364/364 and headless 247/247.

## 2026-11-21 — M3.2.7d passes on attempt 2 (D612)

- **Owner, step 4 and the verdict (verbatim):** "ทดลองแล้วสมจริง", after Stop and Yield were explained here and in
  SUMO and Aimsun.
- Recorded in OWNER_SITTING A2 and the M3_ACCEPTANCE sheet; ROADMAP's M3.2.7d row passed. M3.2 stays open on its
  diverge acceptance-fixture row. Windows version and timings were not given. No code change.
- **Next (D609):** G3 stuck vehicles, then PRIORITY_RULES slice 1.
