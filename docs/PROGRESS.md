# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-21 — G7 slice 1: a waiting driver accepts less, Aimsun's gap reduction and SUMO's impatience (D618)

- **Owner (verbatim):** "G7 ทำแบบ Aimsun"; on the follow-on, "ในสถานะการที่รถทางรองออกแล้วรถทางหลักหยุด แล้วรถทางรองคันถัดไป
  ออกต่อกันกับคนแรก เนื่องจากมีโอกาสจากรถทางหลักหยุด"; "ทบทวนความเต็มใจให้รถทางหลักชะรอของ sumo"; "ทำทั้งสองส่วนตามแนะนำ".
- **Read first** (hosts allowed for this): Vissim 2025's stop-sign and conflict-area help, SUMO's docs and `MSLink.cpp`, Aimsun
  Next's manual. G8 needs nothing (all three hold a regular stop for a step or stop fully); Vissim has no G7 key.
- **D618, [YIELD_IMPATIENCE](reference/YIELD_IMPATIENCE.md):** schema 84; Aimsun's linear gap reduction over a per-vehicle
  hashed yield time, set per rule or marker; SUMO's impatience at conflict areas only, so a standing major vehicle lets the
  next minor vehicle follow. Off by default; `multra-cli 42` keeps 45 / 31.75870154816726 / 1.
- **Verified (Linux, headless):** `yield_impatience` YI1–YI9 pass; `ctest --preset headless` 250/250; docs and sizes pass.
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
