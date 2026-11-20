# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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

## 2026-11-19 — Optimisation pass: measured, one item per commit (D597–)

- **Owner:** "/optimize-program", all four dimensions, then "ทำตามแนะนำ" (items 1–6 in the recommended order).
- **Baseline** (Linux GCC 13.3, Release `-g1` callgrind, seed 42): Ratchada study 32,935 M Ir, M2.6 3,841 M,
  `multra-engine-benchmark 48 900` 14,578 M; desktop ctest 361/361; 44 compiler warnings.
- **Same results:** a probe hashing every event (`eventJson`) of every shipped project that runs, the study and the
  12-crossing benchmark corridor, seeds 42 and 43, matches the baseline after each engine item; CLI summaries byte-identical.
- **D597:** `publishMoves` finds the joins crossed this tick by binary search over the ascending part starts
  instead of reading every part of the route. Benchmark 14,578 → 10,363 M Ir (−28.9 %), study −0.6 %, M2.6 −0.9 %.
- **D598:** `frontPart` bisects routes of more than 8 parts and keeps the scan for shorter ones. Benchmark
  10,363 → 9,786 M Ir (−5.6 %); study 32,730 → 32,889 M (+0.5 %) and M2.6 +0.5 %: GCC no longer inlines the
  scan into `locate`. Bisecting every route cost the study +1.8 %, so the short-route scan stays.
- **Item 3, reverted:** a leader search by rear order cut the study 7.6 % but cost M2.6 2.4 % and the
  benchmark 5.0 %; measurements in ENGINEERING_NOTES. No D-row: nothing changed.
- **D599:** compiler warnings 44 → 0 (desktop Debug, GCC 13.3). 17 were GCC 13's `-Wdangling-reference` on
  lookups that return into their container (each checked: none dangles), now off for GCC before 14 only; the rest
  fixed in place (range-loop copies, misleading indentation, an unused parameter, two dead test helpers removed,
  two shared test helpers `[[maybe_unused]]`, one sign comparison). No behaviour or assertion changed.
- **D600:** one `isBlank` (`src/core/text.hpp`) replaces the blank-id rule written 24 times (12 identical
  `blank()` helpers, 12 inline copies) in `core`, `model` and `project`.
- **Final (same tree as committed, event hashes identical):** benchmark 14,578 → 9,786 M Ir (−32.9 %), study
  32,935 → 32,889 M (−0.1 %), M2.6 3,841 → 3,827 M (−0.4 %); warnings 44 → 0; 361/361. Linux only; MSVC wall
  time is the owner's check (NEXT).
- **NEXT cut to open items again (D319):** the day's done slices (M3.2.4e, STOP_LINES, line approach), the
  M9–M12 done narration and M13's if-changes checklist moved whole to
  [archive/NEXT-state-2026-11-19](archive/NEXT-state-2026-11-19.md); NEXT ≈7,666 → 6,378 tokens (−17 %, estimate
  by characters). Every open item, owner ask, command and D-id is kept.

## 2026-11-19 — A held vehicle rests at its zone line: no crawl, no lurch (D596)

- **Owner:** "ทำเรื่องรถคืบแล้วกระตุกที่เส้นต่อได้เลย".
- **Measured first** ([LINE_APPROACH](reference/LINE_APPROACH.md) §9, T-junction seeds 42–51, env-switched patch, reverted): the crawl is the law's `√v` term in the desired gap and the 2 m is `standstillDistance`. Chosen: behind a conflict zone's hold the desired gap is 0. Crawl 194.7 → 13.8 s per seed, rest median 2.65 → 0 m, go-then-brake 15 → 0, clamps 0. The kinematic floor in every mode was measured and not taken (more go-then-brake).
- **D596:** `Leader::zoneLine` on zone holds only; a vehicle resting at its line is not `committed`; a stop or residue ending at the line rests without a clamp. Signal heads, vehicles, derived rules and W74 unchanged; the four-leg and M2.6 pins pass unchanged.
- **Rows:** new `zonelinerest` (LA1–LA4, failed with the flag off); BA146, BA119, BA121 and PL8 updated, reasons in LINE_APPROACH §9.
- **Verified on Linux (desktop preset, Debug):** full `ctest` 361/361; changed units build without the PCH. Windows not run.

## 2026-11-19 — STOP_LINES slice 3: the Stop line tool (D595)

- **Owner:** "ทำส่วนที่ 3 ต่อได้เลย".
- **D595:** the Stop line tool (`W`, after Conflict area in the palette; tool 15 appended so other indices stay). Ctrl+right-click a lane places a line (`placeStopLine`) named on the decided default-line areas at or past it on its path, or on a Connector leaving its Link lane; refused (`EDIT_NO_CONFLICT_DOWNSTREAM`) when there are none. A click picks and drags, Delete removes (`removeStopLine`). The Conflict areas table gains Waits at and Use default lines; under the Conflict area tool the hovered or selected area's default lines are drawn dashed and faint. English and Thai text.
- **Rows:** `stoplines` SL4 and the new `stop-line-ui` (SL4 in the table, SL7, place/refuse/undo/pick/Delete/Use default lines). `workspace-ui`, `design-system-ui` and `zone-ui` pin the palette and were updated for the sixteenth tool. [STOP_LINES](reference/STOP_LINES.md) §9 lists what still differs from the draft.
- **Verified on Linux (desktop preset, Debug):** full `ctest` 360/360 after the palette pins were updated; the 17 changed `.cpp` units build without the PCH. Windows not run.

## 2026-11-19 — STOP_LINES slices 1–2: derived waiting lines, schema 79 (D594)

- **Owner:** "ทำ STOP_LINES ต่อได้เลย".
- **D594:** a conflict side with an empty `waitingLineId` waits `rightOfWay.frontStopDistance` (default 1 m, (0, 10]) short of its entry (`derivedWaitingStation`); schema 79. Add crossing areas, an automatic crossing, Take over merge and junction fills store no line. Stop/Yield on a derived side places one sign line before the first such area of its path and names it on the ones after. `deriveStopLines` (SL5) and its inverse `storeDerivedLines`. No migration at load; [STOP_LINES](reference/STOP_LINES.md) §9 lists where it differs from the draft.
- **Rows:** `stoplines` SL1–SL3, SL5, SL6 (stored and derived at the same places: equal zones, same seed-42 run), SL8. The T-junction regenerated: two sign lines (`wait-15`, `wait-17`) instead of six. Updated tests are listed in STOP_LINES §9.
- **Not yet:** the Stop line tool, coverage as a derived rule, faint derived lines on hover (slice 3). A derived line is not drawn at all today.
- **Verified on Linux (desktop preset, Debug):** full `ctest` 359/359; the 32 changed `.cpp` units build without the PCH. Windows not run.

## 2026-11-19 — Opaque conflict areas (D592); M3.2.4e merge admission over the mouth (D593)

- **Owner (2026-11-19):** "ความทึบ 100%  และเริ่ม M3.2.4e ได้เลย".
- **D592:** areas draw at alpha 255; an undecided automatic candidate stays faint. Waiting lines stay above.
- **D593:** [MERGE_MOUTH](reference/MERGE_MOUTH.md). `mergeMouth` gives a taken-over merge the measured overlap (first overlapping station on the joining segment to the join) and each side a line 1 m short of it; used by Take over merge, automatic candidates and D86's follow. Rows MM1–MM4 failed first (MM4: 266 ticks with a minor and a major body in one mouth, seed 42) and pass; MM5–MM6 guards. The T-junction file regenerated (`wait-14` 26.23 → 12.31 m, `wait-18` 24.01 → 6.99 m, `wait-13` 169 → 154.65 m, `wait-17` 169 → 151.36 m). Five tests that pinned the strip were updated to the new rule (`rightofway` ×3, `rightofway_runtime`, `tjunction`); D54 amended.
- **Not changed:** derived merges and the solver; stored files are not migrated.
- **Verified on Linux (desktop preset, Debug):** full `ctest` 357/357 passed except the group list, then `mergemouth` and `all-model-tests` pass after listing the group (358 tests); changed units build without the PCH. Windows not run.

## 2026-11-19 — Intermediate points (D589), areas under their tool (D590), STOP_LINES draft (D591)

- **Owner (2026-11-19):** the Intermediate points field was always red and a higher count did not round the Connector; wanted the field in the Connect dialog (default 0, a suitable value chosen); conflict areas only under the Conflict area tool; "ตามแนะนำ" to derived stop positions and a Stop line tool.
- **D589:** a spin box sets its own text with signals blocked, so the inspector's check never saw a value it put in (measured `validationState=invalid` at 3); the inspector now rechecks on refresh and a disabled field is never invalid. A Connector on its default curve is laid on the curve again at the new count (test `raising_the_count_on_a_default_connector_rounds_it`); a bent one keeps its line. The Connect dialog's field: 0 = automatic, the fewest points within 0.10 m of the curve.
- **D590:** areas and plain waiting lines draw only under the Conflict area tool (lines also under Queue counter); Stop/Yield lines always.
- **D591:** [STOP_LINES](reference/STOP_LINES.md), a draft for the owner; nothing switched on before M3.2.4e.
- **Verified on Linux (desktop preset, Debug):** full `ctest` 357/357 after updating `editor-gestures`, `priority-ui` and `priority-canvas` to D588/D590; changed units build without the PCH. Windows not run.

## 2026-11-19 — Row dialogs open once (D587); no white Connector edge (D588)

- **Owner, on Windows (M3.2.7d attempt 1):** Confirm and the close box of the Conflict area dialog each needed two clicks; a selected or hovered Connector still shows a white border; would 100 % opaque areas be better.
- **D587:** a double-click emits `cellDoubleClicked` and `activated` together, and five tables (Conflict areas, Queue counters, Travel-time sections, Zones, OD) had a slot on each, so their dialog opened twice. `openRowOn` connects `activated` alone. New test `row-open-ui` (Conflict areas and Queue counters, double-click and Enter) failed with the old slots and passes.
- **D588:** an emphasised Connector drops its plain solid outer edges as D460 did for Links; `canvas-layers` and `interaction` tests now expect none.
- **Not changed:** conflict opacity waits for the owner (screenshots at 35 %, 75 % and 100 % sent; D435). The line approach and M3.2.4e stay in NEXT.
