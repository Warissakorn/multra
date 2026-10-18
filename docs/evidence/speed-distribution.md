# B-5: desired-speed distribution curve — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1`, on `be07368` (D228's contract) plus this change (D229).

- **Host:** Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores.
- **Build and check:** `--preset desktop` (Debug), then `cmake --build build/desktop --target check`.
- **Linux only.**
- **Contract:** [SPEED_DISTRIBUTION](../reference/SPEED_DISTRIBUTION.md).

## Engine rows (`tests/speed_distribution_tests.cpp`, ctest `speeddist`)

"Before the change" means the rows run against a stub: `speedAt` and `shareOf` ignore the curve, and
validation reads no curve.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA110 | No curve | 1000 draws each equal `min + u·(max − min)` from a copy of the stream; the stream advances by three draws per vehicle | passes (it pins today's draw) |
| BA111 | Curve `(min, 0), (max, 1)` | 2000 draws and driver factors bit-identical to no curve | passes |
| BA112 | Curve `(10, 0), (12, 0.8), (16, 1)` | Share at or below 12 m/s is 0.80 ± 0.01 over 20 000 draws; every speed in [10, 16]; driver factors identical to the uniform run | fails (0.334 vs 0.8) |
| BA113 | That curve and a decision `{8–12}` | 12 m/s (share 0.8) → 11.2; 11 m/s (share 0.4) → 9.6 | fails (9.33 vs 11.2) |
| BA114 | Retune to curve `(20, 0), (22, 0.8), (30, 1)` with rescale | 12 → 22, 11 → 21 | fails (23.33 vs 22) |
| BA114 (Lab) | `fitCurve` after min/max become 20–32 | Points move to 20, 24, 32 with the same shares; a matching range is untouched | fails without the fit |
| BA115 (core) | Validation | Each broken point named at `vehicleTypes[i].desiredSpeed.curve[k]`: one point, first not `(min, 0)`, last not `(max, 1)`, speed not rising, share falling, share outside [0, 1], NaN | fails (no issue) |

BA114's first draft built its retune target from the hand-written scenario rather than from the
placed run's canonical scenario, and was refused with `RETUNE_SHAPE_CHANGED`. The fixture was
corrected before the change was checked against it.

## Codec (BA115) and editor (BA116)

BA115 was written after the code, so each hook was removed in turn:

| Mutation | Caught by |
|---|---|
| No parse of `curve` | BA115 round-trip |
| No writer | BA115 (`contains("curve")`) |
| No schema-40 term | BA115 |
| No refusal below 40 | BA115 |
| No validation | Both BA115 rows |
| No Lab fit | BA114 (Lab) |

BA115 also checks:
- the four-leg file without a curve keeps its schema and bytes;
- an owned catalog with a curve writes 40 and round-trips;
- 39 is refused at the curve's path;
- malformed curve JSON is refused;
- the shared catalog reader takes a curve;
- clearing the curve returns the owned file to its old schema.

**BA116** (`demand-catalog-ui`) failed first on the old dialog (`Missing catalog control`). It then
checks:
- ticking the box disables min and max;
- the table starts at `(min, 0%)` and `(max, 100%)`;
- *Add point* inserts in order;
- the edited points write `{10, 16, (10,0),(12,0.8),(16,1)}` in one revision, at schema 40;
- Undo and Redo work;
- unticking removes the curve.

`check` **173/173**.

## Shipped runs

**Byte-identical** (no shipped type carries a curve):
- `trafficsim-cli 42` against D210's baseline;
- the `--seeds 42–51` JSON (without `buildCommit`) of all four shipped projects against D225's
  baseline.

**With a curve:** a scratchpad copy of the catalog, with the car's 11.11–16.67 m/s range given the
curve `(11.11, 0), (12.5, 0.15), (13.89, 0.6), (15.28, 0.9), (16.67, 1)`. It concentrates drivers
near 50 km/h: 10 % above 15.28 m/s instead of 25 %. `four-leg-signalised`, seeds 42–81:

| | No curve | Curve |
|---|---|---|
| Mean delay (s) | 46.00 | 45.40 (95 % half-width 0.55) |
| Completed trips (mean) | 453.5 | 452.8 |
| Safety clamps | 0 | 0 |

Per movement, travel time moves by at most 0.6 s and delay falls by up to 1.0 s.

Delay is measured against each driver's own desired speed. The curve has fewer fast drivers, so the
same signal-bound trip is compared with a slower free-flow time and counts as less delay. The change
is inside the run-to-run noise, and no curve ships.

## Performance (callgrind Ir, Release `-g`, `four-leg-signalised` seed 42)

Both builds were made in the same build directory.

| Build | Ir |
|---|---|
| `be07368` | 890.71 M |
| This change | 891.21 M (+0.06 %) |

## Not covered here

- Per-input or per-road curves, and named shared distributions (§5).
- A curve on a decision's or area's range: these stay linear (owner).
