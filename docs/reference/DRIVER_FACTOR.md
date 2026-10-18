# Driver-factor spread (B-4)

Written 2026-10-18 (D235). This is tier B-4 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), a
level-4 key under D216: a driver-behaviour key. It is read once, when a vehicle is made, so it does
not change along the vehicle's path (§3). It sets the spread of the per-vehicle `driverFactor`
([SIMULATION](SIMULATION.md), "Following"). Rule 4 applies: nothing here is validated, and no value
ships (D178).

## 1. Scope and the owner's choices

Each vehicle draws `driverFactor = clamp(0.5 + 0.15·N, 0, 1)` once, when it is made
(`samplePending`, `demand.cpp`). Here N is a standard normal from two Box–Muller draws. The factor
scales only the safety distance:
- the prototype's `multiplicativeSafetyDistance` term, in `desiredGap` and `stopLineReach`;
- W74's `bxMult` term, where it is the trait `zBx` ([W74 §5](W74.md#5-parameters-and-traits)).

The 0.15 is fixed, so a study cannot make drivers more alike or more varied.

The owner chose on 2026-10-18:
- **On the driver behaviour.** The vehicle type's own behaviour (`behaviourId`) gives the value when
  the vehicle is made.
- **The standard deviation only.** The mean stays 0.5: a different mean would only repeat what
  `additiveSafetyDistance` already sets.

## 2. The key

- **`driverFactorSpread`** is an optional behaviour key on either model, with no unit. Its range is
  [0, 0.5]. A value outside it, or a non-number, is refused with `INVALID_BEHAVIOUR_PARAMETER` at
  `behaviours[i].driverFactorSpread`.
- **Schema 43.** It is written only when an owned behaviour carries it. An older schema carrying it
  is refused with `EDIT_UNSUPPORTED_FIELD`.
- **Absent means today:** the spread is 0.15. Every run is byte-identical. The catalog ships no
  value.

## 3. The rule

- **The draw.** `driverFactor = clamp(0.5 + s·N, 0, 1)`:
  - s is the key on the type's own behaviour, `scenario.behaviours[index.behaviourOfType[type]]`;
  - without the key, s is 0.15.
- **The random stream is unchanged.** The two draws behind N are taken whatever s is, so arrival
  times, desired speeds and every later draw stay the same. With s = 0.15 the factor equals today's
  in every bit.
- **s = 0** gives every driver 0.5.
- **Clamped to [0, 1].** A wide spread therefore piles some drivers at 0 and 1. At 0.5, about 16 %
  land on each bound.
- **Drawn once.** A segment behaviour assignment (M3.3.2b) does not redraw the factor, as
  DRIVING_BEHAVIOUR requires. A vehicle that enters a road with another behaviour keeps its factor.
  Its spread came from its type's behaviour, not the road's.
- **Every source.** `samplePending` makes input arrivals, a parking lot's initial occupants (M8.4a)
  and Behaviour Lab additions, so the rule reaches all three.
- **Retune.** A live retune that changes the key leaves vehicles already made with their factor.
  Vehicles made after it draw with the new value.
- **W74.** `zBx` is the driverFactor, so it follows the key. `zOp` is hashed from its own
  Irwin–Hall sum with a fixed 0.15 and stays fixed (§5).

## 4. Editor

The behaviour dialog gains an off-until-ticked row for `driverFactorSpread`, among the keys either
model carries. It sits beside `stopDwellTime` and, like it, is labelled with the key name. Editing
it is one Undo step with the library.

## 5. What stays out

- A mean, a curve or another distribution family.
- A spread taken from the road the vehicle enters on.
- W74's `zOp` spread, and any use of the factor beyond the safety distance (gap acceptance,
  acceleration).

## 6. Rows (BA131–BA137)

| Row | Case | Asserts |
|---|---|---|
| BA131 | No key | Factors and shipped outputs byte-identical |
| BA132 | `driverFactorSpread: 0` | Every vehicle's factor is 0.5; arrival times and desired speeds are unchanged |
| BA133 | 0.3 | The sample's standard deviation is near 0.3 (more than 0.15 measured on the same seed), mean near 0.5, every factor in [0, 1] |
| BA134 | 0.15 | Byte-identical to no key |
| BA135 | Two types, only one behaviour carrying the key | The other type keeps 0.15 |
| BA136 | A segment assigned a behaviour with another spread | A vehicle's factor does not change when it crosses onto that segment |
| BA137 | Codec, validation, editor | Schema 43 round-trips; 42 → `EDIT_UNSUPPORTED_FIELD`; −0.1, 0.6 and a string refused at the path; the dialog row writes it |
