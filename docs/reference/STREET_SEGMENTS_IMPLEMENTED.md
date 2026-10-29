# Street segments — as implemented (S3.2–S3.7 and Street segment demand)

What each slice of [STREET_SEGMENTS](STREET_SEGMENTS.md) built, kept apart from the contract so that file stays under
500 lines. The section numbers are the contract's, so "STREET_SEGMENTS §16" still names a section here.

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
- **Rows:** RD1's capacity-basis part, RD2 and RD3, in `tests/street_segment_catalog_tests.cpp` (ctest
  `roadcatalog`).
- **Amendment (D304):** `rank` moved to S3.6. Its reader is the coarse Node, and NODES §5 stores no
  field before its reader.

## 15. S3.3 as implemented (D305)

- **Model:**
  - `Street segment` and `RoadOverrides` in `src/model/network/network.hpp`, held in `Network::streetSegments`;
  - `Link::streetSegmentId`;
  - `CrossSection` in `src/model/demand/definition.hpp`, kept in `definition.crossSections` under
    the presence rule. The installed directory is `data/cross-sections/`, which is absent: none
    ships.
- **The codec** (`src/project/street_segment_codec.*`):
  - `network.streetSegments`, Link `streetSegmentId` and owned templates, each written only when used. Any of them
    makes the file schema 53; below 53 each key is `EDIT_UNSUPPORTED_FIELD` at its path;
  - a bad Street segment shape is `EDIT_STREET_SEGMENT_INVALID`, and a bad template value is `INVALID_CROSS_SECTION`;
  - an owned template that does not exist is `ROAD_CROSS_SECTION_UNKNOWN`, checked on commit.
- **Validation** (`streetSegmentIssues`, `src/model/network/streetSegments.*`): ids, `ROAD_GEOMETRY_INVALID`, and
  `ROAD_LINK_MISMATCH` both ways (a Street segment's `links` against each Link's `streetSegmentId`).
- **Commands** (`src/commands/street_segment_commands.*`), each one Undo step:
  - `putStreetSegment`, `deleteStreetSegment` (its Links stay, detached), `putCrossSection` and `deleteCrossSection`;
  - `applyTemplate`:
    - Links centred at median/2 + width/2 on each side, the backward one reversed, with
      `laneOffset` 0;
    - a re-apply keeps Link and lane ids, through `changeLanes`, and reanchors Connectors;
    - refusals: `EDIT_STREET_SEGMENT_NO_TEMPLATE`, `ROAD_CROSS_SECTION_UNKNOWN`, `EDIT_STREET_SEGMENT_DIRECTIONS` and
      `EDIT_REFERENCED_LANE`.
- **Detaching** (`detail::detachFromStreetSegment`):
  - `changeGeometry`, `changeLanes`, `resizeLinkLanes`, `reverseLink`, `splitLink`,
    `translateObjects` and `rotateObjects` detach a generated Link;
  - `deleteLink` drops it from its Street segment, and a duplicated Link carries no `streetSegmentId`.
- **Rows:** RD1's Street segment part and RD5–RD9, in `tests/street_segment_tests.cpp` (ctest `streetSegments`).
- **The editor** is S3.3b (§16).

## 16. S3.3b as implemented (D307)

The owner chose (2026-10-22): the Street segment tool applies the chosen template at once, its key is **O**,
and a detached Street segment shows in the table and on the canvas.
- **Street segment tool** (`O`, index 14, palette group *Geometry* after Links; `EditorCanvas::Tool::road`):
  - it shares the zone draft (D306): Ctrl+right-click the first centreline point, plain clicks
    the next ones in the forward direction, Backspace drops the last, Esc cancels;
  - Enter, with two points or more, calls `streetSegmentCommitted`. `putStreetSegment` at the visible level and,
    when the Street segments tab's template combo names one, `applyTemplate` run in one `execute`, so the Street segment
    and its Links are one Undo step. A refused apply keeps the draft. With "(no template)" the Street segment
    has no Links;
  - the pointer is the arrow with the road glyph (`EditorIcon::road`); under the tool every Street segment's
    centreline is a thin ink line.
- **Street segments tab** (`src/shell/editor_street_segments.cpp`, group *Network*): id, name, template, lanes (with
  overrides), Links and status. Its toolbar holds the template combo, *Apply template* (every selected row since D385, §24), *Edit road*
  (name, template, lanes each way, lane width and median; a field at its lowest value means "from
  template" and stores no override) and *Delete road* (its Links stay).
- **Status:** no template; unknown template (`ROAD_CROSS_SECTION_UNKNOWN`); not applied; applied; or
  partly detached (*k of n Links*) when it generates fewer Links than its template's directions. The
  Street segment keeps no record of a Link it lost, so a Street segment whose every Link was detached reads as not
  applied.
- **Canvas** (`src/editor/canvas_street_segments.cpp`): the selected row's Street segment is lit, with its centreline,
  a direction arrowhead and its generated Links washed. All three are in the selection colour, or
  in the warning colour while partly detached. A detached Link is not lit.
- **Cross-sections tab** (`src/shell/editor_cross_sections.cpp`, group *Network*): it is listed
  read-only from the installed directory (none ships) until *Add* owns it, as Link types are.
  *Add* and *Edit* set name, one- or two-way, lanes each way, lane width, median, link type and
  source. Per-lane widths are kept only while the lanes and the width are unchanged. *Delete* is
  refused while a Street segment names the template (`EDIT_CROSS_SECTION_IN_USE`).
- **Rows:** RD23–RD26 in `tests/street_segment_ui_tests.cpp` (ctest `road-ui`). Four mutations were caught:
  - no apply on commit;
  - the detached status never given;
  - the canvas never told;
  - a Street segment needing three points.

## 17. S3.4 as implemented (D308)

The owner chose the tolerance (2026-10-22): 15° and half both widths plus 30 m.
- **`joinOppositeLinks(d, forward, backward)`** (`src/commands/street_segment_commands.*`) makes one Street segment with
  no template and `links` = [forward, backward]. Both Links take its `streetSegmentId`; neither Link moves.
  - **Centreline:** `meanCentreline` (`src/model/network/streetSegments.*`) of the two bundle centrelines
    (`linkCentreline`), digitised along the forward Link. Equal vertex counts pair vertex by vertex,
    which is exact for `oppositeLink`'s offsets, bends included. Otherwise each vertex pairs with
    its closest point on the other line, and the midpoints are ordered by station along the
    forward Link.
  - **Refusals:**
    - `EDIT_STREET_SEGMENT_JOIN_TAKEN`: a Link already in a Street segment;
    - `EDIT_STREET_SEGMENT_JOIN_LEVEL`: the Links are on different levels;
    - `EDIT_STREET_SEGMENT_JOIN_DIRECTION`: one Link twice, or the same direction (the end-to-end vectors'
      cosine ≥ 0);
    - `EDIT_STREET_SEGMENT_JOIN_PARALLEL`: more than 15° off opposite, or any centreline vertex farther from
      the other centreline than half of both widths plus 30 m. The constants are
      `joinAngleDegrees` and `joinMedianMetres`.
- **`assignTemplate(d, streetSegments, crossSectionId, installed)`** sets or clears the template on every
  listed Street segment, generating no Link. An unknown template is `ROAD_CROSS_SECTION_UNKNOWN` at
  `crossSectionId`, and an unknown Street segment is `EDIT_UNKNOWN_OBJECT`.
- **Editor (Street segments tab):**
  - *Join opposite Links* joins the two Links selected on the canvas, the first one forward. It is
    enabled only with exactly two selected.
  - *Assign template* puts the combo's template on every selected row, and "(no template)" clears
    it. The table now takes several rows.
  - A Street segment with no template reads *No template* even when it has Links, as a joined Street segment does.
- **Rows:** RD10 and RD11 in `tests/street_segment_tests.cpp` (`streetSegments`), with their editor part in `road-ui`.
  Four mutations were caught:
  - the mean replaced by the forward line;
  - no angle check;
  - no distance check;
  - no unknown-template check.

## 18. S3.5 as implemented (D309)

- **`findCrossings(network, tolerance)`** (`src/model/network/street_segment_crossings.*`, pure). It looks at
  every pair of Street segments on one level that is not an overpass, and records:
  - each proper crossing of their centrelines;
  - each end of one Street segment within the tolerance of the other.

  How places are formed and listed:
  - events within the tolerance of one another are one place, at their mean;
  - each Street segment appears there once, with its station, and is an *end* within the tolerance of either
    end of its centreline;
  - legs are 2 per Street segment passing through and 1 per Street segment ending;
  - a place is listed with two Street segments or more and three legs or more, and only where no Node
    already covers it. So two Street segments meeting end to end are left to S3.6's two-leg merge, and a
    confirmed junction is not asked again;
  - the list is sorted by level, then x, then y.
- **`network.overpasses`** holds sorted pairs of Street segment ids at schema 53. It is written only when
  non-empty; below 53 it is `EDIT_UNSUPPORTED_FIELD`, and a bad shape is `EDIT_STREET_SEGMENT_INVALID`. A pair
  that names no Street segment, or one Street segment twice, is `ROAD_OVERPASS_UNKNOWN` at `overpasses[i]`. `deleteStreetSegment`
  drops the pairs naming its Street segment.
- **`splitStreetSegment(d, road, station)`:**
  - the Street segment keeps the first part of its centreline, and a new Street segment with the same name, template,
    overrides and level takes the rest;
  - each generated Link is split by `splitLink` at that point, which keeps routes, heads,
    decisions and zone connectors;
  - the forward Link's original id stays on the first Street segment; the backward Link's original id,
    which starts at the far end, goes to the new Street segment;
  - overpass pairs naming the Street segment are copied for the new Street segment;
  - a station within 0.2 m of an end is `EDIT_SPLIT_RANGE`. J3a also carries a moved end's junction recipe leg/major ids (D394, JUNCTION_TEMPLATES §4).
- **`applyCrossings(d, places, choices, tolerance, installed)`**, in one Command:
  - **Overpasses first.** Every pair of the place's Street segments is stored; where a Street segment ends there, the
    choice is `EDIT_CROSSING_OVERPASS`.
  - **Then each junction.** The Street segments within the tolerance of the place are found again, because an
    earlier split may have renamed them. Those passing through are split.
  - **A `generated` Node** is added: a 16-gon around the place.
  - **The radius** is the largest of (each generated Link's distance from the place plus half its
    width), plus the widest lane there. A Street segment without Links uses its template's median/2 plus its
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
  - **Street segments tab: *Find crossings*** opens a dialog with the snap tolerance (2 m), *Search again* and
    one row per place (level, x, y, Street segments, legs, and Junction or Overpass). Confirm is one Undo
    step, and the status bar counts both kinds. With nothing to confirm, the status bar says so.
  - **Nodes tab: *Merge nodes*** works on the selected rows (the table now takes several), with the
    current row first.
- **Rows:** RD12–RD15 in `tests/street_segment_crossing_tests.cpp` (`roadcrossings`), with their editor
  part in `road-ui`.

## 19. S3.6a as implemented (D310)

S3.6 is split in two. S3.6a ships `rank` and `coarse` with the Macro and Meso compile that reads
them, so no field is stored before its reader. The editor's coarse dialog, refine, derive back and
the lifecycle are S3.6b (RD17, RD18). RD20's Micro half, `SEAM_STREET_SEGMENT_NOT_GENERATED`, moves to S3.7
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
    `bannedTurns` and `signal {cycle, stages[{green, turns}]}`; turns are `[in Street segment, out Street segment]`;
  - it is written only when present, with fields at their defaults left out, so an all-default
    one is `{}`;
  - below 53 it is `EDIT_UNSUPPORTED_FIELD` at `network.nodes[i].coarse`, and a bad shape is
    `EDIT_NODE_INVALID` at its path.
- **Detail and legs** (`src/model/network/coarse_nodes.*`, pure):
  - a Node has detail when any Link or Connector has a run inside its polygon (`polygonRuns`, moved
    to `polygon.*` from Node membership). **Revised by D311 (§20):** detail is now the interior
    only, and Links reaching in are legs;
  - a leg is a Street segment with a centreline end inside the polygon, named by its id, at the last
    boundary crossing before that end. A Street segment wholly inside gives no leg;
  - a Street segment that passes through without ending is not cut. It is the advisory `NODE_STREET_SEGMENT_THROUGH`,
    and *Find crossings* splits it.
- **Checks** (`coarseNodeIssues`, in the network check):
  - `NODE_COARSE_WITH_DETAIL` at `nodes[i].coarse`;
  - `NODE_COARSE_UNKNOWN_LEG` at `.major[k]`, `.bannedTurns[k]` or `.signal.stages[s].turns[t]`;
  - `NODE_COARSE_SIGNAL_INVALID` at `.signal` for:
    - a signalised Node without a signal, or a signal on another control;
    - no stages;
    - a cycle or green that is not positive;
    - greens summing above the cycle.
- **`compileStreetSegments`**, shared by `compileMacroNetwork` and `compileMesoNetwork`. Both take the
  templates as a new last argument; the inputs pass them only when some Street segment has no Links, so
  nothing else is read or recorded.
  - **Pieces.** A Link-less Street segment gives one piece per direction of its template, trimmed at the
    legs of the Nodes without detail that it ends in.
    - The forward piece runs along the centreline; the backward one has decreasing stations.
    - Lanes are the override, else the template's; the type is the template's `linkType`.
    - A Street segment with Links compiles from its Links, as before.
  - **Joins.**
    - A Node without detail is one, with or without `coarse`: absent fields are the defaults.
      Nodes without detail are no longer `MESO_NODE_REFUSED`.
    - So is a group of free Street segment ends within 2 m of one another (Find crossings' default
      tolerance), as a continuing Link is in Meso. Without this, two templates meeting end to end
      would disconnect.
  - **Turns.**
    - Every (in, out) pair of different Street segments, minus `bannedTurns`, with no free time (D301).
    - At a signalised Node: `signal` `node:<id>` and g/C = the summed greens of the stages that
      serve the turn over the cycle (D299). A turn no stage serves gets 0.
  - **Refusals** at `network.streetSegments[i].crossSectionId`:
    - no template: `MACRO_STREET_SEGMENT_TEMPLATE_MISSING`;
    - an unknown template: `ROAD_CROSS_SECTION_UNKNOWN`;
    - a template with no type: `MACRO_LINK_TYPE_MISSING`;
    - an unknown type: `MACRO_LINK_TYPE_UNKNOWN`.

    A Street segment refused for its template gives no piece.
- **Meso:** one `MesoLink` per piece, with `source` the Street segment id. There is one node per join (with
  the Node id; allWayStop shows as unsignalised) and one per free piece end. Everything is added
  after the Links, so a project without Link-less Street segments compiles exactly as before.
- **Macro:**
  - a `link` arc per piece, between nodes `<road>:forward|backward@<station>`, so a U-turn never
    shares a node. A basis mismatch is at `network.streetSegments[i].crossSectionId`;
  - a `connector` arc per turn (`source`: the Node id), uncapacitated and with time 0;
  - `MacroCompile` gains `advisories`, which `runMacro` reports ahead of demand's.
- **Demand at this slice:** zone connectors attached to Links only. §22 adds direct Street segment targets
  for Macro; Meso follows in [MESO_STREET_SEGMENT_DEMAND](MESO_STREET_SEGMENT_DEMAND.md) (D341).
- **Rows:** RD4, RD16, RD19 and RD20 (Macro and Meso) are in `tests/street_segment_coarse_tests.cpp`
  (`roadcoarse`). The rank field is checked in `macro-ui`.

## 20. S3.6b as implemented (D311)

- **Detail revised (D311, replacing D310's reading).** A Node's detail is its interior: a Connector
  with a run inside the polygon, or a Link wholly inside it (`nodeHasDetail`). A Link that only
  reaches in is a leg (`nodeLinkLegs`: one crossing, ending or starting inside). Under D310's
  reading, deleting a junction's Connectors left the approach Links inside as "detail", so derive
  back could never happen.
  - The Connectors `splitLink` adds between split pieces are detail, so a junction S3.5 makes over
    generated Links is Micro's, with its through turns, until they are deleted.
- **Link legs compile (D311).** At a Node without detail, `compileStreetSegments` takes each Link leg as an
  arm next to the Link-less Street segments' pieces.
  - The arm is named by the Link's `streetSegmentId`, or by the Link's own id when it has no Street segment.
  - The leg's part inside the Node goes into `dropped`. Macro and Meso skip that part and put a
    station at the crossing.
  - Turns, bans and stage g/C apply as for pieces. Meso joins a Node's leg stations into one node.
- **`setNodeCoarse(d, id, value)`** (`src/commands/coarse_commands.*`) stores the values, with an
  all-default value stored as none, and clears `generated`, as any edit of a Node does.
- **`settleNodes(after, before)`** runs inside `History::execute` on every Command, between the change
  and the check. It touches only Nodes present before and after, and is skipped when no Link,
  Connector, Node or Street segment changed.
  - **Refine:** a coarse Node that had no detail and now has some loses `coarse` (RD17).
    A junction recipe may coexist with coarse values until then; template choice/Detach keep
    coarse and the Macro/Meso compile unchanged (D393 review). J3b's fill retains the recipe and
    relies on this existing transition after drawing detail; it does not clear coarse on choice.
  - **Derive back:** a Node that had detail and now has none, with no `coarse` or junction recipe, takes
    `deriveCoarse` of its detail as it was, when every Street segment it names is still a leg (RD18).
  - **Lifecycle:** applies to a Node whose Street segment legs became exactly two in this Command, with no
    `coarse`, no detail and no Link leg, and whose two Street segments:
    - are Link-less;
    - share one template, one level and the same overrides;
    - meet with one ending where the other starts.

    Such a Node is deleted and the Street segments joined. The first Street segment keeps its id and takes the second's
    centreline after its own. The second's id is renamed to the first in overpasses and in other
    Nodes' coarse values (RD27). A Node drawn with two legs is kept, as is one with detail or
    `coarse` or junction recipe.
- **`deriveCoarse(d, node)`** reads `nodeMembership`. Legs are named by the Street segment of each leg Link,
  and nothing is derived if a leg Link has no Street segment ending in the Node.
  - **Control:** signalised from the heads; otherwise priority, which is absent.
  - **Major:** for priority, the in-leg Street segments that never yield in a conflict, when others do. **Amended by D324:** an arm is minor when each of its turns that meets a decided conflict yields in one; an arm with a turn that meets some and never yields is major, so a major far turn yielding to the opposite through leaves its arm major.
  - **Banned turns:** every pair of different Street segments with no turn.
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
  - RD17, RD18 and RD27 are in `tests/street_segment_coarse_command_tests.cpp` (`roadcoarse`).
  - RD19 gains a junction over generated Links, compiled once its Connectors are deleted.
  - RD28, the dialog, is in `road-ui`.
- **Mutations caught:** refine off, banned turns not derived, the wrapped stage not merged, the
  lifecycle on any two-leg Node, and D310's detail.

**J3b (D401):** Street segment *Apply template* on junction-generated Links or template-bound
junction detail refuses `JUNCTION_STREET_SEGMENT_DETAIL`. Undo the junction fill (or reopen
an earlier copy), apply the cross-section, then fill again. Re-apply cannot silently change
base geometry after a cross-section edit. Legacy F3/F4 documents remain identical to the
pre-J3b implementations (JUNCTION_TEMPLATES JT13).

## 21. S3.7 as implemented (D312)

M14 J2 (D391) adds command-only kerb/far pockets with a separate taper and a derived
`approachLayout` ([JUNCTION_TEMPLATES §6](JUNCTION_TEMPLATES.md#6-the-lane-layout-derived-and-never-stored-d272)).
It changes no Street segment/coarse-Node storage or lifecycle rule above. The junction recipe
and its fill are J3a/J3b; the *Find crossings* template column and editor are J6.

S3 is complete with this slice.

- **`seamChecks(network, types, templates, level, speedJump)`** (`src/model/demand/seam_checks.*`,
  pure). The caller resolves link types and templates. It returns rows `{code, path, severity}`
  sorted by code and path; none refuses a run or changes the document.

  | Code | Path | Micro | Meso, Macro |
  |---|---|---|---|
  | `SEAM_STREET_SEGMENT_NOT_GENERATED`: a Street segment without Links | `network.streetSegments[i]` | Advisory | No row (compiled, §9) |
  | `SEAM_LANE_COUNT`: a Link with `streetSegmentId` whose lane count differs from its Street segment's for that direction (override, else template; forward is the Street segment's first Link) | `network.links[i]` | Warning, with the two fixes | Advisory |
  | `SEAM_SPEED_JUMP`: types' free speeds differ by more than the threshold (20 km/h by default) | `network.connectors[i]` | Warning | Warning |
  | `SEAM_LINK_DETACHED`: a Street segment with Links but fewer than its template's directions | `network.streetSegments[i]` | Advisory | Advisory |

- **Readings taken:**
  - **"The Node's leg"** is read as the Street segment the Link belongs to. A coarse Node's leg is a Street segment,
    and a detailed Node's leg is the Link itself, so the seam is a generated Link against its
    Street segment's lanes. It is checked for every generated Link, at a Node or not.
  - **"Consecutive links"** are those joined by a Connector outside every Node. A turn inside a
    junction is not a corridor, so an arterial turning into a local street is not flagged. Two
    Link-less Street segments meeting end to start outside a Node are also consecutive; the row is at the
    later Street segment.
  - **The detached Link** no longer names its Street segment (D305), so the row is on the Street segment.
  - **The threshold** is a display choice, kept in the dialog for the session and not stored.
- **Fixes** (`src/commands/street_segment_commands.*`), each one Undo step that changes only the side chosen:
  - **`useLinkLanes`** (*Use Link*): the Street segment's override for the Link's direction becomes its lane
    count;
  - **`useStreetSegmentLanes`** (*Use Node*): *Apply template* again, so the Links take the Street segment's lanes
    and keep their ids;
  - a Link without a Street segment is `EDIT_SEAM_NOT_GENERATED`.
- **Editor:** *Seam checks…* on the Street segments tab opens a dialog with the level, the threshold, and one
  row per issue (severity, problem, object). A lane-count row at Micro has *Use Link* and *Use
  Node*, and the list is checked again after each fix.
- **Not built in S3:** the readiness panel and *Fill defaults* (S4) run these checks, as STREET_SEGMENTS §7
  says. Street segment-only Macro demand follows in §22.
- **Rows:**
  - RD20 (Micro half), RD21, RD22 and RD29 are in `tests/street_segment_seam_tests.cpp` (`roadseams`);
  - RD21's editor part is in `road-ui`.
- **Mutations caught:**
  - the not-generated row at every level;
  - *Use Link* on the wrong direction;
  - turns inside a junction counted;
  - the threshold made inclusive;
  - the detached row off.

## 22. Street segment-only Macro demand (D340)

The booked seam in D310: a Macro study can attach its existing OD layer directly to Street segments
without generating Micro Links. This first slice covers Macro; Meso follows in
[MESO_STREET_SEGMENT_DEMAND](MESO_STREET_SEGMENT_DEMAND.md) (D341). No model parameter or
milestone gate changes.

### Interface and storage

- A ZoneConnector targets exactly one of:
  - the existing `linkId`, with its unchanged schema-31 JSON;
  - `streetSegmentId` and required `streetSegmentDirection`, `"forward"` or `"backward"`, at schema **57**.
- `direction` still means `"origin"` or `"destination"` relative to the zone. Travel direction is
  along or against the Street segment's centreline. Origin attaches to that direction's compiled start;
  destination to its compiled end (the boundary where a coarse Node trims it, when applicable).
- The target Street segment must have no generated Links. On load/commit, an unknown target is
  `ZONE_CONNECTOR_UNKNOWN_REF` at `.streetSegmentId`; a Street segment already carrying Links gives
  `ZONE_CONNECTOR_STREET_SEGMENT_HAS_LINKS`. Bad shape, extra keys or a direction other than the two above
  gives `EDIT_ZONE_INVALID`, including JSON with both/neither targets. A command supplying
  both/neither targets gives `ZONE_CONNECTOR_TARGET_INVALID`. Either new key below 57 gives
  `EDIT_UNSUPPORTED_FIELD` at that key.
- Empty Street segment targets are not stored; a file without one keeps its feature schema and bytes.
  No inferred direction, mid-Street segment station or extra demand table is introduced.

### Readers and editing

- `compileMacroNetwork` connects the centroid arc to the selected Street segment piece. An unavailable
  direction (including a backward connector on a one-way template, or no surviving piece) gives
  `MACRO_ZONE_STREET_SEGMENT_UNAVAILABLE` at `zoneConnectors[i].streetSegmentDirection`. Existing graph/template
  refusals still apply. Demand shares, modes, PCU, periods, assignment, named runs and exports
  use the existing Macro pipeline unchanged. Mixed Link/Street segment connector pairs can run.
- At D340, Micro and Meso refused Street segment targets with `ZONE_CONNECTOR_STREET_SEGMENT_REQUIRES_LINKS` at the connector;
  this prevents a successful empty run or a partial OD expansion. A Micro study-area cut still
  keeps only its Links and takes outside demand from the selected cut source (S5).
  D341 adds Meso's graph OD reader; Micro retains this refusal until conversion.
- The Zones tab keeps its selected-Link actions and adds *Street segment connector…*. Its dialog selects
  a Link-less Street segment, zone direction, travel direction and share; Cancel changes nothing. Editing a Street segment connector can change its direction
  and share. The table names the Street segment and direction, and the canvas line follows the chosen end.
- *Apply template* remaps all of that Street segment's connectors to the corresponding generated Link in
  the same Command, retaining connector ids, zone ids, direction and shares. A one-way template
  cannot accept a backward connector: the Command refuses before mutation with
  `ZONE_CONNECTOR_STREET_SEGMENT_REQUIRES_DIRECTION`. Re-apply and Link edits then follow the existing rules.
- `splitStreetSegment` retains the endpoint: forward origins/backward destinations stay on the first
  piece; forward destinations/backward origins move to the second. Delete cascades connectors
  as Link deletion does, keeping zones and matrix counts. The two-leg lifecycle remaps external
  endpoints to the joined Street segment, but keeps the Node and both Street segments if a connector uses their
  shared end (merging would turn that endpoint into an unsupported internal station).

### Acceptance rows

| Row | Case | Asserts |
|---|---|---|
| RD30 | Codec | Both travel directions round-trip at 57; either key below 57 refuses at its path; malformed/ambiguous targets fail; existing Link JSON and shipped project bytes stay unchanged |
| RD31 | Assignment | A Street segment-only corridor with OD has exact forward/backward flows and PCU, stable replay, shared connector weights and period isolation; one-way backward and banned turns refuse; a mixed Link/Street segment path runs |
| RD32 | Lifecycle | Apply remaps all four origin/destination and travel-direction combinations in one Undo/Redo; one-way refusal is atomic; split retains ends; delete preserves matrix counts; lifecycle merges only when it preserves connector endpoints |
| RD33 | Editor | Add/Cancel, direction/share edit, Undo/Redo, endpoint drawing, generated-Street segment refusal and save/reopen, in English/Thai; existing Link actions/dialogs remain usable |
| RD34 | CLI and refusals | Real CLI Macro JSON/CSV and named execution keep original source hashes and input bytes; Micro readiness/execution refuse Street segment targets; D341 adds Meso success/refusal coverage (MR6) |

Verification evidence and platform are recorded in PROGRESS; local Linux checks cannot close an
owner Windows/high-DPI appearance gate.

## 23. Review fixes (D382)

A review of the Street segment model (2026-10-28) found four defects, each reproduced by a row before its fix:
- **Levels in *Find crossings*:** places were grouped by distance only, so a crossing above another
  joined it. Events now group only on one level (RD30).
- **Split and coarse values:** `splitStreetSegment` left a coarse Node at the moved end naming the first piece,
  refusing the Command (and *Find crossings*' batch) with `NODE_COARSE_UNKNOWN_LEG`. Those Nodes now
  name the new Street segment; the near end's keep the id (`renameCoarseStreetSegment`, shared with the lifecycle's join; RD31).
- **A Node narrower than its arms:** a junction found before a template gave the Street segments their width
  gets a 4 m disc that the Links pass beside; F4 listed it, then failed on a gap. It is now
  `FILL_NODE_ARM_OUTSIDE` ([READINESS](READINESS.md) RS19). The *Find crossings* dialog warns about it (§24).
- **Direction of a generated Link:** read from the stations of its ends along the centreline
  (`forwardOnStreetSegment`), not from its place in `links`, which a detach shifted (RD32, RD33). Two listed
  Links running one way are `EDIT_STREET_SEGMENT_DIRECTIONS`. No schema change.

## 24. Street segment workflow (D385)

Two Street segments-tab steps from the same review, each with a `road-ui` row that failed first:
- ***Apply template* on several Street segments:** the action takes every selected row, in table order, in one
  `execute`, so the batch is one Undo step. One refusal (`EDIT_STREET_SEGMENT_NO_TEMPLATE`,
  `ROAD_CROSS_SECTION_UNKNOWN`, `EDIT_STREET_SEGMENT_DIRECTIONS`, ...) leaves the whole Command unapplied and is
  shown as any refused edit. It is enabled with one row or more, like *Assign template*; *Edit road*
  and *Delete road* still act on the current row. 200 Street segments apply in about 150 ms (Debug desktop
  build, Linux, table refresh included), so no further speed work was needed after D384 (RD34).
- ***Find crossings* warns:** a place whose Street segments include one with neither a template nor Links is
  drawn in the warning colour, with a tooltip naming those Street segments, and one note under the table says
  to assign templates first, since its Node is sized without that Street segment's width (2 × tolerance in
  radius when no Street segment there has one; F4 then refuses it with `FILL_NODE_ARM_OUTSIDE`). The note does
  not block Confirm: §5 sets no order (RD35). Strings `editorCrossingNoTemplate` and
  `editorCrossingNoTemplateStreetSegment`.
