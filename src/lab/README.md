# src/lab

The Behaviour Lab: the development ring a driving behaviour is tuned on. A development bed, not
evidence. Contract: [BEHAVIOUR_LAB](../../docs/reference/BEHAVIOUR_LAB.md).

## Dependencies

May build documents through `src/commands/` and compile through `src/project/`; never the editor,
the shell or Qt (`multra-check-architecture`), so it stays testable headless. The window is
`src/shell/behaviour_lab_*`.

## Files

| File | Holds | Contract |
|---|---|---|
| `lab_settings.*` | `LabSettings`, every value the window edits; `defaultSettings`; `rescaleAxles` keeps explicit axle proportions consistent with a tuned length (shared by LabRun, the parameter panel and project Apply); `fitSpeedCurves` | [BEHAVIOUR_LAB §2](../../docs/reference/BEHAVIOUR_LAB.md) |
| `lab_network.*` | `buildLabDocument`: the ring built through the editor's own commands, never saved; `referenceLapLength` | [BEHAVIOUR_LAB §2](../../docs/reference/BEHAVIOUR_LAB.md) |
| `lab_build.hpp`, `lab_oval.cpp`, `lab_urban.cpp` | Internal: the two layouts, `oval` and `urban`, and what they share (`addLoop`, `signalise`, `firstLane`, `finishDefinition`, defined in `lab_network.cpp`) | [BEHAVIOUR_LAB](../../docs/reference/BEHAVIOUR_LAB.md) |
| `lab_run.*` | `LabRun`: compiles with `buildScenario`, marks lane chains circuits, places vehicles and steps; retunes in place (`retuneSimulation`) when only values change, restarts otherwise; `maxVehicles` | [BEHAVIOUR_LAB §2](../../docs/reference/BEHAVIOUR_LAB.md) |

## Tests

`tests/lab_tests.cpp`, `tests/lane_change_lab_tests.cpp`, `tests/behaviour_lab_ui_tests.cpp`.
