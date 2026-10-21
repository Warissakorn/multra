# Documentation map

Every file in `docs/`, grouped by what it is for. Engineering documentation is in English;
the supplied Thai specifications in `specs/` retain their documented owner edit. **Status lines live in
[`NEXT.md`](NEXT.md) and [`ROADMAP.md`](ROADMAP.md) only.** Other files describe a design, a
contract or a dated record. When one of them states a status, its own date applies.

## Read by task

Start at [AGENTS.md](../AGENTS.md), then NEXT and the relevant module README. The table
selects additional context; read related decisions and evidence only when needed.

| Task | Read before editing | Code entry points |
|---|---|---|
| Behavior library, class/road assignment or new following models | [DRIVING_BEHAVIOUR contract](reference/DRIVING_BEHAVIOUR.md); [W74 contract](reference/W74.md); [delivery rows](plans/DRIVING_BEHAVIOUR.md); SIMULATION and M3 contracts below | `src/core/`, `src/model/`, `src/project/` |
| Behaviour Lab, circuit routes or live retune | [BEHAVIOUR_LAB](reference/BEHAVIOUR_LAB.md); DRIVING_BEHAVIOUR and SIMULATION below | `src/lab/`, `src/core/retune.cpp`, `src/shell/behaviour_lab_*` |
| Engine, following, lane changes or safety | [SIMULATION](reference/SIMULATION.md); [M3_CONTRACT](reference/M3_CONTRACT.md); [M3_8_CONTRACT](reference/M3_8_CONTRACT.md); [LATERAL](reference/LATERAL.md) and [LANE_CHANGE_DURATION](reference/LANE_CHANGE_DURATION.md) for lateral position and lane-change duration; [SPEED_CONTROL](reference/SPEED_CONTROL.md) for curve speed; [SPEED_DECISIONS](reference/SPEED_DECISIONS.md) for speed limits; [OVERTAKING](reference/OVERTAKING.md) for keep-to-kerb; [COMMITMENT](reference/COMMITMENT.md) for give-way commitment; [MERGE_CLAMPS](reference/MERGE_CLAMPS.md) for the remaining merge and red-head clamps; [JOIN_ANTICIPATION](reference/JOIN_ANTICIPATION.md) for the off-by-default `joinAnticipation` switch and its measured options; [LINE_APPROACH](reference/LINE_APPROACH.md) for the `standingStopFloor` switch and the measured line-stop floors; [MERGE_DECELERATION](reference/MERGE_DECELERATION.md) for mandatory lane-change braking; [BEHAVIOUR_PARAMETERS](reference/BEHAVIOUR_PARAMETERS.md) for what is settable; [M3_ACCEPTANCE](plans/M3_ACCEPTANCE.md) | `src/core/`, `src/model/network/`, `tests/` |
| Link/Connector geometry | [CONNECTOR_FOUR_POINT_MOUTH](reference/CONNECTOR_FOUR_POINT_MOUTH.md); [NETWORK_EDITOR_CONNECTORS](reference/NETWORK_EDITOR_CONNECTORS.md); [CONNECTOR_PARITY_AUDIT](audits/CONNECTOR_PARITY_AUDIT.md) | `src/model/network/`, `src/editor/`, `src/commands/` |
| Demand, catalogs or routing | [DEMAND_IMPROVEMENT](plans/DEMAND_IMPROVEMENT.md); [DEMAND_CATALOGS](reference/DEMAND_CATALOGS.md); [DEMAND_TIME_TYPES](reference/DEMAND_TIME_TYPES.md); [POSITIONED_ROUTING](reference/POSITIONED_ROUTING.md); [SIMULATION](reference/SIMULATION.md) | `src/project/`, `src/model/demand/`, `src/model/network/routing.cpp` |
| OSM background, georeferencing or tiles | [MAP_BACKGROUND](reference/MAP_BACKGROUND.md); [OSM_BACKGROUND](plans/OSM_BACKGROUND.md); [OSM review](audits/OSM_REVIEW.md); NETWORK_EDITOR save/recovery | `src/geo/`, `src/editor/map_*`, `src/project/map_*`, `src/shell/editor_map.cpp` |
| Editor UI, gestures or Undo/Redo | [NETWORK_EDITOR](reference/NETWORK_EDITOR.md); [EDITOR_WORKFLOW](reference/EDITOR_WORKFLOW.md); [VISSIM_PARITY](audits/VISSIM_PARITY.md); [UI_REDESIGN_AUDIT](audits/UI_REDESIGN_AUDIT.md) | `src/editor/`, `src/shell/`, `src/commands/` |
| Vehicle display and turning | [VEHICLE_POSE](reference/VEHICLE_POSE.md); [SIMULATION](reference/SIMULATION.md) | `src/editor/vehicle_pose.*`, `rear_axle_pose.*`, `lane_change_pose.*` |
| Zones, OD matrices or parking lots (M8) | [ZONES_AND_PARKING](reference/ZONES_AND_PARKING.md); [MULTI_LEVEL_MODELLING](plans/MULTI_LEVEL_MODELLING.md) as amended by [MULTI_LEVEL_WORKFLOW](plans/MULTI_LEVEL_WORKFLOW.md); POSITIONED_ROUTING and SIMULATION above | `src/project/`, `src/model/demand/`, `src/core/` (lot runtime) |
| Macro network or static assignment (S2) | [MACRO_NETWORK](reference/MACRO_NETWORK.md); [MACRO_NETWORK_IMPLEMENTED](reference/MACRO_NETWORK_IMPLEMENTED.md); [STUDY_PERIODS](reference/STUDY_PERIODS.md) for periods and PCU | `src/macro/`, `src/model/demand/macro_compile.*`, `src/project/macro_run.*`, `src/shell/editor_macro.*` |
| Study periods, modes or PCU (S1) | [STUDY_PERIODS](reference/STUDY_PERIODS.md); [DEMAND_CATALOGS](reference/DEMAND_CATALOGS.md) | `src/model/demand/`, `src/project/od_demand.*` |
| Load/save or schema changes | [NETWORK_EDITOR](reference/NETWORK_EDITOR.md) save/recovery; relevant Demand/geometry contract; [MIGRATION](reference/MIGRATION.md) | `src/project/`, `tests/project_tests.cpp` |
| Evaluation or reporting | [BATCH](reference/BATCH.md); [LOS](reference/LOS.md); [TRAVEL_TIME_SECTIONS](reference/TRAVEL_TIME_SECTIONS.md); [NODES](reference/NODES.md) for per-junction node evaluation; [DISCHARGE](reference/DISCHARGE.md); [PROBLEM](PROBLEM.md); [SIMULATION](reference/SIMULATION.md); [ROADMAP](ROADMAP.md) M5/M6; [M2_PLAN](plans/M2_PLAN.md) | `src/eval/`, `src/project/evaluation.*`, `src/runner/`, `src/report/` |
| Performance, measurement or a previously reverted idea | [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md) (do-not-retry list, baselines, recipes); D28 and D31 in the [decision record](decisions/RECORD.md); [BUILDING](BUILDING.md) machine notes | `tools/*_benchmark.cpp`, `src/core/`, `src/editor/` |
| Build or portability | [BUILDING](BUILDING.md); [MIGRATION](reference/MIGRATION.md); tools README | `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/` |

Resolve the document names through the indexes below. `src/report/` is a planned module
boundary; `src/runner/` holds batches (M5.2) and scenario comparison (M5.8a).

## Start here, every session

| File | What it is |
|---|---|
| [`NEXT.md`](NEXT.md) | The one live to-do. Read first. |
| [`PROBLEM.md`](PROBLEM.md) | Audience and scope. Every feature traces back to a line here. |
| [`PRINCIPLES.md`](PRINCIPLES.md) | Rules that do not get relitigated. Rules 1–4 are correctness. |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Modules, dependencies and contracts between them. |
| [`BUILDING.md`](BUILDING.md) | Linux/Windows toolchains and dependency setup. |
| [`CMakeUserPresets.windows.example.json`](CMakeUserPresets.windows.example.json) | Local Windows preset example referenced by BUILDING. |
| [`ROADMAP.md`](ROADMAP.md) | Milestones M0–M12: scope, status and closing gates; links to the dated reviews. |
| [`PROGRESS.md`](PROGRESS.md) | Session history, newest first; links to the indexed decision record. |

## Folders

| Folder | What it holds |
|---|---|
| [`reference/`](reference/README.md) | Current behaviour and contracts. |
| [`plans/`](plans/README.md) | Milestone plans and acceptance/gate records. |
| [`audits/`](audits/README.md) | Dated audits and reviews. |
| [`specs/`](specs/README.md) | Supplied Thai target specifications and their original/retained byte manifests. |
| [`evidence/`](evidence/README.md) | Measurement notes behind decisions; read the note, not the raw CSV/JSON. |
| [`images/`](images/) | Screenshots used by the docs. |
| [`archive/`](archive/README.md) | Old PROGRESS logs, ROADMAP bodies, answered reviews and superseded rows, moved whole. Nothing there is current. |
| [`decisions/`](decisions/README.md) | Complete decision record with stable D-number anchors and a topic index. |

## Keeping this tidy

Who owns what is in [AGENTS § Documentation authority](../AGENTS.md#documentation-authority).

- A new maintained doc gets a row in its folder index in the same commit; `multra-check-docs .`
  (in `check`/CTest) checks links, anchors and index coverage.
- Every `.md` stays under 500 lines (`multra-check-file-sizes`); split or archive near 450.
- Status lives only in NEXT and ROADMAP; elsewhere link to it, do not copy it.
- Archive by moving whole blocks: old PROGRESS entries go to the top of the newest
  `archive/PROGRESS-LOG-*.md`; implemented ROADMAP bodies and answered reviews go to `archive/`.
  Fix relative links (`../`) and list new files in `archive/README.md`.
- A contract separates implemented behaviour, planned behaviour and acceptance evidence, and
  records the verification platform and date.
- Keep decision IDs, milestone IDs, gates and source-spec bytes intact.
