# PROGRESS archive — 2026-11-14

Whole entries moved from the live PROGRESS log.

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
