# src/model/demand

Authoring demand and signal control, and the compilers that turn the drawn network into each
level's input: the Micro `Scenario` (through `../network/network.hpp`'s `buildScenario`), the Macro
and Meso graphs, and M13's analysis lane groups. Part of [src/model](../README.md).

## Dependencies

As [src/model](../README.md#dependencies): pure, no Qt, JSON or I/O. The compilers read
`src/macro/`, `src/meso/` and `src/analysis/` value headers and never run those engines.

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `definition.hpp` | `AuthoringDefinition`: routes, inputs with intervals, compositions, routing decisions with per-interval turning proportions, Signal Controllers; the core `Route`, `VehicleInput` and `SignalProgram` are also the authoring types. Catalog presence is explicit: an empty override stays empty | [DEMAND_CATALOGS](../../../docs/reference/DEMAND_CATALOGS.md), [DEMAND_TIME_TYPES](../../../docs/reference/DEMAND_TIME_TYPES.md) | project, shell |
| `signal_control.*` | Fixed-time Signal Controllers expanded into core `SignalProgram`s (`signalGroupPrograms`, `signalGroupColorAt`, `signalGroupGreen`) and `signalControlIssues` | [SIMULATION](../../../docs/reference/SIMULATION.md) | lab, project, shell |
| `zones.hpp` | The Zone/OD layer every level shares, including zones with capacity; compiled into ordinary inputs, so the core never sees a zone | [ZONES_AND_PARKING §2](../../../docs/reference/ZONES_AND_PARKING.md) | project, shell |
| `macro_compile.*` | `compileMacroNetwork`: the drawn network as a Macro graph | [MACRO_NETWORK §3](../../../docs/reference/MACRO_NETWORK.md) | project |
| `meso_compile.*` | `compileMesoNetwork`: the drawn network as a Meso graph | [MESO_NETWORK §3, §4](../../../docs/reference/MESO_NETWORK.md) | project |
| `coarse_compile.*` | What Macro and Meso compile from Roads without generated Links and Nodes without detail | [ROADS §6, §9](../../../docs/reference/ROADS.md) | commands, project |
| `meso_paths.*` | `mesoPath`: one authored route as Meso links and turns | [MESO_NETWORK §6](../../../docs/reference/MESO_NETWORK.md) | project |
| `meso_road_paths.*` | Directed zone endpoints and minimum free-flow-time paths on the compiled Meso graph, with a finite path limit, a 1e-12 relative tie tolerance and no recursion | [MESO_ROAD_DEMAND](../../../docs/reference/MESO_ROAD_DEMAND.md) | project |
| `seam_checks.*` | Seam checks between Roads and Micro detail, run on demand; each row an issue with a severity for the level run, none refusing | [ROADS §7](../../../docs/reference/ROADS.md) | project, shell |
| `node_analysis_compile.*` | A signalised Node's lane groups from its Micro detail and signal programs, ready for `analyseSignalised`; what cannot be analysed is an advisory | [NODE_ANALYSIS §3, §4, §11](../../../docs/reference/NODE_ANALYSIS.md) | project |

## Tests

`tests/demand_*_tests.cpp`, `tests/signal_control_tests.cpp`, `tests/signal_control_schema_tests.cpp`,
`tests/macro_network_tests.cpp`, `tests/meso_network_tests.cpp`, `tests/meso_road_demand_tests.cpp`,
`tests/road_coarse_tests.cpp`, `tests/road_seam_tests.cpp`, `tests/node_analysis_tests.cpp`,
`tests/node_analysis_a2_tests.cpp`.
