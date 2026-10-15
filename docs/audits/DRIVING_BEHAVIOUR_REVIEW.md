# Driving-behaviour completeness review — 2026-10-15

A review, not an owner approval and not a contract. It asks how complete TrafficSim's driving
behaviour is for the studies it exists for, area by area (perception, mirror checks, lateral
behaviour, overtaking, motorcycles and the rest), and proposes the order in which to close
the gaps. Milestones M3.5–M3.9 were carved from it (D177). Every code claim cites the tree
at `65c627a` + D176; re-check before relying on a line number. **Nothing here is measured
fidelity** (rule 4); every proposal still needs its contract, failure-first rows and M6 evidence.

## 1. Scope and method

- **Read:** `src/core/` in full (simulation, following, w74, lanes, conflicts, demand, routes,
  signals, parking), `src/editor/vehicle_pose*` and `lane_change_pose*`, the contracts
  [DRIVING_BEHAVIOUR](../reference/DRIVING_BEHAVIOUR.md), [W74](../reference/W74.md),
  [M3_8_CONTRACT](../reference/M3_8_CONTRACT.md), [AMBER](../reference/AMBER.md),
  [VEHICLE_POSE](../reference/VEHICLE_POSE.md), [BEHAVIOUR_LAB](../reference/BEHAVIOUR_LAB.md),
  the [delivery plan](../plans/DRIVING_BEHAVIOUR.md), ROADMAP M2–M6 and the decision record.
- **Judged against** [PROBLEM](../PROBLEM.md) §1–§2: impact studies, signal retiming,
  intersection redesign and access studies, reporting delay, queues and LOS per movement over
  several seeds. Since D177 that includes **mixed traffic with motorcycles** (Thai counts are
  motorcycle-heavy), and left-hand traffic is first-class (D7).
- **Effect** columns below are engineering judgement about which outputs a gap biases. They are
  hypotheses to measure, not results.

## 2. The current model in one page

A vehicle is **one-dimensional**: a route index and a distance along it (`Vehicle`,
`src/core/types.hpp`). Each tick (`stepSimulation`, `src/core/simulation.cpp:88`):

1. **Snapshot.** Every vehicle reads the same start-of-tick state (`simulation.cpp:207`, "Everyone sees the SAME pre-step state"). There is
   no reaction delay; `followingTime` is a relaxation constant, not a delay.
2. **Decisions.** Route decisions, then lane changes (`decideLaneChanges`,
   `src/core/lanes.cpp:214`), applied **instantaneously** by remapping route and distance
   (`simulation.cpp:212-228`).
3. **Following.** One leader: the nearest vehicle on the own route at any distance
   (`closestVehicle`, `simulation.cpp:35`). Signals, dead ends, gates, priority rules and zones
   become standing virtual leaders; the nearest wins (`:266-345`). A courtesy or cooperative
   hold is a second obstacle; the lower acceleration is kept (`:347-356`). `follow`
   (`src/core/following.cpp:50`) dispatches to the prototype (`followingAcceleration`, `:9`) or
   W74 (`w74Acceleration`, `src/core/w74.cpp:115`).
4. **Caps.** Desired-speed cap; hard cap `allowedDistance = gap − standstillGap`
   (`simulation.cpp:264`) with a counted `SafetyClampEvent`; phase-2 zone caps.
5. **Publish** in vehicle-id order; W74 state is stored only here.

What a driver **observes**: one leader ahead (unbounded distance), the nearest vehicle ahead and
behind on a target lane plus anything alongside (`around`, `lanes.cpp:40`), the colour of the
next head, priority-rule conflict segments and zone occupancy. What it **never** observes: a
second vehicle ahead, a second vehicle behind, its own lateral position, anything beyond a
modelled sight distance (none exists), or its own past.

Behaviour keys today: prototype `standstillDistance`, `additiveSafetyDistance`,
`multiplicativeSafetyDistance`, `followingTime`, `speedThreshold`; optional
`maxDecelerationCooperativeBraking`, `amberDeceleration`, `discretionaryLaneChangeThreshold`,
`acceptedDecelerationTrailingVehicle`, `discretionaryLaneChangeHoldTime`; W74's 18 keys. Type
keys: length, width (display only), axles (display only), desired-speed range, three
acceleration limits, `behaviourId`. Heterogeneity: `driverFactor` N(0.5, 0.15) clipped, which
scales only the multiplicative safety distance, plus W74's hashed traits.

## 3. Gap matrix

Status: **✓** implemented · **◐** partial or off · **✗** absent. "Effect" names the outputs a gap
is expected to bias. "Milestone" is where D177 puts it.

### A. Perception

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Look-ahead distance (sight limit) | ✗ — leader found at any distance | Drivers react to queues they could not see: early, smooth braking, too few hard stops; optimistic shock-wave damping | M3.5 |
| Observed vehicles ahead (≥2; brake-light chain) | ✗ — nearest only | Platoons react one vehicle at a time; string instability differs; discharge start wave too slow or too fast | M3.5 |
| Reaction / perception delay | ✗ — common snapshot | Start-up lost time and queue discharge wave (≈1–2 s per vehicle in the field) come only from the following model's shape | M3.5 |
| Speed-difference estimation error | ◐ — W74's `CX`/`SDV` threshold only | Prototype drivers perceive exactly | M3.5 |
| Visibility at conflict areas | ✗ — booked in M3.4 | Minor-road drivers see the whole major road | M3.4 → M3.5 |

### B. Mirror / look-back (rear checks before a lane change)

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Rear gap test | ✓ — `safeAt` (`lanes.cpp:69`): the target-lane follower's `follow` toward the changer must not exceed `acceptedDecelerationTrailingVehicle` (discretionary) or comfortable deceleration | — | — |
| Look-back distance | ✗ — the nearest follower is found at any distance | Unrealistic refusals because of far vehicles; no "did not see it" errors | M3.5 |
| Second vehicle behind (a fast vehicle closing behind a slow one) | ✗ | Accepts changes in front of a fast second follower; overtaking lanes look safer than they are | M3.5 |
| Relative speed / time-to-collision rear test | ◐ — implicit in `follow`, no TTC threshold | No surrogate-safety measure to report | M3.5 (surrogate output in M5) |
| Blind-spot zone | ✗ — "alongside" is an exact overlap test | — (only matters with errors, C) | M3.5 |
| Minimum lane-change headway; safety-distance reduction during a change | ✗ | Forced merges at lane drops either fail too often or need geometry tricks | M3.4 |

### C. Attention and error

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Temporary lack of attention (seeded probability and duration) | ✗ | No random late reactions; headway distributions too narrow | M3.5 |
| Driver error (missed gap, late brake) | ✗ | Clamp-free runs look safer than reality; no surrogate-safety spread | M3.5 (after C1) |

### D. Longitudinal

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Wiedemann-inspired prototype | ✓ — not validated | — | — |
| W74 | ✓ — not validated; cited preset D176 | — | — |
| W99 (CC0–CC9) | ✗ — contract not started | Motorway/expressway studies lack the model practitioners calibrate | M3.3.3b |
| Smooth close-up to a standing obstacle; separate standstill gap at static obstacles | ✗ | Queue lengths in metres at stop lines differ from field | M3.5 or M3.3.3b |
| Speed-dependent acceleration curves (desired/max per speed) | ✗ — constant `maxAcceleration` | Discharge acceleration too high at speed, too low at start | M3.6 |
| Grades; heavy-vehicle power/weight | ✗ | Heavy-vehicle effect on uphill saturation flow missing | M3.6 (grades) — later |

### E. Speed control

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Desired-speed distribution per type | ✓ — uniform `[min,max]` (`src/core/demand.cpp:21`) | — | — |
| Reduced-speed areas | ✗ (M2.1 lists desired speed vs Link limits) | **Turning vehicles cross the junction at full desired speed.** Turning saturation flow and conflict-zone occupancy times are wrong in a predictable direction | **M3.6** |
| Desired-speed decisions (speed-limit changes) | ✗ | Corridor travel times need geometry workarounds | M3.6 |
| Turning speed from Connector curvature | ✗ | Same as reduced-speed areas, without authoring effort | M3.6 |
| Compliance distribution (share above the limit) | ◐ — only through the range | — | M3.6 |

### F. Lane change, lane-based

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Mandatory (route) changes, gap acceptance, no change in conflict areas | ✓ (`decideLaneChanges`, `inConflictArea` `lanes.cpp:28`) | — | — |
| Cooperation hold and cooperative braking for a stub | ✓ — D71, D90 | — | — |
| Discretionary changes | ◐ — D95/D101 implemented, **off**; A53 failed (B2) | No lane balancing; lane utilisation at approaches comes only from input shares | M3.4 |
| **Lane-change duration with between-lanes occupancy** | ✗ — instantaneous; the 3·max(v,5) m slide is display only (`src/editor/lane_change_pose.cpp`) | A changer blocks nothing on its old lane and appears in the new one at once: weaving capacity too high, no braking wave behind changers, visual and physical states disagree | **M3.4** (needs M3.7.0) |
| `laneChangeDistance`, emergency stop distance | ✗ — window = compiled span (D87) | Late, clustered changes before diverges | M3.4 |
| Politeness / MOBIL-style trade-off | ✗ — own gain only | Selfish changes; too many disruptive changes once discretionary is on | M3.4 |
| Cooperative lane change (moving over for a merger) | ✗ | Merges on motorways rely on braking only | M3.4 |
| Zipper / advanced merging, blocking timeout | ✗ | Lane-drop queues and deadlocks need manual fixes | M3.4 |

### G. Overtaking

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Pass a slower vehicle | ◐ — only as D95's acceleration gain, off | Platoons behind slow heavy vehicles on multi-lane roads | M3.4 |
| Return after passing; keep-left / keep-right bias | ✗ — the Network's `drivingSide` (`src/model/network/geometry.cpp:89`) never reaches core | Lane use on multi-lane links is symmetric; D95 already names "a Thai multilane study that needs a keep-left rule" as its failure case | **M3.4** |
| Lane preference by type (heavy vehicles keep the slow lane), overtaking-lane bans | ✗ | Heavy vehicles spread across lanes | M3.4 |
| Overtaking in the opposing lane (two-lane rural roads) | ✗ | Out of the target studies today | Later |

### H. Lateral behaviour (two-dimensional position)

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Lateral position inside a lane | ✗ — `width` is display only | Every vehicle is centred; nothing below can exist | **M3.7** |
| Desired position at free flow (left/middle/right/any) | ✗ | — | M3.7 |
| Speed-dependent minimum lateral distance | ✗ | — | M3.7 |
| Overtaking inside a wide lane | ✗ | — | M3.7 |
| Position by turning direction at the stop line | ✗ | Shared-lane discharge (a left-turner beside a through vehicle) impossible | M3.7 |
| Swept-body clearance while turning | ✗ — booked in ENGINEERING_NOTES §Session work | Heavy vehicles' swept path does not block | M3.7 |

### I. Motorcycles (core scope since D177)

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Motorcycle type and appearance | ✗ — not offered (`docs/reference/SIMULATION.md`) | Thai counts cannot be entered honestly; converting to PCU hides the queue mechanics | **M3.8** |
| Lane sharing (two abreast; beside a car) | ✗ | Capacity per lane wrong for mixed flow | M3.8 (needs M3.7) |
| Filtering through a standing queue | ✗ | Motorcycles queue like cars: queue lengths overstated, start-up order wrong | M3.8 |
| Forming at the stop line (motorcycle box / "diamond" queue) | ✗ | Start-up discharge of the first seconds of green missing; the dominant effect at Thai signals | M3.8 |
| Smaller accepted gaps at priority rules | ✗ — one `gapTime`/`headway` per rule | Minor-road capacity for motorcycles understated | M3.8 / M3.9 |
| PCU hand-off to Macro (M10) | ✗ — B12 sources open | — | M3.8 → M10 |

### J. Intersection behaviour

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Priority rules and conflict zones, commitment, no blocking the box | ✓ (`src/core/conflicts.cpp`) | — | — |
| Stop signs | ✓ | — | — |
| Amber stop-or-go | ✓ — deterministic `amberDeceleration` (D147) | No red runners; amber dilemma zone has no spread | M3.9 (probability) |
| Stochastic critical gap / follow-up time per driver | ✗ — deterministic threshold | Minor-road capacity curve too sharp; calibrated values moved to M6 (R7) | M3.9 |
| Turn on red (left-turn-on-red is common in Thailand) | ✗ | Left-turn capacity understated at Thai signals | M3.9 |
| U-turn slots and median openings | ✗ (as a behaviour; geometry can be drawn) | Thai arterials route through U-turns; their gap acceptance and weaving are missing | M3.9 |
| Anticipating a turning leader's route (indicator) | ✗ | Followers do not prepare to change lanes behind a turner | M3.9 |

### K. Heterogeneity

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Per-driver factor | ◐ — one `driverFactor` scaling one term | Spread of headways and gaps narrower than field | M3.5 |
| Driver classes (cautious / normal / aggressive) as correlated trait sets | ✗ | Cannot represent a fleet mix; calibration has fewer handles | M3.5 |
| Behaviour by type and by road | ✓ — D127 | — | — |

### L. Side friction and other modes

| Capability | Status today | Effect if missing | Milestone |
|---|---|---|---|
| Bus / songthaew stops that block a lane | ✗ | Kerb-lane capacity overstated on Thai arterials | Later (with transit) |
| On-street parking manoeuvres | ✗ — lots are sinks/sources (M8.4a) | Same | Later |
| Pedestrians at crosswalks | ✗ — already "Later" | Turning capacity at signals overstated | Later |
| Incidents and temporary lane closures | ✗ | — | Later |

## 4. Architecture prerequisites

The gaps are not independent. Three foundations decide the shape of nearly everything else, and
building on the current one-dimensional engine first means rework:

1. **A perception contract (M3.5.0).** One `Observation` per vehicle and tick: a bounded,
   ordered list of obstacles ahead (count and distance limits from behaviour keys), the same
   behind on each adjacent lane, and a **delayed view** for reaction time (a fixed-size ring of
   past snapshots in `SimState`, indexed by tick, so replay stays exact). Every model —
   prototype, W74, W99 — consumes `Observation`, not `closestVehicle`. With the limits unset,
   the observation must equal today's (nearest leader, unbounded), which keeps BA18.
2. **A two-dimensional occupancy contract (M3.7.0).** `Vehicle` gains a lateral offset and
   uses the type's width; spans carry lateral extent; "alongside", leader search and conflict
   tests work on rectangles. A lane-based scenario with no lateral feature keeps every vehicle
   centred and must replay byte-identically. Lane-change duration, overtaking in a lane,
   motorcycles and swept-path clearance all sit on this. It is the largest change in the
   engine since D15 and needs a performance budget (D164's per-tick work) in the contract.
3. **The driving side in core.** `DrivingSide` exists in the model (`geometry.cpp:89`) but not
   in `Scenario`. Keep-left/right bias, the overtaking side, turn-on-red and motorcycle filtering
   side all need it, compiled once (hard rule 3).

Constraints every item keeps: determinism (new state in `SimState`, hashed per-vehicle draws like
D137 so the legacy RNG stream is untouched); content as data (every constant a behaviour or type
key, no code defaults, hard rule 5); `core/` imports nothing; prototype baselines change only by a
recorded decision (BA18).

## 5. Recommended order, with reasons

One system per session; each item is contract → failure-first rows → code, never combined.

1. **M3.5.0 perception contract and M3.7.0 2D-occupancy contract** (two docs-only sessions).
   Everything below depends on one or both, and writing W99 or overtaking against the 1D
   engine would be redone.
2. **M3.4 lane-change duration with between-lanes occupancy** on the M3.7 base. It is the most
   visible unrealism (vehicles teleport between lanes in physics while sliding on screen) and it
   changes weaving capacity.
3. **M3.6 turning and curve speeds.** Cheapest large effect on the outputs users report:
   turning-movement saturation flow and conflict-zone occupancy at every junction.
4. **M3.4 overtaking with the driving side** (keep-left/right bias, return after passing, heavy
   vehicles' lane preference), which reopens D95 through route (i) (B2: look past the nearest
   vehicle — provided by M3.5).
5. **M3.8 motorcycles**: type, lane sharing, filtering, forming at the stop line, smaller gaps.
   The single largest realism gap for Thai studies, and it needs items 1–2.
6. **M3.5 attention, reaction delay and driver classes.**
7. **M3.3.3b W99**, consuming the perception contract.
8. **M3.9 intersection extensions**: stochastic critical gaps (values from M6), turn-on-red,
   U-turns, amber probability, route anticipation.
9. **Side friction** (stops, parking manoeuvres, pedestrians) stays *Later*, with transit.

Throughout: M6 remains the gate for any fidelity statement; the not-yet-validated marker stays.

## 6. Acceptance-row sketches (to be written failure-first in each contract)

- **Perception:** with `lookAheadDistance` 100 m, a standing queue 150 m ahead is not a leader
  until within 100 m; with the key unset the run is byte-identical. Two observed vehicles: the
  follower brakes when the second vehicle ahead brakes hard although the first has not yet.
  Reaction delay 1.0 s at dt 0.1: the follower's first deceleration is exactly 10 ticks after
  the leader's; replay is exact.
- **Mirror:** a fast vehicle second behind on the target lane, inside `lookBackDistance`, blocks
  the change; outside it, the change is accepted and the fast vehicle must brake within its
  limits (no overlap, a counted clamp if not).
- **Lane-change duration:** during a 3 s change the changer occupies both lanes; a follower on
  either lane treats it as its leader; no other vehicle enters the space it vacates until the
  change ends; display and engine positions agree.
- **Overtaking:** on a two-lane left-hand-traffic link, a car behind a slow heavy vehicle passes
  on the right and returns left after a hold; heavy vehicles never choose the right lane without
  a route reason; mirrored for right-hand traffic.
- **Speed control:** a 90° Connector of 15 m radius is crossed at no more than its curve speed;
  a reduced-speed area slows only the types it names; unset, byte-identical.
- **Motorcycles:** at a red signal with standing cars, motorcycles filter to the stop line only
  where lateral clearance ≥ their minimum and form ahead of the first car; on green they
  discharge first; no overlap in 2D at any tick; replay exact.
- **Intersections:** turn-on-red allowed only on movements marked so, after a full stop and a
  gap; seeded critical gaps reproduce per seed and leave the legacy stream untouched.

## 7. Validation and data needs (for M6 and B12)

Each item needs a field quantity before any claim: headway and reaction distributions (M3.5);
lane-change rates and durations per link type (M3.4); turning speeds by radius (M3.6); lane
utilisation on multi-lane links in left-hand traffic (M3.4); motorcycle saturation flow, PCU,
filtering share and stop-line formation (M3.8); critical gaps and turn-on-red use (M3.9). Thai
sources must be named by the owner (B12, B15). Published Vissim calibrations do not transfer
(rule 4).

## 8. What this review does not decide

- The order in §5 (the owner may reorder; OWNER_SITTING B14).
- Keep-left as the default for left-hand-traffic networks, and its strength (B14).
- Whether turn-on-red and U-turn slots are wanted before M6 (B14).
- Motorcycle data sources (B15, with B12).
- Any parameter value, tolerance or validation result.
