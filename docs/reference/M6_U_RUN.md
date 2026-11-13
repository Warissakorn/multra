# M6_U_RUN — M6 kind U: minor-movement capacity, U-c and U-a (D542), and the Stop follow-up split (D543)

Contract for kind U ([m6-benchmark-options](../evidence/m6-benchmark-options.md) §5–§6). The
engine's capacity for a Stop-controlled rank-2 minor movement is compared, at five major flows,
with the closed-form capacity at the engine's own effective `t_c` and `t_f` (U-c, a verification
step) and with HCM 7 Eq. 20-18 at the base values 6.2 / 3.3 s (U-a). Written before code and
before any engine figure; rows U1–U6 fail first. A miss is reported, never tuned away (rule 4).

## 1. Three readings of §6, fixed before any run (D542)

1. **A T, not a four-leg.** The rank-2 movement (the minor left turn on this left-hand drawing)
   has no impedance: HCM 7 Ch. 20 sets its movement capacity equal to its potential capacity. No
   demand would use a fourth leg, so drawing one would add only unloaded conflicts. The far major
   lane carries the same flow as the near lane. That makes a forcing check (U5): the counter must
   count the near stream only.
2. **Minor demand 1,800 veh/h, not 900.** At `v_c` = 200 veh/h, 900 lies only 6 % above HCM's 846.
   If the engine's Stop service gives a shorter `t_f`, the counted figure would be demand, not
   capacity. 1,800 veh/h is the zero-major capacity at `t_f` = 2 s. This is set by arithmetic, not
   by an engine figure. A saturation guard (§4) still applies.
3. **`τ` includes the car's length.** §6 defines `τ` as the time the earlier major vehicle takes
   to clear the conflict area at 50 km/h. The engine holds a grant until the vehicle's **rear**
   clears the area (M3_CONTRACT §4), so `τ = (L + 4.5 m) / (50 / 3.6 m/s)`. Here `L` is the merge
   area's major-side `exit − entry`, read from the built document, and 4.5 m is the length of
   `car.json`. Then `gapTime = 6.2 s − τ` and `headway = 0.1 m`.

## 2. The projects (built in memory, not shipped)

`tools/m6_u_network.hpp` builds `m6u::junction(vc)` through the editor commands. It is modelled
on `tools/t_junction_network.hpp`, left-hand.

- **Major road.** One lane each way, 3.5 m wide, with a 1 m median, 300 m long. The near
  (kerb-side) lane carries the stream the minor left turn joins.
- **Minor approach.** One lane, 150 m long, ending 12 m short of the major axis. One Connector,
  the minor left turn, joins the near lane 20 m past the minor axis. No other turn is drawn.
- **Control.**
  - The merge the Connector creates is taken over (`takeOverMergesOf`).
  - Its priority is set so that the minor side yields, with `gapTime` and `headway` from §1.3.
  - Its line is Stop (`setAreaControl`).
- **Demand.** All `car`, from 0 to 4,500 s, Poisson sources, `perInput` streams:
  - `vc` veh/h on each major lane;
  - 1,800 veh/h on the minor left turn.
- **Run.**
  - dt 0.1 s, duration 4,500 s;
  - evaluation warm-up 900 s, window end 4,500 s, no cool-down;
  - `default.json` behaviour.
- **Levels.** `vc` = 200, 400, 600, 800 and 1,000 veh/h.

## 3. The command

`multra-m6-u-run run <data-dir> <compile_commands.json> <new-output-dir>` (no mode until D543)

- **Refusals.** The tool refuses an existing output directory and a wrong argument count, with
  exit status 1 and 2.
- **Runs.** For each level and each of seeds 42–81, the tool compiles the document. A
  `PriorityLineAccumulator` (D528) built from the document's evaluation spec observes the
  initial state and every step.
- **Per seed**, from the one area's row:
  - `minorPerHour`, the figure;
  - `majorPerHour`, the counted `v_c`;
  - `followUps` and `followUpMean`;
  - the Siegloch points and `sieglochGaps`;
  - `share = sieglochGaps / (majorEntered − 1)`.
- **Outputs**, in the new directory:
  - `runs.csv` (level, seed and the per-seed figures);
  - `summary.json`;
  - `metadata.json`, written **last**, with `complete: true`, the build commit, the compiler
    command and the SHA-256 of the tool's sources.

## 4. Reductions and references

- **Engine capacity per level.** The mean of `minorPerHour` over the 40 seeds, with its 95 % CI
  (`m6s1::Figure`), and the mean counted `v_c`.
- **Saturation guard.** A level is **unavailable** when its mean `share` is below 0.9. Its
  references are still shown.
- **Harders' form**, HCM 7 Eq. 20-18:
  `c(v_c, t_c, t_f) = v_c·e^(−v_c·t_c/3600) / (1 − e^(−v_c·t_f/3600))`.
- **Siegloch's form**, shown without a band: `c = (3600/t_f)·e^(−v_c·t_0/3600)`.
- **U-c reference.**
  - The Siegloch points of all levels and seeds are pooled. Per `n`, `gaps` is the sum and
    `meanGap` is the gap-weighted mean.
  - `sieglochFit` on the pooled points gives one `t_0`, `t_f(slope)` and `t_c`.
  - `t_f` is the pooled `followUpMean`, weighted by `followUps`.
  - The reference is `c(v̄_c, t_c, t_f)` at each level's mean counted `v_c`.
  - Per-level fits are shown, not used.
- **U-a reference.** `c(v̄_c, 6.2, 3.3)` at the counted `v_c`. The §6 values at nominal `v_c`
  (846.1, 654.3, 504.6, 388.1, 297.7 veh/h) are shown beside it.
- **Verdict.** For each level and reference: **pass** when the engine's mean lies within ±15 %
  of the reference, edge inclusive (`m6s1::verdict`, band 0.15); otherwise **miss**. An
  unavailable level has no verdict.

## 5. Acceptance rows

| Row | Statement | Group |
|---|---|---|
| U1 | `junction(600)` has one merge area: minor yields, Stop on its line, rule `headway` 0.1 and `gapTime` = 6.2 − (L + 4.5)/(50/3.6) with `L` from the area (a hand value); inputs of 600 on both major lanes and 1,800 on the minor; duration 4,500 s, window 900–4,500 s, cool-down 0 | `m6u` |
| U2 | `harders(vc, 6.2, 3.3)` gives §6's five U-a values to 0.1 veh/h; `siegloch(vc, t0, tf)` gives a hand value | `m6u` |
| U3 | Pooling hand points from two runs gives the gap-weighted `meanGap` per `n` and the fit on them; hand follow-ups give the weighted pooled mean | `m6u` |
| U4 | A level with mean share 0.89 is unavailable and one at 0.9 is available; a mean exactly at +15 % of the reference passes | `m6u` |
| U5 | `junction(600)`, cut to 300 s with a 60 s warm-up, seed 42: the counted major rate is below 900 veh/h, so the far lane is not counted (both lanes give about 1,200); there are minor crossings; `share` lies in [0, 1] | `m6u` |
| U6 | `multra-m6-u-run` with no arguments prints its usage line and exits 2; with an existing output directory it exits 1 | `cli-m6-u-run-help`, `cli-m6-u-run-refused` |

## 6. The follow-up split (D543): where the 13.7 s goes

**Why.** The first run ([m6-u-first-run](../evidence/m6-u-first-run.md)) measured a follow-up
headway of 13.7 s, against HCM's 3.3 s. This section splits each follow-up into the engine's
own events. It has **no band, decides nothing and changes no parameter**. Written before code;
rows SU1–SU5 fail first.

**Command.**
- `multra-m6-u-run split <data-dir> <compile_commands.json> <new-output-dir>` runs the five §2
  levels, seeds 42–81.
- §3's run becomes mode `run`, so the tool's usage is
  `multra-m6-u-run run|split <data-dir> <compile_commands.json> <new-output-dir>`.

**What is recorded.** The observer is `tools/m6_u_split.hpp`. It sees the initial state and
every step, on the one area.
- **Minor crossings.** The front passes `waitAt`, interpolated as the priority-line counter does
  (PRIORITY_LINES §1).
- **Major entries.** The front passes `entryAt`, interpolated in the same way.
- **First standing.** The first state in which a minor vehicle, short of the line, is slower than
  the queue definition's `beginSpeed`.
- **Service start `s0`.** The first state in which the vehicle has a `StopService` at this line.
- **Served `s1`.** The first state whose tick exceeds `since + ticks`, that is, once `restsAtStop`
  is false.
- **Trace.** While the vehicle is the minor vehicle nearest the line and short of it: time,
  front distance and speed.
- **Blocked, per state.** The gap rule of M3_CONTRACT §4 is applied to every vehicle on a route
  whose major side meets the area, with the area's `gapTime` and `headway`. Let `d = entryAt − front`:
  - `d ≥ 0`: the vehicle blocks when `d ≤ headway`, or when its speed is above 0 and
    `d / speed < gapTime`;
  - `d < 0`: it blocks while its rear is short of `exitAt` (occupying).

  The state is blocked when any major vehicle blocks. This is the rule's own test recomputed
  from the state; the engine's internal flag is not read.

**A pair.** Leader `l` and follower `f` are consecutive minor crossings, both in the window
[900, 4,500] s, and:
- there is no major entry in `(t_l, t_f)`;
- `f` first stood at or before `t_l`;
- `f`'s `s0` lies at or after `t_l`.

This is the counter's follow-up, except that queueing uses standing alone (no `maxGap`): the
queue at the line is dense. Both counts are reported.

**Phases of a pair.** Their sum is the headway `t_f − t_l` exactly.

| Phase | Definition | Sub-measures |
|---|---|---|
| `moveUp` | `s0 − t_l` | **`settle`:** `s0` minus the first trace time at or after `t_l` at which `f`'s front is within 0.5 m of its front at `s0`. **`moveDistance`:** `f`'s travel from `t_l` to `s0`. **`peakSpeed`:** its maximum speed in that interval |
| `dwell` | `s1 − s0` | |
| `release` | `t_f − s1` | **`blocked`:** dt × the states in `[s1, t_f)` that are blocked. **`free`:** `release − blocked`. **`standDistance`:** `waitAt` minus `f`'s front at `s0` |

**Report.** `summary.json` gives, per level:
- the number of pairs, and the counter's `followUps` and `followUpMean` for the same runs;
- each phase and sub-measure as a mean over seeds (each seed's mean over its pairs), with a 95 % CI.

`split.csv` (one row per pair) is not vendored. `metadata.json` is written last.

| Row | Statement | Group |
|---|---|---|
| SU1 | Hand times give `moveUp`, `dwell` and `release` that sum to the headway; hand blocked flags give `blocked` and `free` that sum to `release` | `m6u` |
| SU2 | The block test on hand values: `d` = headway blocks; `d` = 10 m at 2 m/s blocks with `gapTime` 5.804 s (5 s) and does not at 1 m/s (10 s); `d / speed` exactly `gapTime` does not block; a standing vehicle beyond `headway` does not; occupying blocks until the rear passes `exitAt` | `m6u` |
| SU3 | `settle`, `moveDistance` and `peakSpeed` on a hand trace give hand values | `m6u` |
| SU4 | `junction(600)` cut to 300 s with a 60 s warm-up, seed 42, has pairs. Every pair's phases sum to its headway within 1e−9. Every `dwell` equals `(ticks + 1)·dt` = 0.2 s (the forcing: the service is what `restsAtStop` says) | `m6u` |
| SU5 | The usage line lists `run\|split` | `cli-m6-u-run-help` |

Return to [the reference index](README.md).
