# PROGRESS archive — 2026-11-21

Whole entries moved from the live PROGRESS log.

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
