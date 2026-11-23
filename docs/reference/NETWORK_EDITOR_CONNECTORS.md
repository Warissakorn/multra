# Native network editor — Connectors

The Connector sections of [`NETWORK_EDITOR.md`](NETWORK_EDITOR.md), split out on 2026-09-29 (end handles and
attachment stations on 2026-11-23) so both files stay under the 500-line limit. Current, not archived; the same scope and the same caveats apply.

## Connector lane ranges

Select **Connectors (C)**, then Ctrl+right-drag from a source link lane to a target link
lane. The dialog chooses the first lane and contiguous lane count at each end. Left-click
during creation can add intermediate points. One completed gesture creates one connector
object even when it carries several lanes.

Both ends can attach anywhere on the **body** of their Link; endpoints remain valid.
The two-click workflow (C, a Ctrl+right-click on the source, then a plain click or Ctrl+right-click on the target; D84, D306) also picks positions on lane bodies. Picking **snaps to a point**,
which is Vissim's *Snap to Points*: the lane's two endpoints, then a station another Connector
already attaches at on that lane, then the Link's own intermediate points — the first of those
within the radius wins, and the station taken is that point's exactly. Two movements leaving one
corner are therefore authored at one cross-section rather than centimetres apart, which is under
`kMinSectionLength` and would be refused at Run, and a Connector meeting a Link at a bend lands on
the bend. The radius is a screen distance, so zooming in past roughly 20 pixels per metre narrows
it below that limit and fine placement wins, which is what an author zoomed that far in is asking
for. The endpoint snap is not cosmetic: a Connector at a Link end is stored with **no** station at
all and compiles to a departure, while one a few centimetres short is a body attachment whose stub
section is under `kMinSectionLength` and cannot Run. Hidden levels cannot be picked. Properties → Connectors exposes
actual link/lane IDs and `from.station` / `to.station` in metres from the Link's start, as
Vissim stores a position. One station names one cross-section, so every lane of a range meets
the Link square even on a curve, and the number means the same thing whichever lane is picked.
The dialog opens at **one lane per end** — a gesture that starts on a single lane authors a
single-lane Connector, and a wider range is raised deliberately, within the lanes each end has.
Changing lanes preserves the selected stations, and so does adding or removing lanes: a
station is measured on the reference polyline, which `laneOffset` keeps fixed. Releasing outside a target Link reports
why nothing was created. Esc and Cancel leave the document and history unchanged.

A connector stores a base polyline and source/target lane counts. Its lane paths are
derived in monotone order, with stable IDs: the first uses the connector ID and subsequent paths use `id/lane-2`, `id/lane-3`, etc. Routes and signal heads can reference those paths.
Runtime paths follow the centre between adjacent painted lane boundaries (D114). Terminal
points join the named Link lane centres. Intermediate-point edits and lane widths change
these paths, their metre lengths and control positions; the default Bézier only generates
the initial drawing. Zero points and Reset straight produce straight driving legs.
Stored control stations map from the authored geometry to the same lane path used by vehicles.
Unequal counts pair lane for lane over the narrower end, with at most one lane added or dropped on each side, so the counts differ by at most 2 (M3.2.9a, D73). For a one-lane difference
`laneChangeSide` (schema 17, `"left"`/`"right"`, the driver's view; the Inspector's "Lane change side", M3.2.9c) picks the side, absent = kerb side; that lane is the one drawn tapering. Lane tabs stop at a two-lane difference. A Connector asked for across a larger difference (a 2-lane Link dragged onto a 5-lane one) is created narrowed to it: the wider end gets `narrower + 2` lanes, centred on the lane the drag ended on (M3.2.9d, D75). Ranges are limited by the existing lanes, at most 12 per end.

In Select (S), rectangular **lane tabs** exist even on a one-lane Connector. Each sits directly
against the carriageway edge, without a stem or a numeric label:

- Source handles on both sides: grow/shrink the contiguous source lane range.
- Target handles on both sides: grow/shrink the contiguous target lane range independently.
- Middle handles on both sides: set both ends to the same count, limited by available lanes.
- Each selected Link has tabs at the start, middle and end on **both sides** to add/remove lanes (up to 12). Existing
  widths and world positions are retained. Added lanes use the width of the dragged edge lane.

Drag outward to add lanes and inward to remove them; the road geometry previews the result
during the drag. One release is one undo entry. Esc cancels. Each tab changes
its own edge, leaving the opposite edge fixed. The first-side handles add/remove lanes
before the current first lane; the other handles change the last lane. Surviving lane
IDs and positions stay fixed, including on curved Links. Connector lane pairs retain their references; their runtime lane centres are re-derived from the
new painted rails and Link attachments (D114). Unequal ranges can intentionally change lane mappings.
Properties count edits and downstream pocket creation expand the last-lane side.

## Geometry and end handles

Geometry handles sit on the **centreline of the whole bundle**, as Vissim shows them, not on
the stored reference polyline — which ends up at one edge as soon as lanes are added to a
single side — and not on a Connector's first lane path. Dragging one moves the stored point
by the same offset, so what the pointer holds is what moves.

A Connector's two end handles are dark squares at the middle of its end cross-section, and
they are draggable: drop one on any lane to re-attach that end, anywhere along the lane. The
range is centred on the lane under the pointer and slides to stay inside the Link. If the new
end has fewer lanes than the Connector carries, the range is narrowed to what is there rather
than the move being rejected; a wider Link never widens the range on its own. Dropping outside
a Link leaves the attachment alone, Esc cancels, and one release is one undo entry. A
Connector used by a route or head still cannot be re-attached. Properties shows both lane
counts beside the length, so a 3 → 2 lane drop is visible without opening the inspector.
Connector path count is the larger of its source and target counts, not a third independent
lane topology. Its cross-section is built from the widths of the lanes it carries, not blended
between its two mouths: a lane that continues keeps the width its links give it from end to end,
and a lane the far end has no room for is drawn as a taper closing onto its neighbour, the way
Vissim draws a lane drop. Which lane continues follows the selected ranges, pairing them in lane
order, so a lane range anchored one lane over moves the taper to the other side. The taper runs
the whole length of the Connector; there is no separate taper length to set. Those widths are stacked
along the same mitered offset a Link's own lane edges use (`offsetGeometry`), so every lane is its
full width square to the road at every point, through a bend and past a poly point the author has
dragged. Ordinary mouths fit the Link cross-section; steep or reversed arrivals use
the same P1–P4 mouth and may report `WARN_CONNECTOR_ALIGNMENT` (D80, see below). Properties exposes both counts as an alternative. Retargeting
or resizing a connector used by a route or head is rejected; revise those references
first. Reshaping its curve remains allowed if the whole document validates.

## Connector shape: intermediate points and the mouth

The Connector drawing stores its two attachments and a few
**intermediate points**, joined by **straight legs and mitered at each point**, exactly as a Link
is. It is not smoothed — a Connector with two intermediate points is three straight legs with a
corner at each one. The count decides how closely the drawing polygon follows the turn,
in Properties → Connectors. Runtime motion follows the resulting lane-centre legs too;
changing the shape or point count can change travel lengths and times.

Changing the count never re-derives the default curve; `Reset curve` is the button for that.
Raising it splits the longest leg each time, so every point already there survives and the drawn
line does not move at all. Lowering it spaces the points evenly along the shape that is there,
giving up only the corners the lower count cannot hold. A new Connector gets 3; 0 leaves one
straight leg between the attachments. `Reset curve` lays 3 along the arc, and laying more along
it follows the turn more closely — 2.29 m of sag from the arc at one point, 0.60 m at three,
under 0.10 m at fifteen.

D80 mouths use the centred axis and fixed lane-index P1–P4 intersections at every angle,
without slide/square fallbacks or an angle cutoff. Singular or folded surfaces report
`WARN_CONNECTOR_ALIGNMENT`; they are not evidence of a drivable turn. See
[the mouth contract](CONNECTOR_FOUR_POINT_MOUTH.md).

An endpoint grip sits on the middle of the Link lanes that end joins (the mouth's P2, D77), where
a drop is measured too. Dragging it along the lanes it joins keeps the curve, every point shifted
by its blend weight (D78); onto other lanes, or where a kept end leg would run against its lane
(the old wrong-way elbow), it rebuilds the turn at its point count. One undoable edit either way. Moving a Link still follows the separate world-position/deletion contract in
§"Attachment stations, Link edits and deletion" below. Merely opening a file does not regenerate any authored curve.

Interior points are editable; dragging one moves that corner, its derived lane centres,
vehicle paths and control positions together. The Inspector tooltip describes this behaviour. Reset curve to
lane directions lays the points along a cubic. Each control point reaches `(2/3)·chord·tan(α/2)/sin(α)`, where α is the angle
between **that end's** lane direction and the chord — the cubic that stands in for a circular
arc leaving at that angle. It is `chord/3` as α tends to zero, the constant every turn used to
get, `(2/3)·chord` for a U-turn, which used to be drawn at less than half the radius it needs
(0.18 of the chord instead of 0.45), and it is the only reading that catches a reverse curve,
whose two ends are parallel while each still leaves its chord steeply (0.18 of the chord to
0.22 on a measured S). That reach is held at its 120-degree value, `(4/3)·chord`: past there it
runs away — 11.05 times the chord at 160 degrees — and a Connector drawn where two links nearly
touch left the junction altogether, measured at 11.0 times its own chord and now 1.9. Every
ordinary turn, U-turn included, is unchanged to the last bit. A Connector that still turns tighter than its own width is reported in Objects and
issues as `TIGHT_CONNECTOR_RADIUS`; the drawing is kept and Run is not blocked.
Make straight retains only endpoints. There are no separate persisted
Bézier handles and arbitrary edits need not remain smooth, though a reshaped curve is
held to the same geometry rules as a link: no non-finite coordinates, no repeated
consecutive points and a positive total length. Coincident endpoints cannot
generate a default curve; leave a positive gap. Duplicate lane-pair connections at the same source/target stations are
rejected, including pairs already covered by another connector range. Separate stations
on the same lane pair may own separate Connectors.

## Attachment stations, Link edits and deletion

**A Connector keeps its own position.** Its geometry is what the author drew; it is not recomputed
from its Links on every edit. Moving or reshaping a Link therefore leaves the Connector standing
where it is, and what changes is which station of the Link each end now sits on:

- **The end is still on the Link's carriageway** — it snaps onto the middle of the lane under it,
  and its stored station moves to match. The lane it names may change where a lane bundle edit has
  slid the lanes sideways under a standing end; the Link never does.
- **The end is off the Link** — by more than half a lane width, which is where the carriageway
  stops — the Connector has nothing left to connect. It is **deleted**, with the routes and signal
  heads that named it, inside the same transaction as the edit that moved it, so one Undo brings
  the Link edit and the Connector back together.

That is the rule for an edit that MOVES a road: reshaping a Link, translating a selection, dragging
a Connector's body. An edit that RE-LAYS a Link's lanes without moving the road — a lane added or
removed, a width changed, the driving side flipped — is not a move: every Connector follows the lane
it names to where that lane now is, at the same station. Deleting a Connector because the author
added a lane to the Link beside it would be a surprise, not a rule.

Shortening a Link past an attachment now deletes the Connector that hung off it, rather than
clamping the attachment to the new end. Clamping moved a Connector to somewhere the author had not
put it; this leaves it where they did and says so by removing it. (A Signal head still rides its
station and is not moved: its position is validated against its lane, so an edit that would strand
one is rejected instead.) Reset curve re-derives the whole shape when that is what is wanted.

Splitting remaps both source and target attachments to the appropriate child Link by arithmetic
alone — the upstream child's polyline is a prefix, so its stations are unchanged, and downstream
stations shift by the cut. A cut through an attachment inside the 0.2 m continuity span is
rejected; move the split at least 0.1 m away.

**Runtime:** body attachments run (M1.11.1). The lane is cut into sections at every station a
Connector attaches to, so a vehicle leaving part way along travels only that far, and one arriving
part way along joins at the drawn metre rather than at the start of the lane. An arrival is a
merge, and the arriving Connector gives way to the traffic already on the lane under a priority
rule derived from the drawing, whose gap time and headway come from `data/priority-rules/`;
Run reports `EDIT_NO_PRIORITY_DEFAULTS` if those cannot be read, and never invents a zero gap
time. Run and Diagnostics still report `UNSUPPORTED_CONNECTOR_POSITION` for one case: an
attachment within 0.2 m of a lane end or of another attachment on the same lane, which leaves no
section between them. A lane with nothing attached to its body compiles exactly as it always did,
so existing endpoint-only projects keep their runtime behavior and capability guards.

Deleting a connector removes heads on its paths, affected routes and their vehicle
inputs in the same undoable transaction. Heads on unaffected links remain.

## Code seams

What the sections above describe rests on shared functions, each deciding one thing once:

- `connector_commands.hpp` defines creation, geometry, retargeting, reset and deletion. An explicit
  endpoint edit and the canvas preview share model `retargetConnector`: both rebuild the directed
  turn at the current intermediate-point count and narrow ranges to the lanes available. Link
  geometry movement uses `reanchorConnectors`; lane-bundle and driving-side edits use named-lane
  anchoring. These are deliberately different operations. Deletion removes affected routes and
  their inputs through the shared command-side reference cleanup.
- `connectorCurve` samples the existing cubic only for the persisted drawing and its editable
  interior handles; `connectorPaths` derives the lane paths and their IDs from the resulting
  surface, and compilation, heads, validation and reference cleanup share those IDs. No derived
  path or second curve is persisted as an editable object.
- `Link::laneOffset` positions the lane bundle independently of its reference polyline.
  `replaceLaneBundle` anchors the edge opposite the edit; lane geometry and road boundaries share
  the offset, so resizing a curved road does not move surviving lanes. Leading Connector range
  edits freeze `laneBlend` weights and rebase the first path; surviving lane pairs keep their
  curves, and geometry edits clear frozen weights. Schemas 1–3 load with zero offset and legacy
  arclength weights.
- `LaneReference::station` is metres along the reference polyline; `matchedStation` maps that one
  station onto any lane or boundary derived from the same reference. `laneAttachment` is shared by
  curve construction, derived paths, validation and reanchoring. Side-resize gestures submit one
  Link/range command on release; preview data never enters History.
- `runtimeSections` derives the runtime lane sections an interior attachment needs (M1.11.1) and is
  the single source `buildScenario`, head rebasing and the canvas's vehicle layer read; a lane with
  nothing attached yields one section carrying the lane's own id. `derivedPriorityRules` arbitrates
  the merge an arrival creates (M3.1), and `connectorRuntimeIssues` blocks only the too-close
  attachment named under *Runtime* above.
- `connectorLaneWidths` is the single place a Connector's width is decided (M1.12.1), read by
  `connectorBoundaries` for drawing and by `connectorShapeIssues` for `TIGHT_CONNECTOR_RADIUS`.
