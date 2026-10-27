# src/analysis

Analytical junction capacity for M13: HCM 7th edition Chapter 19 with the Chapter 31
supplements, for one signalised Node at a time. Results are analytical, not simulated, and not
validated (rule 4). Contract: [NODE_ANALYSIS](../../docs/reference/NODE_ANALYSIS.md); every formula
cites its PDF page in the owner's copy ([BOOKS](../../docs/reference/BOOKS.md) §2).

## Dependencies

Imports nothing outside this folder and a reviewed set of standard headers; no clock, no random
number, no I/O (`multra-check-architecture`).

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `signalised.*` | One lane group's Ch. 19 result: the saturation factors of Eq. 19-8 (with `fShared` for a shared lane), capacity, v/c, uniform and incremental delay, flow-weighted approach and intersection delay, LOS by Exhibit 19-8 | [§3, §10](../../docs/reference/NODE_ANALYSIS.md) | model/demand, project |
| `lane_flow.*` | Ch. 31 §2's lane group flow rates on an approach with a shared lane (Eqs. 31-46 to 31-66), protected only: `approachLaneFlows`, `laneChangeProbability` | [§11](../../docs/reference/NODE_ANALYSIS.md) | model/demand, project |
| `critical.*` | The critical v/c `criticalRatio` (Eqs. 19-30, 19-31) over the non-overlapping chain of greens with the largest Σy | [§11](../../docs/reference/NODE_ANALYSIS.md) | project |
| `queue.*` | Back of queue and storage ratio (Ch. 31 §4): `approachSpeed`, `accelerationDelay`, `backOfQueue`, `queueSpacing`, `storageRatio` | [§11](../../docs/reference/NODE_ANALYSIS.md) | project |

## Entry points

- `src/model/demand/node_analysis_compile.*` derives the lane groups from a Node's Micro detail and
  its signal programs.
- `src/project/node_analysis_run.*` analyses every Node of a document;
  `src/project/node_analysis_output.*` writes the JSON and CSV.

## Tests

`tests/node_analysis_tests.cpp` and `tests/node_analysis_a2_tests.cpp` (ctest group
`nodeanalysis`); the `cli-node-analysis` ctest runs the four-leg fixture end to end.
