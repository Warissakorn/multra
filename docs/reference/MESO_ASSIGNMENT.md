# Meso dynamic assignment by M9's loop (M11.5, D550)

Written 2026-11-13 as a contract, before code. [MESO_NETWORK](MESO_NETWORK.md) §5a loads fixed
paths "until M9" and says M11.5 replaces them with M9's loop. M9's loop
([DYNAMIC_ASSIGNMENT](DYNAMIC_ASSIGNMENT.md), D549) takes its costs from a provider, so M11.5 is a
second provider: one Meso run measured from its cumulative curves. The owner chose M11.5 and
answered §8 in the same session, each with the recommended option.

Rule 4 holds:
- every output keeps the not-validated marker;
- a hand value checks arithmetic, not behaviour.

M11's own gate still waits on its benchmark (Boyles et al. 2025, D315). This slice does not move it.

**Sources.**
- Travel time read from FIFO cumulative curves (the time between the N-th entrance and the N-th
  exit) is Newell's (1993) reading, as Yperman (2007) uses it for the LTM (MESO_NETWORK §4,
  "FIFO conditions"; cited, not vendored).
- The hand equilibrium below is deterministic queueing at a bottleneck under a constant arrival
  rate: a textbook construction, computed here, not quoted.

## 1. What carries over from M9 unchanged

DYNAMIC_ASSIGNMENT applies unchanged in these parts:
- §2: cells, weights, refusals;
- §4: MSA from w¹, all-or-nothing auxiliary shares, δ (Eq. 10.22), the stop and the written
  weights;
- §5: the route-costs file, except as §4 below;
- §11: unplaced decisions only; placed ones are `DYNAMIC_ASSIGNMENT_UNSUPPORTED`.

Meso maps an entry to its paths directly. Every `MesoPath` of every class whose `source` is the
entry's `routeId` belongs to that entry.

## 2. Cost from the cumulative curves

**Inputs.** For one commodity (class c, path p), the loading gives `demand` D, `entered` E and
`arrived` A, cumulative PCU at k × step.

**Cohorts.** A cell's cohorts are N ∈ [D(s), D(e)) for each piece [s, e) of the cell's window:
- an interval cell is its interval;
- the default cell is [0, horizon) minus every interval of its decision.

**The mean cost of a cohort range** [N0, N1) is (1/(N1 − N0)) ∫ (A⁻¹(N) − E⁻¹(N)) dN, from
entering to arriving. It is computed exactly on the piecewise-linear curves by integration by
parts:

  ∫_{N0}^{N1} T⁻¹(N) dN = N1·T⁻¹(N1) − N0·T⁻¹(N0) − ∫_{T⁻¹(N0)}^{T⁻¹(N1)} T(t) dt

T⁻¹(N) is the earliest t with T(t) = N. On a flat piece, where the curve does not rise, the
earliest time is taken.

**Censored cohorts.** The part of [N0, N1) above A(horizon) uses the horizon for A⁻¹. Its PCU,
divided by the class PCU, is the cell's `censored` count in vehicles. A cohort above E(horizon)
never entered. It counts from the horizon to the horizon (0 s) and is censored too.

**Across classes.**
- An entry's cost is the vehicle-weighted mean over its commodities, where vehicles = PCU / class
  PCU.
- `count` is vehicles, and can be fractional.

**An empty choice** has no cohort in the cell (N1 = N0 for all its commodities). Its cost is the
free-flow time of its path, Σ links L/v + Σ turns τ (D301), and it is listed in `freeFlowFilled`.
The first class with a path for the entry is used. No other choice in the cell changes its cost.

## 3. The loop and the run

- `runDynamicAssignment` gains a level, Micro or Meso. Meso's provider:
  - writes the weights with `applyDecisionWeights`;
  - calls `runMeso` (M11.4, the whole run);
  - measures as §2.
- Meso has no seed, and every run is deterministic. Each iteration is a full `runMeso`.
- **A path refused by the loading** (`MESO_PATH_NOT_LOADED`) has no commodity. Its entry is
  free-flow-filled, and the advisory is kept.

## 4. Files and CLI

- **The level in the file.**
  - The route-costs file gains `"level": "meso"`, and a Meso file omits `provenance.seed`.
  - A file without `level` is Micro, so D549's files stay valid.
  - `applyRouteCosts` refuses a level other than the run's as `ROUTE_COSTS_STALE` at `level`.
- **The flags.**
  - `--dynamic-assignment OUT --meso` runs the Meso loop.
  - `--route-costs FILE --meso` applies a Meso file to a `--meso` run.
  - Every other rule of DYNAMIC_ASSIGNMENT §6 holds.

## 5. Fixture and hand values

**The drawing.** `tools/meso_two_route_network.hpp`, `fixture::mesoTwoRoute(data)`, built through
the editor's commands. Test values, not data.
- **Link types:** `road` (1800 pcu/h/lane) and `bottleneck` (600). Both have a free speed of
  15 m/s and a jam density of 150 pcu/km/lane.
- **Links:**
  - Origin: 2 lanes, 100 m.
  - Short route: S1, 2 lanes, 400 m (storage 120 pcu); then S2, the `bottleneck`, 1 lane, 100 m.
  - Long route: L, 2 lanes, 1,322.84 m.
  - Destination: 2 lanes, 200 m.
- **Demand:** car (PCU 1, test value) at q = 1200 veh/h over T = 600 s; the run lasts 1200 s.
- **Decision:** unplaced {short, long} at 1 : 1, so one `default` cell.

**Free-flow times, read from the compiled graph before any loop code** (the links' L/v, the
turns' τ):
- t_s = 55.356464 s;
- t_l = 109.904888 s.

**Deterministic queueing** at the bottleneck (C = 600 veh/h) for a short share x with xq > C:
- the vehicle entering at time t into the period waits t·(xq/C − 1);
- the cohort's mean is cost_s(x) = t_s + (T/2)(xq/C − 1). When xq ≤ C, cost_s = t_s;
- cost_l = t_l, with no queue on the long route.

**Equilibrium:**
- x* = (C/q)·(1 + 2(t_l − t_s)/T) = **0.590914**, both at 109.905 s;
- the maximum queue is T(x*q − C)/3600 = 18.18 pcu, far below S1's 120.

**MSA on these analytic costs** (script, a reference, not asserted):
- from x = 0.5, δ is first ≤ 0.01 at n = 11, with x = 0.590909;
- n = 2 loads x = 0.75: cost_s 205.36 s, queue 50 pcu, still inside S1.

| Row | Asserts |
|---|---|
| MA1 | The cohort integral by hand (below): mean 4.55 s and 3 PCU censored; a flat piece takes its earliest time |
| MA2 | Cells: an interval cell takes its interval's cohorts; the default cell takes the gaps before, between and after the intervals |
| MA3 | Two classes on one entry, PCU 1 and 2: vehicle-weighted cost, counts in vehicles |
| MA4 | An empty choice (an interval weight of 0) is filled with t_s or t_l to 1e-9, and listed |
| MA5 | Uncongested costs equal free flow to 1e-3 s. Long at 1 : 1 is t_l; short at 0.3 : 0.7 (xq = 360 < C) is t_s |
| MA6 | The loop on the fixture, `gap` 0.01 and `maxIterations` 200: forcing first (the 1 : 1 run's δ > 0.01); then converged, and the short share within 0.01 of x* = 0.590914 |
| MA7 | Two loops give identical bytes; `maxIterations` 2 is unconverged with the stderr line |
| MA8 | `level` is written and read; a Micro file on `--meso` and a Meso file on Micro are `ROUTE_COSTS_STALE` at `level`; a D549 file without `level` still applies to Micro; the Meso run with the file's weights loads them |

Each row first asserts that its forcing worked.

**MA1 by hand.** The curves on a horizon of 12 s:
- E(t) = t on [0, 10], then flat at 10;
- A(t) = 0 on [0, 5], then t − 5 up to A(12) = 7.

Over N ∈ [0, 10):
- E⁻¹(N) = N;
- A⁻¹(N) = N + 5 for N ≤ 7. Above 7, A⁻¹ is the horizon, 12 s.

So the integrand is 5 for N ≤ 7 and 12 − N above. ∫₀⁷ 5 dN = 35, and ∫₇¹⁰ (12 − N) dN = 10.5. The
mean is 45.5 / 10 = **4.55 s**, and the censored range is **3 PCU** (7 → 10).

## 6. Slices

| Slice | Content | Rows |
|---|---|---|
| S0 | This contract | — |
| S1 | `src/project/meso_assignment.*` (the cohort integral and the Meso measurement); the level in `runDynamicAssignment`, the file and the CLI; the fixture | MA1–MA8 |

## 7. Out

- Placed and positioned decisions (with M9's A2b).
- Path enumeration beyond the authored entries.
- A Meso–Micro comparison of the loop's results (M12).
- Type rules (DYNAMIC_ASSIGNMENT §2).

## 8. Owner choices (answered 2026-11-13, D550; each the recommended option)

1. **Cost.**
   - **(a) Cumulative curves, entering to arriving, cohorts by demand time (chosen).**
   - (b) From the demand, including the origin's point queue.
2. **Fixture.**
   - **(a) Two routes and a bottleneck, deterministic-queueing hand values (chosen).**
   - (b) A3's Micro fixture.
3. **CLI.**
   - **(a) `--dynamic-assignment --meso`, with `level` in the file (chosen).**
   - (b) A separate flag.
4. **Decisions.**
   - **(a) Unplaced only, as A2 (chosen).**
   - (b) Placed as well.
