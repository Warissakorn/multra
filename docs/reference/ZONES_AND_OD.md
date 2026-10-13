# Zones and OD matrices (M8.2–M8.3)

**Written 2026-10-10 (M8.1, [D150](../decisions/RECORD.md#d150)).**
- **Implemented:**
  - §1 and §4's commands and cascades (M8.2, 2026-10-13, rows ZO1, ZO2 and ZO4);
  - §2's Run checks and §3's expansion (M8.3, 2026-10-13, rows ZO5–ZO12; owner answers in
    [D151](../decisions/RECORD.md#d151)).
- **Still to come:** the editor (M8.6). It defines the authoring objects that state demand as trips between zones, how a
matrix expands into the inputs and routes the Micro engine already runs, and the rows that close
M8.2 (schema, codec, commands) and M8.3 (expansion). Parking lots, which are zones with a capacity,
are [PARKING_LOTS](PARKING_LOTS.md). The booked design is
[MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §3.2; D150 records where this contract
departs from it. Every result a matrix feeds is **simulated, not validated** (rule 4), and a
matrix states expected trips, not counts.

## 1. The objects (schema 29)

```json
"network": {
  "zones": [{"id": "zone-1", "name": "Site", "polygon": [{"x": 0, "y": 0}, {"x": 40, "y": 0}, {"x": 40, "y": 30}]}],
  "zoneConnectors": [
    {"id": "zc-1", "zoneId": "zone-1", "linkId": "link-7", "direction": "origin", "share": 1},
    {"id": "zc-2", "zoneId": "zone-1", "linkId": "link-8", "direction": "destination"}
  ]
},
"definition": {
  "odMatrices": [{"id": "od-am", "name": "AM peak", "startTime": 0, "endTime": 3600,
                  "compositionId": "urban-mixed", "zones": ["zone-1", "zone-2"],
                  "trips": [[0, 120], [80, 0]]}]
}
```

- **Zone** `{id, name, polygon}`. The polygon (at least three finite points, metres in network
  coordinates) is where the zone is drawn and picked; Micro uses no other zone property. Land-use
  attributes (population, jobs) belong to M10.2's contract and are not stored now: nothing would
  read them, and a shape chosen blind would be reshaped later.
- **ZoneConnector** `{id, zoneId, linkId, direction, share}`. `direction` is `"origin"` (trips
  leave the zone onto the Link) or `"destination"` (trips end at the Link). An **origin attaches at
  the Link's start, a destination at its end**; there is no position along the Link (§5).
  `share` is the connector's relative weight among its zone's connectors of the same direction,
  above 0, written only when not 1.
- **OdMatrix** `{id, name, startTime, endTime, vehicleTypeId | compositionId, zones, trips}`.
  `trips[o][d]` is the **expected number of trips** from `zones[o]` to `zones[d]` released in
  `[startTime, endTime)`, seconds of the run. The demand class is exactly one vehicle type or one
  composition, as for an input. A matrix is one period and one class; a day is several matrices.
- **Schema and bytes.** The three keys are written only when non-empty, so a file without them
  keeps its schema and bytes (the rule of [NETWORK_EDITOR](NETWORK_EDITOR.md)'s schema list). A
  file with any of them is schema 29: 29 heads the schema ternary in `documentJson`
  (`document.cpp`), and the reader's upper bound becomes 29.
- **Refused on load** (the open document is kept):
  - below schema 29: any of the three keys, by a raw-JSON `rejectZonesAndOdBefore29`, as
    `rejectCooldownBefore27` does (`EDIT_UNSUPPORTED_FIELD` with the key's path). It is called by
    `parseDocument` and by the CLI's M0 loader (`loadScenario`), so an M0 file carrying a key is
    refused there too, never silently run without its demand;
  - at 29: any other key in a zone or connector (`knownFields`, `parse.cpp`), and in a matrix by the
    matrix parser's own whitelist (`EDIT_UNSUPPORTED_FIELD definition.odMatrices[i].<key>`).

### Structural rules

Checked on every load and every edit (`validateDocument`); the edit or the load is refused and the
document is unchanged. Paths name the authored object (`zones[i]`, `zoneConnectors[i]`,
`odMatrices[i]`).

| Code | Refused when |
|---|---|
| `INVALID_ID` | an id is blank, or a zone, connector or matrix id contains `/` or `>` (they separate the parts of generated ids, §3) |
| `DUPLICATE_ID` | a zone or connector id is used by any other network object; a matrix id by another matrix |
| `ZONE_POLYGON_INVALID` | fewer than three points, a non-finite coordinate, or zero area |
| `UNKNOWN_ZONE` | a connector or a matrix's `zones` names no zone |
| `UNKNOWN_ZONE_CONNECTOR_LINK` | a connector names no Link (not `UNKNOWN_LINK`, which is already a routeless Run code) |
| `ZONE_CONNECTOR_DIRECTION` | `direction` is neither `origin` nor `destination` |
| `ZONE_CONNECTOR_SHARE_INVALID` | `share` is not finite or not above 0 |
| `OD_MATRIX_SHAPE` | `zones` is empty or repeats a zone, or `trips` is not `zones × zones` |
| `OD_CELL_INVALID` | a cell is negative or not finite |
| `OD_PERIOD_INVALID` | not `0 ≤ startTime < endTime ≤ duration`, or a bound off the time grid. Shortening the duration below a matrix's end is refused with this code, as an input's interval is |
| `OD_PERIOD_OVERLAP` | two matrices of the same class overlap in time (they would add silently) |
| `OD_DEMAND_CLASS` | not exactly one of `vehicleTypeId` and `compositionId`, or the one given is blank |
| `INVALID_ID` (route, input) | while the document holds any zone, zone connector or matrix, an authored route or input id starting with `od:`, the prefix §3 reserves. The rule reads the content, not the file's schema (a document has none): adding the first zone to a document with such an id is refused on that `routes[i]` or `inputs[i]`, and a document without zones is never refused |

Whether the named type or composition exists is a Run check (§2, `OD_UNKNOWN_CLASS`), never an
edit refusal. The catalog's "used" rule (`resolveCatalogs`) counts a matrix that names a
composition, and deleting a type or composition that a matrix names is refused like one an input
names.

## 2. Demand and the Run checks

- **A matrix is demand.** A matrix with at least one positive cell counts as authored demand
  wherever inputs do: `onlyRunSettings` (`diagnostics.cpp`), the `EDIT_NO_INPUTS` guard in
  `compileDocument` and its row in `runDiagnostics` and `documentDiagnostics`. A project whose only
  demand is a matrix therefore runs. Its locale text says "no inputs or OD matrix".
- **Run checks, one walk.** `expandOd(network, definition, classes)` (`src/project/od_expansion.cpp`)
  returns the expanded definition and every OD failure on an authored path from the same pass;
  `odRunIssues` and `withOdDemand` are its two halves.
  - `compileDocument` refuses on the issues before anything compiles.
  - `runDiagnostics` and `documentDiagnostics` list them as runtime rows.
  - `validateNetwork` and `validateAuthoredDemand` never emit them, so authoring tolerates an
    incomplete demand as it tolerates an unconnected route (D18b).
- **The classes the check knows (D151).** `classes` holds the vehicle types and the usable
  compositions.
  - Run reads the project's catalog (`resolvedOdClasses`), and only when the file has a matrix,
    through the run's input manifest. A file without matrices reads nothing new.
  - The Problems tab and authoring know only what the document carries (`ownedOdClasses`); an
    external catalog they did not read accepts every class.
  - A composition that cannot be used counts as unknown: no types, a share not above 0, or an
    unknown type, in its base types or in any interval's.
- **What a row suppresses (D151).**
  - An `OD_UNKNOWN_CLASS` row skips its whole matrix.
  - Any row on a cell or on one of its zones skips the whole cell, so a cell is never half
    generated.
  - `OD_ORIGIN_LINK_INTERNAL` is reported for every origin connector on such a Link, carrying flow
    or not.
- **No Problems row names a generated object.** A row carries the matrix id as its object (named,
  not selectable) or a zone or connector id, which selects through `objectIdForPath` and
  `selectableFor`. The expansion (§3) emits nothing for a cell that a row below names, so no later
  check meets a broken generated input. The catalog checks (`ownedCatalogIssues`,
  `compositionIssues`, `timeTypeIssues`) run on the authored definition. Any other core issue on a
  generated route or input is moved onto the matrix cell its id names (`remapOdRows`; a shared
  route goes to the first cell that uses it). Authoring drops such issues, and Run reports them.

| Code | Run refuses when | Path |
|---|---|---|
| `OD_UNKNOWN_CLASS` | the matrix's vehicle type or composition is not in the catalog the project uses | `odMatrices[i].vehicleTypeId` or `.compositionId` |
| `OD_ORIGIN_LINK_INTERNAL` | an origin connector's Link has an incoming Connector: an engine source must start on a segment with no predecessor (`UNSUPPORTED_INTERNAL_INPUT`, `core/validate.cpp`) | `zoneConnectors[i]` |
| `OD_INTRAZONAL_TRIPS` | a diagonal cell is above 0: such trips use no road in Micro and would otherwise vanish | `odMatrices[i].trips[o][o]` |
| `OD_ZONE_NO_CONNECTOR` | a row (column) with a positive cell belongs to a zone with no origin (destination) connector | `odMatrices[i].zones[o]` |
| `OD_PAIR_UNREACHABLE` | a pair of connectors with flow (§3) has no drivable chain | `odMatrices[i].trips[o][d]` |
| `OD_PATH_SEARCH_LIMIT` | the chain search stopped at its limit, not at the topology (§3) | `odMatrices[i].trips[o][d]` |

## 3. Expansion into Micro inputs and routes (M8.3)

`withOdDemand(network, definition, classes)` returns the definition with the matrices expanded
into **ordinary authored Routes and VehicleInputs**, appended after the authored ones.
- **Indices.** Every authored `routes[i]` and `inputs[i]` path still names the same object.
- **No double expansion.** The result carries no matrices, so expanding it again adds nothing.
- **Without a matrix** the result is an exact copy.

**Where it runs.** In all four callers (`compileDocument`, `runDiagnostics`, `documentDiagnostics`
and `validateAuthoredDemand`), before any step that compiles inputs or routes: the
`EDIT_NO_INPUTS` guard, `resolveCatalogs` (routing decisions and compositions, in its existing
order), `expandRouteless` and `buildScenario`. So the Problems tab, the demand preview and Run
see the same inputs.

**What keeps the authored definition.** The catalog and authoring checks (`compositionIssues`,
`routelessIssues`, `demandAdvisories` and the behaviour assignments) are about what the author
typed. The §1 structural checks run before the expansion, and the cool-down is still added last
(`run.cpp`).

**Volume.** For matrix `m`, origin zone `o`, destination zone `d`, an origin connector `c` of `o` and
a destination connector `k` of `d`:

```text
vph(c, k) = trips[o][d] · 3600 / (endTime − startTime) · share(c) / Σ share(origin connectors of o)
                                                        · share(k) / Σ share(destination connectors of d)
```

Each `(m, o, d, c, k)` with `vph > 0` is spread over its chains (below) as inputs on `c`'s Link
over `[startTime, endTime)` with the matrix's class. A zero cell or zero share emits nothing; there
is no minimum.

**Path: fewest objects (D150).** Until M9 assigns paths by cost, a trip takes the chain through the
fewest Links and Connectors, the rule a placed routing decision already uses to reach its
destination:

1. When `c` and `k` are on the same Link, the chain is that Link alone.
2. Otherwise the chains come from `routeShortestChainsBounded(network, table, c.link, k.link)`, a
   variant of `routeShortestChains` (`model/network/routing.cpp`) that returns
   `{chains, stoppedAtLimit}`. The existing function and the placed decisions that call it are
   unchanged. The search is breadth first, and a chain is kept only when `routeLaneChains` gives
   at least one single-lane chain for it. A level whose chains are all undrivable is the answer:
   the search does not fall back to a longer level, so that pair is `OD_PAIR_UNREACHABLE`, as it
   is for a placed decision.
3. **Limits.** The search takes at most 20 steps after `c`'s Link (chains of up to 21 objects;
   Link-to-Link chains have an odd count, since Links join only through Connectors) and at most
   4 096 partial chains on one level. `stoppedAtLimit` is true when it ends at either bound with
   the target unfound, and at the depth bound only if some chain could still go on (a frontier of
   dead ends is the topology). That is `OD_PATH_SEARCH_LIMIT`, never `OD_PAIR_UNREACHABLE`, so the
   author can tell a network too large for the rule from a disconnected one.
4. **Lanes and chains.** Each lane of `c`'s Link is assigned to the first equally short chain (in
   the search's order) that it can drive in full. A lane that can drive none is assigned to the
   first chain it can reach as a stub, and changes lanes there by the M3.2.8b rules
   ([M3_8_CONTRACT](M3_8_CONTRACT.md) §2): `routeLaneFamily` on that chain already makes it a stub
   member. A lane with neither carries none of the pair's flow. The pair's flow splits **equally
   over the lanes that carry it**, not over all the Link's lanes. Chain `j` therefore gets
   `vph(c, k) · lanes(j) / carryingLanes`, and only chains that some lane uses are generated.
5. The route ends at the end of `k`'s Link, where the vehicle arrives. Nothing walks on past it.
6. **No decision acts on an OD trip.** Static, placed and positioned routing decisions and their
   type rules apply to authored inputs only; an OD vehicle passing a positioned decision keeps its
   route.

**Ids and order (part of the random-number contract).**
- One generated Route per used chain of a connector pair, shared by every matrix and cell that uses
  that pair: `od:route/<c>><k>/path-<j>`. It is keyed by **connector** ids, which §1 restricts;
  Link ids may contain `>` and would not give unique ids.
- One input per `(m, o, d, c, k)` and used chain `j`: `od:<matrix>/<origin zone>><destination
  zone>/<c>><k>/path-<j>`. Its `laneShares` are 1 on the lanes assigned to chain `j` and 0 on the
  others (`laneSplit` accepts 0), so each lane carries exactly the chain step 4 gave it. The §1 id
  rules make both ids injective.
- `<j>` counts only the chains some lane uses, from 0 in the search's order (D151). A chain no
  lane uses gets no id, so adding or removing one does not renumber the rest.
- Order: matrices in file order, then origin by `zones` index, then destination by `zones` index,
  then origin connectors, then destination connectors, each in network order, then chains in the
  search's order.
- Every input draws its arrivals from the run's one random stream in canonical id order
  ([SIMULATION](SIMULATION.md#determinism-snapshots-and-events)). A file **with** a matrix may
  therefore draw differently from the same demand authored as inputs. A file without one draws
  exactly as before (DEMAND_TIME_TYPES's precedent).

**Cost.** Chains are resolved through one `ConnectorPathTable` and one memo per expansion, keyed by
`(c.link, k.link)` and holding the chains with their `stoppedAtLimit` flag, as
`DestinationChainMemo` does for routeless destinations (D140). Validation on every edit then walks
each distinct pair once.

**What the expansion feeds.** The generated inputs are ordinary inputs, so the rest of the
pipeline applies unchanged:
- a composition class expands to `…/type-<type>` inputs (`expandCompositions`);
- the lane split and periods compile as for any input (`buildScenario`);
- the trips' count in a seed is Poisson around the expected trips (exact counts are a
  [DEMAND_IMPROVEMENT](../plans/DEMAND_IMPROVEMENT.md) contract, not this one);
- movement rows group trips by first and last Link and merge with authored routes on the same pair
  ([SIMULATION](SIMULATION.md#movement-evaluation-m25)).

## 4. Edits and commands (M8.2)

- **Commands.** `putZone`, `putZoneConnector`, `putOdMatrix` (empty id allocates `zone-N`,
  `zc-N`, `od-N`; the same id replaces) and `deleteZone`, `deleteZoneConnector`, `deleteOdMatrix`,
  each one Undo step through `History::execute`.
- **Cascades:**
  - Deleting a **Link** deletes its connectors in the same step, as its sections are deleted
    (`removeControlsOn`).
  - Deleting a **zone** deletes its connectors in the same step. It is refused with
    `EDIT_REFERENCED_ZONE` while a matrix or a parking lot names the zone, so no matrix row is
    silently dropped. (The lot half of this rule arrives with lots, M8.4.)
  - **Reversing** a Link that carries a connector is refused with `EDIT_REFERENCED_LINK`, whose
    text is extended to name connectors (`controlsNameLink`).
  - **Splitting** a Link keeps an origin connector on the first piece and moves a destination
    connector to the last (`checkSplitControls`, `splitControls`).
  - Copy, duplicate and the opposite Link never copy connectors ("demand is not copied").
- **Registries every new kind joins:**
  - `allocateId`'s used-id set;
  - the network-wide id set in each issue function;
  - the Link cascades above;
  - `objectIdForPath` and `selectableFor` (zones, connectors), so a Problems row selects its object.
    M8.2 adds `objectIdForPath` only, so a row names the zone or connector; `selectableFor` joins
    in M8.6, when the canvas first draws zones and has something to select;
    A matrix row carries the matrix id as its object, named but not selectable on the canvas
    (`definitionId` maps only routes, inputs, types, behaviours and programs);
  - `deleteObjects` (the canvas Delete);
  - a locale entry in `en.json` and `th.json` for every code above;
  - the catalog's delete refusal (`definitionNamesVehicleType`, `definitionNamesComposition`,
    `zones_od.cpp`), which the shell's catalog dialog calls.
- **Editor (M8.6).**
  - **Zone tool `Z`** (free; `P` is cycle priority): click the polygon's points, close it, then
    click a Link's start or end to add an origin or destination connector.
  - **OD matrix table**: zones × zones cells, period and class, with row and column totals.
  - Zones are selected, moved and deleted like other objects. A zone polygon moves with
    translate and rotate; its connectors stay on their Links.

## 5. Limits of this contract

- **No position.** A connector attaches at a Link end because the engine's sources start on a
  segment with no predecessor and lanes are cut only where Connectors attach. A connector part
  way along a Link needs a core source contract of its own.
- **No `both`.** A Link is one-way, so a zone that sends and receives on one road uses two
  connectors on two Links.
- **No assignment.** Fewest objects ignores congestion and length; M9 replaces it, starting from
  these chains.
- Not in this contract: exact counts, centreline-only Links (M10.6), zone attributes (M10.2), and
  a cut from a Macro model (M12).

## 6. Acceptance rows

Each row first asserts that its forcing worked, then the consequence. The groups `zones` and
`parking` join `TRAFFICSIM_TEST_GROUPS` (`CMakeLists.txt`); `zones-ui` and `parking-ui` are new Qt
executables in `src/shell/CMakeLists.txt`. Slice: when the row must pass. The M8.3 rows are in
`tests/od_expansion_tests.cpp` (ZO6–ZO8) and `tests/od_paths_tests.cpp` (ZO9–ZO12), sharing
`tests/od_fixture.hpp`.

| Row | Slice | Check | Test |
|---|---|---|---|
| ZO1 | M8.2 | Schema-29 round trip of all three keys; an omitted share reads as 1 and is not written; a file without the keys keeps its schema and bytes after a save through the new codec; each key below 29 and any unknown key in the objects is refused with its path, through `parseDocument` and through the CLI's M0 loader | `zones` |
| ZO2 | M8.2 | Every structural code of §1, one case each, including: an id containing `>`; an id clash with a travel-time section; overlapping same-class periods; shortening the duration below a matrix's end, and changing the time step so a matrix bound falls off the grid, each refused with `OD_PERIOD_INVALID`; an authored `od:` route accepted with no zone, then adding the first zone refused on that route's path | `zones` |
| ZO3 | M8.2 | Retired by M8.3, with its code, rows and locale entries. In M8.2 a matrix with a positive cell was refused on Run with `OD_NOT_EXPANDED` on `odMatrices[i]`, ahead of `EDIT_NO_INPUTS` | — |
| ZO4 | M8.2 | Commands and cascades of §4, each one Undo step: Link delete removes its connectors; zone delete removes its connectors; a zone named by a matrix, or by a lot, is refused; reverse refused; split keeps an origin connector on the first piece and moves a destination connector to the last; copy, duplicate and opposite carry no connector; deleting a type or composition that a matrix names is refused | `zones` |
| ZO5 | M8.2, M8.3 | Byte-identical without matrices: the four reference fixtures (events, checkpoints, summary, trajectory digest), and single-run, `--seeds` and `--compare` CSV and JSON for the four shipped projects, before and after each slice (JSON equal apart from the `buildCommit` value) | `zones`, session `cmp` |
| ZO6 | M8.3 | Every Run code of §2 on its authored path, including `OD_UNKNOWN_CLASS` as a Run row and not an edit refusal; the expansion emits nothing for a named tuple, and no Problems row names a generated input or route; authoring accepts each case | `zones` |
| ZO7 | M8.3 | A project whose only demand is one matrix compiles and runs; a matrix of zero cells counts as no demand | `zones` |
| ZO8 | M8.3 | Hand-computed expansion: two origin and two destination connectors with unequal shares give the four `vph` values, ids and order of §3; a zero cell emits no input | `zones` |
| ZO9 | M8.3 | `c` and `k` on one Link give a one-Link route. Two equally short chains with one lane able to drive only the second: two routes `…/path-0` and `…/path-1`, inputs with the `laneShares` and `vph` of §3 step 4. An undrivable shortest level is `OD_PAIR_UNREACHABLE` even though a longer drivable chain exists | `zones` |
| ZO10 | M8.3 | Search limits: a pair 20 steps apart resolves; the same layout 22 steps apart is `OD_PATH_SEARCH_LIMIT`; a level of more than 4 096 partial chains is `OD_PATH_SEARCH_LIMIT`; a disconnected control is `OD_PAIR_UNREACHABLE` | `zones` |
| ZO11 | M8.3 | A matrix whose composition has two or more types expands to `…/type-<type>` inputs (one type keeps the plain id); a positioned decision on an OD vehicle's Link does not change its route; two matrices sharing a connector pair produce one set of generated routes | `zones` |
| ZO12 | M8.3 | Same project and seed ⇒ same bytes with matrices | `zones` |
| ZO13 | M8.6 | The Z tool draws a zone and adds both connector kinds; the OD table edits a cell through History; every new code and UI string exists in `en.json` and `th.json` (one test lists them all) | `zones-ui`, `project` |
