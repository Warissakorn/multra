# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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

## 2026-11-18 — M3.2.7d attempt 1: step 1 fails on merge waiting lines

- **Owner, on Windows:** conflict areas display correctly; the westbound stop line should be east of its area and the Connector's merge stop lines before the Link. Recorded verbatim in [OWNER_SITTING](plans/OWNER_SITTING.md) A2 and kept as a failure.
- **Measured in the fixture:** the merge waiting lines (`wait-13/14/17/18`) sit at the D50/D59 one-metre strip at the Connector ends, on the major lanes' centre lines; the crossing's lines are correct. This is the planned M3.2.4e (merge admission over the physical mouth), not a fixture typo: moving the lines alone would leave admission at the strip.
- No code change; a fill-in sheet for the exercise was given to the owner outside the repo.
- **2026-11-19, steps 3–4:** gap/headway plausible; conflict washes too faint; the minor creeps to its line and lurches. Measured (seed 42, probes outside the repo): a held minor crawls ~20 s and rests 2–3 m short in `approaching`/`following` (F1a binds only in `braking`); release is 2.5 m/s² in one tick, re-blocking brakes it at up to 3.6 m/s². Screenshots of alpha 80/95 against 180/200 went to the owner. The owner's double click was not reproduced on Linux (every Conflict areas button acted on one click). Nothing changed in code; NEXT holds the three asks.

## 2026-11-18 — Docs context: PROGRESS and NEXT compacted; AGENTS' RECORD size

- **Pick:** the owner ran the optimisation workflow on docs and AI context and chose items 1 and 3 ("ทำข้อ 1 กับ 3 เลย"), then item 2 after a draft.
- **PROGRESS:** 29 entries against AGENTS' "about ten"; the 19 oldest (D558–D575 and the Windows CI fix) moved verbatim to the top of [`PROGRESS-LOG-2026-11-14.md`](archive/PROGRESS-LOG-2026-11-14.md), its links rebased to `../`, and its index row updated. Estimated 8,600 → 3,100 tokens (docs_audit's character heuristic).
- **AGENTS:** RECORD.md is ≈53k tokens by D584, not ≈28k.
- **Item 2 (owner: "ตามแนะนำ"):** NEXT 444 → 358 lines, ≈9,100 → ≈6,900 tokens: finished-slice history already in PROGRESS and RECORD moved verbatim to [`NEXT-state-2026-11-18.md`](archive/NEXT-state-2026-11-18.md) (links rebased; a check found no line lost). The M9 next-pick list was stale (A2b, A4 and M11.5 done) and went; D537's M6 ask is settled by D544 and went, D535's stays. The clamp line now states the measured count (seeds 42–51: M2.6 1, M6-S1 1, lane lab 0) and that D252's lab clamp is at seed 74.
- **Found and fixed:** D583's rows reused BA179–BA180, held by the weaving lab since D449; renumbered BA209–BA210.
- **D585 (owner: "ทำการตรวจทั้งสองแบบเลย"):** `multra-check-docs` now fails past twelve PROGRESS entries and when a BA row id names tests in more than one group (self-test fixtures for both; each fails on a deliberate violation in the real tree). It found an older collision: merge clamps' BA158, BA159 and BA162 (MERGE_CLAMPS) and D444's rows of the same ids (DRIVING_BEHAVIOUR §3, `lanechangeduration`). BA30 and BA39 are exempt by their own text. Then (owner: "เปลี่ยนเลย") D444–D448's five rows, the later ones, became BA211–BA215 in DRIVING_BEHAVIOUR §3, RECORD D444–D448, their evidence and test names (bodies unchanged), and the known-collision list was removed; merge clamps keep BA158–BA167. The archive keeps the old numbers as written.
- **Verified:** `multra-check-docs` and `multra-check-file-sizes` pass. No code change.

## 2026-11-18 — The step reuses insertion's snapshot when it is exact (D584)

- **Pick:** the owner ran the optimisation workflow and chose item 1: "ทำข้อ 1 เลย".
- **D584:** `insertArrivals` hands its refs, spans and buckets to the step (`detail::Snapshot`, `tick.hpp`); the step takes them when no arrival was inserted, the survivors were already in id order, there is no parking lot and no source route decision was applied, and rebuilds otherwise.
- **Measured (Release headless, GCC 13.3, seed 42):** study 35,057 → 32,756 M Ir (−6.6 %), wall 2.76 → 2.55 s (medians of 5); M2.6 3,907 → 3,845 M Ir (−1.6 %), wall in noise. Numbers in [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).
- **Same results:** a probe hashing every event (kind, time, vehicle, segment, position, speed, acceleration, route ids) matches before and after for every shipped project and the study at seeds 42 and 43; the study summary is identical.
- **Verified on Linux (headless):** `check` 241/241. Windows not run (no UI change).

## 2026-11-18 — The floor behind a standing vehicle aims at the cap (D583)

- **Pick:** the owner asked for the fix proposed after the trace: "แก้ตามที่เสนอเลย".
- **D583 (LINE_APPROACH §8):** `standingVehicleFloor` stops a vehicle braking behind a standing vehicle at the standstill gap; a remaining cap hit behind it within the type's maximum is braking (`stopAtLine`), not a clamp. Same scope as D258.
- **Measured:** study, seed 42: clamps 1 → 0, completed 6,627. Every shipped project, seeds 42–51, and `multra-cli 42` identical except the build commit.
- **Tests:** `stopfloor.ba209_…` and `stopfloor.ba210_…` (first numbered BA179–BA180, which the weaving lab already held; renumbered the same day), each forcing its clamp with the switch off.
- **Verified on Linux (headless):** `check` 241/241. The desktop preset and Windows were not run (no UI change).

## 2026-11-18 — The study's last clamp traced (no code change)

- **Pick:** the owner asked for the cause of vehicle 3915's clamp.
- **Cause:** D258's standing-obstacle floor (LINE_APPROACH §7) aims at the leader's rear, but behind a vehicle the hard cap is the standstill distance (2 m) short of it, so the floor almost never binds there. The prototype law alone under-brakes the last half metre behind standing 3913 (queued at a merge); 3915 meets the cap at 0.67 m/s.
- **Measured with a temporary patch (reverted):** the floor aimed at `gap − standstill` behind a standing vehicle gives a constant 2.81 m/s² stop; one clamp stays, 1 mm late at 0.17 m/s (the 0.01 m guard). Shipped projects, seeds 42–51, and `multra-cli 42` unchanged. Details in NEXT.
- **Verified:** probes only; no code changed.
