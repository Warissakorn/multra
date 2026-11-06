# src/macro

The Macro level (S2, M10): static user-equilibrium assignment on a graph compiled from the drawn
network. Contract: [MACRO_NETWORK](../../docs/reference/MACRO_NETWORK.md); as built:
[MACRO_NETWORK_IMPLEMENTED](../../docs/reference/MACRO_NETWORK_IMPLEMENTED.md). Not validated
(rule 4): the Sheffi (1985) Table 5.4 benchmark (MA15) is one row of the M10 gate.

## Dependencies

Imports nothing outside this folder; no clock, no random number, no I/O
(`multra-check-architecture`).

## Files

| File | Holds | Contract |
|---|---|---|
| `graph.*` | The Macro graph as plain values: links, connectors, centroid arcs and link types | [MACRO_NETWORK §3](../../docs/reference/MACRO_NETWORK.md) |
| `settings.hpp` | `MacroSettings` (target gap, iteration cap) alone, so the authoring definition need not include the graph | [MACRO_NETWORK_IMPLEMENTED](../../docs/reference/MACRO_NETWORK_IMPLEMENTED.md) |
| `trip_generation.*` | M10.2 trip generation: `tripEnd` (cross-classification Σ N·t or regression a0 + Σ a·x) and `generateTrips`, productions and attractions per zone and purpose, attractions balanced to productions by Eq. (4.14); negative ends and an unbalanceable purpose returned as index-located issues | [TRIP_GENERATION §3-§4](../../docs/reference/TRIP_GENERATION.md) |
| `trip_distribution.*` | M10.3 trip distribution: `furness` (row-first passes, factors as products of corrections), `originConstrained` (one row normalisation, D476), `distributeTrips` (doubly or origin-constrained exponential gravity seeded by T/Σf, intrazonal share set aside, unreachable and infeasible ends as index-located issues, the stopping measure), `meanCost`, `tripLengthBands` | [TRIP_DISTRIBUTION §3-§4](../../docs/reference/TRIP_DISTRIBUTION.md) |
| `assign.*` | `freeFlowSkim` (free-flow minutes between node lists, by mode); the assignment: BPR `macroTime`, `shortestTree` (Dijkstra, ties to the lower link index), `allOrNothing`, and `assignMacro`, multi-class Frank-Wolfe on the shared pcu load with an exact line search, the relative gap, a per-iteration record and a cancelling observer | [MACRO_NETWORK §7](../../docs/reference/MACRO_NETWORK.md) |

## Entry points

`compileMacroNetwork` in `src/model/demand/macro_compile.*` builds the graph;
`src/project/macro_run.*` runs one study period; `src/project/macro_output.*` writes the outputs. `src/project/trip_generation_run.*` runs trip generation; `src/project/trip_distribution_run.*` runs distribution.

## Tests

`tests/macro_assign_tests.cpp`, `tests/macro_benchmark_tests.cpp` (MA15), `tests/macro_network_tests.cpp`, `tests/trip_generation_tests.cpp` (TG rows), `tests/trip_distribution_tests.cpp`, `tests/trip_distribution_origin_tests.cpp` (TD rows).
