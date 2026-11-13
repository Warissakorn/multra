# M6_U_RUN — M6 kind U: minor-movement capacity, U-c and U-a (D542)

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

`multra-m6-u-run <data-dir> <compile_commands.json> <new-output-dir>`

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

Return to [the reference index](README.md).
