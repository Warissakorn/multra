# B-8: Stop dwell time — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1`, on `00ab227` (D230's contract) plus this change (D231).

- **Host:** Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores.
- **Build and check:** `--preset desktop` (Debug), then `cmake --build build/desktop --target check`.
- **Linux only.**
- **Contract:** [STOP_DWELL](../reference/STOP_DWELL.md).

## Engine rows (`tests/stop_dwell_tests.cpp`, ctest `stopdwell`)

The rows use stop_control's hand-built crossing: the minor approach crosses an empty major road, and
the waiting line is at 90 m. A tick "at rest" is a tick with speed zero and the front within
`stopLineReach` of the line.

"Before the change" is the same rows run against a stub where `stopDwellTicks` always returns 1 and
the validator ignores the key.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA117 | No key | 2 ticks at rest (the arrival tick plus one) | passes (it pins today's rule) |
| BA118 | 2 s, dt 0.1 | 21 ticks at rest; crosses 1.9 s later than without the key | fails |
| BA119 | 2 s, dt 0.1 / 0.25 / 0.5 | Rest after the arrival tick ∈ [2, 2 + dt); without the key it is 0.5 s at dt 0.5 | fails |
| BA120 | 0.05 s, and 0.3 s at dt 0.1 | 2 ticks at rest; 0.3 s is 3 ticks (an exact multiple is not rounded up) | fails (3 vs 1) |
| BA121 | 1 s, the minor already at the line, a major vehicle approaching | It still waits for the gap (more than 1 s longer than with a clear road) and crosses on the same tick as without the key; it rests rather than creeping up to the line | fails |
| BA122 | 2 s on a behaviour assigned to the minor road, then to the major road only | 21 ticks at rest, then 2 | fails |
| BA123 (core) | 0, −1, 61 and ∞, on both the prototype and the W74 branch | `INVALID_BEHAVIOUR_PARAMETER` at `.stopDwellTime`; 60 is accepted | fails |

**Two fixture faults were fixed before the rows were run against the change:**
- The first draft counted rest as within 1.5 m of the line. A stopped vehicle stands about 3.2 m
  short: the standstill gap plus the reach.
- The first BA121 fixture's major vehicle passed before the minor arrived, so the gap was never closed.

## Codec (BA123) and editor

BA123 loads four-leg with owned catalogs, then checks:
- a behaviour carrying the key writes schema 41 and round-trips;
- 40 is refused at `behaviours[0].stopDwellTime`;
- 0, 61 and a string are refused;
- removing the key returns the file to its old schema;
- the compiled run carries the key.

The codec hooks were proved by mutation:

| Mutation | Caught by |
|---|---|
| No parse | BA123 |
| No writer | BA123 |
| No schema-41 term | BA123 |
| No refusal below 41 | BA123 |
| No library key | BA123 (`EDIT_UNSUPPORTED_FIELD`) |
| No validation, W74 branch | BA123 (core). The first run missed it; the W74 case was then added |
| No validation, prototype branch | Both BA123 rows |

**`behaviour-library-ui`** failed first on the old dialog (`stopDwellTimeSet` missing). It now
checks:
- the row is off by default;
- ticking it and entering 2.5 writes the key to the edited behaviour only;
- the saved file is at schema 41. Its schema line moved from 37 to 41 for that reason.

`check` **174/174**.

## Shipped runs

**Byte-identical** (no shipped project has a Stop, and no behaviour carries the key):
- `trafficsim-cli 42` against D210's baseline;
- the `--seeds 42–51` JSON (without `buildCommit`) of all four shipped projects against D225's
  baseline.

**With a Stop:** a scratchpad copy of `t-junction-priority` with both minor controls set from `yield`
to `stop`. The key comes from a copy of the catalog's default behaviour with `stopDwellTime: 2`.
Seeds 42–81:

| dt | Key | Mean delay (s) | Minor → westbound | Minor → eastbound | Completed | Clamps |
|---|---|---|---|---|---|---|
| 0.1 | none | 22.09 (±4.04) | 94.8 | 103.4 | 274.9 | 8 |
| 0.1 | 2 s | 25.30 (±4.42) | 111.3 | 121.5 | 274.9 | 15 |
| 0.5 | none | 21.29 (±3.70) | 91.8 | 95.6 | 275.3 | 12 |
| 0.5 | 2 s | 23.93 (±3.94) | 105.0 | 111.3 | 275.3 | 10 |

- **The dwell lengthens every minor service.** The minor approaches are saturated, so their queue
  delay rises by 13–18 s. Major movements are unchanged.
- **Time step.** Without the key, the minor delay differs between dt 0.1 and 0.5 by 3.0 and 7.8 s,
  because the rest is one tick long. With 2 s it still differs by 6.3 and 10.2 s. The dwell itself
  is now the same at both steps (BA119), so the gap that remains comes from everything else that
  depends on the step: gap acceptance and car-following in whole ticks. B-8 removes only the Stop's
  share.
- **Clamps.** All of them, with and without the key, are on minor routes at their Stop lines, in
  vehicles holding Stop service: 8 → 15 at dt 0.1, 12 → 10 at dt 0.5. This is an existing source of
  clamps at Stop lines. It was not traced here and is listed in NEXT.

## Performance (callgrind Ir, Release `-g`, seed 42, no key set)

Both builds were made in the same build directory.

| Project | `00ab227` | This change |
|---|---|---|
| `four-leg-signalised` (no Stop) | 891.21 M | 893.46 M (+0.25 %) |
| Stop T-junction copy | 397.59 M | 399.24 M (+0.41 %) |

Four-leg has no Stop, so its cost is not the Stop rule. It is the larger `DriverBehaviour` and
`StopService`, which are copied, and the shift in code generation. It is recorded rather than hidden.

## Not covered here

- A dwell set on each Stop line, or a range per driver (STOP_DWELL §5).
- The cause of the Stop-line clamps above.
