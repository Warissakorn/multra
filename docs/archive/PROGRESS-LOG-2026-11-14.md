# PROGRESS archive — 2026-11-14

Whole entries moved from the live PROGRESS log.

## 2026-11-13 — M9 A4: the Dynamic assignment Results page (D552)

- **D552** (owner picked A4: a Micro-level Results page, Apply as one Undo step): contract first (DYNAMIC_ASSIGNMENT §17), then `DynamicAssignmentPanel` (level, iteration cap, gap, seed; Run/Cancel on a worker via a new `keepGoing` hook in `iterateMsa`; δ and cell tables; CSV; *Apply weights*; *Save route costs…*), en/th keys. `dynamic-assignment-ui` passes DA14–DA17 first time; an offscreen capture checked the layout. Owner check: OWNER_SITTING_WINDOWS C17.

## 2026-11-13 — M9 A2b: placed and positioned decisions in the loop (D551)

- **D551** (owner: "ทำต่อตามแนะนำ"): contract first (DYNAMIC_ASSIGNMENT §15, MESO_ASSIGNMENT §10), then `routeFamilies` exported from `expandRouteless` into `RunSnapshot`, `decisionFamilyName`, placed (departure) and positioned (`RoutingEvent`) costs in `measureDecisionCosts`, families in the Meso measurement. Group `dynassignplaced` passes DA10–DA13; `mesoassign` passes MA9.
- **Recorded:** the first DA10/DA11 recomputation mapped by departure route and dropped stub left-turners (test fix, values unchanged); MA9's first fixture contradicted §15 (both routes end on one Link) and was amended before re-running (§10.1).
