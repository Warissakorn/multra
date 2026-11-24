# PROGRESS archive — 2026-11-21

Whole entries moved from the live PROGRESS log.

## 2026-11-23 — Results tabs at 1024 in Thai; the clamp note's marker (slice C, D634)

- **Strip:** measured first, on Linux offscreen at 1024 × 768 with every tab of each level forced visible: Thai Micro
  605 px, Macro 572 px, English 673 and 483, of a 1008 px Results page. D497's one-level strip had already fixed the
  scroll NEXT carried from D490's review, so no label changed and no control was added (D634).
- **Clamp note:** now opens "Not yet validated." / "ยังไม่ผ่านการตรวจสอบความถูกต้อง", as every other Results note does
  (rule 4); "Safety clamp" stays untranslated.
- **Test:** `results-tabs-ui` replaces its conditional strip check with one at 1024 × 768 in Thai and English that first
  asserts the size and language took effect, then per level: no scroll button, no tab past the page, no elided label;
  and the clamp note's statement in both locales (a third locale fails it until added). Proved to bite: the old
  locales fail on the note; a 1614 px Thai Micro label fails on the strip.
- **Verified (Linux only):** desktop build, full `ctest --preset desktop` and `check`. Screenshots taken for the check
  only, not committed (no contract cites them). The Windows Thai look is the owner's.

## 2026-11-23 — Slice A: the kept leader order measured and not shipped (D627); long routes bisect (D626)

- **Brief:** [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md) A. Callgrind, GCC 13.3, Release `-g1`, seed 42; baseline study
  34,553.6 M Ir, M2.6 4,031.1 M, `multra-engine-benchmark 48 900` 10,226.8 M. Event hashes: every event (`eventJson`) of
  every shipped project that runs and the study, seeds 42 and 43, identical after every variant.
- **D627, not shipped:** buckets ordered by (rear, span index), seeded from the fleet's id order and repaired by
  insertion, the scan stopping at the first pass. The best variant cut the study 8.8 % but cost the benchmark 0.6 %
  (0.8 % on top of D626), over the 0.5 % bar; code reverted, five variants' numbers in ENGINEERING_NOTES.
- **D626, shipped:** `closestVehicle` bisects routes of more than 8 parts to the first part not behind the vehicle.
  The walk over passed parts was 5.7 % of the benchmark. Benchmark −5.9 %, study +0.15 %, M2.6 +0.25 % (D598's
  bar). New test in `straddlerleader` compares it with the full scan at 501 positions.
- **Verified (Linux only, desktop preset, GCC 13.3):** full build, `ctest` 368/368, `check` passes. No Windows run.

## 2026-11-23 — `CMakeLists.txt` split by responsibility (D630, parallel slice B)

- **Brief:** [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md) B. The root file was at 498 of 500 lines.
- **Moved whole, in place and order:** tool executables → `cmake/tools.cmake`; check tools and `check` →
  `cmake/check_target.cmake`; `multra-tests` and its groups → `cmake/model_tests.cmake`; tool, CLI and script
  tests → `cmake/cli_tests.cmake`. Root 498 → 194 lines; the map is in
  [BUILDING](../BUILDING.md#where-the-build-rules-live).
- **No behaviour change**, on `desktop` (Qt 6) and `headless`: sorted `ctest -N`, `--target help`, sorted
  `compile_commands.json`, `cmake_install.cmake` and sorted `ninja -t commands` identical before and after;
  `CTestTestfile.cmake` differs only in each test's backtrace (file and line).
- Checks (Linux only, GCC 13.3): full build, `ctest` and `check` pass on `headless` (251/251) and `desktop` (368/368).

## 2026-11-23 — Four slices briefed for parallel sessions

- **User:** "พัฒนาต่อ โดยเปิด Branch และเขียน prompt เพื่อให้ Agent ทำงาน พิจารณาการทำหลายงานพร้อมกันด้วย".
- Of NEXT's open items, four need no owner choice and touch disjoint files: the leader order kept across
  ticks (A), the `CMakeLists.txt` split (B), the Results tab strip at 1024 in Thai and the clamp note (C),
  and ARCHITECTURE's *Editor boundary* prose into the contracts (D). Briefs:
  [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md); one branch and one cloud session each.
- To keep four merges trivial: decision numbers 626–641 and BA216–BA223 reserved per slice; only B edits
  `CMakeLists.txt`; the four oldest PROGRESS entries moved whole to
  [PROGRESS-LOG-2026-11-21](../archive/PROGRESS-LOG-2026-11-21.md) to leave room for four new ones.
- Docs only; `multra-check-docs` and `multra-check-file-sizes` pass.

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
  [lab](../../data/projects/README.md) `yield-impatience-lab` ships for OWNER_SITTING_WINDOWS item 52. Seed 42: the minor
  delays fall from 60.9 to 24.7 s and 38.7 to 12.8 s, the major ones rise 0.14–0.29 s; no clamp, no overlap.
- **Verified (Linux, offscreen):** desktop 367/367, headless 250/250 (YI1–YI12 pass), docs and sizes pass; `multra-cli 42`
  keeps 45 / 31.75870154816726 / 1. The no-PCH check and Windows wait for the package build.

## 2026-11-21 — G7 slice 1: a waiting driver accepts less, Aimsun's gap reduction and SUMO's impatience (D618)

- **Owner (verbatim):** "G7 ทำแบบ Aimsun"; on the follow-on, "ในสถานะการที่รถทางรองออกแล้วรถทางหลักหยุด แล้วรถทางรองคันถัดไป
  ออกต่อกันกับคนแรก เนื่องจากมีโอกาสจากรถทางหลักหยุด"; "ทบทวนความเต็มใจให้รถทางหลักชะรอของ sumo"; "ทำทั้งสองส่วนตามแนะนำ".
- **Read first** (hosts allowed for this): Vissim 2025's stop-sign and conflict-area help, SUMO's docs and `MSLink.cpp`, Aimsun
  Next's manual. G8 needs nothing (all three hold a regular stop for a step or stop fully); Vissim has no G7 key.
- **D618, [YIELD_IMPATIENCE](../reference/YIELD_IMPATIENCE.md):** schema 84; Aimsun's linear gap reduction over a per-vehicle
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
  [NEXT-state-2026-11-22](NEXT-state-2026-11-22.md) and the `procedures` brief to
  [PROCEDURE_SCHEMA_SLICE](../plans/PROCEDURE_SCHEMA_SLICE.md) (NEXT ~7,100 → ~5,400 tokens); D400–D599 frozen in
  [RECORD-D400-D599](../decisions/RECORD-D400-D599.md) as D486 splits every 200 IDs (RECORD.md ~61,700 → ~5,900 tokens).
- **Verified (Linux, offscreen):** desktop `check` 367/367 after each pass; docs and sizes pass. Nothing visible changed,
  so there is no Windows look.

## 2026-11-22 — M3.2's diverge rows written: A64 fails, bodies overlap inside a split (D622)

- **Owner:** "พัฒนาต่อ"; the one NEXT item needing no owner was M3.2's diverge acceptance-fixture row (review O6/R7).
- **D622:** `tests/diverge_tests.cpp` (`diverge.*`), rows A62–A66 in M3_ACCEPTANCE: one lane splitting by two Connectors.
  A62 (branching only, nothing compiled), A63 (free flow: departure order kept, no clamp, no overlap), A65 (a branch
  blocked at the split holds everyone behind it, first-in first-out) and A66 (copied-state replay) pass. **A64 fails:**
  when a branch queue reaches back into the split, a car to the other branch loses its leader once the leader's rear
  leaves the shared lane and drives through it where the lanes still overlap (8 of 10 seeds, up to 1.69 m, no clamp).
  Bodies are checked as rectangles in the plane; station overlap alone over-reports (lanes 11.76 m, bodies about 8 m).
- **Not done:** the engine is unchanged; the remedy changes runs at every diverge, so it is the owner's choice
  (OWNER_SITTING item 53; recommended (a), sibling-branch leaders). [Evidence](../evidence/diverge-acceptance.md).
- **Verified (Linux, offscreen):** desktop ctest 368/368 (the new `diverge` group included); docs, file sizes pass.

## 2026-11-22 — Red-red branching enforced: A64 passes (D623)

- **Owner:** "อ่าน sumo ก่อน", then "ทำตามแนะนำ" on item 53: option (a), the extent being the branching area the editor
  draws. Read first: Vissim's manual (branching red-red, vehicles see each other) and SUMO's `MSLink.cpp` (same-source
  sibling lanes are foes, car-following until the foe's back passes the divergence point).
- **D623:** `compiledBranchings` (`src/model/network/branching_compile.cpp`) gives `Scenario::branchings` from the
  Connector pairs leaving one lane; `appendVehicleSpans` copies a body whose rear is inside its area onto the sibling
  (`own = false`). Rows A64, A67–A70 written first; all nine `diverge` rows pass, A64 with 0 overlaps on every seed.
- **Measured:** completed trips unchanged except the study (−1.5 %); clamps unchanged; `multra-cli 42` byte-identical;
  three movements beyond their noise, the T-junction's minor → eastbound 9.9 → 14.2 s (owner item 54). The first
  version compiled through `automaticConflicts` (+18.8 % on the benchmark); compiling only same-lane pairs fixed that:
  callgrind study +2.5 %, M2.6 +3.1 %, benchmark +2.0 %.
  [Evidence](../evidence/branching-leaders.md).
- **Verified (Linux, offscreen):** desktop ctest 368/368; docs, sizes, architecture pass; 682 units build without the PCH.

## 2026-11-23 — Windows package; branching stays read-only (D624)

- **Package:** *Package binaries* run #144 on this branch (`088955e`), both jobs green (their ctest passed on Linux and
  Windows); artifacts `multra-windows-x64` and `multra-linux-x86_64`, kept to 2026-12-07.
- **Owner, on the package:** asked whether branching should be adjustable like other conflict areas; after reading
  Vissim's manual ([review](../evidence/vissim-branching-review.md)): "คงไว้แบบนี้ (แนะนำ)". **D624:** branching stays derived
  and read-only; a visibility key waits for a study and a cited default (Vissim prints none).
- **Item 54 passed:** the owner, after watching the T-junction's one-lane minor approach on Windows: "สมจริง".
- Docs only; `multra-check-docs` passes.

## 2026-11-23 — M3.2 closed by the owner (D625)

- **Owner:** item 54 "สมจริง"; then, shown each gate piece with its evidence, "ตกลง" to closing M3.2.
- **D625:** ROADMAP, M3_ACCEPTANCE and NEXT say closed. The one Windows look with no verdict of its own (M3.2.4e merge
  mouth, M3.2.12 stop lines and tool) was added to M3.2.13 as item 4 rather than read into A2. Still open on their own
  gates: M3.2.10, M3.2.13, M3.2.8a.1, M3.4, M6; the not-yet-validated marker stays.
- Docs only; `multra-check-docs` passes.
