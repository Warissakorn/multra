# src/runner

Multi-seed batches and scenario comparison over one compiled scenario. Contract:
[BATCH](../../docs/reference/BATCH.md).

## Dependencies

Reads the `src/core/` and `src/eval/` headers only; no model, project, Qt or I/O
(`multra-check-architecture`). Formatting lives in `src/project/batch_output.*` and
`comparison_output.*`; worker threads in `src/project/parallel_seeds.*`, so this module stays
thread-free.

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `batch.*` | `runSeed` (one seed observed exactly as a single run), `runSeeds`, `aggregate` in seed order, `tQuantile975`, `movementsWithUnfinished`. An optional cancellation predicate interrupts a seed at 32-tick checkpoints (`RunCancelled`); without one, replay is unchanged | [BATCH](../../docs/reference/BATCH.md) | project, shell |
| `compare.*` | `welch` and `compareBatches`: alternative minus base, row by row, over one seed list (equal seeds are not common random numbers) | [BATCH §7](../../docs/reference/BATCH.md) | project |

## Tests

`tests/batch_tests.cpp`, `tests/compare_tests.cpp`; most evaluation tests run through `runSeed`.
