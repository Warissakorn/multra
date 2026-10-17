# Overtaking contract: keep to the kerb lane (M3.4)

Written 2026-10-17 (D213). **Slice 1 implemented** (D215, [evidence](../evidence/kerb-bias.md)); A53 still fails,
so no D95 value ships (§5). The owner settled three
choices on 2026-10-17:
- an asymmetric bias rule;
- undertaking is allowed but costs more;
- lane preference by vehicle type is a later slice.

Rows BA64–BA71 are in [the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows).
It builds on M3_8_CONTRACT §2's discretionary changes (D95/D101) and is tier B-1 of
[BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md). Rule 4 applies: nothing here is validated.

## 1. Scope and sources

The [driving-behaviour review](../audits/DRIVING_BEHAVIOUR_REVIEW.md) §3 G names the gap: there is no
return after passing and no keep-left/right bias. D95's free lane selection has "no side preference
and no pull back to the kerb lane" (M3_8_CONTRACT §2, discretionary item 2). D178 made keep-left the
default bias on left-hand-traffic networks, mirrored for right-hand ones, held in a behaviour key so
a study can weaken it.

Discretionary changes are off in shipped data. A53 fails, at about 5 % flip-backs against a 1 %
cap, and they reopen only by route (i) (D201). That route's prerequisite, the look-past over more
than the nearest vehicle, is now in: perceived leaders feed the incentive (D183).

**In scope:** returning to the kerb lane when free, passing on the passing side, and undertaking at
a higher cost.

**Out of scope:**
- lane preference by vehicle type (a later slice, owner; tier B-7);
- overtaking-lane bans;
- cooperative lane change;
- politeness toward other drivers (MOBIL's `p`).

## 2. The key

- **`kerbLaneBias`** is an optional behaviour key, in m/s². Its range is (0, 5], refused at its path
  with `INVALID_BEHAVIOUR_PARAMETER`.
- It is meaningful only with D95's pair. A behaviour carrying it without
  `discretionaryLaneChangeThreshold` and `acceptedDecelerationTrailingVehicle` is refused with
  `INCOMPLETE_DISCRETIONARY_BEHAVIOUR` at its path.
- W74 behaviours refuse the D95 keys today, so they refuse this key as well.
- **Absent means today's behaviour:** byte-identical, and every discretionary test is unchanged
  (BA64).
- **Schema 35.** The key is written only when an owned behaviour carries it. An older schema
  carrying it is refused with `EDIT_UNSUPPORTED_FIELD`.
- **No shipped value** (D178 requires cited values). Shipped runs keep discretionary changes off.

## 3. Which side is the kerb

Lane 0 is the kerb lane on both driving sides (`connector_paths.cpp`; `laneCentreOffset` mirrors
the offset by `drivingSide`). Each discretionary `LaneChangeSpan` gains a compiled `towardKerb`
flag: the target lane's index is lower than the source's (`lane_family.cpp` `spansBetween`, which
already requires adjacent indices).

A span in a hand-written scenario file has no lane index, so its `towardKerb` defaults to false and
the bias treats every move on it as outward.

The lane index therefore already carries the driving side. The rule needs neither the lateral switch
nor `Scenario::drivingSide`, and D178's "compiled from drivingSide" holds through the compile (hard
rule 3).

## 4. The rule

D95's incentive is `g = a_there − a_here ≥ T` with `T` = `discretionaryLaneChangeThreshold`. With
the bias `β` it becomes:

| Move | Condition | Required gain |
|---|---|---|
| **Return**: toward the kerb, while the vehicle is free on its own lane | its own lane's following mode is free | `g ≥ T − β` |
| **Pass**: away from the kerb | — | `g ≥ T + β` |
| **Undertake**: toward the kerb, while the vehicle follows a leader on its own lane | its own lane's following mode is not free | `g ≥ T + β` |

- With `β > T`, a free vehicle returns into an equally fast kerb lane (`g = 0`). This is the
  keep-left (or keep-right) behaviour.
- Passing costs `2β` more than a return, which damps flip-backs.
- Undertaking costs as much as passing: allowed, but only for a large gain (owner).
- The mode is the `FollowingMode` the vehicle already computes for its own lane this tick, so the
  rule stays stateless.
- **Unchanged:**
  - the trailing vehicle's safety test (`acceptedDecelerationTrailingVehicle`);
  - mandatory changes first;
  - vehicle-id order;
  - D101's hold;
  - D183's look-past with perception keys;
  - the best gain wins, and a tie keeps the lower slot.

## 5. Reopening D95

No shipped behaviour gets the D95 pair, with or without the bias, until A53 passes on the
lane-change lab's sweep. BA69 measures A53 with the bias and records it. Shipping a value stays an
owner decision with cited values (D178, D201).

## 6. Consumers

| Site | Change |
|---|---|
| `src/model/network/lane_family.cpp` `spansBetween` / `appendDiscretionaryLaneChanges` | Sets `towardKerb` |
| `LaneChangeSpan` (`types.hpp`) | Gains `towardKerb`. The file is at 499 of 500 lines, so add it on an existing line or free one |
| `src/core/lanes.cpp` discretionary pass | Applies §4's required gain |
| `src/core/validate.cpp` | Range check, and the D95 pair required |
| Codec (`parse.cpp`, `definition.cpp`, `behaviour_library.cpp`, `document.cpp`) | The key; schema 35 |
| Behaviour editor | One optional row beside the D95 keys (BEHAVIOUR_PARAMETERS §5) |

## 7. Performance

With the key absent, there is no cost. With it present, the cost is one comparison per discretionary
candidate.

## 8. Delivery

Slice 1 is §2–§4 with rows BA64–BA71, in one session, rows first: BA65 and BA67 fail first. Lane
preference by type (B-7) follows with its own section.

## 9. Failure conditions (reopen D213)

- D178's own: a study shows a keep-left bias harms lane utilisation on Thai urban arterials.
- A53 is still above its cap for every `β` in (0, 5].
- Flip-backs cluster at the end of D101's hold.
