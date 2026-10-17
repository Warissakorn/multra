# Desired-speed decisions and reduced-speed areas (B-6)

Written 2026-10-17 (D222). §2–§6 (decisions) are implemented the same day (D223,
[evidence](../evidence/speed-decisions.md)); §7 (reduced-speed areas) is a contract only. This is
tier B-6 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), level 5 under D216: a property of a
place, not of a driver. It follows [SPEED_CONTROL](SPEED_CONTROL.md) (curve speed, M3.6). Rule 4
applies: nothing here is validated.

## 1. Scope and the owner's choices

Today only the vehicle type's desired-speed range limits speed. Nothing on the road can change it,
so a speed-limit change along a corridor cannot be modelled. Limits, and how far drivers exceed
them, differ by country.

The owner chose on 2026-10-17:
- **Vissim's model.** A *desired-speed decision* gives each vehicle type a new desired-speed range.
  A vehicle keeps its place in the range: a fast driver stays fast. Speeding is expressed by the
  range itself (for example 50–65 km/h behind a 50 km/h sign).
- **Delay against the speed in force.** Driving slower because of a sign is not delay. The curve
  bound still counts as delay (SPEED_CONTROL §5).
- **Order:** decisions first, complete with engine, file and editor placement. Reduced-speed areas
  are next (§7).

## 2. The object

`definition.speedDecisions` is an array; the object belongs to the definition, beside the routing
decisions, because it names vehicle types. Each entry has:
- `id` and an optional `name`;
- `linkId` and `station`: metres along the Link's reference line, `0 ≤ station ≤ length`. It applies
  across every lane, placed as a travel-time section line is.
- `speeds`: one or more `{vehicleTypeId, min, max}` in m/s, as type ranges are, with
  `0 < min ≤ max`, finite. Type ids must be known and must not repeat. A type not listed is
  unaffected.

Validation refuses each fault at its path:
- `UNKNOWN_SPEED_DECISION_LINK` for an unknown Link;
- `INVALID_POSITION` for a station off the Link;
- `UNKNOWN_VEHICLE_TYPE` and `DUPLICATE_ID` for the type ids;
- `INVALID_RANGE` for the speeds, or for an empty list;
- `INVALID_ID` and `DUPLICATE_ID` for the decision's own id.

**Schema 38.** The array is written only when non-empty. An older schema carrying it is refused with
`EDIT_UNSUPPORTED_FIELD`.

## 3. The rule

- **Quantile.** Each vehicle keeps a quantile `q`, set once at generation:
  `q = (desired − typeMin) / (typeMax − typeMin)`, or 0.5 when the span is 0. This is the same mapping
  as D191's `rescaled`.
- **Passage.** When a vehicle's front crosses a decision on its current route within a tick (it was
  at or before `at` and is now beyond it: `from ≤ at < to`), and its type is listed, its
  `desiredSpeed` becomes `min + q · (max − min)`.
  - The half-open test applies a decision once per passage. A vehicle standing exactly on the
    station takes it on its first move.
  - Every lane chain of the Link carries the decision, so a lane change neither repeats nor skips
    it.
  - A later decision replaces an earlier one.
- **Speed.** Nothing else changes:
  - a vehicle above its new desired speed brakes toward it at comfortable deceleration (D191's
    `aboveTarget` cap);
  - the curve bound still takes the minimum.
- **Insertion.** A vehicle inserted at its route's start has passed nothing. A decision at station 0
  of the first Link is crossed on the first tick that moves the front past it.
- **Parking.** A vehicle leaving a lot (M8.4) starts a new trip with the desired speed it parked
  with, and its quantile is taken again at its next decision. The quantile is not carried through a
  lot.

## 4. Delay (owner)

The free-flow time is piecewise: each stretch's distance over the desired speed in force on it.
- A vehicle carries `freeFlowSoFar` (seconds) and `freeFlowFrom` (route distance).
- At each decision it adds `(at − freeFlowFrom) / oldSpeed` and moves `freeFlowFrom` to `at`. A lane
  change shifts `freeFlowFrom` by the same amount as its distance.
- At arrival: `freeFlowTime = freeFlowSoFar + (routeLength − freeFlowFrom) / desiredSpeed`. With no
  decision passed, this is `routeLength / desiredSpeed` bit for bit.
- A travel-time section's free flow uses the speed in force too: the section's track keeps the
  piecewise sum.

## 5. What does not change

- With no decision in a project, every run, output and checkpoint is byte-identical. The new vehicle
  fields are written to a checkpoint only when they differ from their defaults.
- Following, perception and W74 read `desiredSpeed` as before. A perceived decision already in the
  reaction history ages out.
- The Lab has no decisions.

## 6. Lifecycle and editor

- **Commands:** put, delete and move-station, following travel-time sections:
  - deleting the Link deletes its decisions;
  - splitting the Link moves a decision to the piece that holds its station;
  - reversing the Link is refused while a decision names it.
- **Editor:**
  - a *Speed decision* tool places one by clicking a Link;
  - the canvas marks each decision across the Link;
  - a table lists them;
  - an inspector edits name, station and one min/max row per vehicle type.
  - Each edit is one Undo.

## 7. Reduced-speed areas (contract only; the next slice)

A reduced-speed area is a stretch of a Link or Connector with per-type speed ranges. It is
temporary: a vehicle returns to its previous desired speed when its front leaves the area. As in
Vissim, it slows *before* the area at a deceleration.

Questions for the owner before code:
- the anticipation deceleration: a behaviour key, a per-area value, or the type's comfortable one;
- whether delay inside an area counts (it is a "speed in force", so by §4 it would not);
- whether the area applies by lane.

Rows BA97 onward are reserved for it.

## 8. Rows (BA86–BA96)

| Row | Case | Asserts |
|---|---|---|
| BA86 | No decision | CLI pin, the shipped projects and a checkpoint byte-identical |
| BA87 | Two cars with quantiles 0.2 and 0.8 pass `{car: 8–12}` | New speeds 8.8 and 11.2 (±1e-9); forcing: their quantiles first |
| BA88 | A type not listed | Its desired speed unchanged |
| BA89 | A lower and a higher range | Brakes at comfortable deceleration with no clamp; accelerates |
| BA90 | A lane change before, and one after, the decision | Applied exactly once |
| BA91 | Two decisions in a row | The second replaces the first; the quantile is kept |
| BA92 | A lone free car through a slower decision | Trip delay < 0.5 s |
| BA93 | A travel-time section over the same road | Section delay < 0.5 s |
| BA94 | A checkpoint after the decision | Replays byte-identically |
| BA95 | Codec and validation | Round-trip at 38; 37 → `EDIT_UNSUPPORTED_FIELD`; each fault of §2 at its path; 39 → `EDIT_VERSION` |
| BA96 | Commands | Link delete cascades; split moves; reverse refused |

## 9. Failure conditions (reopen D222)

- A study needs a decision per lane, or on a Connector.
- A study needs drivers to redraw at a sign rather than keep their quantile.
