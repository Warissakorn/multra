# Desired-speed distribution curve (B-5)

Written 2026-10-17 (D228). This is tier B-5 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md),
settings level 3 under D216: a property of a vehicle type, the same on every road. It extends the
type's desired-speed range, which ROADMAP M2.1 lists as "distributions". Rule 4 applies: nothing here
is validated, and no curve ships.

## 1. Scope and the owner's choices

Today each new vehicle draws its desired speed uniformly between its type's `min` and `max`
(`samplePending`). Measured spot speeds are rarely uniform: most drivers sit near a typical speed,
with fewer at either end.

The owner chose on 2026-10-17:
- **A piecewise-linear cumulative curve,** as in Vissim. Points of (speed, cumulative share) are
  entered from a field count. A uniform range is the two-point curve.
- **Decisions and areas stay linear.** A vehicle's place in its type's distribution, its cumulative
  share, is the quantile that [SPEED_DECISIONS](SPEED_DECISIONS.md) §3 and §7 map linearly into their
  own ranges. A driver faster than 80 % of their type stays at 0.8 of a decision's range.
- **On the vehicle type,** in the project catalog's type dialog. A per-input curve is not offered.

## 2. The key

`vehicleType.desiredSpeed.curve` is optional. It is an array of `{"speed": m/s, "share": 0..1}`
points:
- there are two or more points;
- the speeds increase strictly, and the shares never decrease;
- the first point is `(min, 0)` and the last is `(max, 1)`. `min` and `max` stay in the file and
  must equal the curve's ends, so every reader of the range sees the same bounds.

A flat stretch (two points with the same share) holds no drivers. A steep one holds many.

**Absent means today.** Without `curve`, the draw and the quantile use the old formulas exactly, so
every existing file runs byte-identically.

## 3. The rule

- **Draw.** The vehicle uses the same single draw `u` from the run's random stream, in the same
  place. Its speed is the curve's inverse at `u`: find the segment with `share_i < u ≤ share_{i+1}`
  and interpolate the speed linearly in that segment. Because the draw count does not change, every
  later draw (driver factor, arrivals) is unchanged.
- **Without a curve:** `min + u · (max − min)`, as now.
- **Two points `(min, 0), (max, 1)`:** the same value as without a curve, to the last bit.
- **Quantile.** Wherever a vehicle's place in its type's range is read, it is the curve's share at
  the vehicle's speed, interpolated in the segment that holds the speed; without a curve it is the
  old linear formula. The places are:
  - `quantileOf` for decisions and areas (SPEED_DECISIONS §3, §7);
  - D191's rescale when a live retune changes the range.
- **Retune.** A live retune that changes a type's range rescales each vehicle's speed by its share:
  the new range's curve is inverted at the old curve's share.
- **The Lab.** The Behaviour Lab edits `min` and `max` only. When the type carries a curve, the Lab
  moves every curve speed linearly onto the new range, keeping the shares.

## 4. File, validation and editor

**Schema 40.** A project that owns its vehicle types (schema 18 catalogs) and has a type carrying a
curve is written at schema 40. An older schema carrying `curve` is refused with
`EDIT_UNSUPPORTED_FIELD`. The shared catalog files in `data/vehicle-types` have no schema and accept
it, but no shipped type carries one.

**Validation.** `INVALID_SPEED_CURVE` is reported at `vehicleTypes[i].desiredSpeed.curve[k]` for:
- fewer than two points;
- a speed or share that is not finite;
- a share outside [0, 1];
- a speed not above the previous one, or a share below the previous one;
- a first point other than `(min, 0)`, or a last point other than `(max, 1)`.

**Editor.** The catalog's type dialog gains:
- a *Distribution curve* tick box;
- a two-column table: speed (m/s) and cumulative share (%);
- *Add point* and *Remove point*.

When the box is ticked, `min` and `max` are taken from the table's ends and their boxes are disabled.
When it is not ticked, no curve is written. The change is one Undo step with the rest of the
catalog.

## 5. What stays out

- A per-input or per-road curve.
- Named shared distributions referenced by id (Vissim's list).
- Other families (normal, triangular): a curve can approximate them.
- Curves inside decisions or areas. They keep linear ranges (owner).

## 6. Rows (BA110–BA116)

| Row | Case | Asserts |
|---|---|---|
| BA110 | No curve | Every draw equals `min + u · (max − min)`; shipped outputs byte-identical |
| BA111 | Curve `(min, 0), (max, 1)` | Each draw is bit-identical to the no-curve draw |
| BA112 | Curve `(10, 0), (12, 0.8), (16, 1)` | Over 20 000 draws, the share at or below 12 m/s is 0.80 ± 0.01; every speed lies in [10, 16]; the driver-factor sequence is unchanged |
| BA113 | That curve and a decision `{8–12}` | A vehicle at 12 m/s (share 0.8) gets 11.2 m/s at the decision |
| BA114 | Live retune of the curved type's range with rescale | Each vehicle keeps its share |
| BA115 | Codec and validation | Schema 40 round-trips the curve; 39 → `EDIT_UNSUPPORTED_FIELD`; each fault in §4 at its path; a file without a curve keeps its schema and bytes |
| BA116 | Editor | Ticking the box and entering points writes the curve and its ends as `min`/`max`; unticking removes it; one Undo step |
