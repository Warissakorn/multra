# Lane preference by vehicle type (B-7)

Written 2026-10-17 (D233). This is tier B-7 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), a
level-3 key under D216: a vehicle type's property, the same on every road. It is the "later slice"
of [OVERTAKING](OVERTAKING.md) §1. Rule 4 applies: nothing here is validated, and no value ships
(D178).

## 1. Scope and the owner's choices

Today a vehicle's lane is fixed before it exists. Compile splits each input over the lanes of its
route's first Link, equally or by the input's `laneShares`. The split is the same for every type, and
a composition is expanded into one input per type before the split. Discretionary changes are off
in every shipped project, so a heavy vehicle stays on the lane it was given unless its route needs
another. Heavy vehicles therefore spread across all lanes.

The owner chose on 2026-10-17:
- **Entry lane by type.** The type enters only on the lanes it may use. Mandatory changes are
  unchanged, and no lane is closed to it.
- **On the vehicle type.**
- **The kerb-most N lanes.**

## 2. The key

- **`kerbLanes`** is an optional whole number on a vehicle type, in [1, 16]. A value outside that
  range, a fraction or a non-number is refused with `INVALID_KERB_LANES` at
  `vehicleTypes[i].kerbLanes`.
- **Schema 42.** It is written only when an owned vehicle type carries it. An older schema carrying
  it is refused with `EDIT_UNSUPPORTED_FIELD`. A shared catalog file has no schema; its reader takes
  the key.
- **Absent means today:** every split and every run is byte-identical. The catalog ships no value.

## 3. The rule

- **Lanes count from the kerb.** Lane 0 is the kerb lane on both driving sides (OVERTAKING §3), so
  "the kerb-most N lanes" are lanes 0 to N−1 of the Link.
- **The split.** For an input whose type carries `kerbLanes = N`, each lane chain on the first Link
  keeps its weight if its lane index is below N, and gets 0 otherwise:
  - the input's `laneShares`, or equal weights without them, are renormalised over the allowed
    lanes;
  - if the allowed lanes' authored weights sum to 0, they take equal weights;
  - a Link with N lanes or fewer is unchanged, and so is a split where no chain is allowed.
- **Where.** The rule applies wherever `laneSplit` sets a split:
  - routes expanded per lane at compile;
  - routeless (`linkId`) inputs, whose paths take the share of the lane they start on.

  The editor's input table shows the same split.
- **A zero share.** It behaves as an authored zero share does today. A route's per-lane input stays
  at 0 veh/h and generates nothing, so input ids and the random stream are unchanged. A routeless
  path with a zero share is dropped, as now.
- **After entry nothing changes.**
  - A vehicle that entered on the kerb lane but whose route needs another lane is a stub, and it
    changes lanes as any stub does (M3.2.8b).
  - Discretionary changes (D95) do not read the key.
  - No lane is ever closed to a type.

## 4. Editor

The vehicle-type dialog gains a *Kerb lanes* row: a check box with a spin box (1–16), off until
ticked. Editing it is one Undo step with the catalog, like the curve box.

## 5. What stays out

- Keeping a type out of the outer lanes after entry: a discretionary rule or a hard lane ban. Bans
  are overtaking-lane bans (OVERTAKING §1).
- A preference that varies by road (a behaviour key) or by input.
- Weights per lane.

## 6. Rows (BA124–BA130)

| Row | Case | Asserts |
|---|---|---|
| BA124 | No key | Splits and shipped outputs byte-identical |
| BA125 | `kerbLanes: 1` on 2- and 3-lane Links | Every vehicle of the type enters on lane 0; another type keeps the equal split |
| BA126 | `kerbLanes: 2` on 3 lanes | ½, ½, 0 |
| BA127 | Authored `laneShares` | Renormalised over the allowed lanes; all-zero allowed weights → equal |
| BA128 | A route that needs a far lane | Enters on the kerb lane and changes lanes as a stub; arrives |
| BA129 | A routeless `linkId` input | The same rule on its paths |
| BA130 | Codec and editor | Schema 42 round-trips; 41 refused; 0, 17, 1.5 and a string refused at the path; removal restores the schema; the dialog row writes it |
