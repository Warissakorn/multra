# Stopping cleanly at a line and behind a hard stop (M3.2.8a.1c)

Written 2026-10-19 (D256), as options measured for the owner's choice. The owner chose F1a the same
day, on by default (D257, §7); it is implemented by D258. The owner asked for this contract with measured prototypes before any code (D254). It covers the
clamps that D249, D252 and D253 traced to the prototype following law braking too softly. The
measurements are in the [evidence](../evidence/line-approach.md). No clamp-free claim follows from
it, and no gate is closed. Rule 4 applies: nothing here is validated.

## 1. The problem

The reduced prototype law (SIMULATION; `followingAcceleration`) brakes by a gap-based rule. Near a
standing obstacle, a line or a standing vehicle, that rule softens as the gap shrinks. A vehicle can
then reach the obstacle still moving, and the hard cap clamps it. The traced cases are:
- **Pull-out (J1, 9 of M2.6's 17 after D251).** A minor crawls onto its give-way line still moving,
  counts as `committed`, and pulls out in front of a major vehicle (MERGE_CLAMPS §1, D249's J).
- **Merge creep (M, 3) and one other.** The same softness at a held line or a standing leader.
- **The lane lab's one clamp (D252).** A follower brakes at only 4.4–4.8 m/s² (maximum 8) behind a
  leader stopping hard at its stub lane's dead end.
- **D227's red-onset clamp.** A slow car held at red overshot its line by 1.6 cm at 0.33 m/s.
- **D241's follower class.** Followers of red runners; D246/D247 removed the shipped cases, so a
  5 % red-running copy of M2.6 now has the same 17 clamps as M2.6 (measured, §3).

The late catch-up class (J2, 4) is a major vehicle catching a minor that is still accelerating
from rest. Neither floor below reaches it.

## 2. Options measured

Each option was a temporary patch of `src/core/following.cpp` (and, for J, of
`src/core/simulation.cpp`), built and reverted. All act on the prototype law only; the W74 law is
unchanged.

### F1: a kinematic floor behind a standing obstacle (D227's)

In the law's `braking` mode, with a leader standing (`speed == 0`, a line or a stopped vehicle) and
the vehicle moving, the acceleration is at most `−v² / (2 · gap)`. That is the deceleration that
stops it exactly at the obstacle. The law's final clamp to the type's `maxDeceleration` still
applies, so a stop beyond the maximum still clamps.

- **Unscoped, it changes bare M0 scenarios.** One of the five TypeScript reference rows fails
  (seed 2³²−1). Those rows must never be regenerated (AGENTS).
- **F1a, scoped:** the same floor only for a behaviour that carries `amberDeceleration`, the scope
  MERGE_CLAMPS §2 Q uses. The reference rows pass 5/5 and the CLI pin keeps its bytes. Every shipped
  project's behaviours carry the key, so F1 and F1a give identical shipped outputs.

### F2: a floor behind a leader braking hard

In any following mode but `free`, behind a moving leader that is braking (`acceleration < −0.1`),
the acceleration is at most `−v² / (2 · (gap − s0 + sL))`, where `sL = vL² / (2 · −aL)` is where the
leader would stop at its current braking. This is aimed at the lane lab's follower.

- **F2 unscoped** changes the pin and adds 2 M2.6 clamps.
- **F2c** applies the floor only when it asks for more than the type's
  `comfortableDeceleration`. **F2ca** is F2c scoped by `amberDeceleration`, like F1a.
- **It is broad.** F2c binds in about 1,900 law evaluations per M2.6 run, mostly just past the
  comfortable rate, so it reshapes ordinary braking everywhere, the four-leg included.

### J: a vehicle held at its line on the previous tick is not committed

D249's J, measured again on D251's defaults. A minor that has crept onto its line cannot stop in
about 0 m, so it clamps itself instead.

## 3. Measured (seeds 42–81, from D251's defaults)

| | Four-leg clamps / delay | Lane lab clamps | M2.6 clamps / delay | T-junction delay | Pin, TS rows | Rows (954) |
|---|---|---|---|---|---|---|
| D251 / D255 (today) | 0 / 46.001 | 1 | 17 / 46.010 | 7.069 | same, 5/5 | pass |
| **F1a** | **0 / 46.001, identical** | 1, identical | **5** / 46.026 | 7.099 | same, 5/5 | 1 forcing fails (BA164) |
| F1 (unscoped) | identical | identical | 5 / 46.026 | 7.099 | same, **4/5** | BA164, BA165 |
| F2ca | 0 / 45.986 | **0** | 17 / 46.031 | 7.069 | same, 5/5 | — |
| **F1a + F2ca** | 0 / 45.986 | **0** | **5** / 46.048 | 7.097 | same, 5/5 | 1 forcing fails (BA164) |
| F1 + F2 (unscoped) | 0 / 45.993 | 0 | 5 / 46.071 | 7.125 | **differs, 1/5** | 7 fail |
| J | identical | identical | **27** / 46.010 | 7.069 | same | — |

- **F1a's 12 removals** are J1 9 → 1, M 3 → 0 and the 1 other → 0, with no new clamp. The 5 left are
  J1 1 and J2 4. No overlap in any variant.
- **Delays.** F1a moves M2.6 by +0.016 s and the T-junction by +0.03 s (its minor westbound
  movement 25.13 → 25.49 s, ±2.96), all within the half-widths. Major movements move by ≤ 0.01 s.
  23 of 40 seeds change on M2.6 and on the T-junction; none on the four-leg or the lane lab.
- **How hard F1a brakes.** It binds in about 5 law evaluations per M2.6 or T-junction run, at a median of
  5.9–6.2 m/s² against the law's 3.9–4.1, never past the car's maximum of 8.
- **With `joinAnticipation` on** (D255), F1 gives M2.6 16 → 4 and T-junction 7.545 → 7.606 s.
- **BA164's forcing.** That row forces today's red-head creep clamp with `redLineStop` off. Under F1a
  the floor stops that creeper itself, so the forcing must also switch F1a off.

## 4. Recommendation (the session's, for the owner)

**Adopt F1a as a fifth `clampRules` switch, `standingStopFloor`, on by default.**
- It removes 12 of M2.6's 17 remaining clamps and adds none.
- It keeps the four-leg and lane lab byte-identical, the pin, the reference rows, and every row but
  one forcing.
- It moves delays by less than their half-widths.
- It is the law change D227 measured and booked for this moment, on a real case set this time.

**Do not adopt F2 in any form now.** It removes only the lane lab's clamp, and it reshapes ordinary
braking on every project (about 1,900 binding evaluations per M2.6 run). The lane lab's clamp stays traced and open (D252).

**Do not adopt J.** It turns the pull-outs into self-clamps (17 → 27).

**On or off by default is the owner's call.** On follows D250 for clamp relief, and it moves the
T-junction exercise by 0.03 s. Off keeps every shipped number.

## 5. Rows (BA173–BA178; implemented, D258)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| BA173 | A car with `amberDeceleration` in braking mode toward a standing obstacle that the law under-brakes (D227's slow car 0.33 m/s at its line; a creeper at a give-way line) | It stops at the obstacle with no clamp; the deceleration is `v²/(2·gap)` |
| BA174 | The same without `amberDeceleration` | Unchanged; the M0 demo, `multra-cli 42` and the TS reference rows keep their bytes |
| BA175 | A moving leader, or a mode other than `braking` | Unchanged |
| BA176 | A stop beyond the type's `maxDeceleration` | Capped at the maximum, so it still clamps (or goes as committed at a line) |
| BA177 | The switch off, alone and with all others off | Today's bytes on the four shipped projects; BA164's forcing switches it off |
| BA178 | Codec and editor | The key at a new schema, round trip, an older schema refused, a non-boolean refused at its path, the Run settings checkbox as one Undo step |

## 6. What stays out

- The late catch-up class (J2) and the lane lab's clamp.
- The W74 law, which has its own braking (W74 §6).
- Calibration of either floor.
- Comfort measures for the harder stops F1a makes.

## 7. The owner's choice and its settings (D257)

The owner took the recommendation on 2026-10-19: F1a, as a project switch that is **on by default**.

- **The key.** `clampRules.standingStopFloor`, a boolean beside MERGE_CLAMPS §3's three and
  JOIN_ANTICIPATION §5's `joinAnticipation`.
- **Absent means on.** The file writes the key only when it is `false`, and that file is
  **schema 47**. A schema 46 or older file carrying it is refused with `EDIT_UNSUPPORTED_FIELD` at
  `definition.clampRules.standingStopFloor`. A non-boolean is refused with `INVALID_CLAMP_RULE`.
- **Scope.** Only a behaviour carrying `amberDeceleration` gets the floor. Bare M0 scenario files
  take no key, so their behaviours without the key keep their bytes, and so do the TypeScript
  reference rows and the CLI pin.
- **Editor.** Run settings gains a fifth checkbox, labelled `standingStopFloor` and ticked by
  default. Confirming the dialog is one Undo step with the other run settings.
- **Engine (D258).**
  - `ClampRules::standingStopFloor`, default `true`.
  - `DriverBehaviour::stopFloor` is runtime-only and never in a file. `createSimulation` (and retune,
    through the same canonical copy) sets it to the switch and `amberDeceleration` together.
  - `followingAcceleration` reads it, so every evaluation of the law gets the floor, hypothetical
    ones included, exactly as D256 measured.
- **Measured (D258).**
  - The four shipped projects reproduce D256's F1a outputs byte for byte on seeds 42–81.
  - Switched off, they are byte-identical to D255.
  - The pin is unchanged, and the reference rows pass 5/5.
  - Ir is +0.13 % (four-leg), +0.18 % (lane lab) and +0.13 % (M2.6).

