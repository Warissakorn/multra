# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-21 — PRIORITY_RULES slice 2: the Stop line tool adds conflict markers (D616)

- **Owner:** "ทำงานถัดไป".
- **D616:** the Stop line tool places a line with no area downstream (amends D595's refusal); with a line picked,
  Ctrl+right-click on another lane adds a marker at 3.0 s / 5.0 m, starting the rule; a marker drags along its path
  and Delete removes it, the rule with its last marker; one Undo step each (`priority_marker_commands.cpp`).
  Markers follow their roads (split, delete, copy, study-area cut) and show as green bars (`canvas_markers.cpp`).
- **Rows:** `priority_marker` PR7, PR8 (5 tests); `stop-line-ui` extended (place alone, add, drag, Delete, Undo);
  `stoplines` SL4 updated for the lone line.

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

## 2026-11-21 — Pointing shows a road's direction; a selected conflict area shows its roads (D611)

- **Owner (verbatim):** "ปรับให้การชี้ที่ Link และ connector แสดงเส้นขอบที่มีลูกศรสีดำ ส่วนการเลือกให้เป็นเป็นสีฟ้า
  แล้วปรับให้การเลือก conflict area ทำให้ link และ connector ขึ้นสถานะถูกชี้", then "ไม่ปรับสีถนน เพิ่มแค่ข้อ 2"
  (the rule in every tool; not hovering an area, no road colour change).
- **D611:** a hovered Link or Connector shows its long edges near-black (`#1F1F1F`) with arrowheads; a selected one
  stays the accent blue with blue arrows; under the Conflict area tool, the Links and Connectors a selected area
  runs on are drawn as hovered. D610's arrows on every edge under the right-of-way tools are withdrawn.
- **Tests:** `priority-canvas` (no arrows with nothing pointed at or selected; the selected area's Links black
  and pointing downstream; a selected Link blue) and `editor-interaction-ui` (hover black, selection blue; its old
  "hover draws no arrows" check reversed). Desktop `check` green, 363 tests.

## 2026-11-20 — Conflict areas in Vissim's colours; direction arrows on every edge (D610)

- **Owner, A2 attempt 2 step 3 (verbatim, with a picture):** "gap/headway สมเหตุสมผล แต่ต้องการปรับการแสดง conflict
  area ให้มีสีแบบในรูปและลูดศรชี้ตามทิศทาง ลักษณะคล้ายในรูป". In chat: arrows on every edge under the tool, no A/B
  labels, colours as bright as the picture.
- **D610:** sides with right of way `#00FF00`, yielding `#FF0000`, undecided `#C0C000` (canvas-only tokens);
  branching keeps the dark error ink, an automatic candidate its faint wash. Under the Conflict area and Stop line
  tools every Link and Connector edge is a near-black line (`#1F1F1F`) with arrowheads along travel.
- **Tests:** `priority-canvas` checks the three colours and arrows on every Link and Connector under both tools,
  pointing the digitised way, and no unselected edge under Select; it failed before the change. Desktop `check`
  green, 363 tests. Step 4 and the verdict are still the owner's.
- **D610 (b), 2026-11-21:** owner (verbatim) "เหลือลูกที่กึ่งแล้วแนวที่เกิด conflict แต่ละแนว": each band carries a
  dark centre line with an arrowhead at its exit end; `priority-canvas` checks every band has one, the major
  road's point downstream, and none under Select (failed before).

## 2026-11-20 — G1 start-up as Vissim: acceleration by speed, lack of attention (D608)

- **Owner (verbatim):** "เริ่ม G1 ทำเหมือน vissim", then "ก+ข".
- **D608:** a vehicle type's optional `accelerationCurve` and a behaviour's optional `inattentionDuration` /
  `inattentionProbability` (schema 81); editors in the type dialog and the behaviour editor.
- **Unchanged without the keys:** `multra-cli 42` on M0 keeps 45 / 31.75870154816726 / 1 clamp.
- **Measured** on a saturated copy of M0 (START_UP §5): lost time today 1.53 s; Vissim-like values 0.96 s; a
  2.0 m/s² standstill curve 2.16 s with saturation headway 2.01 s (HCM 2.0 s / 1.9 s). SU9 (benchmark) not measured.
- **Owner, "ตามแนะนำ" (D609):** shipped data keeps no curve (a study's choice); next A2, then G3, then priority rules.
- **Fixed after the commit:** the `visibility` and `startup` groups joined `all-model-tests`; the W74 branch of
  `validate.cpp` no longer reports `missedExitAfter` twice (a check in `lanechange`, failing before the fix).
  Headless `check` green, 246 tests, before that fix; `lanechange` 28/28 and the docs check after it.
- **SU8 run on desktop:** the type dialog's curve used 2 m/s² above the 1.75 the same test set, so the dialog
  (rightly) refused it and the test hung; values now under it. The behaviour editor's attention pair joins the
  library UI test (its file now saves at 81). Desktop `check` green, 363 tests.
- **Also:** stop-line tests pinned to their own schema step (79) instead of the newest; `right_of_way.cpp` kept
  under 500 lines.

## 2026-11-20 — Visibility at conflict areas (D607); realism study against Vissim, SUMO, Aimsun

- **Owner (verbatim):** "V ทำเหมือน vissim เริ่มตัดสินใจเรื่อง gap ทำด้วยหรือไม่", "1ก 2ก", "ทุกไฟล์"; then "ศึกษาส่วนอื่นที่ต้อง
  เพิ่มอย่างละเอียด เพื่อให้ ฐาน M0 สมบูรที่สุดด้วย" and "ลองศึกษาจาก Vissim SUMO Aimsun มาต่อยอดเพื่อให้พฤติกรรมสมจริงและมี
  ประสิทธิภาพมากที่สุด".
- **D607:** a priority rule's `visibility` (schema 80, absent = 100 m); beyond it the yielding driver plans to
  stop at its line, within it the gap test decides; commitment unchanged. Dialog field per group, one Undo.
- **Measured:** at 100 m every shipped project and the study are identical (batch means, seeds 42–51;
  single-seed reports, 42–43), so M0 does not move; below about 50 m minor delay rises (VISIBILITY §4).
- **Study:** [realism-gaps-vissim-sumo-aimsun](evidence/realism-gaps-vissim-sumo-aimsun.md): G1 start-up delay
  (M0's lost time 0.89 s against HCM 2.0 s) and G2 speed-dependent acceleration proposed first. No code.
- **Verified:** see the commit; Windows not run.

## 2026-11-20 — Owner's choices on the sitting's findings (D603–D605)

- **Owner (verbatim):** "1 ข. และ เครื่องมือเส้นหยุดสามารถสร้างได้โดยไม่ต้องใช้ Conflict area ก็ได้ ให้สร้างพื้นที่ให้ทางเหมือน vissim
  · 2 แก้ตามแนะนำ"; then "ข. Priority rule แบบ Vissim".
- **D603:** waiting lines carrying Stop or Yield are drawn only under the Conflict area, Stop line and Queue
  counter tools (amends D590); `priority_canvas_tests` checks a Stop line is gone under Select and marked under
  the Stop line tool.
- **D605:** block-selecting tables draw one outline around each selected block over a light tint, from an
  overlay (`tableSelectionOutline`); `table-grid-ui` checks a 2 × 2 block gives only its eight perimeter edges.
- **D604:** [PRIORITY_RULES](reference/PRIORITY_RULES.md) drafted: stop line plus conflict markers without a
  conflict area, compiled to the engine's existing `PriorityRule`; the next schema step proposed, rows PR1–PR10. Five
  owner choices (§8) come before code.
- **Verified on Linux (desktop, offscreen):** `table-grid-ui`, `priority-canvas` and `stop-line-ui` pass; full desktop suite 361/361. Windows not run.

## 2026-11-20 — Windows sitting, part 1: seven looks answered

- **Owner's words (verbatim, by the numbers of the list sent in chat):** 1 (A2 step 1) "ตอนออกจาก เครื่องมือ
  conflict area แล้วยังมี เส้นหยุดให้เห็นอยู่" · 3 (C15 + E33) "ตรง" · 4 (E34 with F4) "กดถึงได้ตลอด" · 5 (C14)
  "นิ่ง" · 6 (E35 with C1, C7, E18) "สามารถ copy และวางบน Excel ได้ แต่ตอนครอบ ส่วนที่เลือกครอบไม่มีกรอบการเลือกคุม
  ทั้งหมดคุมแต่ช่องสุดท้าย" · 7 (E36 with E5, E8) "สามารถทำได้" · 8 (B45 with E13–15) "แยกได้". Item 2 (E32) not yet answered.
- **Closed and removed from the sheets:** C14, C15, E33, E34, E35, E36, B45 and the items they took in (D602).
- **Open:** A2 (steps 3–4 and the verdict; the step 1 finding is D590's rule for controlled lines, an owner
  choice); the E35 selection defect (NEXT). No code changed.

## 2026-11-20 — Windows owner checks triaged before new work (D601, D602)

- **Owner:** "ต้องการ เคลียร์ owner checks บน Windows ก่อนเริ่มงานใหม่"; chose "Triage first", "Yes, cut it"
  for M3.2.7d, then "Approve as proposed".
- **D601:** M3.2.7d's owner exercise is steps 1, 3–4 and the verdict. New test
  `tjunction.an_edited_fixture_saved_and_reopened_repeats_its_run_on_the_same_seed` (gap 4 s, headway 9 m,
  Stop; the reopened file is byte-identical, seed 42's rows equal) covers steps 5–7; step 2 was already in
  `rightofway_editor`. M3_ACCEPTANCE §3 updated.
- **D602:** OWNER_SITTING_WINDOWS rewritten as one sitting of 17 items with an order; 31 looks listed more
  than 14 days ago carved out to the new plans/OWNER_LOOKS_CARVED (ROADMAP M1.30, M3.2.13, M14), which also takes D201's M1.29/M3.2.10 text so ROADMAP stays under 500 lines; I.1, I.3, I.4, F2, D2, E22 closed; merged
  items say what they took in. No look was passed by the session.
- **Verified on Linux (headless):** `tjunction` 9/9; `check` 244/244 (in a fresh build directory `cli-consistency` first fails: `check` does not build `multra-consistency-cli-fixture`, a gap that predates this session). Desktop preset and Windows not run; the sitting is the owner's.
