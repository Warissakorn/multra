# Zones, OD matrices and parking lots (M8 contract, D153)

**Status:** M8.2 implemented 2026-10-13 (D154): the schema-31 codec, the structural checks and
the undoable commands for Zone, ZoneConnector and OdMatrix (rows Z1–Z6). M8.3 implemented
2026-10-13 (D155): the OD expansion of §3 and the Run tier of §6 without lots (rows OD1–OD6).
ParkingLot, its runtime, outputs and editor tools are **not implemented**. This document fixes the
authoring objects, the Micro expansion, the parking-lot runtime and the failure-first rows that
M8.2–M8.6 build against. Milestone status lives in [ROADMAP](../ROADMAP.md) M8; the design
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
| `dwellId` | string | A dwell distribution in `data/parking-dwell/` (content, hard rule 5), or the project's own catalog entry |

A dwell distribution is data: `{ "id", "kind": "empirical", "points": [[minutes, cumulativeShare], …] }`,
strictly increasing in both, last share 1. Sampling inverts the piecewise-linear cumulative
curve with one draw of the run's PRNG. Further kinds need their own row. The default values and
their source are an owner question (§8); no default dwell file ships until it is answered.

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
   its own chain. A pair with no chain is skipped; a cell with none at all is `OD_NO_PATH`.
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

## 4. Parking-lot runtime (core contract)

The lot is the one part the core must learn. Interface first; names are binding for M8.4.

```cpp
// src/core/types.hpp, additions (sketch; standard C++ only)
struct DwellPoint { double seconds{}, cumulative{}; };
struct ParkingLotSpec {
    std::string id;
    std::uint32_t capacity{}, initialOccupancy{};
    double gateServiceTime{};
    std::vector<DwellPoint> dwell;              // inverted cumulative curve
    std::vector<std::string> gateSegmentIds;    // last segment of each inbound route's lane
    std::vector<std::size_t> exitInputs;        // compiled inputs on the exit Links (no Poisson)
    std::vector<RoutingChoice> exitDestinations; // the lot row's shares (§4.4)
    std::string initialCompositionId;
};
struct ParkedVehicle { PendingVehicle vehicle; double admitted{}, dwellEnd{}; };
struct LotState {
    std::vector<ParkedVehicle> parked;          // sorted by (dwellEnd, vehicle id)
    std::uint32_t occupied{};                   // parked + departed-but-not-inserted (§4.3)
};
// ScenarioDefinition::parkingLots; SimState::lots, parallel to it. Empty ⇒ no lot code runs.
```

### 4.1 Arrival at the gate

A route whose last segment is a gate segment does **not** leave the network at its end. The gate
line is the segment end. The vehicle is held there as at a red head, and it comes to the line and
serves exactly as a Stop does (`stopService`, SIMULATION "Serving"): below `kStoppedSpeed` within
`stopLineReach`, then `gateServiceTime` standing (rounded up to whole ticks; 0 ⇒ the Stop's own
one-tick rest). After service it is **admitted only if `occupied < capacity`**; otherwise it stays
held, standing, and the vehicles behind it queue by ordinary following and spill back along the
driveway and onto the street. Nothing else removes it.

**A vehicle is never deleted.** It leaves the road only by admission (it becomes a `ParkedVehicle`)
or by reaching a non-gate route end, as today.

### 4.2 Admission order and dwell

Within one tick: (1) departures (§4.3) are released, (2) arrivals are inserted as today, (3) gate
admissions run in order of service completion tick, then vehicle id, while space remains. An
admitted vehicle's trip ends at that tick (`ArrivedEvent`), so its delay includes its gate wait;
the wait itself is also reported (§5). Its dwell is drawn at admission from the state's PRNG, in
admission order, so the stream is fixed. `dwellEnd = admitted + dwell`, rounded up to a tick.

### 4.3 Departure

When `dwellEnd ≤ t` the parked vehicle moves to the queue of one exit input, keeping its id and
type, with `scheduledTime = dwellEnd`; its destination is drawn by §4.4. It then enters through
the ordinary queue and D108 guard. **It keeps its space until it is inserted**: `occupied` falls
only when it is on the exit Link. A blocked exit therefore fills the lot and queues the gate,
which is the behaviour a study needs to see. Exit inputs are chosen by normalised connector share
with one PRNG draw in (dwellEnd, id) order.

### 4.4 A lot zone's own row

For a zone with a lot, the matrix row `o = lot` sets **destinations, not timing**: departures come
from dwell expiries, and each departing vehicle draws its destination zone from the row's cells as
relative weights (summed over every matrix whose period contains its `dwellEnd`; outside every
period, over all matrices). A lot with any arrivals or initial occupants whose row is empty is
refused on Run (`PARKING_LOT_NO_EXIT_DEMAND`). This is D153's choice; the owner may revise it.
The lot column (`d = lot`) is ordinary demand: §3 generates it, and its trips end at the gate.

### 4.5 Initial occupants

`initialOccupancy` vehicles exist at t = 0, ids `lot:<id>/<k>`, types drawn from
`initialCompositionId`, each with a full dwell drawn at t = 0. This overstates their stay (a car
parked before the run has already used part of it); the bias is stated, not corrected, and a
study controls it with warm-up.

### 4.6 Accounting

At every tick, for the whole run:

`generated + Σ initialOccupancy = completed + Σ parked + active + pending`

where `completed` counts trips ended at network exits and at gates, `parked` is vehicles in lots
(`occupied` minus those waiting in exit queues), `pending` includes exit queues. A vehicle that
parks and leaves is one vehicle with two trips; trips are counted per trip, vehicles per identity.

## 5. Outputs (M8.5 scope)

Per lot, reusing the M5 report surfaces and CSV formatters, with the marker:

- occupancy at each evaluation interval end, peak and mean over the evaluation period;
- gate queue: the D40 queue counter at the gate line (mean and max length);
- gate wait per admitted vehicle (service completion → admission), mean and max, and the number
  of arrivals that waited more than one tick;
- in/out flows per interval; vehicles in the lot, in its gate queue and in its exit queue at run end.

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

**Run tier** — authoring keeps the document (as for `UNSUPPORTED_ROUTE_TOPOLOGY`); Run refuses it
(M8.3, M8.4). These depend on topology, the run's duration or catalogs outside the document:

| Code | When |
|---|---|
| `ZONE_CONNECTOR_POSITION_UNSUPPORTED` | Origin on a Link a Connector arrives on, destination on a Link a Connector leaves (every connector, used or not) |
| `OD_PERIOD_INVALID` | `endTime` beyond the run's `duration` |
| `OD_COMPOSITION_UNKNOWN` | `compositionId` names no composition the run resolves |
| `OD_ZONE_NO_CONNECTOR` | A positive cell whose origin lacks an origin connector or destination lacks a destination connector |
| `OD_NO_PATH` | A positive cell with no complete path between any origin and destination Link pair |
| `PARKING_LOT_INVALID` | Capacity, occupancy, service time or composition rule of §2.4 broken; two lots in one zone |
| `PARKING_LOT_NO_GATE` / `PARKING_LOT_NO_EXIT` | The zone lacks a destination / origin connector |
| `PARKING_LOT_NO_EXIT_DEMAND` | §4.4 |
| `PARKING_DWELL_INVALID` | Unknown dwell id, or points not strictly increasing / not ending at 1 |

**Cascades (D154).** Deleting a Link deletes the zone connectors on it; the zones stay. Deleting a
Zone deletes its connectors, and is refused (`EDIT_ZONE_IN_USE`) while a matrix names it, so a
matrix's counts are never dropped as a side effect. `splitLink` keeps origin connectors on the
upstream piece, which keeps the Link id, and moves destination connectors to the downstream piece.
`reverseLink` keeps them; the Run tier then reports a position that no longer fits. Duplicating
objects does not copy zone connectors.

## 7. Acceptance rows

Failure-first: each test first asserts that its forcing worked (the lot is full, the exit is
blocked, the key is below 31), then the consequence. Test groups are planned names; each joins
`TRAFFICSIM_TEST_GROUPS` (D70) when written; `zonescodec` and `zonescommands` exist since M8.2.

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
| OD7 | A lot zone's row creates **no** Poisson input (asserted by input count), only exit destinations | `odexpand` |
| PL1 | **Full lot queues, never deletes.** A capacity-2 lot with dwell longer than the run and 5 arrivals: first assert `occupied = 2`; then 3 vehicles stand at or behind the gate to the end, and the §4.6 identity holds at every tick | `parkinglot` |
| PL2 | `occupied` never exceeds `capacity` over 40 seeds of a saturated lot | `parkinglot` |
| PL3 | The gate queue spills back past the driveway onto the street Link and blocks a through vehicle behind it (queue counter length > driveway length) | `parkinglot` |
| PL4 | Gate service: with `gateServiceTime` 10 s an empty lot admits no earlier than 10 s after the vehicle comes to the line; 0 s gives the Stop's one-tick rest | `parkinglot` |
| PL5 | A departure frees its space only when inserted: with the exit Link blocked (asserted), a full lot stays full and its gate queue grows | `parkinglot` |
| PL6 | Admission order is service completion then id; the dwell draws are reproducible (same seed ⇒ same admitted/dwellEnd list) | `parkinglot` |
| PL7 | `initialOccupancy` vehicles exist at t = 0 with the composition's types and leave by their dwell | `parkinglot` |
| PL8 | A departing vehicle keeps its id and type, and its destination frequencies over 40 seeds match the lot row's weights within the binomial 95 % band | `parkinglot` |
| PL9 | Without any lot, `trafficsim-cli 42`, the four frozen baselines and BA18 are unchanged | `parkinglot` |
| PL10 | Run refuses a lot whose row is empty (`PARKING_LOT_NO_EXIT_DEMAND`); authoring keeps it | `parkinglot` |
| PL11 | Reports (M8.5) give occupancy, gate queue, gate wait and flows that reconcile with the event stream, with the not-validated marker | `parkingoutputs` |

The M8 gate (ROADMAP) additionally needs the editor tools and the OD table (M8.6) and a
development-access study run end to end; no row above closes it alone.

## 8. Owner questions

Listed in [OWNER_SITTING](../plans/OWNER_SITTING.md) B10:

1. **Dwell default and source** — which distribution ships in `data/parking-dwell/`, cited.
2. **Lot outbound timing** (§4.4, D153) — dwell-driven departures with destinations from the row,
   or Poisson outbound volumes from the matrix with the lot only bounding occupancy.
3. **Lot trips in movement tables** — keep inbound lot trips in movement delay (as now, ending at
   admission, so gate wait is included), or report them only in the lot table.
4. On-street parking that blocks a lane stays design §8 question 6.
