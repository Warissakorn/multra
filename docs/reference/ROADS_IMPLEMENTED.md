# Roads — as implemented (S3.2–S3.5)

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
