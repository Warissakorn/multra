# Zones, OD matrices and parking lots (M8 contract, D153)

Implemented by M8.2–M8.6b (D154–D157, D160–D162); slice history is in PROGRESS and milestone
status in [ROADMAP](../ROADMAP.md) M8. [D175](../decisions/RECORD.md#d175) will move the matrix
period into StudyPeriod ([workflow](../plans/MULTI_LEVEL_WORKFLOW.md)); until that slice lands,
§2.3 is the contract; [STUDY_PERIODS](STUDY_PERIODS.md) (D266) specifies the period and mode forms; a matrix may name a study period since D268, and a mode in place of its composition since D270. This document fixes the
authoring objects, the Micro expansion, the parking-lot runtime and the failure-first rows that
M8.2–M8.6 build against. The design
behind it is [MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §3.2–3.3 (D145). The
reasoning for the choices below is [D153](../decisions/RECORD.md#d153); the check tiers and
cascades of §6 are [D154](../decisions/RECORD.md#d154).

Every result these objects produce is **not validated** (hard rule 4) and carries the marker. A
lot's occupancy and gate queue are model outputs of an uncalibrated engine, not a parking study.

## 1. Scope

In M8, zones and OD matrices are a **demand layer for Micro only**. Macro (M10) and Meso (M11)
will read the same objects; nothing here may be copied into a side table for them (hard rule 3).

| In M8 | Out of M8 (own slice and contract later) |
|---|---|
| Zones, zone connectors at **Link ends**, OD matrices per period and composition | Zone connectors part-way along a Link (internal insertion, see §2.2) |
| OD → ordinary inputs and routes, **equal weight over complete paths** | Path choice by cost or assignment (M9) |
| Parking lot: capacity, initial occupancy, gate service, dwell, gate queue | Bays, parking search, manoeuvres in the lane, on-street parking ([OWNER_SITTING](../plans/OWNER_SITTING.md) B7 / design §8.6) |
| Occupancy, gate queue/wait and lot flows (M8.5) | Exact-count demand (Poisson only, as today); trip chaining beyond one dwell |

## 2. Authoring objects (schema 31)

All four are new top-level arrays of the editor document (`*.traffic.json`), each written **only
when non-empty**. A document without them keeps its feature schema and its bytes; a key below
schema 31 is refused on load, and an unknown field inside one, or an empty array, fails on load (D19a). IDs share
the network's global uniqueness rule.

### 2.1 Zone

| Field | Type | Rule |
|---|---|---|
| `id` | string | Non-empty, unique |
| `name` | string | Optional, display only |
| `polygon` | `[[x, y], …]` | ≥ 3 points, finite, non-self-intersecting; planar metres like Links. Presentation and Macro only: Micro never reads it |
| `attributes` | object of string → number | Optional, finite numbers; read by M10 trip generation only. Unknown names are kept, not interpreted |

### 2.2 ZoneConnector

| Field | Type | Rule |
|---|---|---|
| `id` | string | Non-empty, unique |
| `zoneId` | string | An existing Zone |
| `linkId` | string | An existing Link |
| `direction` | `"origin"` \| `"destination"` | Origin: trips leave the zone here. Destination: trips end here |
| `share` | number | Finite, > 0; default 1. Relative weight among the zone's connectors **of the same direction** |

**Position (M8): Link ends only.** An origin connector names an **entry Link** (no Connector
arrives on it); its trips are inputs at station 0, through today's insertion and the D108 guard.
A destination connector names a **terminal Link** (no Connector leaves it); its trips end at the
Link end, exactly as a route that leaves the network does today. Any other Link is refused with
`ZONE_CONNECTOR_POSITION_UNSUPPORTED`, naming the connector: inputs start only on entry Links
(`UNSUPPORTED_INTERNAL_INPUT`), and a stop part-way along a Link needs an internal-insertion core
contract. A site driveway is drawn as its own entry and terminal Links.

**Road-only Macro demand (schema 57, D340):** `roadId` plus `roadDirection` (`forward` or
`backward`) can replace `linkId` on a Road without generated Links. Origin/destination and
relative shares keep their meaning; travel direction is along/against the centreline. Macro
reads the selected directed Road piece. *Apply template* remaps the connector to its generated
Link in the same Undo step. Micro and Meso refuse Road targets until that conversion. The codec,
editor, lifecycle and rows RD30–RD34 are in [ROADS_IMPLEMENTED §22](ROADS_IMPLEMENTED.md#22-road-only-macro-demand-d340).

### 2.3 OdMatrix

| Field | Type | Rule |
|---|---|---|
| `id` | string | Non-empty, unique |
| `startTime`, `endTime` | seconds | `0 ≤ start < end ≤ duration` (cool-down excluded, as for inputs) |
| `compositionId` | string | An existing composition (`data/compositions/` or the project's own); every trip of the matrix draws its type from it |
| `zoneIds` | string[] | Existing Zones, no repeats; the row and column order |
| `cells` | number[][] | `zoneIds.length` rows of `zoneIds.length` entries, finite, ≥ 0; row = origin, column = destination; **vehicle trips in the period** (not veh/h). The diagonal is allowed and needs a path like any cell |

Several matrices may cover the same period with different compositions, or different periods.
Overlapping periods add, as overlapping inputs do today.

### 2.4 ParkingLot

| Field | Type | Rule |
|---|---|---|
| `id` | string | Non-empty, unique |
| `zoneId` | string | An existing Zone; **at most one lot per zone**. The lot's gates are the zone's destination connectors, its exits the zone's origin connectors |
| `capacity` | integer | ≥ 1 |
| `initialOccupancy` | integer | `0 ≤ initialOccupancy ≤ capacity`; default 0 |
| `initialCompositionId` | string | Required when `initialOccupancy > 0`, refused otherwise; the types of the vehicles parked at t = 0 |
| `gateServiceTime` | seconds | Finite, ≥ 0; default 0. Time standing at the gate before admission is possible (ticket, barrier) |
| `dwell` | `[[minutes, cumulative], …]` | The lot's own dwell curve: minutes ≥ 0 and cumulative shares in [0, 1], both strictly increasing, the last share 1. A single point is a fixed dwell |

The dwell curve is stored **on the lot** (D157) and is **required: no default ships** (owner,
D158). Dwell depends on land use, so a study's own survey is its source. A catalog reference can be
added later beside the inline curve, if a cited distribution becomes available. Sampling inverts the piecewise-linear curve with one draw of the run's
PRNG (§4.1). Every field is always written; `capacity` and `initialOccupancy` must be JSON
unsigned integers.

## 3. OD expansion for Micro (compile time)

`withOdDemand` (`src/project/od_demand.*`, D155) adds every matrix to a copy of the definition as
**ordinary authored routes and inputs**, before compositions, the routeless walk and
`buildScenario` run; the core sees no zone, and the document is not changed. `compileDocument`
and the editor's run diagnostics both use it; a document without a zone layer takes the old path.
For each matrix and each cell `(o, d)` with `cells[o][d] > 0`, where zone `o` has no parking lot
(lot rows are §4.4):

1. **Rate.** `rate = cells[o][d] / (endTime − startTime) · 3600` veh/h, Poisson, over
   `[startTime, endTime)`.
2. **Connectors.** The rate is split over `o`'s origin connectors and `d`'s destination
   connectors by their normalised `share` (the product of both).
3. **Paths.** Between an origin connector's Link and a destination connector's Link the path set
   is the **fewest-object chains** that a lane can drive (`routeShortestChains` with the
   `routeLaneChains` filter): exactly what a placed routing decision already uses for a
   destination Link. Each gets an **equal** part. One Link that is both origin and destination is
   its own chain. A pair with no chain is skipped, and its share of the cell is
   left out rather than redistributed: Run lists the cell as the advisory `OD_SKIPPED_DEMAND`
   (`odDemandGaps` gives the omitted vehicles). A cell with no chain at all is `OD_NO_PATH`.
   Detours longer by even one object get nothing: weighting every complete path equally would
   send traffic round loops like the direct road. M9 replaces the equal weight; nothing else here
   changes then.
4. **Objects.** Each chain becomes one authored route `od:<origin Link>><destination Link>`
   (with `/path-k` when there are several), shared by every cell that uses it, and each part one
   input `od:<matrix>/<o>/<d>/<oc>><dc>[/path-k]` with the matrix's composition and equal lane
   weights. `buildScenario` then expands the route per entry lane with mandatory lane changes, as
   for any authored route. Ids contain `:`, which no allocated id does.

The expansion is deterministic: matrices in document order, then row, column, origin connector,
destination connector and chain order. The same document and seed give the same bytes; reordering
`zoneIds` (with the cells) changes nothing that runs.

Object shortest-path search uses BFS distances and a shortest-path DAG, with no 20-object
cutoff. It enumerates at most 4096 alternatives in stable object order; exceeding that bound
reports `OD_ROUTE_SEARCH_LIMIT` and blocks compilation, rather than reporting `OD_NO_PATH` or
silently skipping cell demand. Destination-based routing decisions use `ROUTE_SEARCH_LIMIT`.
The existing lane-chain post-filter and unsupported topology limits remain (`odexpand`).

## 4. Parking-lot runtime (core contract)

The lot is the one part the core had to learn. **Implemented by M8.4a (D156)** in
`src/core/parking.*` and tested on hand-built scenarios (`parkinglot`); M8.4b compiles an authored
ParkingLot into it.

```cpp
// src/core/parking_types.hpp (ids, because createSimulation re-sorts routes, inputs and types)
struct DwellPoint { double seconds{}, cumulative{}; };              // inverted cumulative curve
struct LotExit { std::string routeId, inputId; double weight{}; };  // a way out and its queue
struct LotType { std::string vehicleTypeId; double share{}; };
struct ParkingLotSpec {
    std::string id; std::uint32_t capacity{}, initialOccupancy{};
    double gateServiceTime{};
    std::vector<DwellPoint> dwell;
    std::vector<std::string> gateRouteIds;  // a vehicle on one of these ends its trip at the gate
    std::vector<LotExit> exits;
    std::vector<LotType> initialTypes;      // required when initialOccupancy > 0
};
// src/core/types.hpp: ScenarioDefinition::parkingLots; SimState::lots, parallel to it
struct ParkedVehicle { PendingVehicle vehicle; double admitted{}, dwellEnd{}; };
struct GateArrival { std::uint64_t vehicleId{}, since{}; };   // came to the line at tick `since`
struct LotState { std::vector<ParkedVehicle> parked;          // sorted by (dwellEnd, id)
                  std::vector<std::uint64_t> leaving;         // released, not yet on the road
                  std::vector<GateArrival> gate;              // sorted by (since, id)
                  std::size_t occupied() const; };            // parked + leaving
```

Without a lot every hook returns at once and no random number is drawn: lot-free runs are
byte-identical (the frozen baselines, `multra-cli 42` and every existing test pin it).
`validateScenario` reports `PARKING_LOT_INVALID` with the path of the broken field: an empty or
repeated id, capacity < 1, initial occupancy above capacity or without types, an unknown type or a
share ≤ 0, a negative service time, a dwell curve that is empty, not strictly increasing in both
columns or not ending at cumulative 1, an unknown gate route, a route gating two lots (no gate route at all is legal since M8.4b), no exit,
an exit whose route or input is unknown, whose route does not start where its input's route
starts, or whose weight is ≤ 0.

### 4.1 One tick

1. **Release** (after this tick's arrivals are generated): each parked vehicle with
   `dwellEnd ≤ t`, in (dwellEnd, id) order, takes an exit — one draw by weight, none when there is
   only one — and joins that exit input's queue with its own id, type, desired speed and driver
   factor, `scheduledTime = dwellEnd`. It moves to `leaving` and **keeps its space**.
2. **Insertion** as for any queue (D108 guard). An inserted vehicle leaves `leaving`, freeing its
   space. A blocked exit therefore fills the lot and queues the gate.
3. **Admission** (from the snapshot, before anything moves): a vehicle on a gate route that has
   come to the line — `atStopLine`, the Stop's own test (below walking pace, front within
   `stopLineReach` of the route end) — enters `gate` at that tick. In (since, id) order, while
   `occupied < capacity`, each one whose `gateServiceTime` (rounded up to whole ticks) has run since
   then is admitted: removed from the road, `ArrivedEvent` (its trip ends; `completed` counts it),
   dwell drawn (one draw, in admission order), `dwellEnd` rounded up to a tick.
4. **Hold** (phase 1): while the lot is full, or the lot has a gate service, a gate-route vehicle is
   held at the route end exactly like a dead end — a standing leader there. Vehicles behind queue
   by ordinary following and spill back onto the street.
5. **Driving in** (publish): with space and no service, a vehicle reaching the route end is
   admitted at once, without stopping. If this tick's earlier admissions took the last space, it
   is placed at the line at speed 0 and a `SafetyClampEvent` reports the stop (row PL2b).

**A vehicle is never deleted.** A gate route never ends in the ordinary way: the vehicle leaves
the road only by admission. Every id is always in exactly one place — the road, an input queue, a
lot, or gone through an ordinary route end (checked every tick by PL1, PL2, PL5).

Departure destinations are fixed weights on `exits`. The time-sliced lot row of §4.4 is compiled
into those weights by M8.4b (one set per run).

### 4.4 A lot zone's own row

For a zone with a lot, the matrix row `o = lot` sets **destinations, not timing**: departures come
from dwell expiries (D153, confirmed by the owner in D158). `withOdDemand` adds the row's routes but
**no input** for them (OD7). `compileParkingLots` (`src/project/parking_compile.*`, D157) then
builds the engine's lot after `compileScenario`:

- **Weights:** the row's cells **summed over every matrix** (one set of weights per run; a
  time-sliced row would need per-period exits, not booked), times the origin and destination
  connector shares, divided over the chains — `lotExitWeights`, the same split as §3 with trips in
  place of rates — and then equally over each `od:` route's runtime lanes.
- **Gate routes:** every runtime route whose last segment is a section of one of the lot zone's
  destination-connector Links — OD routes, their lane families and any other route ending there.
  A lot nobody drives to has none, which is legal.
- **Exit queues:** one 0-veh/h input `lot:<lot>/exit/<first segment>` per distinct first segment.
- **Initial types:** the base shares of `initialCompositionId` (its time intervals are not used).

A lot whose row is empty is refused on Run (`PARKING_LOT_NO_EXIT_DEMAND`): it could only fill.
The lot column (`d = lot`) is ordinary demand: §3 generates it, and its trips end at the gate.
A study whose only demand is a lot parked full at t = 0 runs: `EDIT_NO_INPUTS` does not apply.

### 4.5 Initial occupants

`initialOccupancy` vehicles exist at t = 0, numbered from 1 before any arrival, types drawn by
`initialTypes` share (M8.4b compiles them from `initialCompositionId`), each with its desired speed
and driver factor drawn as an arrival's and a full dwell drawn at t = 0. This overstates their stay (a car
parked before the run has already used part of it); the bias is stated, not corrected, and a
study controls it with warm-up.

### 4.6 Accounting

At every tick, for the whole run:

`generated = completed + Σ parked + active + pending − admittedAllTime`

where `generated = nextVehicleId − 1` already includes initial occupancy, `admittedAllTime`
counts gate admissions from the start of the run, and `completed` counts trips ended at network exits and at gates, `parked` is vehicles in lots
(`occupied` minus those waiting in exit queues), `pending` includes exit queues. A vehicle that
parks and leaves is one vehicle with two trips; trips are counted per trip, vehicles per identity.

## 5. Outputs (implemented M8.5, D160)

`LotAccumulator` (`src/eval/lots.*`) reads snapshots only; the engine emits nothing new for it.
`MovementAccumulator` owns one, so every M5 surface carries it. The not-validated marker applies.

- **Row per lot**, named by its zone (else the lot id), with these fields:
  - `capacity`;
  - occupancy at the evaluation end (`occupancyEnd`, the last snapshot at or before it), and its
    mean and peak over the window's snapshots;
  - `admitted` (in) and `departed` (out) in the window;
  - gate wait mean and max;
  - `waitedOverTick`;
  - at the report: `endParked`, `endAtGate`, `endInExitQueue`.
- **Admission:** the `ArrivedEvent` on one of the lot's gate routes. It is a completed trip, so
  movement delay keeps the gate wait (D158).
- **Gate wait:** admission time − (arrival at the line + `gateServiceTime`), floored at 0. It is
  the time spent waiting for a space after service. A vehicle that drove in without stopping
  waits 0.
- **Departure:** the vehicle's id drops out of the lot's parked and leaving lists together, so it
  was inserted. That can happen in the step that released it.
- **Window:** the M5 evaluation period `[warmup, end]` (D132). There are no sub-intervals: the M5
  surfaces have none, and a per-interval series waits for one.
- **Gate queue:** a derived D40 queue counter `<lot> gate`. It has one line at the end of every
  distinct last segment of the lot's gate routes, and appears with the approaches (mean and max
  length).
- **Accounting:** the report's `parked` counts the vehicles in a lot that have not been released.
  A released vehicle is already pending in its exit queue. Then
  `generated = active + pending + parked + completed − admittedAllTime`.
  The row's `admitted` is windowed, so it cannot audit whole-run identity when admissions are
  excluded by warm-up or the evaluation end.
- **Surfaces** (all written only with a lot, so other projects keep their bytes):
  - `movementJson` `lots` {validated, measure, rows} and `parked`;
  - a `movementCsv` lot block and a `parked` line;
  - `batchJson` `lots` with estimates, and a `batchCsv` lot block (n, means, CI);
  - `parked` per seed in the batch JSON and in the CLI's single-run summary;
  - `checkpointJson` `lots` (parked vehicles with `admitted`/`dwellEnd`, leaving ids, gate
    `{vehicleId, since}`).
- **Not booked:** a lot comparison in `--compare`.

## 6. Validation issues

**Structural tier** — refused on load and on every commit (`ValidationError`, code and path), so
History stays atomic. Implemented by M8.2 (`zoneLayerIssues`, `src/project/zone_codec.*`).

| Code | When | Path |
|---|---|---|
| `EDIT_ZONE_INVALID` | A malformed value, an unknown or missing field, a wrong type, or an empty array in the file | — |
| `ZONE_ID_INVALID` | Empty id, or one that repeats any network, definition or zone-layer id | `zones[i].id`, … |
| `ZONE_POLYGON_INVALID` | Fewer than 3 points, non-finite, or two non-adjacent edges touch | `zones[i].polygon` |
| `ZONE_ATTRIBUTE_INVALID` | Empty name or non-finite value | `zones[i].attributes.<name>` |
| `ZONE_CONNECTOR_UNKNOWN_REF` | Unknown zone or Link | `zoneConnectors[i].zoneId` / `.linkId` |
| `ZONE_CONNECTOR_SHARE_INVALID` | Share not finite or ≤ 0 | `zoneConnectors[i].share` |
| `OD_ZONE_UNKNOWN` / `OD_ZONE_REPEATED` | A `zoneIds` entry | `odMatrices[i].zoneIds[k]` |
| `OD_CELLS_SHAPE` / `OD_CELL_INVALID` | Not square in `zoneIds` (or no zones); negative or non-finite cell | `odMatrices[i].cells` / `.cells[r][c]` |
| `OD_PERIOD_INVALID` | `startTime < 0`, `endTime ≤ startTime` or non-finite | `odMatrices[i]` |
| `OD_COMPOSITION_MISSING` | Empty `compositionId` | `odMatrices[i].compositionId` |
| `PARKING_LOT_INVALID` | Unknown zone or a second lot in it, capacity < 1, initial occupancy above capacity, a composition missing with occupants or present without, a negative service time (M8.4b) | `parkingLots[i].<field>` |
| `PARKING_DWELL_INVALID` | Empty curve, not strictly increasing in both columns, negative, or not ending at 1 | `parkingLots[i].dwell` |

**Run tier** — authoring keeps the document (as for `UNSUPPORTED_ROUTE_TOPOLOGY`); Run refuses it
(M8.3, M8.4). These depend on topology, the run's duration or catalogs outside the document:

| Code | When |
|---|---|
| `ZONE_CONNECTOR_POSITION_UNSUPPORTED` | Origin on a Link a Connector arrives on, destination on a Link a Connector leaves (every connector, used or not) |
| `OD_PERIOD_INVALID` | `endTime` beyond the run's `duration` |
| `OD_COMPOSITION_UNKNOWN` | `compositionId` names no composition the run resolves |
| `OD_ZONE_NO_CONNECTOR` | A positive cell whose origin lacks an origin connector or destination lacks a destination connector |
| `OD_NO_PATH` | A positive cell with no complete path between any origin and destination Link pair |
| `OD_SKIPPED_DEMAND` (advisory) | A positive cell served by some connector pairs while others have no path; their share is omitted |
| `OD_COMPOSITION_UNKNOWN` | Also at `parkingLots[i].initialCompositionId` |
| `PARKING_LOT_NO_GATE` / `PARKING_LOT_NO_EXIT` | The zone lacks a destination / origin connector |
| `PARKING_LOT_NO_EXIT_DEMAND` | §4.4 |

**Cascades (D154).** Deleting a Link deletes the zone connectors on it; the zones stay. Deleting a
Zone deletes its connectors, and is refused (`EDIT_ZONE_IN_USE`) while a matrix names it, so a
matrix's counts are never dropped as a side effect; likewise while a parking lot names it. `splitLink` keeps origin connectors on the
upstream piece, which keeps the Link id, and moves destination connectors to the downstream piece.
`reverseLink` keeps them; the Run tier then reports a position that no longer fits. Duplicating
objects does not copy zone connectors.

## 7. Acceptance rows

Failure-first: each test first asserts that its forcing worked (the lot is full, the exit is
blocked, the key is below 31), then the consequence. Test groups are planned names; each joins
`MULTRA_TEST_GROUPS` (D70) when written; `zonescodec` and `zonescommands` exist since M8.2.

| Row | Check | Test |
|---|---|---|
| Z1 | Every shipped example project loads and saves to the **same JSON**, with its schema; none gains a key. The M0 scenario loads with an empty layer | `zonescodec` |
| Z2 | A document with zones, connectors and a matrix round-trips and saves as schema 31; clearing the layer gives back the earlier JSON (ParkingLot joins in M8.4) | `zonescodec` |
| Z3 | Each new key in a schema-30 file is refused on load (`UNSUPPORTED_FIELD`); an extra, null, missing or mistyped field, or an empty array, is `EDIT_ZONE_INVALID` | `zonescodec` |
| Z4 | Each structural §6 code is reported for its case with its path, and a refused commit leaves document, Undo and dirty state unchanged; a hand-edited file is refused alike | `zonescodec` |
| Z5 | Add/replace/delete of each object is one undoable command; deleting a Link (alone or in a selection) cascades its zone connectors and Undo restores them; a Zone a matrix names is refused; `splitLink` moves a destination connector downstream; new ids never repeat zone-layer ids | `zonescommands` |
| Z6 | A connector on a Link that is not an entry/terminal is kept by authoring (M8.2); Run refuses it with `ZONE_CONNECTOR_POSITION_UNSUPPORTED` (M8.3) | `zonescommands`, `odexpand` |
| OD1 | A 2 × 2 matrix compiles to inputs whose volumes equal cell / period · 3600 split by origin share, to 1e-9; the document is unchanged | `odexpand` |
| OD2 | Destination connector shares split the rate (3 : 1 gives 30 and 10 of 40) | `odexpand` |
| OD3 | Over 40 well-mixed seeds the departures of a cell lie within its Poisson 95 % band, and its origin split within the binomial band. Seeds 42–81 fail it, through the engine's first-draw bias (D155), not the expansion | `odexpand` |
| OD4 | A positive cell with no path reports `OD_NO_PATH`; a zero cell with no path compiles | `odexpand` |
| OD5 | Two equal-short branches get ½ each; a detour one Link longer gets no input | `odexpand` |
| OD6 | Same document and seed ⇒ the same event stream; reordering `zoneIds` with the cells gives the same inputs; no zone layer ⇒ the definition passes unchanged. Run-tier codes of §6 refused with their paths, and shown as runtime diagnostics | `odexpand` |
| OD7 | A lot zone's row creates **no** input (asserted by input count), only its routes; compiled, they are the lot's exits with one exit queue (M8.4b) | `parkingproject` |
| PL1 | **Full lot queues, never deletes.** A capacity-2 lot with dwell longer than the run: first assert `occupied = 2` and ≥ 5 arrivals; then every other arrival is on the road or in its queue, ≥ 3 on the road, none past the line, and every id is in exactly one place at every tick (M8.4a) | `parkinglot` |
| PL2 | `occupied` never exceeds `capacity`, and the identity holds, over 40 well-mixed seeds of a saturated lot (M8.4a). PL2b: two vehicles driving in side by side for one space — one is admitted, the other stays at the line with one reported clamp | `parkinglot` |
| PL3 | The gate queue spills back past a 30 m driveway onto the shared approach and a through vehicle stands behind it (M8.4a) | `parkinglot` |
| PL4 | Gate service: with `gateServiceTime` 10 s an empty lot admits no earlier than 10 s after the vehicle comes to the line; with 0 s it drives in without stopping (M8.4a) | `parkinglot` |
| PL5 | A departure frees its space only when inserted: with the exit blocked by a red head (asserted: a departure is still queued), every queued departure is in `leaving` and the lot stays full; unblocked, the lot keeps turning over (M8.4a) | `parkinglot` |
| PL6 | Same seed ⇒ the same events and lot states every tick; on one lane admissions follow arrival order (M8.4a) | `parkinglot` |
| PL7 | `initialOccupancy` vehicles exist at t = 0 with their `initialTypes`, ids 1…n, and leave by their dwell (M8.4a); compiled from the composition's shares (M8.4b) | `parkinglot`, `parkingproject` |
| PL8 | Departures' destinations over 40 well-mixed seeds (2,000 departures) match the lot row's 3 : 1 within the binomial 95 % band (M8.4b) | `parkingproject` |
| PL9 | Without any lot, `multra-cli 42`, the four frozen baselines and BA18 are unchanged; a lot-free document compiles no lot and no `lot:` input | `parkinglot`, `parkingproject` |
| PL10 | Run refuses a lot whose row is empty (`PARKING_LOT_NO_EXIT_DEMAND`), one with no gate or no exit connector, and an unknown initial composition; authoring keeps them. Codec and structural checks for `parkingLots` as Z2–Z5 (`parkingcodec`). The M8 gate run: a zone-OD study fills a capacity-3 lot, queues at the gate and keeps every vehicle (M8.4b) | `parkingproject` |
| PL11 | Reports (M8.5, implemented) give occupancy, gate queue, gate wait and flows that reconcile with the event stream, with the not-validated marker | `parkingoutputs` |

The M8 gate (ROADMAP) additionally needs the editor tools and the OD table (M8.6) and a
development-access study run end to end; no row above closes it alone.

## 8. Owner questions (answered 2026-10-14, D158)

Sitting B10, answered "as recommended":

1. **Dwell default:** none ships; every lot carries its own curve (§2.4).
2. **Lot outbound timing:** D153 stands: dwell-driven departures, destinations from the row (§4.4).
3. **Lot trips in movement tables:** inbound lot trips stay in movement delay, ending at admission
   so gate wait is included; the lot table (§5, M8.5) reports gate wait separately.
4. **On-street parking that blocks a lane:** after M8 (design §8 question 6, D158).

## 9. Editor (M8.6a implemented, D161; M8.6b implemented, D162)

- **Zone tool.**
  - `Z` selects it (index 12, palette group *Demand*); the Zones tab opens with it.
  - Ctrl+right-click adds the first corner, then a plain click (or Ctrl+right-click) each next one
    (D306); Backspace drops the last; Esc cancels.
  - Enter commits once there are three corners or more, as one Undo step (`putZone`). Fewer
    corners, or a refused polygon, keep the draft.
- **Canvas.**
  - A zone is a translucent polygon under every Link, named at its centroid.
  - A zone connector is a dashed line from the centroid to its Link's start (origin) or end
    (destination). Items are tagged `zone` / `zone-connector`.
- **Zones tab** (object tab 12).
  - The zone table lists id, name, connector count and lot.
  - *Edit zone* renames it.
  - *Delete zone* cascades the zone's connectors in one Undo step, and is refused with the
    localized `EDIT_ZONE_IN_USE` while a matrix or lot names it.
  - *Add origin/destination connector* attaches the selected zone to the one Link selected on the
    canvas.
  - The connector table beside it lists the selected zone's connectors (direction, Link, share),
    with *Edit* (share) and *Delete*.
- **Placement.** Authoring accepts a connector on any Link. *Recheck* lists a misplaced one in
  Problems as `ZONE_CONNECTOR_POSITION_UNSUPPORTED` (§6).
- **No lot tool.** `P` is already *cycle priority*, and a lot is one per zone, so M8.6b edits it
  from the zone's row.
- **Rows ZE1–ZE5** live in `tests/zone_ui_tests.cpp` (ctest `zone-ui`, offscreen).
- **OD matrices tab** (object tab 13, M8.6b).
  - Columns: id, period, composition, zone count, total trips. *Add* (with a zone), *Edit*
    (double-click/Enter) and *Delete*, each one step.
  - The dialog has start/end (s), a composition from the resolved catalog and a checkable zone
    list.
  - It also has a zones × zones grid (row = origin) whose values are kept by zone id across
    unchecking. It refuses an empty zone set and end ≤ start before closing.
- **Lot dialog** (*Parking lot…* on the Zones tab, M8.6b): "this zone is a parking lot",
  capacity, parked at start and their composition, gate service, and the dwell rows (minutes,
  cumulative share).
  - Unchecking deletes the lot.
  - The structural tier refuses a bad curve or occupancy with its own code.
- **Results → Parking lots** (M8.6b): one row per lot from the single run's report (§5), and a
  note giving the parked count. The run status appends ` · parked N` when the scenario has lots.
  The batch view keeps its CSV/JSON lot block; a batch lot table is not booked.
- **Rows OL1–OL5** live in `tests/od_lot_ui_tests.cpp` (ctest `od-lot-ui`).
