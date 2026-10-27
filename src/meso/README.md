# src/meso

The Meso level (M11): link-transmission loading of class paths on a graph compiled from the drawn
network. Contract: [MESO_NETWORK](../../docs/reference/MESO_NETWORK.md); as built:
[MESO_NETWORK_IMPLEMENTED](../../docs/reference/MESO_NETWORK_IMPLEMENTED.md). Not validated (rule 4).

## Dependencies

Imports nothing outside this folder; no clock, no random number, no I/O
(`multra-check-architecture`). Direct Road OD paths are filled outside the engine
([MESO_ROAD_DEMAND](../../docs/reference/MESO_ROAD_DEMAND.md)); the engine receives ordinary class
paths and time-window flows.

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `graph.*` | The Meso graph as plain values: links (Link pieces outside every Node, with capacity, free speed and jam density), turns, nodes (a drawn Node with its membership turns and conflicts, or a simple junction) and `mesoStepLimit` | [MESO_NETWORK §3, §4](../../docs/reference/MESO_NETWORK.md) | model/demand (fills it) |
| `settings.hpp` | `MesoSettings` (step, reporting slice) alone, so the authoring definition need not include the graph | [MESO_NETWORK](../../docs/reference/MESO_NETWORK.md) | model/demand |
| `demand.*` | The loaded demand: `MesoPath`, `MesoClass`, turn shares and entry flows | [MESO_NETWORK §6](../../docs/reference/MESO_NETWORK.md) | model/demand, project |
| `node_model.*` | `mesoNodeFlows`, Tampère et al. (2011) §5.1, checked against the paper's Table 8 | [MESO_NETWORK §5](../../docs/reference/MESO_NETWORK.md) | this folder |
| `link_model.*` | Yperman (2007) Eqs. 4.31 and 4.35 over `MesoCurve` cumulative counts | [MESO_NETWORK §4](../../docs/reference/MESO_NETWORK.md) | this folder |
| `loading.*` | `loadMeso`: three steps per time step, origins as point queues, turn splits by route composition (Eq. 5.2), `mesoNodeFlows` at every node. Signals are averaged green: each in-link's turns under one program form a Tampère §5.2 internal supply. A turn's free-flow time delays entry to the out-link, and its vehicles count against that link's storage meanwhile | [MESO_NETWORK §4-§6](../../docs/reference/MESO_NETWORK.md) | project |

## Entry points

`compileMesoNetwork` in `src/model/demand/meso_compile.*` builds the graph and
`src/project/meso_input.*` resolves its link types. `mesoDemand` fills the demand from Micro's
authored routes (`src/model/demand/meso_paths.*`) and from routeless inputs and placed decisions.
`runMeso` (`src/project/meso_run.*`) drives `loadMeso` with a cancelling progress callback;
`src/project/meso_output.*` writes the outputs by slice.

## Tests

`tests/meso_node_model_tests.cpp`, `tests/meso_loading_tests.cpp`, `tests/meso_demand_tests.cpp`,
`tests/meso_network_tests.cpp`, `tests/meso_output_tests.cpp`.
