# Vehicle pose — rear axle reference and continuous lane changes

This is display kinematics, not a change to traffic following, conflict admission or
reported measurements. It has not been calibrated against Vissim or real vehicles.

## Reference points and data

The engine's `Vehicle.distance` remains the **front bumper's station** along the complete
ordered route. The lane-centre Link geometry and derived Connector lane-centre path prescribe
that point's track. The rear axle is the rigid body's local origin in the Run view.
The front axle can therefore depart from the lane-centre curve while turning; this is
a deliberately bumper-guided model, not a claim that the front wheel follows that curve.

Vehicle-type JSON accepts one optional, complete `axles` object (metres):

```json
"axles": { "wheelbase": 2.7, "frontOverhang": 0.9, "rearOverhang": 0.9 }
```

Wheelbase is positive; both overhangs are nonnegative and all three are finite.
Their sum equals `length` within `1e-6 * max(1, length)` metres. A partial/null object,
unknown axle field or invalid dimension fails loading/compilation rather than being lost.
Inline types round-trip the optional object; external types stay in their catalog.
Schema 17 stays unchanged: the catalog/inline type extension is additive.

Old files without `axles` retain the omission on save and resolve nominal proportions
`0.6 * length`, `0.2 * length`, `0.2 * length`. These are compatibility defaults,
not measured dimensions. Shipped examples explicitly specify car 2.7/0.9/0.9 m and
rigid heavy vehicle 6/2/4 m; they are modelling assumptions, not manufacturer data.

## Turning equation

Let `F(s)` be the front bumper, `u = (cos(theta), sin(theta))` the body direction,
and `a = wheelbase + frontOverhang`. The rear axle is `R = F - a*u`.
Requiring its velocity to have no lateral component gives:

```
dtheta/ds = dot(dF/ds, (-sin(theta), cos(theta))) / a
```

`dF/ds` is the unit guide tangent. Front axle is `F - frontOverhang*u`;
rear bumper is `R - rearOverhang*u`. Both axle and body lengths remain rigid.
The initial heading is the entry tangent, with the rear extending behind route entry.
This uses the prescribed-track/no-slip construction described by
[Bor et al., Tire tracks and integrable curve evolution](https://arxiv.org/abs/1705.06314),
applied here to the bumper-to-rear-axle lever rather than the bicycle's wheelbase.

`RearAxlePath` integrates unwrapped heading with RK4 in spatial steps at most 0.1 m
(also at most `a/20`), splitting at route boundaries and every polyline vertex.
Query interpolates heading; front position still evaluates the original geometry/equation
directly. It is deterministic for a given path/type and independent of speed, timestep,
paint count, seeking or frame order. A path is limited to one million heading samples;
missing/disconnected geometry or a path exceeding that budget has no drawable axle pose.

The Canvas retains one derived path per route/type. A changed Run network clears all
paths; a different immutable Scenario snapshot clears them too. No document revision
or vehicle pose history keys this data. Zoom and colour do not enter the solution.
The phase-1 chord helper remains the route/front sampler, not the turning model.

## Continuous lane-change guide (phase 3)

The engine still remaps a vehicle to its target route at the start of one tick.
`Vehicle.laneChangeTrace` records source/target route indices, both stations before
motion, and speed before the remap. It is snapshot-owned, runtime-only display data;
physics and evaluation never read it. `lastLaneChange` remains the discretionary-hold
record. A later suffix selection records `RouteRemapTrace` (old/new routes, unchanged station,
number of preceding physical changes). The guide validates their shared physical prefix and uses
the selected suffix, preserving the blend and rolling heading across recognition and later changes.
No project schema, event JSON or traffic parameter changes.

`LaneChangePath` reconstructs longitudinal travel `x` by undoing those station remaps.
For each accepted change at `x0`, it blends the existing guide `G(x)` into the target
route point `Q(toDistance + x - x0)`:

```
f = clamp((x - x0) / L, 0, 1)
w = 10*f^3 - 15*f^4 + 6*f^5
F(x) = (1-w)*G(x) + w*Q(x)
L = 3 * max(speedBeforeChange, 5) metres
```

The three-second nominal duration and 5 m/s floor are display assumptions inherited
from D102, not calibrated driver parameters. Actual duration varies with travelled
distance: stopped vehicles hold position and heading. Endpoint blend derivatives
vanish; a second change composes with the ongoing guide, preserving its position and
initial tangent. An exhausted source route extends along its end tangent for display.
Route geometry can still have a sharp corner; blending does not certify steerability.

The composite guide is sampled at longitudinal steps at most 0.1 m, split at route
boundaries, polyline vertices and blend endpoints. Its polyline arc length drives
`RearAxlePath`, initialized with the source route's rolling heading at the first
change. Front queries interpolate this derived polyline; changing Connector paint
still does not change guidance. Heading keeps settling after the front reaches its
lane rather than snapping to an independently calculated target-route heading.
The scalar traffic station remains authoritative for occupancy and measurements.

The Canvas caches each changed vehicle's journey against its entire physical and routing traces,
current route and type,
immutable Scenario ownership and Run-network replacement (D28). Seeking with an older
trace rebuilds it; arrivals/reset discard it. Tick, current speed, colour and zoom do
not key this spatial solution. The trace lasts for the vehicle's lifetime because old
changes influence heading after their guides have finished; it is not serialized.
Polyline integration walks its tangents once instead of re-searching each leg.
Derived front guidance is limited to 500,000 samples and rolling headings to one
million. Invalid/inconsistent traces, missing/disconnected geometry and budget excess
have no drawable pose. These are numerical limits, not engine admission decisions.

Acceptance rows for this display slice:

| Row | Required behaviour | Regression |
|---|---|---|
| P3.1 | Rigid axle/body and rear rolling on the lane-change guide | Independent analytic-guide RK4, lateral-slip residual, two vehicle lengths |
| P3.2 | Source/remapped stations form one continuous journey | Different target station offset and overlapping changes |
| P3.3 | Stops, seeking and repaint leave pose unchanged at the same journey station | Headless query order and actual stopped Canvas frames |
| P3.4 | Heading continues across joins and blend completion | Curved/polyline join, spatial convergence and no completion snap |
| P3.5 | Cache reads every changing input and ignores zoom | Actual Canvas trace/type/network replacement, seeking, both sizes at low zoom |
| P3.6 | Display history cannot change traffic | Trace erasure vs exact engine states/events; baseline CLI reports/CSV |
| P3.7 | A positioned suffix selection during a blend retains position and heading, follows the selected turn, and supports a later change | Shared-prefix remap, stale cache rejection and malformed-prefix rejection (`lane-change-pose`) |

## Verification and limits

Geometry tests compare a right-angle guide with its analytic tractrix solution, check
rear lateral slip, rigid axle/body distances, circle offtracking for two sizes, join
continuity, sectioning, spatial-step convergence, invalid input and query-order replay.
UI tests check actual rear-axle item origins, front stations, curved joins and low zoom.
Codec tests cover inline/external types, defaults, malformed data and event-stream
equality under changed/omitted axle dimensions.

The two `evidence/vehicle-pose-original-*.json` files preserve the pre-extension
catalogs from main `5d823be`; their hashes match the frozen M3.2.7 sweep metadata.
Its comparison permits exactly the display-only `axles` addition and verifies that
every other type field equals the archived input. Tests reject length/acceleration
changes; frozen metadata and measured results are never regenerated by this slice.

This is a planar rigid vehicle with an ideal rolling rear axle. No steering limit, tire
forces, speed-dependent slip or articulated trailer joints are modelled. A sharp guide
corner can imply infeasible front steering; finite heading is not a feasibility check.
Conflict zones, safety gaps and queues still use scalar length, not the displayed
swept body. There is no between-lanes occupancy or swept-body clearance; changing those needs a separate
engine contract and acceptance rows.

## Plan-view appearance (D333)

`data/vehicle-appearance/*.json` maps `vehicleTypeId` to a required `color` (`#rrggbb`)
and an optional `shape`: `car`, `motorcycle`, `pickup`, `van`, `bus` or `truck`.
These are reusable drawing archetypes, not engine classes: another vehicle type can
select an existing archetype by adding data alone. Invalid shape values fail with
`EDIT_DISPLAY_CATALOG`. Without a user override, a missing folder retains the road's vehicle colour; an entry
without `shape` retains the length-based car/truck choice (truck above 7 m).
Appearance never enters `core/`, project schemas or vehicle-type parameter hashes.

`vehicleDrawing` produces a solid body and separate glass, roof/load panels and pale
headlamps in the front-bumper frame. Cars have a tapered nose and cabin; pickups an open
bed; vans a short cab-over nose, continuous high roof, divided side windows and upright
tailgate glass; buses a continuous body with side windows and roof equipment;
rigid trucks a rounded cab, exposed chassis neck and fixed load box with roof corrugations
and a rear-door header. At the owner's request, motorcycles use the original D333
pointed silhouette, windshield, tank highlight, saddle and small nose light.
Truck cabs are capped at a nominal 2.04 m, matching the smaller 8.5 m rigid truck's
existing head. On longer bodies, the load-box front moves forward and its roof detail
stretches along the remaining length. The overall bounds, front bumper, rear bumper
and physical axle dimensions remain unchanged; overview symbols retain their size floor.
No trailer articulation or brake/signal state is implied. Glass and cargo are painted
inside the body rather than cut out, so clicking them selects the same vehicle.

Each vehicle remains one reused `RunVehicleItem`, with its contrasting cosmetic casing
and followed outline. Body bounds remain exactly the type's length and width at
working zoom; the nose remains at the traffic front station and the rear axle remains
the item origin. Overview symbols retain the 4 by 2.5 logical-pixel floor. Interior
detail appears only at both 14 pixels in physical length and 5 pixels in physical
width; the solid silhouette and colour remain below either threshold. Appearance
replacement redraws a stopped frame, and a new immutable Scenario discards retained
vehicle items so reused IDs cannot carry old dimensions or decoration.

The `vehicle-appearance-ui` suite checks all archetypes' bounds, solid glass hit areas,
equal-size bus/truck distinction, actual viewport clicks for all seven bundled types,
painted detail, overview floors, front/rear references, stopped item reuse, followed
outlines, appearance replacement and changed Scenario dimensions. It accepts an optional
second argument to save a gallery using the production painter. Platform verification
is recorded in [PROGRESS](../PROGRESS.md); the drawing is illustrative, not a measured
manufacturer model or calibrated traffic behaviour.

## Vehicle colour preferences (D334)

**View → Vehicle colours…** edits appearance per vehicle-type ID, including project-owned
custom types. One colour fixes the type's body colour; two to sixteen distinct colours
mix within that type. The colour picker edits any selected swatch, Add/Remove changes the
palette, and the standard mixed palette comes from optional `data/vehicle-colors.json`
(`colors`, one to sixteen unique `#rrggbb` entries). Reset removes the selected type's
override and restores its installed/road colour. Single-colour mode retains the selected
swatch. A directory without the standard palette can still add colours manually.

Preferences are application display settings for this computer, shared by the editor and
Behaviour Lab, rather than authored traffic data. They live in `vehicle-colors.json` in
Qt's application-local data directory, with version 1 and a `palettes` object keyed by type ID.
Confirm atomically replaces that file, then redraws the current frame and any open lab.
Cancel changes neither the file nor the live appearance; failed saves leave the dialog open
and preserve the old appearance. Missing or malformed preference files retain installed defaults.
Opening another project preserves these display preferences; saving a project does not embed them.

`vehicleDisplayColor` hashes the stable vehicle ID and type ID to an equal-choice palette
index. It uses no simulation PRNG or wall clock. A vehicle retains its colour through
pause, repaint, zoom and backward/forward seeking; changing the palette deliberately
recolours the frame. One colour per vehicle is still a body fill plus the existing neutral
detail/casing. Preferences do not change Scenario snapshots, seeds, schema versions or results.

`vehicle-colors-ui` covers fixed/mixed choices, palette editing and persistence, Cancel,
malformed settings and failed-save atomicity, and the View action. `vehicle-appearance-ui`
checks actual same-type mixed vehicles, fixed recolouring, and colour stability through
zoom and seeking. `ranges` validates the optional standard palette.
