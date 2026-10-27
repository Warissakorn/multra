# src/analysis

Analytical junction capacity for M13: HCM 7th edition Chapter 19 with the Chapter 31
supplements, for one signalised Node at a time. Results are analytical, not simulated, and not
validated (rule 4). Contract: [NODE_ANALYSIS](../../docs/reference/NODE_ANALYSIS.md); every formula
cites its PDF page in the owner's copy ([BOOKS](../../docs/reference/BOOKS.md) §2).

## Dependencies

Imports nothing outside this folder and a reviewed set of standard headers; no clock, no random
number, no I/O (`multra-check-architecture`).

## Files

| File | Holds | Contract |
|---|---|---|
| `signalised.*` | One lane group's Ch. 19 result: the saturation factors of Eq. 19-8 (with `fShared` for a shared lane), capacity, v/c, uniform and incremental delay, flow-weighted approach and intersection delay, LOS by Exhibit 19-8 | [§3, §10](../../docs/reference/NODE_ANALYSIS.md) |
| `lane_flow.*` | Ch. 31 §2's lane group flow rates on an approach with a shared lane (Eqs. 31-46 to 31-66), protected only: `approachLaneFlows`, `laneChangeProbability` | [§11](../../docs/reference/NODE_ANALYSIS.md) |
| `critical.*` | The critical v/c `criticalRatio` (Eqs. 19-30, 19-31) over the non-overlapping chain of greens with the largest Σy | [§11](../../docs/reference/NODE_ANALYSIS.md) |
| `queue.*` | Back of queue and storage ratio (Ch. 31 §4): `approachSpeed`, `accelerationDelay`, `backOfQueue`, `queueSpacing`, `storageRatio` | [§11](../../docs/reference/NODE_ANALYSIS.md) |
| `qap.*` | Ch. 31 §3's queue accumulation polygon over one cycle (Step 7): `queueAccumulation`, giving d1 (Eqs. 31-115, 31-116), the queue service time and the queue at each period end | [NODE_ANALYSIS_A3 §4](../../docs/reference/NODE_ANALYSIS_A3.md) |
| `permitted.*` | Permitted and protected-permitted far turns (Ch. 31 §3 Steps 1-7, §2's permitted terms): `permittedSaturation`, the through-car equivalents, `permittedGreen` (Exhibit 31-12 as one rule), `firstBlockTime`, shared-lane shares and Step 7 B capacities | [NODE_ANALYSIS_A3](../../docs/reference/NODE_ANALYSIS_A3.md) |

## Entry points

- `src/model/demand/node_analysis_compile.*` derives the lane groups from a Node's Micro detail and
  its signal programs.
- `src/project/node_analysis_run.*` analyses every Node of a document;
  `src/project/node_analysis_output.*` writes the JSON and CSV.

## Tests

`tests/node_analysis_tests.cpp`, `tests/node_analysis_a2_tests.cpp` and `tests/node_analysis_a3_tests.cpp` (ctest group
`nodeanalysis`); the `cli-node-analysis` ctest runs the four-leg fixture end to end.
