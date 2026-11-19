# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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

## 2026-11-18 — Lane locks closed: the owner accepts the defaults (D582)

- **Owner:** "ยอมรับค่าเริ่มต้นทั้ง 3 ข้อ ใช้ตามแนะนำ": `entryLanes` on the routing decision, no shipped `missedExitAfter`, a diverted trip's delay on the movement it arrives on and `diverted` on the counted one. ROADMAP M3.2.11 is done; LANE_LOCKS §7 is answered; OWNER_SITTING B50 is closed.
- **Verified:** docs checks only. No code changed.

## 2026-11-18 — Option B: missed exit after a dead-end wait, schema 78 (D581)

- **Pick:** "ทำต่อเลย" after A: B, off by default (B50). A probe first: all 24 stubs of the study's spread layout have a full route driving their exact segments, so a diverted vehicle keeps its route distance.
- **D581:** a behaviour's `missedExitAfter` (schema 78, (0, 3600] s). After that many whole ticks waiting at its dead end, the vehicle takes the full route sharing its lane (tie: most shared segments, then route id); none: it keeps waiting. `src/core/missed_exit.*`, `MissedExitEvent` appended to the event variant and handled by output (`missed-exit`), discharge passages and priority lines; `diverted` per run and per counted movement only with the key. Behaviour editor: a `missedExitAfter` field.
- **Measured (probe, spread layout, seed 42):** 60 s: completed 4,698 → 5,182, diverted 165, longest wait per dead end 59.9 s, clamps 6 → 2; 120 s: 4,944, 53, 119.9 s. LL3: every shipped project, seeds 42–51, and `multra-cli 42` identical except the build commit. No threshold is set in the study or any shipped file.
- **Tests:** `lanechange.a_vehicle_held_past_missed_exit_after_takes_the_full_route_on_its_lane` (50th waiting tick, replay, accounting), `lanechange.without_missed_exit_after_or_an_onward_route_the_vehicle_keeps_waiting` (key absent, LL6, gap before the threshold, range), `lanechange.missed_exit_after_round_trips_at_schema_78_and_reaches_the_run`.
- **Verified on Linux:** headless `check` 241/241; `behaviour_editor.cpp` and `editor_decision.cpp` compile under the desktop preset. Neither dialog was opened; the full desktop build and Windows were not run.
- **Owner questions open:** §7 q3 (threshold) and q4 (diverted delay); defaults recorded in D581. LL4 reworded: `diverted` is not added to the accounting sum.

## 2026-11-18 — Option A: entry lanes by destination, schema 77 (D580)

- **Pick:** the owner's order (B50): A after LL1. The 6 clamps were checked first: 3 come 0.3 s after the clamped vehicle's own lane change into a standing queue on the entry Link; 3 show no change of its own.
- **D580:** a routing decision's `"entryLanes": "serving"` (schema 77) puts each destination's share on the entry Link's lanes with a full chain to it; stubs only where none has one. Compile only (`entryDecision`), refused with `position` or without `linkId`. Editor: a checkbox in the decision dialog (en/th). `multra-count-study` reads `entrylanes,serving` from `site.csv`, and the study uses it.
- **Measured, seed 42 (study):** completed 4,698 → 6,627, active 539 → 428, pending 5,503 → 3,666; dead-end waits 187 / 19,600 s → 0; clamps 6 → 1 (vehicle 3915 on a Connector at 14 m, no lane change of its own: open). LL3: every shipped project, seeds 42–51, and `multra-cli 42` identical to the pre-change binary except the build commit.
- **Tests:** `pocketdecision.serving_entry_lanes_put_each_destination_on_its_full_chains` (LL5, both driving sides), `pocketdecision.entry_lanes_is_refused_where_it_means_nothing`, `countstudy.serving_entry_lanes_leave_no_stub_on_an_entry_link`; LL1's test now runs on the study without the row.
- **Verified on Linux:** headless `check` 241/241; the decision dialog (`editor_decision.cpp`) compiles under the desktop preset. The dialog was not opened, the full desktop build and Windows were not run.
- **Owner question open:** LANE_LOCKS §7 q2 (key on the decision or on the Link's lanes) was not answered; the decision was taken as the default and is recorded in D580.

## 2026-11-18 — LL1: a dead end past the point of no return (D579)

- **Pick:** the owner answered B50 with "ทำตามแนะนำ": A, then B off by default; not D. The contract's first row, LL1, ran first.
- **Found:** a left-turn stub's dead end was the end of its own lane (280 m), not where the kerb lane's left Connector leaves (273 m). A span outward to another stub had carried it on. A vehicle past 273 m could only change to a stub that could no longer turn left, so it stood for good (vehicle 848).
- **D579:** `appendLaneChanges` sets the dead end to the last metre from which a change still leads to a full chain. It uses a fixed point over the family's spans: a span onto a stub counts as far as that stub's own dead end, carried back. Spans are cut at the dead end. The engine is unchanged.
- **Measured, seed 42:** study completed 3,385 → 4,698, never entered 6,754 → 5,503, waiting 36,267 → 19,600 s (longest 3,411 → 2,683), clamps 1 → 6. Every shipped project and `multra-cli 42` give byte-identical output. Test `countstudy.a_left_turn_stub_is_held_before_its_turn_leaves` fails without the fix.
- **Verified on Linux (headless):** `check` 241/241. The desktop preset, check_no_pch.py and Windows were not run.

## 2026-11-17 — Purpose review; lane locks recorded and a draft contract (D578)

- **Pick:** the owner asked whether the stuck vehicles matter for future development and for a review of the test's purpose, then asked for both to be recorded and a contract drafted.
- **Purpose:** M2.6/M5.7 test whether the owner can build and report a study in the editor. Tuning a builder-made project's lanes and timing for throughput is not that test, and it is close to calibration (D544). The study stops being tuned and is kept as a failing case.
- **D578:** lane locks are recorded as an engine defect: ROADMAP M3.2.11 (proposed), OWNER_SITTING B50 and draft [LANE_LOCKS](reference/LANE_LOCKS.md). The draft has the evidence (study: 82 waiting, longest 3,411 s; shipped projects at most 42 s), the four options (entry lane by destination, missed exit, zipper, removal) under fixed constraints (replay, M3 §4, accounting, byte-identical shipped files), a recommendation (A then B), rows LL1–LL8 and the owner's questions. Nothing is implemented.
- **Verified:** docs checks only (`multra-check-docs`, file sizes). No code changed.

## 2026-11-17 — Study lanes from the owner's drawing, two right-turn lanes (D577)

- **Pick:** the owner asked for a check of a junction drawing (D576's lanes were from the aerial image), then for items 1, 2 and 4: lanes from the drawing, more than one right-turn lane, and a redesigned plan. Item 3 (each movement entering on its own lanes) was not picked.
- **Measured on the drawing** (pixel widths at 19.4 px a lane; arrows at low resolution): N 5/3 with two right lanes, S 4/3 with one right and one through-or-right lane, NW 5/3 with one right lane and a channelised left slip, SE 3/3. D576 had N 3 and NW exit 4.
- **D577:** `FourLegOptions::rightLanes` (default 1). With two or more, every stop-line lane has its own upstream lane, because a destination takes one lane chain per entry lane: a lane split into two right lanes left the inner one empty (seen: 0 vehicles). Shipped files are byte-identical. The study is now N 5/3/2, S 4/3/2, NW 5/3/1, SE 3/3/1. Sheet redesign: Y 0.81, Webster cycle 155 s, greens S 0–16, SE 20–100, N 104–118, NW 122–151.
- **Result, seed 42, one run, not validated:** worse again. Completed 4,014 → 3,385, never entered 6,241 → 6,754, clamps 1. Runs are byte-identical. A probe found 22 blocked lanes, each behind a vehicle waiting at a lane end for a lane change into a standing queue: at the end of the upstream Links, and at 38 of 40 m where left-turners missed the left connector. More single-movement lanes mean more forced changes. Item 3 is now the condition for any lane layout to work.
- **Verified on Linux (headless):** `check` 241/241; the shipped four-leg and M2.6 files regenerate byte for byte. Desktop preset, check_no_pch.py and Windows not run.

## 2026-11-17 — Study lanes from the image (D576)

- **Pick:** the owner asked to match the lane counts first and answered "use the image" for each value.
- **D576:** `FourLegOptions` gains lanes per leg (`approachLanes`, `exitLanes`) and `stopLine`. The defaults give the old drawing: the shipped four-leg and M2.6 files regenerate byte for byte. `site.csv` takes `lanes` and `stopline` rows. The study is now N 3/3, S 4/3, NW 5/4, SE 3/3 with the stop line at 20 m. The Sheet's design was recomputed with these lanes: Y 0.94 → 0.87, greens S 0–18, SE 22–111, N 115–141, NW 145–176.
- **Result, seed 42, one run, not validated:** it got worse. Completed 4,422 → 4,014, active 302 → 464, never entered 6,012 → 6,241, mean delay 996 → 829 s, clamps 1 → 2; two runs byte-identical. NW through fell 477 → 224 with its queue nearly full (mean 243 of 283 m). A probe with a 4-lane SE exit (no 4→3 merge) gave only 282. The cause is open: lane choice on a queued multi-lane approach is the next suspect, and motorcycles still take a full lane slot.
- **Verified on Linux (headless):** `check` 241/241; `fourleg` and `m26study` unchanged. Desktop preset, check_no_pch.py and Windows not run.
