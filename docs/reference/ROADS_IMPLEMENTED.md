# Roads — as implemented (S3.2–S3.7 and Road demand)

What each slice of [ROADS](ROADS.md) built, kept apart from the contract so that file stays under
500 lines. The section numbers are the contract's, so "ROADS §16" still names a section here.

## 14. S3.2 as implemented (D304)

- **Values** (`src/model/demand/definition.hpp`): `LinkType` and `Vdf` gain `capacityBasis`, where
  empty means `practical`.
- **The codec** (`src/project/link_type_codec.*`):
  - `capacityBasis` is optional on both, written only when set;
  - a value other than `practical` or `ultimate` is `INVALID_LINK_TYPE` or `INVALID_VDF` at
    `.capacityBasis`;
  - an owned type with a basis makes the file schema 53 (`usesCapacityBasis`); below 53 the key is
    `EDIT_UNSUPPORTED_FIELD` (`rejectCapacityBasisBefore53`).
- **Macro** (`compileMacroNetwork`): a Link whose type and VDF have different bases is
  `MACRO_CAPACITY_BASIS_MISMATCH` at its `linkType`. Meso reads any basis.
- **Data:**
  - `data/vdf/bpr.json` is `practical`;
  - `data/link-types/` holds `freeway-55` … `freeway-75` and `multilane-45` … `multilane-70`, each
    with:
    - the capacity of Exhibit 12-4;
    - FFS × 0.44704 m/s;
    - `jamDensity` 118.0605 pc/km/ln (190 pc/mi/ln, Exhibit 10-7);
    - `vdfId` `bpr` and basis `ultimate`;
    - a source naming its exhibits and pages.

  Macro therefore refuses them until an `ultimate` VDF is cited, as chosen (6b).
- **The editor** (`src/shell/editor_link_types.cpp`): the Link types tab gains a Capacity basis
  column. A project with no owned types lists the eleven HCM types read-only. The dialog keeps a
  type's basis, and a new type has none.
- **Rows:** RD1's capacity-basis part, RD2 and RD3, in `tests/road_catalog_tests.cpp` (ctest
  `roadcatalog`).
- **Amendment (D304):** `rank` moved to S3.6. Its reader is the coarse Node, and NODES §5 stores no
  field before its reader.

## 15. S3.3 as implemented (D305)

- **Model:**
  - `Road` and `RoadOverrides` in `src/model/network/network.hpp`, held in `Network::roads`;
  - `Link::roadId`;
  - `CrossSection` in `src/model/demand/definition.hpp`, kept in `definition.crossSections` under
    the presence rule. The installed directory is `data/cross-sections/`, which is absent: none
    ships.
- **The codec** (`src/project/road_codec.*`):
  - `network.roads`, Link `roadId` and owned templates, each written only when used. Any of them
    makes the file schema 53; below 53 each key is `EDIT_UNSUPPORTED_FIELD` at its path;
  - a bad Road shape is `EDIT_ROAD_INVALID`, and a bad template value is `INVALID_CROSS_SECTION`;
  - an owned template that does not exist is `ROAD_CROSS_SECTION_UNKNOWN`, checked on commit.
- **Validation** (`roadIssues`, `src/model/network/roads.*`): ids, `ROAD_GEOMETRY_INVALID`, and
  `ROAD_LINK_MISMATCH` both ways (a Road's `links` against each Link's `roadId`).
- **Commands** (`src/commands/road_commands.*`), each one Undo step:
  - `putRoad`, `deleteRoad` (its Links stay, detached), `putCrossSection` and `deleteCrossSection`;
  - `applyTemplate`:
    - Links centred at median/2 + width/2 on each side, the backward one reversed, with
      `laneOffset` 0;
    - a re-apply keeps Link and lane ids, through `changeLanes`, and reanchors Connectors;
    - refusals: `EDIT_ROAD_NO_TEMPLATE`, `ROAD_CROSS_SECTION_UNKNOWN`, `EDIT_ROAD_DIRECTIONS` and
      `EDIT_REFERENCED_LANE`.
- **Detaching** (`detail::detachFromRoad`):
  - `changeGeometry`, `changeLanes`, `resizeLinkLanes`, `reverseLink`, `splitLink`,
    `translateObjects` and `rotateObjects` detach a generated Link;
  - `deleteLink` drops it from its Road, and a duplicated Link carries no `roadId`.
- **Rows:** RD1's Road part and RD5–RD9, in `tests/road_tests.cpp` (ctest `roads`).
- **The editor** is S3.3b (§16).

## 16. S3.3b as implemented (D307)

The owner chose (2026-10-22): the Road tool applies the chosen template at once, its key is **O**,
and a detached Road shows in the table and on the canvas.
- **Road tool** (`O`, index 14, palette group *Geometry* after Links; `EditorCanvas::Tool::road`):
  - it shares the zone draft (D306): Ctrl+right-click the first centreline point, plain clicks
    the next ones in the forward direction, Backspace drops the last, Esc cancels;
  - Enter, with two points or more, calls `roadCommitted`. `putRoad` at the visible level and,
    when the Roads tab's template combo names one, `applyTemplate` run in one `execute`, so the Road
    and its Links are one Undo step. A refused apply keeps the draft. With "(no template)" the Road
    has no Links;
  - the pointer is the arrow with the road glyph (`EditorIcon::road`); under the tool every Road's
    centreline is a thin ink line.
- **Roads tab** (`src/shell/editor_roads.cpp`, group *Network*): id, name, template, lanes (with
  overrides), Links and status. Its toolbar holds the template combo, *Apply template*, *Edit road*
  (name, template, lanes each way, lane width and median; a field at its lowest value means "from
  template" and stores no override) and *Delete road* (its Links stay).
- **Status:** no template; unknown template (`ROAD_CROSS_SECTION_UNKNOWN`); not applied; applied; or
  partly detached (*k of n Links*) when it generates fewer Links than its template's directions. The
  Road keeps no record of a Link it lost, so a Road whose every Link was detached reads as not
  applied.
- **Canvas** (`src/editor/canvas_roads.cpp`): the selected row's Road is lit, with its centreline,
  a direction arrowhead and its generated Links washed. All three are in the selection colour, or
  in the warning colour while partly detached. A detached Link is not lit.
- **Cross-sections tab** (`src/shell/editor_cross_sections.cpp`, group *Network*): it is listed
  read-only from the installed directory (none ships) until *Add* owns it, as Link types are.
  *Add* and *Edit* set name, one- or two-way, lanes each way, lane width, median, link type and
  source. Per-lane widths are kept only while the lanes and the width are unchanged. *Delete* is
  refused while a Road names the template (`EDIT_CROSS_SECTION_IN_USE`).
- **Rows:** RD23–RD26 in `tests/road_ui_tests.cpp` (ctest `road-ui`). Four mutations were caught:
  - no apply on commit;
  - the detached status never given;
  - the canvas never told;
  - a Road needing three points.

## 17. S3.4 as implemented (D308)

The owner chose the tolerance (2026-10-22): 15° and half both widths plus 30 m.
- **`joinOppositeLinks(d, forward, backward)`** (`src/commands/road_commands.*`) makes one Road with
  no template and `links` = [forward, backward]. Both Links take its `roadId`; neither Link moves.
  - **Centreline:** `meanCentreline` (`src/model/network/roads.*`) of the two bundle centrelines
    (`linkCentreline`), digitised along the forward Link. Equal vertex counts pair vertex by vertex,
    which is exact for `oppositeLink`'s offsets, bends included. Otherwise each vertex pairs with
    its closest point on the other line, and the midpoints are ordered by station along the
    forward Link.
  - **Refusals:**
    - `EDIT_ROAD_JOIN_TAKEN`: a Link already in a Road;
    - `EDIT_ROAD_JOIN_LEVEL`: the Links are on different levels;
    - `EDIT_ROAD_JOIN_DIRECTION`: one Link twice, or the same direction (the end-to-end vectors'
      cosine ≥ 0);
    - `EDIT_ROAD_JOIN_PARALLEL`: more than 15° off opposite, or any centreline vertex farther from
      the other centreline than half of both widths plus 30 m. The constants are
      `joinAngleDegrees` and `joinMedianMetres`.
- **`assignTemplate(d, roads, crossSectionId, installed)`** sets or clears the template on every
  listed Road, generating no Link. An unknown template is `ROAD_CROSS_SECTION_UNKNOWN` at
  `crossSectionId`, and an unknown Road is `EDIT_UNKNOWN_OBJECT`.
- **Editor (Roads tab):**
  - *Join opposite Links* joins the two Links selected on the canvas, the first one forward. It is
    enabled only with exactly two selected.
  - *Assign template* puts the combo's template on every selected row, and "(no template)" clears
    it. The table now takes several rows.
  - A Road with no template reads *No template* even when it has Links, as a joined Road does.
- **Rows:** RD10 and RD11 in `tests/road_tests.cpp` (`roads`), with their editor part in `road-ui`.
  Four mutations were caught:
  - the mean replaced by the forward line;
  - no angle check;
  - no distance check;
  - no unknown-template check.

## 18. S3.5 as implemented (D309)

- **`findCrossings(network, tolerance)`** (`src/model/network/road_crossings.*`, pure). It looks at
  every pair of Roads on one level that is not an overpass, and records:
  - each proper crossing of their centrelines;
  - each end of one Road within the tolerance of the other.

  How places are formed and listed:
  - events within the tolerance of one another are one place, at their mean;
  - each Road appears there once, with its station, and is an *end* within the tolerance of either
    end of its centreline;
  - legs are 2 per Road passing through and 1 per Road ending;
  - a place is listed with two Roads or more and three legs or more, and only where no Node
    already covers it. So two Roads meeting end to end are left to S3.6's two-leg merge, and a
    confirmed junction is not asked again;
  - the list is sorted by level, then x, then y.
- **`network.overpasses`** holds sorted pairs of Road ids at schema 53. It is written only when
  non-empty; below 53 it is `EDIT_UNSUPPORTED_FIELD`, and a bad shape is `EDIT_ROAD_INVALID`. A pair
  that names no Road, or one Road twice, is `ROAD_OVERPASS_UNKNOWN` at `overpasses[i]`. `deleteRoad`
  drops the pairs naming its Road.
- **`splitRoad(d, road, station)`:**
  - the Road keeps the first part of its centreline, and a new Road with the same name, template,
    overrides and level takes the rest;
  - each generated Link is split by `splitLink` at that point, which keeps routes, heads,
    decisions and zone connectors;
  - the forward Link's original id stays on the first Road; the backward Link's original id,
    which starts at the far end, goes to the new Road;
  - overpass pairs naming the Road are copied for the new Road;
  - a station within 0.2 m of an end is `EDIT_SPLIT_RANGE`.
- **`applyCrossings(d, places, choices, tolerance, installed)`**, in one Command:
  - **Overpasses first.** Every pair of the place's Roads is stored; where a Road ends there, the
    choice is `EDIT_CROSSING_OVERPASS`.
  - **Then each junction.** The Roads within the tolerance of the place are found again, because an
    earlier split may have renamed them. Those passing through are split.
  - **A `generated` Node** is added: a 16-gon around the place.
  - **The radius** is the largest of (each generated Link's distance from the place plus half its
    width), plus the widest lane there. A Road without Links uses its template's median/2 plus its
    wider direction. With nothing to measure, the radius is twice the tolerance.
  - A refused split refuses the batch.
- **`mergeNodes(d, ids)`** (`src/commands/node_commands.*`):
  - the first Node takes the convex hull of all their vertices (`convexHull`, now shared from
    `node_suggestions`) and keeps its id and name;
  - it stays `generated` only if all were, and the others are deleted;
  - fewer than two or an unknown id is `EDIT_NODE_MERGE`;
  - when an input held valid detail (its `nodeMembership` had no issue) and the result has issues,
    those issues are thrown at `nodes[k]` and nothing changes.
- **Editor:**
  - **Roads tab: *Find crossings*** opens a dialog with the snap tolerance (2 m), *Search again* and
    one row per place (level, x, y, Roads, legs, and Junction or Overpass). Confirm is one Undo
    step, and the status bar counts both kinds. With nothing to confirm, the status bar says so.
  - **Nodes tab: *Merge nodes*** works on the selected rows (the table now takes several), with the
    current row first.
- **Rows:** RD12–RD15 in `tests/road_crossing_tests.cpp` (`roadcrossings`), with their editor
  part in `road-ui`.

## 19. S3.6a as implemented (D310)

S3.6 is split in two. S3.6a ships `rank` and `coarse` with the Macro and Meso compile that reads
them, so no field is stored before its reader. The editor's coarse dialog, refine, derive back and
the lifecycle are S3.6b (RD17, RD18). RD20's Micro half, `SEAM_ROAD_NOT_GENERATED`, moves to S3.7
with the other seam checks.

- **`rank`** is an optional integer on a link type, written only when set; an owned ranked type
  makes the file 53, and below 53 it is `EDIT_UNSUPPORTED_FIELD` at
  `definition.linkTypes[i].rank`. A rank that is not an integer is a malformed entry
  (`EDIT_CATALOG_READ`). The shipped types stay unranked; the owner ranks them. The Link-type
  dialog has a *Rank* field whose lowest value is *None*, and the table shows it.
- **`majorByRank(legs)`** (`src/model/demand/coarse_compile.*`) returns the legs with the highest
  rank. When a leg has no rank, or every leg shares the top rank, it returns nothing, and Macro and
  Meso advise `NODE_PRIORITY_UNDETERMINED` at `network.nodes[i]` for a priority Node of three legs
  or more without `major`. Neither reads priority yet (D297), so this is not a refusal.
- **`Node::coarse`** (`network.hpp`):
  - fields: `control` (`priority`, `signalised`, `allWayStop`; absent is priority), `major`,
    `bannedTurns` and `signal {cycle, stages[{green, turns}]}`; turns are `[in Road, out Road]`;
  - it is written only when present, with fields at their defaults left out, so an all-default
    one is `{}`;
  - below 53 it is `EDIT_UNSUPPORTED_FIELD` at `network.nodes[i].coarse`, and a bad shape is
    `EDIT_NODE_INVALID` at its path.
- **Detail and legs** (`src/model/network/coarse_nodes.*`, pure):
  - a Node has detail when any Link or Connector has a run inside its polygon (`polygonRuns`, moved
    to `polygon.*` from Node membership). **Revised by D311 (§20):** detail is now the interior
    only, and Links reaching in are legs;
  - a leg is a Road with a centreline end inside the polygon, named by its id, at the last
    boundary crossing before that end. A Road wholly inside gives no leg;
  - a Road that passes through without ending is not cut. It is the advisory `NODE_ROAD_THROUGH`,
    and *Find crossings* splits it.
- **Checks** (`coarseNodeIssues`, in the network check):
  - `NODE_COARSE_WITH_DETAIL` at `nodes[i].coarse`;
  - `NODE_COARSE_UNKNOWN_LEG` at `.major[k]`, `.bannedTurns[k]` or `.signal.stages[s].turns[t]`;
  - `NODE_COARSE_SIGNAL_INVALID` at `.signal` for:
    - a signalised Node without a signal, or a signal on another control;
    - no stages;
    - a cycle or green that is not positive;
    - greens summing above the cycle.
- **`compileRoads`**, shared by `compileMacroNetwork` and `compileMesoNetwork`. Both take the
  templates as a new last argument; the inputs pass them only when some Road has no Links, so
  nothing else is read or recorded.
  - **Pieces.** A Link-less Road gives one piece per direction of its template, trimmed at the
    legs of the Nodes without detail that it ends in.
    - The forward piece runs along the centreline; the backward one has decreasing stations.
    - Lanes are the override, else the template's; the type is the template's `linkType`.
    - A Road with Links compiles from its Links, as before.
  - **Joins.**
    - A Node without detail is one, with or without `coarse`: absent fields are the defaults.
      Nodes without detail are no longer `MESO_NODE_REFUSED`.
    - So is a group of free Road ends within 2 m of one another (Find crossings' default
      tolerance), as a continuing Link is in Meso. Without this, two templates meeting end to end
      would disconnect.
  - **Turns.**
    - Every (in, out) pair of different Roads, minus `bannedTurns`, with no free time (D301).
    - At a signalised Node: `signal` `node:<id>` and g/C = the summed greens of the stages that
      serve the turn over the cycle (D299). A turn no stage serves gets 0.
  - **Refusals** at `network.roads[i].crossSectionId`:
    - no template: `MACRO_ROAD_TEMPLATE_MISSING`;
    - an unknown template: `ROAD_CROSS_SECTION_UNKNOWN`;
    - a template with no type: `MACRO_LINK_TYPE_MISSING`;
    - an unknown type: `MACRO_LINK_TYPE_UNKNOWN`.

    A Road refused for its template gives no piece.
- **Meso:** one `MesoLink` per piece, with `source` the Road id. There is one node per join (with
  the Node id; allWayStop shows as unsignalised) and one per free piece end. Everything is added
  after the Links, so a project without Link-less Roads compiles exactly as before.
- **Macro:**
  - a `link` arc per piece, between nodes `<road>:forward|backward@<station>`, so a U-turn never
    shares a node. A basis mismatch is at `network.roads[i].crossSectionId`;
  - a `connector` arc per turn (`source`: the Node id), uncapacitated and with time 0;
  - `MacroCompile` gains `advisories`, which `runMacro` reports ahead of demand's.
- **Demand at this slice:** zone connectors attached to Links only. §22 adds direct Road targets
  for Macro; Meso follows in [MESO_ROAD_DEMAND](MESO_ROAD_DEMAND.md) (D341).
- **Rows:** RD4, RD16, RD19 and RD20 (Macro and Meso) are in `tests/road_coarse_tests.cpp`
  (`roadcoarse`). The rank field is checked in `macro-ui`.

## 20. S3.6b as implemented (D311)

- **Detail revised (D311, replacing D310's reading).** A Node's detail is its interior: a Connector
  with a run inside the polygon, or a Link wholly inside it (`nodeHasDetail`). A Link that only
  reaches in is a leg (`nodeLinkLegs`: one crossing, ending or starting inside). Under D310's
  reading, deleting a junction's Connectors left the approach Links inside as "detail", so derive
  back could never happen.
  - The Connectors `splitLink` adds between split pieces are detail, so a junction S3.5 makes over
    generated Links is Micro's, with its through turns, until they are deleted.
- **Link legs compile (D311).** At a Node without detail, `compileRoads` takes each Link leg as an
  arm next to the Link-less Roads' pieces.
  - The arm is named by the Link's `roadId`, or by the Link's own id when it has no Road.
  - The leg's part inside the Node goes into `dropped`. Macro and Meso skip that part and put a
    station at the crossing.
  - Turns, bans and stage g/C apply as for pieces. Meso joins a Node's leg stations into one node.
- **`setNodeCoarse(d, id, value)`** (`src/commands/coarse_commands.*`) stores the values, with an
  all-default value stored as none, and clears `generated`, as any edit of a Node does.
- **`settleNodes(after, before)`** runs inside `History::execute` on every Command, between the change
  and the check. It touches only Nodes present before and after, and is skipped when no Link,
  Connector, Node or Road changed.
  - **Refine:** a coarse Node that had no detail and now has some loses `coarse` (RD17).
  - **Derive back:** a Node that had detail and now has none, with no `coarse`, takes
    `deriveCoarse` of its detail as it was, when every Road it names is still a leg (RD18).
  - **Lifecycle:** applies to a Node whose Road legs became exactly two in this Command, with no
    `coarse`, no detail and no Link leg, and whose two Roads:
    - are Link-less;
    - share one template, one level and the same overrides;
    - meet with one ending where the other starts.

    Such a Node is deleted and the Roads joined. The first Road keeps its id and takes the second's
    centreline after its own. The second's id is renamed to the first in overpasses and in other
    Nodes' coarse values (RD27). A Node drawn with two legs is kept, as is one with detail or
    `coarse`.
- **`deriveCoarse(d, node)`** reads `nodeMembership`. Legs are named by the Road of each leg Link,
  and nothing is derived if a leg Link has no Road ending in the Node.
  - **Control:** signalised from the heads; otherwise priority, which is absent.
  - **Major:** for priority, the in-leg Roads that never yield in a conflict, when others do. **Amended by D324:** an arm is minor when each of its turns that meets a decided conflict yields in one; an arm with a turn that meets some and never yields is major, so a major far turn yielding to the opposite through leaves its arm major.
  - **Banned turns:** every pair of different Roads with no turn.
  - **Signal:** the turns' programs (`turnSignalPrograms`, moved from Meso to Node membership). They
    give green intervals over their common cycle; a controller group is offset and wrapped, and a
    legacy program uses its green phases.
    - The cycle is cut at every interval end. Each span with a different set of green turns is a
      stage, and a stage running over the cycle's end is one stage.
    - Without a common cycle or any green there is no signal, and control stays priority.
- **Editor:** the Nodes tab's *Coarse values…* is enabled for a Node without detail. Its dialog has:
  - the control;
  - the major legs (for priority);
  - a turn table with *Allowed* and one checkable column per stage (for signalised);
  - the cycle and the stage greens, with *Add stage* and *Remove stage*.

  Confirm is one Undo step. The table's Control column shows a Node without detail as "… (coarse)".
- **Rows:**
  - RD17, RD18 and RD27 are in `tests/road_coarse_command_tests.cpp` (`roadcoarse`).
  - RD19 gains a junction over generated Links, compiled once its Connectors are deleted.
  - RD28, the dialog, is in `road-ui`.
- **Mutations caught:** refine off, banned turns not derived, the wrapped stage not merged, the
  lifecycle on any two-leg Node, and D310's detail.

## 21. S3.7 as implemented (D312)

S3 is complete with this slice.

- **`seamChecks(network, types, templates, level, speedJump)`** (`src/model/demand/seam_checks.*`,
  pure). The caller resolves link types and templates. It returns rows `{code, path, severity}`
  sorted by code and path; none refuses a run or changes the document.

  | Code | Path | Micro | Meso, Macro |
  |---|---|---|---|
  | `SEAM_ROAD_NOT_GENERATED`: a Road without Links | `network.roads[i]` | Advisory | No row (compiled, §9) |
  | `SEAM_LANE_COUNT`: a Link with `roadId` whose lane count differs from its Road's for that direction (override, else template; forward is the Road's first Link) | `network.links[i]` | Warning, with the two fixes | Advisory |
  | `SEAM_SPEED_JUMP`: types' free speeds differ by more than the threshold (20 km/h by default) | `network.connectors[i]` | Warning | Warning |
  | `SEAM_LINK_DETACHED`: a Road with Links but fewer than its template's directions | `network.roads[i]` | Advisory | Advisory |

- **Readings taken:**
  - **"The Node's leg"** is read as the Road the Link belongs to. A coarse Node's leg is a Road,
    and a detailed Node's leg is the Link itself, so the seam is a generated Link against its
    Road's lanes. It is checked for every generated Link, at a Node or not.
  - **"Consecutive links"** are those joined by a Connector outside every Node. A turn inside a
    junction is not a corridor, so an arterial turning into a local street is not flagged. Two
    Link-less Roads meeting end to start outside a Node are also consecutive; the row is at the
    later Road.
  - **The detached Link** no longer names its Road (D305), so the row is on the Road.
  - **The threshold** is a display choice, kept in the dialog for the session and not stored.
- **Fixes** (`src/commands/road_commands.*`), each one Undo step that changes only the side chosen:
  - **`useLinkLanes`** (*Use Link*): the Road's override for the Link's direction becomes its lane
    count;
  - **`useRoadLanes`** (*Use Node*): *Apply template* again, so the Links take the Road's lanes
    and keep their ids;
  - a Link without a Road is `EDIT_SEAM_NOT_GENERATED`.
- **Editor:** *Seam checks…* on the Roads tab opens a dialog with the level, the threshold, and one
  row per issue (severity, problem, object). A lane-count row at Micro has *Use Link* and *Use
  Node*, and the list is checked again after each fix.
- **Not built in S3:** the readiness panel and *Fill defaults* (S4) run these checks, as ROADS §7
  says. Road-only Macro demand follows in §22.
- **Rows:**
  - RD20 (Micro half), RD21, RD22 and RD29 are in `tests/road_seam_tests.cpp` (`roadseams`);
  - RD21's editor part is in `road-ui`.
- **Mutations caught:**
  - the not-generated row at every level;
  - *Use Link* on the wrong direction;
  - turns inside a junction counted;
  - the threshold made inclusive;
  - the detached row off.

## 22. Road-only Macro demand (D340)

The booked seam in D310: a Macro study can attach its existing OD layer directly to Roads
without generating Micro Links. This first slice covers Macro; Meso follows in
[MESO_ROAD_DEMAND](MESO_ROAD_DEMAND.md) (D341). No model parameter or
milestone gate changes.

### Interface and storage

- A ZoneConnector targets exactly one of:
  - the existing `linkId`, with its unchanged schema-31 JSON;
  - `roadId` and required `roadDirection`, `"forward"` or `"backward"`, at schema **57**.
- `direction` still means `"origin"` or `"destination"` relative to the zone. Travel direction is
  along or against the Road's centreline. Origin attaches to that direction's compiled start;
  destination to its compiled end (the boundary where a coarse Node trims it, when applicable).
- The target Road must have no generated Links. On load/commit, an unknown target is
  `ZONE_CONNECTOR_UNKNOWN_REF` at `.roadId`; a Road already carrying Links gives
  `ZONE_CONNECTOR_ROAD_HAS_LINKS`. Bad shape, extra keys or a direction other than the two above
  gives `EDIT_ZONE_INVALID`, including JSON with both/neither targets. A command supplying
  both/neither targets gives `ZONE_CONNECTOR_TARGET_INVALID`. Either new key below 57 gives
  `EDIT_UNSUPPORTED_FIELD` at that key.
- Empty Road targets are not stored; a file without one keeps its feature schema and bytes.
  No inferred direction, mid-Road station or extra demand table is introduced.

### Readers and editing

- `compileMacroNetwork` connects the centroid arc to the selected Road piece. An unavailable
  direction (including a backward connector on a one-way template, or no surviving piece) gives
  `MACRO_ZONE_ROAD_UNAVAILABLE` at `zoneConnectors[i].roadDirection`. Existing graph/template
  refusals still apply. Demand shares, modes, PCU, periods, assignment, named runs and exports
  use the existing Macro pipeline unchanged. Mixed Link/Road connector pairs can run.
- At D340, Micro and Meso refused Road targets with `ZONE_CONNECTOR_ROAD_REQUIRES_LINKS` at the connector;
  this prevents a successful empty run or a partial OD expansion. A Micro study-area cut still
  keeps only its Links and takes outside demand from the selected cut source (S5).
  D341 adds Meso's graph OD reader; Micro retains this refusal until conversion.
- The Zones tab keeps its selected-Link actions and adds *Road connector…*. Its dialog selects
  a Link-less Road, zone direction, travel direction and share; Cancel changes nothing. Editing a Road connector can change its direction
  and share. The table names the Road and direction, and the canvas line follows the chosen end.
- *Apply template* remaps all of that Road's connectors to the corresponding generated Link in
  the same Command, retaining connector ids, zone ids, direction and shares. A one-way template
  cannot accept a backward connector: the Command refuses before mutation with
  `ZONE_CONNECTOR_ROAD_REQUIRES_DIRECTION`. Re-apply and Link edits then follow the existing rules.
- `splitRoad` retains the endpoint: forward origins/backward destinations stay on the first
  piece; forward destinations/backward origins move to the second. Delete cascades connectors
  as Link deletion does, keeping zones and matrix counts. The two-leg lifecycle remaps external
  endpoints to the joined Road, but keeps the Node and both Roads if a connector uses their
  shared end (merging would turn that endpoint into an unsupported internal station).

### Acceptance rows

| Row | Case | Asserts |
|---|---|---|
| RD30 | Codec | Both travel directions round-trip at 57; either key below 57 refuses at its path; malformed/ambiguous targets fail; existing Link JSON and shipped project bytes stay unchanged |
| RD31 | Assignment | A Road-only corridor with OD has exact forward/backward flows and PCU, stable replay, shared connector weights and period isolation; one-way backward and banned turns refuse; a mixed Link/Road path runs |
| RD32 | Lifecycle | Apply remaps all four origin/destination and travel-direction combinations in one Undo/Redo; one-way refusal is atomic; split retains ends; delete preserves matrix counts; lifecycle merges only when it preserves connector endpoints |
| RD33 | Editor | Add/Cancel, direction/share edit, Undo/Redo, endpoint drawing, generated-Road refusal and save/reopen, in English/Thai; existing Link actions/dialogs remain usable |
| RD34 | CLI and refusals | Real CLI Macro JSON/CSV and named execution keep original source hashes and input bytes; Micro readiness/execution refuse Road targets; D341 adds Meso success/refusal coverage (MR6) |

Verification evidence and platform are recorded in PROGRESS; local Linux checks cannot close an
owner Windows/high-DPI appearance gate.

## 23. Review fixes (D382)

A review of the Road model (2026-10-28) found four defects, each reproduced by a row before its fix:
- **Levels in *Find crossings*:** places were grouped by distance only, so a crossing above another
  joined it. Events now group only on one level (RD30).
- **Split and coarse values:** `splitRoad` left a coarse Node at the moved end naming the first piece,
  refusing the Command (and *Find crossings*' batch) with `NODE_COARSE_UNKNOWN_LEG`. Those Nodes now
  name the new Road; the near end's keep the id (`renameCoarseRoad`, shared with the lifecycle's join; RD31).
- **A Node narrower than its arms:** a junction found before a template gave the Roads their width
  gets a 4 m disc that the Links pass beside; F4 listed it, then failed on a gap. It is now
  `FILL_NODE_ARM_OUTSIDE` ([READINESS](READINESS.md) RS19). A warning in the *Find crossings* dialog is booked in NEXT.
- **Direction of a generated Link:** read from the stations of its ends along the centreline
  (`forwardOnRoad`), not from its place in `links`, which a detach shifted (RD32, RD33). Two listed
  Links running one way are `EDIT_ROAD_DIRECTIONS`. No schema change.
