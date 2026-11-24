# M0 plausibility — measured run and SUMO comparison, 2026-10-27

Evidence for the owner's M0 ruling ([D369](../decisions/RECORD-D200-D399.md#d369)). It is an observation aid
and a cross-tool comparison. It is **not** calibration or validation: M6 and the unvalidated marker
(D13) are unaffected.

## What was run

- **MULTRA:** the bare M0 scenario `data/scenarios/crossing.json` (engine `0.2.0-cpp-m0`), seeds
  42–51. Seed 42 reproduces the `multra-cli 42` baseline (45 completed, mean delay
  31.75870154816726 s, 1 safety clamp).
- **SUMO 1.18.0** (Ubuntu package), with the same geometry, timing, demand and vehicle data:
  - two one-lane approaches of about 150 m, with the 64 s cycle split as the scenario's programs;
  - Poisson inputs of 900 and 700 veh/h from 0 to 90 s;
  - `accel` 2.5, `decel` 2, `emergencyDecel` 8 and length 4.5 m;
  - desired speeds of about 40–60 km/h (`speedFactor` normal 0.833 ± 0.1, cut at 0.667–1);
  - step 0.1 s.

  Two car-following models ran with SUMO defaults otherwise: **Krauss** (SUMO's default) and
  **IDM**. Input files are `m0-plausibility-sumo.*.xml`.

## Results (means over seeds 42–51)

| Measure | MULTRA | SUMO Krauss | SUMO IDM |
|---|---|---|---|
| Saturation headway (queued vehicle 6 onwards) | **1.83 s (1,967 veh/h)** | 1.42 s (2,537 veh/h) | 1.89 s (1,907 veh/h) |
| Headways of queued vehicles 2–5 (s) | 2.93 / 2.20 / 2.04 / 1.94 | 2.43 / 1.69 / 1.45 / 1.33 | 2.90 / 2.20 / 2.09 / 1.95 |
| Start-up lost time (h1–h4 minus 4 × saturation headway) | 0.89 s | 0.69 s | 0.44 s |
| Standing front-to-front spacing (median, range) | 7.24 m (6.50–7.75) | 7.00 m | 6.98 m |
| Maximum acceleration | 2.5 m/s² | 2.5 | 2.5 |
| Red passes | 0 | 0 | 0 |
| Safety clamps per run | **1–3** | 0 | 0 |
| Ticks at full braking (−8 m/s²) per run | **6–30** | 0 (worst −3.1 m/s²) | 0–1 |
| Amber passes per run | 0–2 | 0–4 | 1–3 |

For reference, HCM's base saturation flow is 1,900 pc/h/ln (headway of about 1.9 s), and its
default start-up lost time is 2.0 s.

## Reading

- **Accelerate, queue at red, discharge at green:** MULTRA's discharge-headway profile and standing
  spacing lie with SUMO IDM and near HCM's base values. SUMO's default Krauss discharges faster
  than HCM's base. No model has a perception-reaction delay at green, so all three under-run HCM's
  start-up lost time.
- **Amber is the one difference.** Every MULTRA clamp and every full-braking tick falls on **amber
  within 40 m of the stop line**. A bare M0 scenario holds amber as red (D147, AMBER §AM7), so a
  driver who cannot stop is held at the line. SUMO lets that driver go on amber. Projects use
  M4.2's stop-or-go rule (`amberDeceleration`); the bare M0 path keeps the frozen baselines.
- Mean delay is not compared. SUMO's `timeLoss` is defined differently, and insertion and random
  streams differ, so completed-trip counts differ too.

## Reproduce

From the repository root, with a configured `desktop` build:

```sh
cmake --build build/desktop --target multra-cli
g++ -std=c++20 -O2 -I. docs/evidence/m0-plausibility-trace.cpp -o m0trace \
  -Wl,--start-group build/desktop/libmultra_*.a -Wl,--end-group -lproj
for s in 42 43 44 45 46 47 48 49 50 51; do ./m0trace data/scenarios/crossing.json data $s > OUT/tr_$s.csv; done
```

Then, in `OUT/`, build the net with `netconvert -n …sumo.nod.xml -e …sumo.edg.xml -x …sumo.con.xml
-o net.xml --no-turnarounds --tls.default-type static`. For each model and seed, run `sumo -n
net.xml -r …sumo.<krauss|idm>.rou.xml -a …sumo.tl.xml --step-length 0.1 --end 180 --seed S
--fcd-output fcd_<Krauss|IDM>_S.xml --fcd-output.acceleration --tripinfo-output
ti_<Krauss|IDM>_S.xml`. Finally run `python3 -I docs/evidence/m0-plausibility-compare.py OUT`.

## Under project rules (D652, D653)

Added 2026-11-24. The bare M0 scenario holds amber as red, and LINE_STOP leaves it unchanged (D652). Projects run
under the project rules instead:
- **Amber:** stop or go (`amberDeceleration` 3 m/s², the shipped default behaviour's value);
- **Lines:** `clampRules.lineRest`, which stops the front at the line.

This look runs the same scene under those rules. The trace tool's optional arguments give every behaviour
`amberDeceleration` and set the switch. The SUMO runs are the same as above (SUMO 1.18.0, re-run on the same
inputs). The compare script and its measurement are unchanged; it computes its own crossings (`x < 0`).

| Measure | M0 (above) | Amber stop-or-go, switch off | Amber stop-or-go, switch on | SUMO Krauss | SUMO IDM |
|---|---|---|---|---|---|
| Saturation headway (vehicle 6 on) | 1.83 s | 1.82 s | 1.83 s | 1.42 s | 1.89 s |
| Headways h1–h5 (s) | 1.03 / 2.93 / 2.20 / 2.04 / 1.94 | 1.39 / 2.63 / 2.18 / 2.02 / 1.93 | 0.10 / 3.57 / 2.29 / 2.10 / 1.94 | 0.79 / 2.43 / 1.69 / 1.45 / 1.33 | 0.80 / 2.90 / 2.20 / 2.09 / 1.95 |
| Start-up lost time (h1–h4 − 4 × saturation) | 0.89 s | 0.92 s | 0.74 s | 0.69 s | 0.44 s |
| Standing spacing, median (range) | 7.24 m (6.50–7.75) | 7.35 m (6.79–7.74) | 7.21 m (6.74–7.80) | 7.00 m | 6.98 m |
| Safety clamps per run | 1–3 | 0 | 0 | 0 | 0 |
| Ticks at full braking per run | 6–30 | 0–15 | 0–6 | 0 | 0–1 |
| Amber passes per run | 0–2 | 2–6 | 2–4 | 0–4 | 1–3 |
| Red passes, total over 10 runs | 0 | 6 | 0 | 0 | 0 |

**Reading:**
- **With both project rules, MULTRA's profile is closest to SUMO's:**
  - no clamps and no red passes;
  - full braking 0–6 ticks per run;
  - amber passes 2–4 per run, as SUMO's 0–4;
  - saturation headway and standing spacing unchanged.
- **h1 is 0.10 s.** The queue head stands at the line, so it crosses one tick after green. SUMO's heads cross at
  about 0.8 s, and M0's head, 2–3 m short, at 1.0 s. The rest of the queue is correspondingly later (h2 3.57 s).
  The sum, start-up lost time 0.74 s, lies between SUMO's two models. All of them lie below HCM's 2.0 s, since no
  model reacts at green.
- **The amber rule without the line rule passes on red 6 times in ten runs.** A committed driver's speed term
  crawls on toward the line past amber's end. This script counts a crossing at red as a red pass. With the line
  rule there are none.
- **Not a gate:**
  - M0 stays closed (D369);
  - the four TypeScript references and `multra-cli 42` are untouched;
  - nothing is calibrated or validated.

Reproduce:
1. Build the trace as above.
2. Run `./m0trace data/scenarios/crossing.json data S 3 1` (switch on) or `… S 3 0` (switch off) for seeds
   42–51 into separate directories.
3. Add the SUMO outputs to each directory.
4. Run the compare script on each directory.
