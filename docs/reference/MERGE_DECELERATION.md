# Accepted deceleration for mandatory lane changes (B-3)

Written 2026-10-17 (D220), and implemented the same day (D221,
[evidence](../evidence/merge-deceleration.md)). This is tier B-3 of
[BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), a level-4 key pair under D216: driver behaviour, local
by road. Rule 4 applies: nothing here is validated, and no value ships (D178).

## 1. Scope and the owner's choices

A mandatory lane change (M3_8_CONTRACT §2) is accepted only when the car-following model itself
accepts it, ahead and behind (`lanes.cpp` `safeAt`):
- **forward:** the changer, behind its new leader, would brake no harder than its type's
  `comfortableDeceleration`;
- **rear:** its new follower would brake no harder than the follower type's
  `comfortableDeceleration`.

Those two values set merging and weaving capacity. Drivers in Thai cities accept much harder braking
when they force in, so a study needs them.

The owner chose on 2026-10-17:
- **Two keys, as Vissim has:** one for the changer's own braking, one for the trailing vehicle's.
- **The trailing limit is read from the changer's behaviour,** as in Vissim and as D95's
  `acceptedDecelerationTrailingVehicle` already is. The driver who forces in decides how hard the
  follower must brake.

## 2. The keys

Both are optional, in m/s², range (0, 15], on either model, and independent of each other. A value
outside the range, or a non-number, is refused with `INVALID_BEHAVIOUR_PARAMETER` at its path.

| Key | Test | Limit used |
|---|---|---|
| `mandatoryDecelerationOwn` | forward | the key, for the changer's braking |
| `mandatoryDecelerationTrailingVehicle` | rear | the changer's key, for the follower's braking |

- **Absent means today's limit:** the comfortable deceleration of the same type, so runs are
  byte-identical.
- **Above the maximum:** the car-following model never asks for more than the braking vehicle's
  `maxDeceleration` (`following.cpp` clamps it), so a key above it acts as that maximum. The
  contract first wrote an explicit `min(key, maxDeceleration)`; the mutation check showed it could
  never change an outcome, so the code has none (D221). Unlike D219, no vehicle is ever asked for more
  than its brakes give here.
- **Schema 37.** The keys are written only when an owned behaviour carries one. An older schema
  carrying one is refused with `EDIT_UNSUPPORTED_FIELD`.

## 3. Where they apply, and where not

- **Only the mandatory pass.** `safeAt` gains a `mandatory` flag; the mandatory call (`lanes.cpp`
  :348) sets it. The forward test with perception keys (against the perceived leaders) uses the same
  own limit.
- **Unchanged:**
  - the discretionary pass: it keeps the comfortable values, and D95's
    `acceptedDecelerationTrailingVehicle` stays its stricter rear test (M3_8 §2);
  - the condition that this tick's move fits inside the room the leader leaves, so an accepted change
    never clamps;
  - the cooperative and courtesy holds (D90, D207), whose limits are the follower's own;
  - rule 5 (nothing alongside), the hold, and vehicle-id order.
- **Not in scope:** Vissim's reduction of the accepted deceleration with distance to the emergency
  stop; cooperative lane change; a Lab row (the Lab clears lane changes until the circuit's wrap rule
  exists).

## 4. Consumers

| Site | Change |
|---|---|
| `src/core/lanes.cpp` `safeAt` | `mandatory` flag, the two limits |
| `src/core/lanes.cpp` `mandatoryParameterIssues` | Range; called from both model branches of `validate.cpp` |
| `src/core/types.hpp` | The two keys on the existing optional line (the file is at 499 lines) |
| Codec (`parse.cpp`, `definition.cpp`, `behaviour_library.cpp`, `document.cpp`) | The keys; schema 37 |
| Behaviour editor | Two rows in D217's either-model section |

## 5. Rows (BA79–BA85)

| Row | Case | Asserts |
|---|---|---|
| BA79 | Keys absent | `lanechange` and `discretionary` groups unchanged; shipped outputs byte-identical |
| BA80 | A stub changer whose target-lane leader would make it brake between comfortable and maximum | Forcing: refused today. With `mandatoryDecelerationOwn` 4 it changes, with no clamp |
| BA81 | A target-lane follower that would brake between comfortable and maximum | Refused today. With `mandatoryDecelerationTrailingVehicle` 4 on the changer's behaviour it changes. The key on the follower's behaviour only still refuses |
| BA82 | Each key at 15 | Every tick equals the key at the type's `maxDeceleration` (following's own clamp) |
| BA83 | A discretionary scene with the D95 pair and both keys | Every tick equals the scene without the keys |
| BA84 | BA80 with a W74 behaviour | It changes with the key, not without |
| BA85 | Codec | Schema 37 round-trips; 36 carrying a key → `EDIT_UNSUPPORTED_FIELD`; `"4"`, 0 or 16 → `INVALID_BEHAVIOUR_PARAMETER`; 15 accepted; 38 → `EDIT_VERSION` |

Plus a measurement: `four-leg-signalised`, `m2.6-study-template` and `lane-change-lab` at seeds 42–51,
with (own, trailing) at (3, 3), (4, 4) and (2, 4) m/s², against absent. Record delay, clamps and the
number of lane changes.

## 6. Performance

One test of each key per mandatory candidate. With the keys absent, the cost is unchanged
within noise.

## 7. Failure conditions (reopen D220)

- Clamps rise at values a cited study uses, although an accepted change cannot clamp by construction:
  then the follower's later braking needs its own model.
- A study needs the distance-dependent reduction (Vissim's −1 m/s² per distance): a later slice.
