# PROGRESS archive — 2026-11-14

Whole entries moved from the live PROGRESS log.

## 2026-11-19 — Intermediate points (D589), areas under their tool (D590), STOP_LINES draft (D591)

- **Owner (2026-11-19):** the Intermediate points field was always red and a higher count did not round the Connector; wanted the field in the Connect dialog (default 0, a suitable value chosen); conflict areas only under the Conflict area tool; "ตามแนะนำ" to derived stop positions and a Stop line tool.
- **D589:** a spin box sets its own text with signals blocked, so the inspector's check never saw a value it put in (measured `validationState=invalid` at 3); the inspector now rechecks on refresh and a disabled field is never invalid. A Connector on its default curve is laid on the curve again at the new count (test `raising_the_count_on_a_default_connector_rounds_it`); a bent one keeps its line. The Connect dialog's field: 0 = automatic, the fewest points within 0.10 m of the curve.
- **D590:** areas and plain waiting lines draw only under the Conflict area tool (lines also under Queue counter); Stop/Yield lines always.
- **D591:** [STOP_LINES](../reference/STOP_LINES.md), a draft for the owner; nothing switched on before M3.2.4e.
- **Verified on Linux (desktop preset, Debug):** full `ctest` 357/357 after updating `editor-gestures`, `priority-ui` and `priority-canvas` to D588/D590; changed units build without the PCH. Windows not run.

## 2026-11-19 — Row dialogs open once (D587); no white Connector edge (D588)

- **Owner, on Windows (M3.2.7d attempt 1):** Confirm and the close box of the Conflict area dialog each needed two clicks; a selected or hovered Connector still shows a white border; would 100 % opaque areas be better.
- **D587:** a double-click emits `cellDoubleClicked` and `activated` together, and five tables (Conflict areas, Queue counters, Travel-time sections, Zones, OD) had a slot on each, so their dialog opened twice. `openRowOn` connects `activated` alone. New test `row-open-ui` (Conflict areas and Queue counters, double-click and Enter) failed with the old slots and passes.
- **D588:** an emphasised Connector drops its plain solid outer edges as D460 did for Links; `canvas-layers` and `interaction` tests now expect none.
- **Not changed:** conflict opacity waits for the owner (screenshots at 35 %, 75 % and 100 % sent; D435). The line approach and M3.2.4e stay in NEXT.

## 2026-11-18 — M3.2.7d attempt 1: step 1 fails on merge waiting lines

- **Owner, on Windows:** conflict areas display correctly; the westbound stop line should be east of its area and the Connector's merge stop lines before the Link. Recorded verbatim in [OWNER_SITTING](../plans/OWNER_SITTING.md) A2 and kept as a failure.
- **Measured in the fixture:** the merge waiting lines (`wait-13/14/17/18`) sit at the D50/D59 one-metre strip at the Connector ends, on the major lanes' centre lines; the crossing's lines are correct. This is the planned M3.2.4e (merge admission over the physical mouth), not a fixture typo: moving the lines alone would leave admission at the strip.
- No code change; a fill-in sheet for the exercise was given to the owner outside the repo.
- **2026-11-19, steps 3–4:** gap/headway plausible; conflict washes too faint; the minor creeps to its line and lurches. Measured (seed 42, probes outside the repo): a held minor crawls ~20 s and rests 2–3 m short in `approaching`/`following` (F1a binds only in `braking`); release is 2.5 m/s² in one tick, re-blocking brakes it at up to 3.6 m/s². Screenshots of alpha 80/95 against 180/200 went to the owner. The owner's double click was not reproduced on Linux (every Conflict areas button acted on one click). Nothing changed in code; NEXT holds the three asks.

## 2026-11-18 — Docs context: PROGRESS and NEXT compacted; AGENTS' RECORD size

- **Pick:** the owner ran the optimisation workflow on docs and AI context and chose items 1 and 3 ("ทำข้อ 1 กับ 3 เลย"), then item 2 after a draft.
- **PROGRESS:** 29 entries against AGENTS' "about ten"; the 19 oldest (D558–D575 and the Windows CI fix) moved verbatim to the top of [`PROGRESS-LOG-2026-11-14.md`](PROGRESS-LOG-2026-11-14.md), its links rebased to `../`, and its index row updated. Estimated 8,600 → 3,100 tokens (docs_audit's character heuristic).
- **AGENTS:** RECORD.md is ≈53k tokens by D584, not ≈28k.
- **Item 2 (owner: "ตามแนะนำ"):** NEXT 444 → 358 lines, ≈9,100 → ≈6,900 tokens: finished-slice history already in PROGRESS and RECORD moved verbatim to [`NEXT-state-2026-11-18.md`](NEXT-state-2026-11-18.md) (links rebased; a check found no line lost). The M9 next-pick list was stale (A2b, A4 and M11.5 done) and went; D537's M6 ask is settled by D544 and went, D535's stays. The clamp line now states the measured count (seeds 42–51: M2.6 1, M6-S1 1, lane lab 0) and that D252's lab clamp is at seed 74.
- **Found and fixed:** D583's rows reused BA179–BA180, held by the weaving lab since D449; renumbered BA209–BA210.
- **D585 (owner: "ทำการตรวจทั้งสองแบบเลย"):** `multra-check-docs` now fails past twelve PROGRESS entries and when a BA row id names tests in more than one group (self-test fixtures for both; each fails on a deliberate violation in the real tree). It found an older collision: merge clamps' BA158, BA159 and BA162 (MERGE_CLAMPS) and D444's rows of the same ids (DRIVING_BEHAVIOUR §3, `lanechangeduration`). BA30 and BA39 are exempt by their own text. Then (owner: "เปลี่ยนเลย") D444–D448's five rows, the later ones, became BA211–BA215 in DRIVING_BEHAVIOUR §3, RECORD D444–D448, their evidence and test names (bodies unchanged), and the known-collision list was removed; merge clamps keep BA158–BA167. The archive keeps the old numbers as written.
- **Verified:** `multra-check-docs` and `multra-check-file-sizes` pass. No code change.

## 2026-11-18 — The step reuses insertion's snapshot when it is exact (D584)

- **Pick:** the owner ran the optimisation workflow and chose item 1: "ทำข้อ 1 เลย".
- **D584:** `insertArrivals` hands its refs, spans and buckets to the step (`detail::Snapshot`, `tick.hpp`); the step takes them when no arrival was inserted, the survivors were already in id order, there is no parking lot and no source route decision was applied, and rebuilds otherwise.
- **Measured (Release headless, GCC 13.3, seed 42):** study 35,057 → 32,756 M Ir (−6.6 %), wall 2.76 → 2.55 s (medians of 5); M2.6 3,907 → 3,845 M Ir (−1.6 %), wall in noise. Numbers in [ENGINEERING_NOTES](../reference/ENGINEERING_NOTES.md).
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
- **D578:** lane locks are recorded as an engine defect: ROADMAP M3.2.11 (proposed), OWNER_SITTING B50 and draft [LANE_LOCKS](../reference/LANE_LOCKS.md). The draft has the evidence (study: 82 waiting, longest 3,411 s; shipped projects at most 42 s), the four options (entry lane by destination, missed exit, zipper, removal) under fixed constraints (replay, M3 §4, accounting, byte-identical shipped files), a recommendation (A then B), rows LL1–LL8 and the owner's questions. Nothing is implemented.
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

## 2026-11-17 — Counted study: Ratchadaphisek × Lat Phrao (D575)

- **Pick:** the owner asked for an M2.6-style study of this junction. They counted it (16 movements, 15:00–16:00, four classes) but did not survey the signal timing. The count sheet is a claude.ai Sheet. The timing plan was designed from the counts (Webster, split phasing, 180 s), not surveyed.
- **D575:** a new home `studies/` for counted studies: not installed, not a shipped example, not validated. `tools/count_study_network.hpp` builds on the M2.6 four-leg drawing from `counts.csv` (the Sheet's count tab) and `site.csv` (leg mapping and greens), with one `volumeFromCounts` input and one per-approach composition per leg. U-turns join the right turn because the drawing has no U-turn exit. `multra-count-study` writes the file, and test `countstudy` fails on drift.
- **First run (seed 42, one run, not validated):** 4,422 completed, 302 active and 6,012 never entered of about 10,700 counted. Mean movement delay was 996 s, with one safety clamp. Two runs were byte-identical. The model gives every approach two lanes widening to three, while the site has more. Motorcycles (about half the count) do not filter, and the flyover is absent, so this is a model limit, not a count result.
- **Verified on Linux (headless):** build and `check` 241/241, including `countstudy`. `CMakeLists.txt` is held at 500 lines. The desktop preset, check_no_pch.py and Windows were not run.

## 2026-11-15 — Code-review fixes to the schema slices (D574)

- **Pick:** the owner ran `/code-review` on D570–D573 and asked for its recommendations ("แก้ไขตามแนะนำ").
- **D574:** named floor constants in `document.cpp`; one `onlyKnownKeys` helper at `kLatestSchema` for the four Macro-set readers; the shared source row renamed `citedSource`; a Field attribute `atLeastOne` for the cost-source `anyOf`; three identical nonempty-text kinds merged; the refusal helpers in `tests/schema_refusal.hpp`; the `--help`, PROJECT_SCHEMA and tools README texts fixed.
- **Label check in `check`:** ctest `schema-labels` validates 118,858 labelled cases from 22 slices and all 18 projects in about 50–70 s; a deliberately wrong `phi` rule made it fail (6 disagreements). The envelope slice's labels target `projectEnvelopeSchema()` and are excluded, not relabelled. Linux CI installs `python3-jsonschema`.
- **Proof of no behaviour change:** the emitted schema is byte-identical to D573's except the `$defs` name `citedSource66`.
- **Deferred:** one kind for every "nonempty array of Object" rule (finding 7), proposed in NEXT.
- **Verified on Linux (headless):** `check` 240/240; the 16 changed units that use the PCH compile without it. Desktop preset and Windows not run; the CI change has not run on GitHub.

## 2026-11-15 — Owned mode-choice sets in the schema and feature-doc split (D573)

- **Pick:** the owner accepted D570–D572 and explicitly started modeChoiceSets on `claude/optimistic-bohr-lpfu98`; D571/D572's diffs were read first.
- **D573:** root modeChoiceSets 70, nests 71 and logsum 76 join the composed schema ([MCS1–MCS3](../reference/PROJECT_SCHEMA_FEATURES.md#owned-mode-choice-set-extension-d573)). Alternatives use inclusive anyOf: modeId, costMatrix or both. Only key membership moves through knownKey at 76; zero reference, blank/unique ids, nest ownership/membership and logsum coefficient rules stay with the codec, with codes/paths/issue order unchanged. Pins passed first on unchanged readers (project 114/114), before table/schema/codec edits; four new tests remain.
- **Doc split:** whole extension sections D566–D572 moved from PROJECT_SCHEMA_DEMAND to PROJECT_SCHEMA_FEATURES before adding MCS1–MCS3. Both files are indexed and every moved-section link is rewritten, including decision-record links; prior decision reasoning is unchanged. The files are 286/242 lines.
- **One-off jsonschema 4.26.0:** all 6,151 labels agree: 1,609 mode-choice cases, 4,501 D570–D572 cases, writer output at 70/71/76, all 18 TrafficSim projects under data/ and tests/ (independently enumerated), and 20 probes outside cases(). Probes: 9 both accept, 7 codec-only refusals, 4 both refuse. Schema-refused/codec-accepted is zero across all labels. Temporary tests/probes and CMake entries were deleted; removing nests/logsum restores the original MNL writer bytes.
- **Verified on Linux (headless):** build and check 238/238; project 115/115. The existing linker-only -Wl,--strip-debug cache flag fits the local disk; repository presets are unchanged. Desktop preset, check_no_pch.py and Windows were not run. NEXT briefs procedures 73/feedback 75; stop for owner review, without a PR or merge.

## 2026-11-15 — Period factors in the schema (D572)

- **Pick:** the owner accepted D570/D571 and explicitly selected `periodFactors` at schema 69 on `claude/optimistic-bohr-lpfu98`; D571's diff and D567's exactFields pattern were read first.
- **D572:** the root is false below 69, then a nonempty array of closed four-required-key rows, text ids and shares in [0, 1] ([PF1–PF3](../reference/PROJECT_SCHEMA_FEATURES.md#period-factor-extension-d572)). Only the codec's exact-key expression changes, through `networkCodec::exactFields`; text/share/sum checks and UNKNOWN_PERIOD/DUPLICATE_ID retain their order. Blank text, sums, known periods and unique pairs remain codec checks. Writer, version selection and stored forms are unchanged. Pins and refusal cases passed first on the unchanged reader (project 110/110), before tables or codec edits.
- **One-off jsonschema 4.26.0:** 4,540 labels agree: 1,104 PF cases, 3,397 D570/D571 cases, one writer output with 0/1 and exact-sum-one shares, all 18 TrafficSim projects under data/ and tests/ (independently enumerated), and 20 probes authored outside cases(). Probes: 13 both accept, 4 codec-only refusals (blank purpose, unknown period, duplicate pair, sum), 3 both refuse. Schema-refused/codec-accepted: zero across all labels. Temporary tests/probes and their CMake registration were deleted.
- **Verified on Linux (headless):** build and check 238/238; project 111/111, including three new tests. The local linker-only `-Wl,--strip-debug` cache flag is retained for disk capacity; no repository preset change. The desktop preset, check_no_pch.py and Windows were not run. NEXT briefs modeChoiceSets 70/nests 71 (and its existing logsum floor 76), awaiting owner review; no PR or merge.

## 2026-11-14 — Owned deterrence sets and cost matrices in the schema (D571)

- **Pick:** the owner explicitly selected NEXT's schema-67 slice on `claude/optimistic-bohr-lpfu98`; D570's diff was read first. Both roots have the same floor, so they stay one slice.
- **D571:** `deterrenceSets` and `costMatrices` join the composed partial schema with closed rows, codec-owned constants and numeric bounds, and the existing constraint floors 68/72. Deterrence reuses the trip-rate source table; matrix source allows citation only. Both codecs use `knownKey`; codes, paths and order stay unchanged. Blank/unique ids and n×n agreement remain codec checks. DS1/CM1 and refusal cases passed first on unchanged readers (`project` 106/106), before tables or codec edits; DS2/CM2 followed.
- **One-off `jsonschema` 4.26.0:** all 3,418 labels agree: deterrence 984, matrices 854, D570 1,559, three writer outputs (67, 68 and destination at 72, all with a null matrix cell), and every TrafficSim project under data/ and tests/ (18, independently enumerated). The temporary dump test and CMake registration were deleted.
- **Verified on Linux (headless):** build and `check` 238/238; `project` 108/108, including six new tests. The initial Debug link exhausted the environment's disk; a local CMake cache linker flag `-Wl,--strip-debug` let the same headless preset fit, with no repository preset change. The desktop preset, `check_no_pch.py` and Windows were not run. NEXT briefs `periodFactors` 69 after owner review; no PR or merge.

## 2026-11-14 — Owned trip-rate sets in the schema (D570)

- **Pick:** the owner said to continue as recommended ("ทำต่อตามแนะนำ"); D569's NEXT proposed `tripRateSets` first.
- **D570:** the root `tripRateSets` is `false` below 66; from it a nonempty array of closed set rows, a closed `source`, closed purposes and, per side, a branch on `method` (`regression` row, else the cross-classification row with `method` a const); `rate` ≥ 0. `trip_rate_codec.cpp` checks keys with `knownKey` on the same tables; code, path and order unchanged. Pins (TR1) passed first.
- **One-off `jsonschema`:** 1,559 labels, writer output with the two Ortúzar example sets, and all 18 projects agree.
- **Verified on Linux (headless):** build; `check` 238/238 (`multra-tests` group `project` 102/102). The desktop preset and `check_no_pch.py` were not run; Windows not run.

## 2026-11-14 — Macro settings and runs in the schema (D569)

- **Pick:** the owner asked to continue ("ทำต่อ"); D568's NEXT proposed `macroRuns`, whose rows embed the settings row `definition.macroSettings` uses.
- **D569:** `definition.macroSettings` is `false` below 51 and root `macroRuns` below 56; from them one shared closed settings row (0 < `targetGap` < 1, integer `maxIterations` 1–100,000) and closed run rows (`id`, `name`, `period`, `settings`). Both readers check membership with a new `networkCodec::knownKey`; codes and paths unchanged. Pins (MR1) passed on the unchanged readers first.
- **One-off `jsonschema`:** 1,557 labels, writer output, D566–D568's labels and all 18 projects (2 with Macro runs or settings) agree.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1671/1671, group `project` 99/99); `check_no_pch.py` on the changed units 6/6. Windows not run (no CI run on a branch push).

## 2026-11-14 — The root map in the schema (D568)

- **Pick:** the owner asked to continue ("พัฒนาต่อ"); D567's NEXT proposed the map first.
- **D568:** root `map` is `false` below 29; from 29 a closed object (`providerId` matching `^[a-z0-9-]{1,80}$`, `opacity` 0–1, `reference`) with a closed reference (`projectedCrs` a UTM `EPSG:326xx`/`327xx` zone, longitude −180–180, latitude −80–84, anchors within ±10⁷ m, rotation ±180°). `map_codec.cpp` checks keys with `exactFields` on the same tables; `EDIT_MAP_INVALID` unchanged. The CRS must also be the anchor's own zone: that stays the codec's. The root key table is renamed `rootFeatures` (zone layer, studies, map). Pins (MP1) passed on the unchanged reader first.
- **One-off `jsonschema`:** 4,636 labels, writer output (a southern zone at the range limits), D566/D567's labels and all 18 projects agree; no shipped project has a map.
- **Archive:** the oldest entry starts `PROGRESS-LOG-2026-11-14.md`; the 2026-11-13 file's index row was stale (it stopped at D540) and now lists all 35 entries' range.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1668/1668, group `project` 96/96); `check_no_pch.py` on the changed units 4/4. Windows not run (no CI run on a branch push).

## 2026-11-14 — Study periods, clockStart and the study area in the schema (D567)

- **Pick:** the owner asked for the next work ("ทำงานถัดไป"); D566's NEXT proposed these root keys.
- **D567:** root `studyPeriods` and `definition.clockStart` are `false` below 49, root `studyArea` below 55; from them closed period rows, a number clockStart, a closed area (`nodes` nonempty, `flowSource` typed/macro/meso, optional `gateFlows`) and closed gate flows taking `compositionId` or `modeId` through `unless`. D566's `exactKeys` moved to `networkCodec::exactFields`; the period, area and zone codecs all use it and take the file version. Codes unchanged; pins (SP1) passed on the unchanged readers first.
- **One-off `jsonschema`:** 2,888 labels, writer output (periods with clockStart; the SA2 corridor at 55, 58 and 59; a mode gate flow), D566's 9,668 labels and all 18 projects agree.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1665/1665, group `project` 93/93); `check_no_pch.py` on the changed units 7/7. Windows not run (no CI run on a branch push).

## 2026-11-14 — The root zone layer in the schema (D566)

- **Pick:** the owner asked for the next work ("ทำงานถัดไป"); D565's NEXT proposed the zone layer.
- **D566:** `zones`, `zoneConnectors`, `odMatrices` and `parkingLots` join each version branch beside `network`/`definition`: `false` below 31, nonempty arrays of closed rows from it. A connector picks the street row from 57 by its id key (`roadId` 57–58, `streetSegmentId` from 59); a matrix's either-or keys use a new field attribute `unless` (`if required [modeId] then compositionId false else required`, the same for `periodId` and the times); `generatedBy` from 70; lot counts are unsigned tokens. `zone_codec.cpp`'s `exactKeys` now reads the same tables; the ambiguity and `generatedBy` codes keep their order. Pins (ZL1) passed on the unchanged reader first. D565's unused `knownFields` helper in `parse.cpp` is removed.
- **One-off `jsonschema`:** 9,668 labels, writer output (period, mode and generated matrices; the street fixture at 57, 58 and 59) and all 18 projects (8 with zones) agree.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1662/1662, group `project` 90/90); `check_no_pch.py` on the changed units 5/5. Windows not run (no CI run on a branch push).

## 2026-11-14 — Vehicle classes and link behaviour types in the schema (D565)

- **Pick:** the owner asked for the next work ("ทำงานถัดไป"); D564's NEXT proposed these catalogs.
- **D565:** both `false` below 21 and closed rows from it (class: `id`, `vehicleTypeIds`; link type: `id`, optional `name`/`defaultBehaviourId`/`overrides` of `{classId, behaviourId}`). `parseBehaviourLibrary`'s three `requireKnownFields` lists became `networkCodec::knownFields` on the tables; `requireKnownFields` had no other caller and is removed. The `text()` sentences stay (a null name is `Expected text: name`). Pins (BC1) passed on the unchanged reader first.
- **One-off `jsonschema`:** 3,040 labels, a round-trip and all 18 projects agree.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1659/1659, group `project` 87/87); `check_no_pch.py` on the changed units 5/5. Windows not run (no CI run on a branch push).

## 2026-11-14 — Behaviour schema slice B2: floors and closure (D564)

- **Pick:** the owner asked for the next work ("ทำงานถัดไป"); NEXT's B2.
- **D564:** both behaviour rows close from 21; the 18 floored keys are `false` below their floors (28, 32–37, 41, 43, 44), with the floors as `network_fields.hpp` constants that `document.cpp`'s `reject*` calls use. `readOptionals` now iterates `behaviourOptionalNumbers` (name, member, floor, code) in the old order, removing D563's duplicate list. Floors pinned on the unchanged reader first (BH4). B3 had nothing left.
- **One-off `jsonschema`:** 14,076 labels, a W74 round-trip and all 18 projects agree.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1656/1656, group `project` 84/84); `check_no_pch.py` on the changed units 5/5. Windows not run (no CI run on a branch push).

## 2026-11-14 — Behaviour schema slice B1: the model tag and per-model rows (D563)

- **Pick:** continued "ตามแนะนำ" into B1, the slice D561 carved.
- **D563:** the composed schema's `definition.behaviours`: prototype row below 25, `if model == "w74"` W74 row else prototype from 25; `model`/`name` `false` below 21, `model` required from 21 (prototype-only enum at 21–24). Rows are built from core's `w74ParameterKeys`, `perceptionParameterKeys`, `lateralParameterKeys` plus one project-side list; `parseBehaviourLibrary`'s hand-kept `common[]` (equal to the table's non-core keys, checked) is replaced by `behaviourKey`. Rows open; floors are B2.
- **One-off `jsonschema`:** 7,036 labels, a W74 writer round-trip and all 18 projects agree.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1655/1655, group `project` 83/83); `check_no_pch.py` on the changed units 6/6. Windows not run (no CI run on a branch push).

## 2026-11-14 — Owner item 49: vehicle-type reader limits fixed (D562)

- **Owner:** "ทำต่อตามแนะนำ", read as accepting the session's recommendations on OWNER_SITTING 49, (a) and (b2), then continuing.
- **(a):** `parseSpeedRange` reads `min`/`max` through `networkCodec::value()`: `Missing field: min`, `Expected number: max`, `Field is null: min` replace the JSON library's message. What loads is unchanged.
- **(b2):** from 21 `parseBehaviourLibrary` also checks `desiredSpeed` (`vehicleTypes[i].desiredSpeed.<key>`) and each curve point (`….curve[k].<key>`) against the same table; the schema closes both from 21. Below 21, the installed catalog and bare definitions stay additive. All 18 projects in the repository still validate (`jsonschema`, one-off), 5,700 labels agree.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1652/1652); `check_no_pch.py` on the changed units 4/4. Windows not run (no CI run on a branch push).

## 2026-11-14 — Behaviour schema inventory, before code (D561)

- **Pick:** the owner replied "เห็นด้วย" to D560's report; read as agreement with the proposed order (behaviours next), not as an answer to OWNER_SITTING 49, which stays open.
- **D561 (docs only):** PROJECT_SCHEMA_DEMAND's behaviour inventory: the model tag (refused below 21, required from 21, `w74` refused below 25), the prototype and W74 rows and the keys each refuses, the both-model keys, closure from 21 by `common[]` plus the core lists, ten presence floors, census (no W74 row anywhere in the repository). Slices B1–B3 carved; B1 is next.
- **Item 49(b) reworded:** a refusal "from the next schema step" would cover almost nothing, since the writer picks a version from features; options (b1)–(b3) now state their effect.

## 2026-11-14 — Owned vehicle types in the composed project schema (D560)

- **Pick:** the owner asked for the next work again ("ทำงานถัดไป"); D559's NEXT proposed vehicle types.
- **Inventory finding:** the type row is closed from 21 by a second hand-kept key list in `parseBehaviourLibrary` (open, additive, below 21). That list is now the field table.
- **D560:** the schema maps the row (open < 21, closed from 21), closed `axles` and `pcu`, open `desiredSpeed` and curve points, `curve`/`kerbLanes`/`pcu` `false` below 40/42/49 (shared constants with the guards), `kerbLanes` an `integer-token`. `parseVehicleType`'s scalars and axles read through `value()` with unchanged sentences; kerbLanes, pcu and the curve keep their own codes. Pins (VT1, VT3) passed on the unchanged reader first.
- **One-off `jsonschema`:** 5,700 labels, 9 projects, 7 installed type files and a round-trip carrying every gated key agree.
- **Owner (OWNER_SITTING 49):** (a) `desiredSpeed.min`/`max` report the JSON library's message; (b) an unknown key inside `desiredSpeed` or a curve point is dropped on save (round-trip at the latest schema). `min: true` is refused, so the schema is not over-strict there.
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1652/1652, group `project` 80/80); `check_no_pch.py` on the changed units 7/7. Windows not run (no CI run on a branch push).

## 2026-11-14 — Owned modes in the composed project schema (D559)

- **Pick:** the owner asked for the next work ("ทำงานถัดไป"); D558's NEXT named owned modes first.
- **D559:** `definition.modes` is `false` below 49 and an array of closed `{id, name, compositionId}` text rows from it. `parseMode` takes its key list and count from the new `Object::mode` metadata but keeps its bare `EDIT_CATALOG_READ` (no `value()` sentences); `document.cpp`'s guard and the schema share `networkCodec::modeVersion`. Pins (MS1, MS3) passed on the unchanged reader first.
- **One-off `jsonschema`:** all 1,672 version-dependent case labels agree with the emitted schema; a project owning the two installed modes validates at 50 and is refused at 48. No shipped or test file owns modes.
- **Left codec-owned:** `odMatrices[i].modeId` and a vehicle type's `pcu` (same schema-49 guard).
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1649/1649, group `project` 77/77); `check_no_pch.py` on the changed units 5/5. Windows not run (no CI run on a branch push).

## 2026-11-14 — Owned compositions in the composed project schema (D558)

- **Pick:** NEXT's AI-automation "Next" (map Composition/type-share/interval rows), the only open item not waiting on the owner.
- **D558:** `networkCodec` gains composition, type-share and interval rows; `definition.compositions` is optional, never null; all three rows are closed at every version (the reader always was); no read gate (18/19 stay writer floors). `parseComposition` now reads through that metadata; its diagnostics were pinned by tests on the unchanged reader first (CS1). Coverage id ends `-compositions`.
- **Docs:** PROJECT_SCHEMA.md was at the 500-line limit, so its D363/D364 sections and the new one moved to [PROJECT_SCHEMA_DEMAND](../reference/PROJECT_SCHEMA_DEMAND.md).
- **CS4 (one-off, Python `jsonschema` 4.26):** the 2 shipped and 7 test projects with owned compositions validate against the emitted schema; an interval row validates, unknown keys and null `name`/`intervals` are refused; all 4,408 CS3 case labels agree with the emitted schema.
- **Environment:** the session had no toolchain; the personal cloud environment now has a setup script installing BUILDING's apt line (unproven until a fresh session).
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1646/1646, group `project` 74/74); `check_no_pch.py` on the changed units 5/5. Windows not run (CI).

## 2026-11-14 — Windows CI fixed: a header leaned on the PCH

- **Cause:** run 754 failed both Windows jobs (C2027 in the M6 run tools). Windows CI configures with `CMAKE_DISABLE_PRECOMPILE_HEADERS=ON`; on Linux the shared PCH supplied `<nlohmann/json.hpp>`, so `tools/m6_s1_summary.hpp` (Json used whole from line 138) compiled with only `json_fwd.hpp`. The last green Windows build was `main@fcfeb56`, 103 commits before.
- **Fix (`bb2ac4f`):** the header includes `<nlohmann/json.hpp>`. Run 755 is green on all six jobs, Windows build and tests included.
- **Guard:** `tools/check_no_pch.py` repeats the syntax pass without the PCH: 647 units, 0 failures, 8 m 52 s on 8 jobs; run by hand (NEXT standing facts).

## 2026-11-14 — O1 read: TAG M3.1 §3.3.10 (D557)

- **D557** (owner: "เข้า gov.uk และช่วยตรวจสอบ"): the environment gained `www.gov.uk` and `assets.publishing.service.gov.uk` (owner approved). TAG unit M3.1, May 2026, §3.3.10, p. 31: GEH = √((M − C)² / ((M + C)/2)), the report's formula; Table 2's GEH < 5 criterion is a validation guideline against counts and stays unapplied. The PDF stays out of the repository. Docs only.

## 2026-11-14 — O1: the GEH source confirmed by the owner (D556)

- **D556** (owner: "O1 ยืนยัน"): TAG unit M3.1 recorded as the GEH source on the owner's confirmation; a second fetch of gov.uk and its asset host failed (outside the session's network), so no paragraph or edition is cited. Docs only.

## 2026-11-14 — O2: a gate counts the demand loaded onto its Link (D555)

- **D555** (owner asked for a recommendation, then approved (b)): amendment first (SUBAREA_CONSISTENCY §15, SC11), then the fed-flow bookkeeping in `consistencyReport` adds each inside zone's flows by connector share to the gate on its connector's Link. SC11 passes first time: the corridor's west entry gate now reads Macro 600 = its Link row (GEH 0); shares 3 : 1 split 450 / 150. Verified on Linux: `check` 353/353.

## 2026-11-14 — M12 R2: the Cross-level Consistency page (D554)

- **D554** (owner: "ทำงานถัดไป", then both recommended options): amendment first (SUBAREA_CONSISTENCY §13, SC8–SC10), then `ConsistencyPanel` under a new *Cross-level* Results level (seeds, Run/Cancel on a worker via a `cancelled` hook on `consistencyReport`, the 19-column table, CSV through Export). `consistency-ui` passes; pins `workspace-ui` (14 tabs) and `results-tabs-ui` (5 levels) updated. The capture showed open finding O2 (§14). Owner check: C18. Verified on Linux: `check` 353/353.

## 2026-11-14 — M12 contract and R1: the subarea workflow and consistency report (D553)

- **D553** (owner: "ช่วยตัดสินใจ", then "ทำต่อตามแนะนำ"; every recommended option): [SUBAREA_CONSISTENCY](../reference/SUBAREA_CONSISTENCY.md), rows SC1–SC7, before code. Meso runs opt-in on the study area's run-time copy (the same cut Micro runs on; `"meso"` as the source is `MESO_AREA_SOURCE_SELF`); per study period, every gate and kept Link gets Macro, Meso and Micro (multi-seed `Estimate`) veh/h with Δ, % and GEH, reported only. R1 is the CLI `--consistency-report`, R2 a Results page. Open: O1, TAG M3.1's GEH paragraph (the publisher's host was not reachable).
- **R1:** `src/project/subarea_consistency.*` (`runMesoArea`, `consistencyReport`, JSON/CSV), `tools/consistency_cli.*`, `estimate` made public. Groups `consistency` (SC1–SC6) and `cli-consistency` (SC7) pass first time. Corridor (area {A}, 600 veh/h): Macro 600, Meso 592.7 (bound 12.0), Micro 240 over seeds 42–43; Micro's west→east movement gives way and queues at the source, Meso ignores Node priority (SUBAREA_CONSISTENCY §12). Nothing tuned. Verified on Linux: `check` 352/352.

## 2026-11-13 — M9 A4: the Dynamic assignment Results page (D552)

- **D552** (owner picked A4: a Micro-level Results page, Apply as one Undo step): contract first (DYNAMIC_ASSIGNMENT §17), then `DynamicAssignmentPanel` (level, iteration cap, gap, seed; Run/Cancel on a worker via a new `keepGoing` hook in `iterateMsa`; δ and cell tables; CSV; *Apply weights*; *Save route costs…*), en/th keys. `dynamic-assignment-ui` passes DA14–DA17 first time; an offscreen capture checked the layout. Owner check: OWNER_SITTING_WINDOWS C17.

## 2026-11-13 — M9 A2b: placed and positioned decisions in the loop (D551)

- **D551** (owner: "ทำต่อตามแนะนำ"): contract first (DYNAMIC_ASSIGNMENT §15, MESO_ASSIGNMENT §10), then `routeFamilies` exported from `expandRouteless` into `RunSnapshot`, `decisionFamilyName`, placed (departure) and positioned (`RoutingEvent`) costs in `measureDecisionCosts`, families in the Meso measurement. Group `dynassignplaced` passes DA10–DA13; `mesoassign` passes MA9.
- **Recorded:** the first DA10/DA11 recomputation mapped by departure route and dropped stub left-turners (test fix, values unchanged); MA9's first fixture contradicted §15 (both routes end on one Link) and was amended before re-running (§10.1).
