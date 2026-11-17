# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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
- **D573:** root modeChoiceSets 70, nests 71 and logsum 76 join the composed schema ([MCS1–MCS3](reference/PROJECT_SCHEMA_FEATURES.md#owned-mode-choice-set-extension-d573)). Alternatives use inclusive anyOf: modeId, costMatrix or both. Only key membership moves through knownKey at 76; zero reference, blank/unique ids, nest ownership/membership and logsum coefficient rules stay with the codec, with codes/paths/issue order unchanged. Pins passed first on unchanged readers (project 114/114), before table/schema/codec edits; four new tests remain.
- **Doc split:** whole extension sections D566–D572 moved from PROJECT_SCHEMA_DEMAND to PROJECT_SCHEMA_FEATURES before adding MCS1–MCS3. Both files are indexed and every moved-section link is rewritten, including decision-record links; prior decision reasoning is unchanged. The files are 286/242 lines.
- **One-off jsonschema 4.26.0:** all 6,151 labels agree: 1,609 mode-choice cases, 4,501 D570–D572 cases, writer output at 70/71/76, all 18 TrafficSim projects under data/ and tests/ (independently enumerated), and 20 probes outside cases(). Probes: 9 both accept, 7 codec-only refusals, 4 both refuse. Schema-refused/codec-accepted is zero across all labels. Temporary tests/probes and CMake entries were deleted; removing nests/logsum restores the original MNL writer bytes.
- **Verified on Linux (headless):** build and check 238/238; project 115/115. The existing linker-only -Wl,--strip-debug cache flag fits the local disk; repository presets are unchanged. Desktop preset, check_no_pch.py and Windows were not run. NEXT briefs procedures 73/feedback 75; stop for owner review, without a PR or merge.

## 2026-11-15 — Period factors in the schema (D572)

- **Pick:** the owner accepted D570/D571 and explicitly selected `periodFactors` at schema 69 on `claude/optimistic-bohr-lpfu98`; D571's diff and D567's exactFields pattern were read first.
- **D572:** the root is false below 69, then a nonempty array of closed four-required-key rows, text ids and shares in [0, 1] ([PF1–PF3](reference/PROJECT_SCHEMA_FEATURES.md#period-factor-extension-d572)). Only the codec's exact-key expression changes, through `networkCodec::exactFields`; text/share/sum checks and UNKNOWN_PERIOD/DUPLICATE_ID retain their order. Blank text, sums, known periods and unique pairs remain codec checks. Writer, version selection and stored forms are unchanged. Pins and refusal cases passed first on the unchanged reader (project 110/110), before tables or codec edits.
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
- **Docs:** PROJECT_SCHEMA.md was at the 500-line limit, so its D363/D364 sections and the new one moved to [PROJECT_SCHEMA_DEMAND](reference/PROJECT_SCHEMA_DEMAND.md).
- **CS4 (one-off, Python `jsonschema` 4.26):** the 2 shipped and 7 test projects with owned compositions validate against the emitted schema; an interval row validates, unknown keys and null `name`/`intervals` are refused; all 4,408 CS3 case labels agree with the emitted schema.
- **Environment:** the session had no toolchain; the personal cloud environment now has a setup script installing BUILDING's apt line (unproven until a fresh session).
- **Verified on Linux:** desktop build; `check` 353/353 (`multra-tests` 1646/1646, group `project` 74/74); `check_no_pch.py` on the changed units 5/5. Windows not run (CI).

## 2026-11-14 — Windows CI fixed: a header leaned on the PCH

- **Cause:** run 754 failed both Windows jobs (C2027 in the M6 run tools). Windows CI configures with `CMAKE_DISABLE_PRECOMPILE_HEADERS=ON`; on Linux the shared PCH supplied `<nlohmann/json.hpp>`, so `tools/m6_s1_summary.hpp` (Json used whole from line 138) compiled with only `json_fwd.hpp`. The last green Windows build was `main@fcfeb56`, 103 commits before.
- **Fix (`bb2ac4f`):** the header includes `<nlohmann/json.hpp>`. Run 755 is green on all six jobs, Windows build and tests included.
- **Guard:** `tools/check_no_pch.py` repeats the syntax pass without the PCH: 647 units, 0 failures, 8 m 52 s on 8 jobs; run by hand (NEXT standing facts).
