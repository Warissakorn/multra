# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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
