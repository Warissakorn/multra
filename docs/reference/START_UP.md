# START_UP — moving off at green, as Vissim: acceleration by speed and lack of attention (D608)

The owner's choice, 2026-11-20 (verbatim): "เริ่ม G1 ทำเหมือน vissim", then "ก+ข", from
[realism-gaps-vissim-sumo-aimsun](../evidence/realism-gaps-vissim-sumo-aimsun.md) G1 (and G2). Not validated
(rule 4): Vissim's mechanisms are copied, not calibrated, and no shipped data uses them.

## 1. Why

[m0-plausibility](../evidence/m0-plausibility.md) measures the bare M0 crossing at a start-up lost time
of 0.89 s, against HCM's default of 2.0 s; the saturation headway (1.83 s) is already near HCM's 1.9 s.
Vissim has no start-up delay parameter: the lost time emerges from speed-dependent acceleration (W99's
CC8 at standstill and CC9 at 80 km/h, linear between and CC9 above, per PTV's 2023 help; acceleration
functions per vehicle type) and from *temporary lack of attention* (PTV help, *driver error*).

## 2. Acceleration by speed — `vehicleType.accelerationCurve` (ก)

- An array of `{"speed": m/s, "acceleration": m/s²}`, at least two points; the first at speed 0;
  speeds strictly rising; each acceleration in (0, 10] and none above the type's `maxAcceleration`.
- The type's acceleration at speed v is linear between points and the last value beyond the last
  point. Absent: `maxAcceleration` at every speed, to the last bit.
- It replaces `maxAcceleration` wherever the engine limits acceleration: the prototype's free flow,
  the W74 clamp and the perception clamp. Decelerations are unchanged.
- Vissim-like values (CC8 3.5 m/s², CC9 1.5 m/s² at 22.2 m/s) are cited in the measurement only:
  Louisiana DOTD lists CC8's default as 11.48 ft/s² (3.5 m/s²); 1.5 m/s² for CC9 comes from one study's
  settings, not a confirmed PTV default. PTV's help pages label CC8/CC9 inconsistently across versions.

## 3. Temporary lack of attention — `inattentionDuration`, `inattentionProbability` (ข)

- On the driver behaviour, both or neither: duration in (0, 10] s, probability in [0, 1].
- Time is cut into windows of `duration` (whole ticks, at least one). A vehicle is inattentive for a
  whole window when a draw from (seed, the vehicle's trait identity, the window index) falls below
  `probability`, as `redRunner` draws: the same seed gives the same windows, and no other vehicle
  changes them. **Assumption:** the help read does not define Vissim's probability precisely; here it
  is the share of windows.
- While inattentive, a vehicle keeps its previous tick's acceleration instead of reacting to the
  vehicle or line ahead (a standing vehicle stays standing), unless the fresh decision brakes harder
  than the type's comfortable deceleration: then it takes the fresh one (Vissim: it still responds to
  emergency braking). The tick cap and the safety clamp are unchanged. Amber, red running and give-way
  gap decisions are not attention lapses and keep their own rules.

## 4. Rows

| Row | Statement |
|---|---|
| SU1 | Both keys round-trip at schema 81 and are refused at 80 |
| SU2 | A curve not starting at 0, with falling speeds, an acceleration outside (0, 10] or above `maxAcceleration`, and a lone attention key are each refused at their path |
| SU3 | Without the keys every shipped project and the study give identical reports (the M0 guard) |
| SU4 | The curve (0 → 3.5, 22.2 → 1.5) gives 3.5, 2.5 and 1.5 m/s² at 0, 11.1 and 30 m/s |
| SU5 | Probability 1: a standing vehicle stays standing through its window after its leader moves off, then follows |
| SU6 | An inattentive vehicle behind a leader braking hard brakes too (the emergency exception) |
| SU7 | Same seed, same run; another vehicle's change leaves this vehicle's windows unchanged |
| SU8 | The type dialog's curve and the behaviour editor's pair each set in one Undo step |
| SU9 | Cost on `multra-engine-benchmark 48 900` measured before and after |

## 5. Measured on M0's crossing (seeds 42–51)

**Method.** M0's bare crossing has queues too short for a saturation headway, so a copy with saturated
demand (1,500 veh/h on each approach from 0 to 540 s, run 600 s; geometry, timing and vehicles unchanged)
was run, seeds 42–51, through `multra-cli --events`. At each green the vehicles standing on the approach
are ranked by position; h1 is green onset to the first one's front crossing the head (148 m), h_i the
gap between successive crossings, counting only crossings before the green and amber end. Saturation
headway is the mean of ranks 5 and on (queues are about seven long); start-up lost time is
h1+h2+h3+h4 − 4 × saturation headway (HCM's definition). Catalog copies outside `data/` carried each
variant; nothing shipped changed. On this method the shipped bare M0 gives 1.87 s and 1.34 s from only four
rank-5 samples, which is why the saturated copy is used; the m0-plausibility figures (0.89 s) used a
different ranking and are not comparable row for row.

| Variant (car type / default behaviour) | h1–h5 (s) | Saturation headway | Start-up lost time | Clamps per run |
|---|---|---|---|---|
| Today | 2.48 / 2.51 / 2.14 / 1.98 / 1.90 | 1.90 s | 1.53 s | 8–15 |
| ก, Vissim-like W99 values: max 3.5, curve 0 → 3.5, 22.2 → 1.5 m/s² | 2.09 / 2.34 / 2.06 / 1.93 / 1.87 | 1.86 s | 0.96 s | 8–14 |
| ข, Vissim's neutral preset: 1 s, 5 % | 2.28 / 2.56 / 2.16 / 2.01 / 1.92 | 1.92 s | 1.35 s | 6–13 |
| ก + ข | 2.22 / 2.35 / 2.06 / 1.94 / 1.88 | 1.87 s | 1.08 s | 7–16 |
| ข stronger: 2 s, 20 % | 2.99 / 3.01 / 2.44 / 2.20 / 2.47 | 2.48 s | 0.71 s | 6–11 |
| ก gentler: curve 0 → 2.0, 22.2 → 1.5 m/s² (max 2.5) | 3.06 / 2.68 / 2.31 / 2.12 / 2.01 | 2.01 s | 2.16 s | 10–18 |

HCM: saturation headway about 1.9 s (1,900 pc/h/ln), start-up lost time 2.0 s.

**Reading (no verdict).** Vissim's mechanisms do not raise lost time by themselves: its standstill
acceleration of 3.5 m/s² makes cars leave faster than today's 2.5 m/s² (0.96 s), and attention lapses at
the neutral preset hardly change it, while strong lapses slow every headway alike (the saturation headway
rises with them, so the lost time falls). Lost time follows the **standstill acceleration**: a curve
starting at 2.0 m/s² gives 2.16 s and 2.01 s, near HCM. Which value a study uses is calibration (M6), not a
default. The saturated copy clamps 8–15 times a run already today, as the bare M0's 1–3; the gentler
curve raises that to 10–18, to be looked at before any shipped value changes.

**SU9 not measured.** No before binary of `multra-engine-benchmark` was built in this session. Without
the keys the added work is one empty-curve test per limit and one absent-key test per vehicle and tick.
