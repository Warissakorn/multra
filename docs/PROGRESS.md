# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-14 — Owned compositions in the composed project schema (D558)

- **Pick:** NEXT's AI-automation "Next" (map Composition/type-share/interval rows), the only open item not waiting on the owner.
- **D558:** `networkCodec` gains composition, type-share and interval rows; `definition.compositions` is optional, never null; all three rows are closed at every version (the reader always was); no read gate (18/19 stay writer floors). `parseComposition` now reads through that metadata; its diagnostics were pinned by tests on the unchanged reader first (CS1). Coverage id ends `-compositions`.
- **Docs:** PROJECT_SCHEMA.md was at the 500-line limit, so its D363/D364 sections and the new one moved to [PROJECT_SCHEMA_DEMAND](reference/PROJECT_SCHEMA_DEMAND.md).
- **CS4 (one-off, Python `jsonschema` 4.26):** the 2 shipped and 7 test projects with owned compositions validate against the emitted schema; an interval row validates, unknown keys and null `name`/`intervals` are refused.
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

- **D553** (owner: "ช่วยตัดสินใจ", then "ทำต่อตามแนะนำ"; every recommended option): [SUBAREA_CONSISTENCY](reference/SUBAREA_CONSISTENCY.md), rows SC1–SC7, before code. Meso runs opt-in on the study area's run-time copy (the same cut Micro runs on; `"meso"` as the source is `MESO_AREA_SOURCE_SELF`); per study period, every gate and kept Link gets Macro, Meso and Micro (multi-seed `Estimate`) veh/h with Δ, % and GEH, reported only. R1 is the CLI `--consistency-report`, R2 a Results page. Open: O1, TAG M3.1's GEH paragraph (the publisher's host was not reachable).
- **R1:** `src/project/subarea_consistency.*` (`runMesoArea`, `consistencyReport`, JSON/CSV), `tools/consistency_cli.*`, `estimate` made public. Groups `consistency` (SC1–SC6) and `cli-consistency` (SC7) pass first time. Corridor (area {A}, 600 veh/h): Macro 600, Meso 592.7 (bound 12.0), Micro 240 over seeds 42–43; Micro's west→east movement gives way and queues at the source, Meso ignores Node priority (SUBAREA_CONSISTENCY §12). Nothing tuned. Verified on Linux: `check` 352/352.

## 2026-11-13 — M9 A4: the Dynamic assignment Results page (D552)

- **D552** (owner picked A4: a Micro-level Results page, Apply as one Undo step): contract first (DYNAMIC_ASSIGNMENT §17), then `DynamicAssignmentPanel` (level, iteration cap, gap, seed; Run/Cancel on a worker via a new `keepGoing` hook in `iterateMsa`; δ and cell tables; CSV; *Apply weights*; *Save route costs…*), en/th keys. `dynamic-assignment-ui` passes DA14–DA17 first time; an offscreen capture checked the layout. Owner check: OWNER_SITTING_WINDOWS C17.

## 2026-11-13 — M9 A2b: placed and positioned decisions in the loop (D551)

- **D551** (owner: "ทำต่อตามแนะนำ"): contract first (DYNAMIC_ASSIGNMENT §15, MESO_ASSIGNMENT §10), then `routeFamilies` exported from `expandRouteless` into `RunSnapshot`, `decisionFamilyName`, placed (departure) and positioned (`RoutingEvent`) costs in `measureDecisionCosts`, families in the Meso measurement. Group `dynassignplaced` passes DA10–DA13; `mesoassign` passes MA9.
- **Recorded:** the first DA10/DA11 recomputation mapped by departure route and dropped stub left-turners (test fix, values unchanged); MA9's first fixture contradicted §15 (both routes end on one Link) and was amended before re-running (§10.1).

## 2026-11-13 — M11.5 Meso assignment contract (D550)

- **D550** (owner: "ทำงานถัดไปตามแนะนำ" after asking what M11.5 is, then every recommended option): [MESO_ASSIGNMENT](reference/MESO_ASSIGNMENT.md), rows MA1–MA8, before code. Free-flow times read from the compiled fixture graph (t_s 55.356464, t_l 109.904888 s); deterministic-queueing equilibrium x* = 0.590914; an analytic MSA reference stops at n = 11 for gap 0.01.
- **S1:** `src/project/meso_assignment.*` (cohort integral, `measureMesoDecisionCosts`), `DaLevel` through `runDynamicAssignment`, the file's `level` and `--dynamic-assignment --meso` / `--route-costs --meso`. Group `mesoassign` passes MA1–MA8 first time; converged at n = 6 with short share 0.5833 (x* 0.5909), costs 107.49 / 109.905 s. Two CLI runs `cmp`-identical.

## 2026-11-13 — M9 dynamic assignment contract (D549)

- **D549** (owner: "ทำงานถัดไป", chose the M9 contract, then every recommended option): [DYNAMIC_ASSIGNMENT](reference/DYNAMIC_ASSIGNMENT.md), rows DA1–DA9, before code. MSA (Ortúzar Eq. 10.23) over routing-decision cells, cost from the decision to arrival, δ (Eq. 10.22) stopping, a separate `MultraRouteCosts` file and `--route-costs`.
- **Hand values:** Example 10.6's MSA trace (book p. 371) computed exactly; every row equals the printed table (row 7 printed rounded), reaching Example 11.1's 600 / 1400 at 22 min at n = 10 with δ = 0.
- **A1:** `src/project/dynamic_assignment.*` (`iterateMsa` behind a cost provider, `allOrNothing`, `msaStep`, `msaDelta`); group `dynassign` passes DA1–DA3 against that table to 1e-9.
- **A2:** amendments recorded before code (§11: unplaced decisions only, placed ones refused until A2b; cost = departure to arrival; free-flow fill by authored route length over the mean desired speed); measurement, the `MultraRouteCosts` codec and `--route-costs`; group `dynassignrun` passes DA4–DA6 and DA9.
- **A3:** fixture (§13) fixed before running; the first run showed the 1 : 1 start already inside the gap (δ 0.027) and the 5 % cost criterion inconsistent with δ, so §13.1 amends both openly (start 19 : 1; δ alone). `runDynamicAssignment`, `--dynamic-assignment`, `fixture::twoRoute()`; δ 0.810 → 0.0108, converged at n = 2 with 0.475 / 0.525 at 70.2 / 68.7 s. Group `dynassignloop` passes DA7–DA8; two CLI runs `cmp`-identical.

## 2026-11-13 — Logsum composite cost into distribution, G/D/MS/A (D548)

- **D548** (owner: "ทำงานถัดไป", chose logsum feedback, then every recommended option): contract first ([MODE_CHOICE_LOGSUM](reference/MODE_CHOICE_LOGSUM.md), Ortúzar §6.5.2), hand values before code (Example 6.3's C_B = 49.2298; a one-origin, two-destination table).
- **L1:** `distributionCost: "logsum"` per mode-choice purpose at schema 76 (one shared negative coefficient); `logsum`/`compositeCost` in `src/macro/mode_choice`; `runModeChoice` checks Eq. 6.11 (β > λ refused) and re-runs the distribution on the composite; skim-pass balancing refusals are held until the set is known. LS1–LS8 pass against the hand values; the first test run's zone order was fixed in the test only.
- **L2:** a per-purpose check box in the *Nests* dialog; `nested-logit-ui` covers LS9.

## 2026-11-13 — Procedure feedback: congested costs back into demand (D547)

- **D547** (owner: "ทำงานถัดไป", chose M10 congested-cost feedback, then answered the contract's four choices with every recommended option): contract first ([PROCEDURE_FEEDBACK](reference/PROCEDURE_FEEDBACK.md), Ortúzar §11.3; the CW formula checked on the p. 432 image), hand fixed point of Example 11.3 before code.
- **F1:** `feedback` on a procedure at schema 75; `skimAt` and an optional `linkTimes` through `zoneSkim`, distribution and mode choice; `procedure_feedback.cpp` (re-run mode choice on congested times, average the OD-mode matrices by CW or MSA, assign, TMF/RSE); JSON `feedback` block and CLI lines. Fixture `tests/data/procedure-feedback/`. MSA's fixture stop (2 persons in 200 iterations) and FB7's naive case (λ 0.6, where naive's factor 2λ > 1) were amended after the first test run, before commit; no definition or hand value changed.
- **F2:** the dialog's *Feedback* group and the page's iterations table and CSV section; `procedure-ui` covers FB10.
- **Result:** CW reaches 999.80 / 999.07 veh/h against the hand 999.79 / 999.08 in 7 and 10 iterations; MSA takes 127 and 49. Not a validation.

## 2026-11-13 — Sensitivity sweep phase 2: engine levers D and E (D546)

- **D546** (owner: "ทำงานถัดไป", plan approved): contract first ([M6_SWEEP](reference/M6_SWEEP.md) §4, rows SW5–SW10), then runtime-only `DriverBehaviour` flags `standingTarget` (D) and `stopArrival` (E), off by default (`following.cpp`, `conflicts.cpp` `stopMade`), and candidates `D`, `E`, `DE`. Rows SW7 and SW9 were amended after the first test run, before any sweep figure; no definition changed. No `src/core` split was needed (largest file 469 lines).
- **Result** ([evidence](evidence/m6-sweep-phase2.md)): D: S1 1,910.2 veh/h (jam spacing 6.50 m, `t_x` 1.35 s), S2 misses shrink, U `t_f` 6.91 s with U-c passing at `v_c` 200 and 400; E alone equals base; DE `t_f` 5.82 s. U-a misses everywhere. Nothing adopted.

## 2026-11-13 — No calibration on M6; sensitivity sweep phase 1 (D544, D545)

- **D544** (owner asked for calibration options and chose "ไม่ calibrate + sweep"): [options sheet](evidence/m6-calibration-options.md) with five options A–E and estimated effects; D540's hold is closed as no calibration on M6.
- **D545:** contract first ([M6_SWEEP](reference/M6_SWEEP.md), rows SW1–SW4), then `tools/m6_candidates.hpp` and `--candidate base|B|C` on the S1, S2 (`ab`) and U (`run`) tools; no flag keeps outputs byte-identical (a `base` S1 rerun matched the first run).
- **Result** ([evidence](evidence/m6-sweep-phase1.md)): B 2,029.8 veh/h, C 2,109.5 (S1-f would pass, S1-a would miss for both); S2 misses grow; U `t_f` 12.89 / 12.98 s (base 13.67). The D544 arithmetic overstated both levers on U. Nothing adopted.

## 2026-11-13 — Where the Stop follow-up goes (D543)

- **D543** (owner: "ทำงานถัดไป"): contract first ([M6_U_RUN](reference/M6_U_RUN.md) §6, rows SU1–SU5), then `tools/m6_u_split.hpp` and mode `split` of `multra-m6-u-run` (the D542 run is mode `run`). Row SU4 was moved to `v_c` 200 and its dwell value corrected to one observed tick from the first test run, before any split figure; no definition changed.
- **Result** ([evidence](evidence/m6-u-split.md)): move-up 10.7–12.2 s of the 12.5–13.9 s headway (settle 3.75 s), service 0.1 s, release 1.6 s with almost no gap-rule blocking. Nothing tuned.

## 2026-11-13 — Kind U first run: U-c and U-a (D542)

- **D542** (owner: "ทำงานถัดไป", plan approved): contract first ([M6_U_RUN](reference/M6_U_RUN.md), rows U1–U6), then `tools/m6_u_network.hpp`, `m6_u_summary.hpp` and `multra-m6-u-run`. A left-hand T, Stop on the minor left turn's merge, `gapTime` 5.804 s from `t_c` 6.2 s, minor demand 1,800 veh/h, seeds 42–81.
- **Result** ([evidence](evidence/m6-u-first-run.md)): 228.5, 196.0, 159.4, 119.2, 81.6 veh/h at `v_c` 200 … 1,000; U-a (HCM 846 … 298) and U-c miss at every level. The follow-up headway is 13.7 s, so the Stop service caps the movement. Nothing tuned.
- `CMakeLists.txt`: the scattered `add_dependencies(check …)` lines folded into one to stay at 500 lines.
