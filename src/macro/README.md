# src/macro

The Macro level (S2, M10): static user-equilibrium assignment on a graph compiled from the drawn
network. Contract: [MACRO_NETWORK](../../docs/reference/MACRO_NETWORK.md); as built:
[MACRO_NETWORK_IMPLEMENTED](../../docs/reference/MACRO_NETWORK_IMPLEMENTED.md). Not validated
(rule 4): the Sheffi (1985) Table 5.4 benchmark (MA15) is one row of the M10 gate.

## Dependencies

Imports nothing outside this folder; no clock, no random number, no I/O
(`multra-check-architecture`).

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `graph.*` | The Macro graph as plain values: links, connectors, centroid arcs and link types | [MACRO_NETWORK §3](../../docs/reference/MACRO_NETWORK.md) | model/demand (fills it), project |
| `settings.hpp` | `MacroSettings` (target gap, iteration cap) alone, so the authoring definition need not include the graph | [MACRO_NETWORK_IMPLEMENTED](../../docs/reference/MACRO_NETWORK_IMPLEMENTED.md) | model/demand, project |
| `assign.*` | The assignment: BPR `macroTime`, `shortestTree` (Dijkstra, ties to the lower link index), `allOrNothing`, and `assignMacro`, multi-class Frank-Wolfe on the shared pcu load with an exact line search, the relative gap, a per-iteration record and a cancelling observer | [MACRO_NETWORK §7](../../docs/reference/MACRO_NETWORK.md) | project |

## Entry points

`compileMacroNetwork` in `src/model/demand/macro_compile.*` builds the graph;
`src/project/macro_run.*` runs one study period; `src/project/macro_output.*` writes the outputs.

## Tests

`tests/macro_assign_tests.cpp`, `tests/macro_benchmark_tests.cpp` (MA15), `tests/macro_network_tests.cpp`.
