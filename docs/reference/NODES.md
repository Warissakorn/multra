# Nodes: a polygon over a junction (S0, M5.1)

Written 2026-10-19 (D259). The owner accepted every §7 recommendation the same day (D260); slices
S0.1, S0.2, S0.3 and S0.5 are implemented (D261, D263–D265, §8); D262 amended §3. Stage S0
of the multi-level workflow ([MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) §3, §10,
D175) merged with M5.1's evaluation nodes (ROADMAP M5.1). A Node is first a Micro evaluation
object, the per-junction delay and queue a traffic impact study reports
([VISSIM_PARITY](../audits/VISSIM_PARITY.md) §2b "Nodes", absent today). It is later the junction
that Macro and Meso compile (S2, M10, M11). Rule 4 applies: every number a Node produces is
**simulated, not HCM control delay, and not validated (M6)**.

## 1. Scope

- **In S0:** the object and its file form; how a Node's legs, turns, conflicts and control are
  derived from the Micro detail inside it; Micro node evaluation (per turn, per leg, per Node);
  the editor tool; the lifecycle rules.
- **Specified here, implemented later:** coarse values for a Node without Micro detail, and the
  refine and derive-back Commands. They have no consumer until Macro compiles (S2, M10.1), so
  they land with S3 (§5, slice S0.4; moved from S2 by D272).
- **Out:** Roads and cross-section templates (S3); Nodes created from Road crossings, and the
  derived-disc form of a Node with no polygon (S3); Readiness and *Fill defaults* (S4); the
  run-time cut (S5); any Macro or Meso value.

## 2. The object (schema 48)

```json
"network": { "nodes": [
  { "id": "node-12", "name": "Main St / 5th Av",
    "polygon": [ {"x": 10, "y": 4}, {"x": 60, "y": 4}, {"x": 60, "y": 52}, {"x": 10, "y": 50} ],
    "generated": true } ] }
```

- **Where it lives.** `Network::nodes`, after `travelTimeSections` (`src/model/network/network.hpp`;
  new fields go last). It is network geometry, unlike zones, which sit in the demand layer.
- **Fields:**
  - `id` comes from `allocateId(d, "node")` and is added to `documentIds`.
  - `name` is free text and never a key; empty shows the id.
  - `polygon` has at least 3 points, in project coordinates, and is simple. The rule is the zone
    polygon's (`simplePolygon`, `src/project/zone_codec.cpp`), moved to a shared helper.
  - `generated` is written only when true. It is cleared by the first edit of that Node
    (WORKFLOW §1.3).
- **File form.** The key is written only when a Node exists, so other files keep their bytes and
  schema. A file of schema 48 carries it. An older file that does is refused with
  `EDIT_UNSUPPORTED_FIELD network.nodes`.
- **Refused on load or edit:** `INVALID_ID` and `DUPLICATE_ID`, `NODE_POLYGON_INVALID` (fewer than
  3 points, a non-finite coordinate, or a self-crossing outline), and any other key.
- **Nodes may not overlap** (`NODE_OVERLAP`). Each piece of road then belongs to at most one Node.
- **Edits elsewhere never move a Node.** A Node stores no references, so deleting, splitting or
  reversing a Link needs no cascade. Membership is derived again on every read (§3).

## 3. What a Node holds: derived from the Micro detail

Derived, never stored (WORKFLOW §1.3). This uses the Links' and Connectors' centrelines
(`linkCentreline`, `connectorCentreline`) and one shared point-in-polygon and segment-crossing
helper, extracted from `conflict_groups.cpp` and `zone_codec.cpp`. The boundary counts as inside.

- **Inside objects.** A Link or Connector whose centreline lies wholly inside the polygon is
  *internal*. A conflict area, waiting line or signal head is inside when its point (or both
  sides' entry points, for a conflict area) is inside.
- **Crossings** are taken on each Link's reference polyline (`link.geometry`), the line conflict,
  waiting-line and head stations are measured on, so a leg's station and a conflict side's station
  are in one coordinate (D262).
- **Legs are boundary crossings, not Links** (D262). A Link's entry crossing (outside → inside) is
  an **in-leg** and its exit crossing an **out-leg**. A Link that enters once and later leaves once
  gives one of each: the shipped T-junction's major roads are single Links running through the
  junction. A leg is named after its Link (Name, else id).
- **Pass-through.** A crossing Link counts only when something inside connects to it: an internal
  Connector ends on it inside its span, or a conflict area inside has a side on it. Otherwise it is
  ignored, such as an overpass or a road on another `level`. Roads on different levels therefore
  never form a turn.
- **Refusals** are reported as advisory diagnostics; the Node then gives no rows, and the run still
  runs:
  - `NODE_LINK_REENTERS`: a connected Link crosses the boundary in any other pattern than one entry
    followed by one exit (a second entry, or an exit followed by a re-entry). This replaces D259's
    `NODE_LINK_CROSSES_TWICE`, which refused the shipped T-junction;
  - `NODE_CONNECTOR_CROSSES`: a Connector is partly inside;
  - `NODE_NO_TURN`: no turn is found.
- **Turns.** A turn is an (in-leg, out-leg) pair joined by a lane chain through internal objects.
  The chain is found with the same walk as `routeLaneChains` (`src/model/network/routing.cpp`)
  over the internal Links and Connectors. Several lanes of one pair make one turn. A turn is
  named "in-leg → out-leg". A U-turn that is drawn is a turn, and so is a Link's own run from its
  entry to its exit (a through movement).
- **Turn conflicts.** A conflict area whose two sides lie on paths of two different turns of this
  Node gives a row (turn A, turn B, kind, priority) of the Node's conflict table.
- **Control.** The Node's control type comes from what is inside it:
  - `signalised` when a signal head is inside or on an in-leg within the delay window (§4);
  - `unsignalised` when a priority rule, Stop control or prioritised conflict area is inside;
  - none otherwise.

  A turn's g/C (green time over cycle time) is read from its heads' program when it has heads.
  That is a reported value, not a stored one.

## 4. Micro node evaluation (slice S0.3)

- **One section per turn.** Each turn becomes an automatic travel-time section
  ([TRAVEL_TIME_SECTIONS](TRAVEL_TIME_SECTIONS.md) §2, reusing `SectionSpec` and its
  accumulator, `src/eval/sections.hpp`):
  - its **start** line is on the in-leg, where the delay window begins (owner choice 1, §7);
  - its **end** line is on the out-leg, at the station where it crosses the boundary.

  A vehicle counts for a turn when it crosses that turn's start line and then its end line. The
  turn's delay is section delay: travel time minus free-flow time over the same path.
- **Rows:**
  - **turn**: vehicles, mean delay, unfinished;
  - **leg**: the turns from that in-leg, weighted by vehicles (Σ vᵢdᵢ / Σ vᵢ), plus queue
    (owner choice 2);
  - **Node**: every turn weighted the same way.

  A group with no vehicle has no delay. Movements, authored sections and queue counters are
  unchanged, and no Node row replaces them.
- **Letters** (owner choice 3). A Node with a control type gives each turn, leg and the Node an LOS
  letter, through `losLetter` with that type and the same label as [LOS](LOS.md) §4. Batches letter
  the mean over seeds and never an interval bound (LOS §3).
- **Output.**
  - JSON gains `nodes[]`: `{name, controlType, vehicles, delay, los, legs[], turns[], conflicts[],
    issues[]}`.
  - The CSV gains, after the LOS block, a marker line
    `# Nodes from simulated section delay, not HCM control delay; not validated (M6)` and a block
    `level,node,name,controlType,vehicles,delay_s,unfinished,queueMean_m,queueMax_m,los`.
  - Batch JSON uses `{n, mean, sd, halfWidth95}` values (BATCH §4).
  - All of it is written only when a Node exists, so every other output keeps its bytes.

## 5. Coarse values and lifecycle (specified now; slice S0.4 lands with S3)

The owner moved S0.4 to S3 (D272, [MACRO_NETWORK](MACRO_NETWORK.md) §10): Macro compiles a Node's
interior from its detail, and Nodes without detail first arise from Roads. This amends D260's 5a.


- **Coarse values** are stored only on a Node with no Micro detail (no internal Link or Connector):
  - turns that differ from the default (every in→out pair except U-turns is allowed);
  - control;
  - major and minor legs;
  - a stage signal (cycle and stages).

  The defaults are absent fields. Priority follows the `linkType` hierarchy (S3), and a signal needs
  user-entered values. Their fields and schema are fixed by S2's contract, against M10.1's first
  consumer, so no field is stored before something reads it.
- **Refine and derive back** each happen in one Command (WORKFLOW §3):
  - refining deletes the coarse values as the detail is drawn;
  - deleting the detail first derives coarse values from it.
- **Lifecycle:**
  - a dead-end Node stays;
  - a two-leg Node with only default values is merged away by the Command that made it so (from S3,
    when Roads exist);
  - a Node with detail or authored values is never removed automatically, only reported.

## 6. Editor (slices S0.1, S0.5)

- **Node tool** (`N`, as `docs/specs/network-editor/part-02.md` names it). It draws the way the zone
  tool does (`src/editor/canvas_zones.cpp`):
  - Ctrl+right-click adds a vertex, Backspace removes the last one, Esc cancels;
  - Enter commits once there are at least 3 vertices;
  - the commit is `execute("editorNodeTool", …)`, one Undo step.
- **Nodes panel.** A table of Nodes with name, control type, legs, turns and issues. Rename and
  delete are each one Undo step. Selecting a row highlights the polygon and its legs.
- **Moving a vertex** is one Undo step and clears `generated`.
- **Suggest nodes** (S0.5, D265) draws one `generated` polygon around each junction no Node covers
  yet, as one Undo step for the whole set. D259 named `conflictGroups`, but a `ConflictGroup` is one
  pair of road objects, not a junction. The junction rule, all of it taken from the drawing:
  - **Conflicts:** every authored conflict area and every automatic conflict, except the
    display-only branching kind.
  - **Clusters:** conflicts join through the Connectors on their sides. Connectors join when they
    leave, or arrive at, the same station of one Link. Links never join a cluster, because one Link
    can pass several junctions.
  - **A junction** is a cluster fed from at least two Links. A Connector feeds from its `from`
    Link; a Link side feeds from its own Link unless a Connector of the cluster arrives on it at or
    before that side. A lane added on one road (the four-leg's approach pockets) is not a junction.
  - **Polygon:** the convex hull of the cluster's Connector polylines and conflict points, padded
    by the widest lane at those Connectors' ends.
  - **Overlaps:** overlapping hulls merge into one. A hull overlapping an existing Node is dropped.

## 7. Owner choices (answered 2026-10-19, D260: every recommendation, 1a–5a)

1. **Where a turn's delay starts.**
   - **(a) At the start of the in-leg Link (recommended).** The window covers the approach the
     author drew, so a queue on it is counted. It needs no number, so no uncited default (WORKFLOW
     §1.4). A per-Node distance override can come later if a study asks.
   - (b) At the polygon boundary. Only time inside the junction is measured, so queueing delay is
     missed and the figure falls well short of what a study means by junction delay.
   - (c) A fixed distance upstream of the boundary. This needs a cited value, and none is chosen.
2. **Queue per leg.**
   - **(a) Mean and maximum (recommended),** from an automatic queue counter at the in-leg's
     boundary crossing, with the existing queue definition
     (`QueueRow{meanLength, maxLength}`).
   - (b) No queue in S0.
3. **LOS letters on Node rows.**
   - **(a) Yes, labelled as in LOS §4 (recommended).**
   - (b) No letters; delay only.
4. **Suggest nodes.**
   - **(a) A separate slice S0.5 after evaluation (recommended).**
   - (b) In S0.1 with the tool.
5. **Coarse values.**
   - **(a) Implemented with S2 (recommended),** so nothing is stored before Macro reads it.
   - (b) In S0 now.

## 8. Slices

| Slice | Content | Rows |
|---|---|---|
| S0.1 | Object, codec (schema 48), validation, Node tool, Nodes panel, delete and rename. **Implemented (D261)** | N1–N5 |
| S0.2 | Membership: inside objects, legs, turns, refusals, conflicts, control (pure functions in `src/model/network/`). **Implemented (D263)** | N6–N11 |
| S0.3 | Micro evaluation: automatic turn sections, leg and Node rows, queue, letters, single-run and batch outputs. **Implemented (D264)** | N12–N17 |
| S0.4 | Coarse values, refine and derive back, lifecycle (with S3, D272) | written with S3 |
| S0.5 | Suggest nodes. **Implemented (D265)** | N18 |
| S0.6 | Results → Nodes. **Implemented (D278)** | N19–N21 |
| S0.7 | The Nodes tab lights the selected Node and its legs (§6). **Implemented (D288)** | N22 |
| S0.8a | The scenario comparison compares Node, leg and turn rows (model, JSON, CSV). **Implemented (D289)** | CMP8–CMP10 (BATCH §7) |
| S0.8b | Results → Nodes shows a comparison's Node rows (§18). **Implemented (D290)** | N21 |

## 9. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| N1 | A project with no Node | Saved bytes and schema unchanged; no `nodes` key |
| N2 | One Node | Schema 48 round-trips; 47 carrying `network.nodes` refused at its path |
| N3 | Bad polygons | Fewer than 3 points, non-finite, self-crossing: `NODE_POLYGON_INVALID` at the point's path; two overlapping Nodes: `NODE_OVERLAP` |
| N4 | `generated` | Written only when true; moving a vertex clears it in the same Undo step |
| N5 | Editor | The tool commits one Node as one Undo step; Enter with 2 vertices keeps the draft; delete and rename are one step each |
| N6 | The four-leg junction (shipped `four-leg-signalised`) with a polygon over it | 4 in-legs, 4 out-legs and the drawn turns, named "in → out"; control `signalised` |
| N7 | The T-junction (shipped `t-junction-priority`) | 3 in-legs and 2 out-legs (the major Links run through, D262), 4 turns, control `unsignalised`; the conflict table lists its conflict areas by turn pair |
| N8 | An overpass Link crossing the polygon unconnected | Ignored: not a leg, no issue |
| N9 | A connected Link that leaves and re-enters; a Connector partly inside | `NODE_LINK_REENTERS`, `NODE_CONNECTOR_CROSSES`; the Node gives no rows; the run still runs |
| N10 | A polygon with no turn | `NODE_NO_TURN` |
| N11 | Editing the network (split, delete, reverse a Link) | Membership follows; no Node field changes |
| N12 | No Node | Single-run and batch JSON/CSV byte-identical |
| N13 | A turn's section | Start at the in-leg's start (owner choice 1), end at the out-leg's boundary crossing; a vehicle counts only after both lines |
| N14 | A free-flowing turn (lone vehicles, no conflict), its in-leg starting downstream of the source (D264) | Delay ≤ 0.1 s; at a source the same vehicles show the start-up loss |
| N15 | Leg and Node rows | Vehicle-weighted means equal a hand computation over the turn rows; a group with no vehicle has no delay |
| N16 | Queue and letters | Leg queue equals a hand-placed counter at the same line; letters from `losLetter` with the Node's type; the CSV marker line |
| N17 | Batch | `{n, mean, sd, halfWidth95}` per row; the same seeds give the same bytes |
| N18 | Suggest nodes | One `generated` polygon per junction (§6) not already covered by a Node, whose membership has no issue on the shipped junctions; overlapping hulls merge, apart junctions do not; one Undo step |

## 10. What stays out

- HCM control delay, v/c and capacity: a Micro Node reports simulated section delay only.
- Node delay in Macro: owner answer 10 of WORKFLOW §9; link VDFs and fixed turn penalties first.
- Node shapes other than one simple polygon, and Nodes nested inside Nodes.
- Calibration: rule 4.

## 11. S0.1 as implemented (D261)

- **Model.**
  - `Node` and `Network::nodes` are in `src/model/network/network.hpp`, and `nodeIssues` is in
    `nodes.*`, called by `validateNetwork`.
  - The polygon tests are in `polygon.*` (`segmentsMeet`, `simplePolygon`, `pointInPolygon`,
    `polygonsOverlap`). Zones now use them too, with unchanged behaviour.
  - The overlap check also catches one polygon wholly inside another.
- **File.** `src/project/node_codec.*` writes and parses the array.
  - A malformed Node, an unknown key, a point that is not `{x, y}`, or `generated` written as
    anything but `true` is `EDIT_NODE_INVALID` at its path.
  - Below schema 48 the network's known fields refuse the key.
  - `documentIds` holds node ids, so `allocateId(d, "node")` never collides.
- **Commands.** `src/commands/node_commands.*` holds `putNode`, `renameNode` and `deleteNode`.
  Replacing or renaming clears `generated`, and an allocated Node keeps the flag it was given.
- **Editor.**
  - `EditorCanvas::Tool::node` shares the zone draft (Ctrl+right-click, Enter, Backspace, Esc) and
    hands it to `nodeCommitted`.
  - `drawNodes` draws a solid outline, dotted while `generated`, with the name at the centroid.
  - The Nodes tab is `src/shell/editor_nodes.cpp`, in the Measurement group: table, rename dialog
    and delete. Tool 13 has the chord `N` and reuses the zone icon until a node icon is drawn.
- **Rows.** N1–N4 are in `tests/node_tests.cpp` (ctest `nodes`) and N5 in `tests/node_ui_tests.cpp`
  (ctest `node-ui`). The workspace and object-group UI tests list the new tool and page.
- **Mutations caught:** no overlap check (N3), `generated` always written (N2, N4), no schema gate
  (N2), an edit keeping `generated` (N4).

## 12. S0.2 as implemented (D263)

- **Model.** `nodeMembership(network, node)` in `src/model/network/node_membership.*` returns the
  inside objects, `inLegs` and `outLegs` (Link id, name, crossing station), `turns` (in, out, name,
  and each object chain as Link spans and whole Connectors), `conflicts` (area, turn pair, kind,
  priority), `control` (`SectionControl`, absent for none) and `issues`. It never throws and stores
  nothing.
- **Crossings.** Each Link's reference polyline is cut wherever it meets the boundary into runs
  inside and outside; a touch without a crossing cuts nothing. A Connector is placed by its stored
  polyline, else `connectorCentreline`.
- **Turns.** A walk from each in-leg over internal Connectors (leaving within the Link's inside run)
  and internal Links, ending at an exit crossing. An object chain counts only if
  `routeLaneChains` gives it a lane chain. Turns are ordered by (in-leg, out-leg).
- **Conflicts.** A Link side matches a turn whose span on that Link overlaps the side's stations
  (touching does not); a Connector side matches a turn through that Connector.
- **Control.** A head inside, or on an in-leg upstream of its entry, makes the Node `signalised`.
- **Diagnostics.** `nodeMembershipIssues` adds the refusals to `runtimeDiagnostics` as advisories
  at `nodes[i]`, resolved to the Node id; Run and save are never blocked.
- **Editor.** The Nodes tab adds control, legs (in / out), turns and issues. Selecting a row lights
  the polygon and its legs since S0.7 (§16).
- **Rows.** N6–N11 are in `tests/node_membership_tests.cpp` (ctest `nodemembership`); `node-ui`
  checks the new columns.
- **Mutations caught:** the through turn dropped (N7), heads on an in-leg ignored (N6), the lane
  chain filter removed (N6), an unconnected crossing Link counted (N10, N11), re-entry allowed (N9).
  Letting touching spans overlap changes no shipped row (the extra pair is a turn with itself).

## 13. S0.3 as implemented (D264)

- **Spec.** `nodeSpecs(network, table)` (`src/project/node_output.*`) turns each Node's membership
  into `NodeSpec`s (`src/eval/nodes.hpp`): one `SectionSpec` per turn, from `{in-leg Link, 0}` to
  `{out-leg Link, crossing station}` through `locateSectionLine`, and one set of queue lines per
  in-leg at its crossing, on every lane. `evaluationSpec` sets `spec.nodes` and loads the LOS pack
  also when a Node has a control type.
- **Measured by the existing machinery.** `MovementAccumulator` appends the turn sections after the
  authored sections and the leg lines after the authored counters, then splits the rows back, so
  authored rows keep their indices and numbers. `nodeRows` groups them: leg and Node delay are
  Σ vᵢdᵢ / Σ vᵢ over turns, unfinished sums, a group with no vehicle has no delay.
- **The mid-Link join is counted correctly.** A line binds on every route that holds its segment,
  so the T-junction's minor → eastbound turn (joining `link-1` at station 170) ends at station 175
  on its own route, and a through vehicle never holds `link-5`'s start line.
- **Output.**
  - Single run: JSON `nodes[]` with `{name, controlType, vehicles, delay, unfinished, los, legs[],
    turns[], conflicts[], issues[]}`. A leg adds `queueMean` and `queueMax`, and a turn adds
    `travelTime`. Conflicts are `{conflictArea, first, second, kind, priority}`, naming turns.
  - CSV, after the LOS block: the marker line, then rows at level `node`, `leg` and `turn`. Queue
    cells are filled on legs only.
  - Batch: the same shapes with `{n, mean, sd, halfWidth95}` values, and a CSV block
    `level,node,name,controlType,n,delay_s,ci95_s,sd_s,vehicles_mean,unfinished_mean,queueMean_m,queueMax_m,los`.
    Letters use the mean over seeds. A leg's or Node's batch delay estimates each seed's weighted
    mean.
  - Nothing is written without a Node, and the shipped projects' outputs are byte-identical.
  - The editor's Results tables show Node rows from S0.6 (§15). The batch comparison compares them
    from S0.8a (§17); its editor table does not yet.
- **Finding: the delay window includes start-up from rest.** Vehicles enter the network from rest,
  so a turn whose in-leg Link starts at a source carries the start-up loss. On the T-junction, lone
  major vehicles show about 3.0 s against 3.2 s for the full exercise (seed 42). Choice 1a stands
  as chosen; a study that wants only junction delay draws its approach Link from upstream of the
  source, or splits it (N14).
- **Cost.** Callgrind Ir at seed 42 (Release):
  - without a Node, unchanged (four-leg −0.09 %, T-junction −0.25 %);
  - with one Node, +40 % on the four-leg (12 turns) and +26 % on the T-junction (4 turns), almost
    all of it in the section accumulator's per-tick work.
- **Rows.** N12–N17 are in `tests/node_evaluation_tests.cpp` (ctest `nodeevaluation`).
- **Mutations caught:**
  - the start line moved to the boundary (N13, N14);
  - unweighted leg means (N15);
  - leg lines dropped (N16);
  - letters from the wrong type (N16);
  - the CSV block written with no Node (N12).

## 14. S0.5 as implemented (D265)

- **Model.** `nodeSuggestions(network)` (`src/model/network/node_suggestions.*`) applies §6's rule
  and returns `{"", "", hull, generated}` Nodes in cluster order. It never throws.
- **Command.** `suggestNodes(document)` (`src/commands/node_commands.*`) adds each suggestion through
  `putNode` and returns the ids.
- **Editor.** The Nodes tab's *Suggest nodes* action is one `execute`. When nothing is suggested it
  changes nothing and says so in the status bar; otherwise it reports the count.
- **On the shipped projects:**
  - the four-leg and the M2.6 template each give one Node with 4 + 4 legs and 12 turns, and the
    approach lane adds are left out;
  - the T-junction gives one Node with 3 + 2 legs, 4 turns and 3 conflict rows;
  - the lane-change lab gives none.
- **Rows.** N18 is in `tests/node_suggestion_tests.cpp` (ctest `nodesuggest`), and `node-ui` covers
  the action.
- **Mutations caught:**
  - no padding;
  - Connectors joined by Link without the station;
  - no existing-Node skip;
  - no merge;
  - `generated` false;
  - single-source clusters kept.

  Keeping branching conflicts changes no shipped suggestion: their sides add no second source.

## 15. S0.6: Node rows in the editor's Results (D278)

- **Results → Nodes:** one table in the CSV block's order. Each Node's row comes first, then its
  legs, then its turns.
  - **A single run:** level, name, control type, vehicles, delay (s), unfinished, queue mean and max
    (m; legs only), travel time (s; turns only) and LOS. The LOS is `losCell` with the Node's control
    type, as `writeNodesCsv` gives it.
  - **A batch:** level, name, control type, n, mean delay (s), 95 % CI half-width (s), mean
    vehicles, mean unfinished, mean queue mean and max (legs only), and LOS on the mean delay.
  - **A comparison:** from S0.8b (§18), the rows of `Comparison::nodes`. Until then it showed none
    and said so.
- **Note:** it starts with the CSV block's marker. Without a Node it says how to draw one; with no
  run, nothing yet; with an unfinished single run, the partial-run prefix.
- **Unchanged:** every output, since the change is editor-only.
- **Rows** (`tests/node_results_ui_tests.cpp`, ctest `node-results-ui`):

| Row | Case | Asserts |
|---|---|---|
| N19 | The T-junction with Suggest nodes, one finished run | 1 + 5 + 4 rows equal to `runReport()->nodes`; the queue cells only on legs; the LOS as `losCell`; the marker first |
| N20 | The same, seeds 42–43 | Rows equal to `aggregate(runs).nodes` (n, mean, CI, LOS on the mean); an edit returns the tab to no run |
| N21 | No Node, or a comparison (S0.8b, §18) | The four-leg without a Node shows the no-Node note and no rows. A comparison with a busier copy gives 8 rows × 12 columns: each row's two sides, difference and CI as `Comparison::nodes` holds them, with queue cells on legs only. A Node renamed in the alternative gives no rows, and the note names it base-only and the new name alternative-only |

- **As implemented:** `src/shell/editor_results_nodes.cpp` (`refreshResultNodes`), the
  `editorResultsTabNodes` page after Parking lots.
- **Mutations caught:**
  - the LOS without the control type;
  - legs dropped;
  - the batch showing the run's rows;
  - queue cells on turns;
  - the note without the marker;
  - a comparison showing rows;
  - the batch LOS on mean + CI. It survived a Node-row-only check; N20 now checks every row's
    letter.

## 16. S0.7: the selected Node and its legs on the canvas (D288)

- **The highlight.** Selecting a Nodes row lights that Node; clearing the row, deleting the Node or
  an Undo that removes it puts it out. A Select click on the canvas clears it, as it clears the
  conflict table's.
  - **The outline:** drawn in the selection colour at 2.5 px, still dotted while `generated`.
  - **The legs:** each Link's span outside the polygon up to its crossing, as `nodeMembership`
    gives the legs: `[0, station]` for an in-leg, `[station, end]` for an out-leg. The span is
    clipped to the Link's own surface. In-legs are filled, out-legs hatched with a dashed edge, so
    the direction reads without arrows. A Link on a hidden level is not drawn.
  - **A Node with an issue** has no legs (§3), so only its outline is lit.
- **Unchanged:** every output, since the change is editor-only.
- **As implemented:**
  - `EditorCanvas::setHighlightedNode`, and `drawNodeLegs` in `src/editor/canvas_zones.cpp`;
  - the Nodes tab sets the highlight on selection and on every refresh;
  - the canvas's `selectionCleared` clears the Nodes table with the others.
- **Row** (`tests/node_ui_tests.cpp`, ctest `node-ui`):

| Row | Case | Asserts |
|---|---|---|
| N22 | The drawn triangle (refused), then the four-leg's suggested Node | The triangle's outline is lit with no legs. The suggested Node draws exactly its 4 in-legs and 4 out-legs, each tagged by Link id and direction as `nodeMembership` names them. Clearing the row returns the ink outline with no legs. A deleted Node is not highlighted |

- **Mutations caught:**
  - legs never drawn;
  - in and out swapped;
  - the highlight kept after delete;
  - a refused Node drawing legs;
  - the outline not lit.

## 17. S0.8a: Node rows in the scenario comparison (D289)

- **Matching.** `compareBatches` matches Nodes by name, then each matched Node's in-legs and turns
  by name within it, by BATCH §7's rule. Unmatched names are listed per level, never dropped.
- **Figures.** Welch differences (BATCH §7) of the Node's, each leg's and each turn's delay, and of
  each leg's mean queue. A difference has no LOS letter.
- **Output.**
  - JSON: `nodes` and `unmatched.nodes`, only when either side has a Node.
  - CSV: two blocks after `network`, delay (levels `node`, `leg`, `turn`) and leg queue.
  - The `# Unmatched` line names Node rows as `node …`, and leg or turn rows as `"Node / name"`.
  - A comparison without a Node keeps its bytes.
- **Editor.** Copy and Export carry the blocks, since they use `comparisonCsv`. Results → Nodes
  shows the rows from S0.8b (§18).
- **As implemented:** `ComparisonNodeRow` and `ComparisonLegRow` in `src/runner/compare.hpp`.
  `match` now takes the row builder, and `matchValue` keeps the one-value blocks. The output is in
  `src/project/comparison_output.cpp`.
- **Rows:** CMP8–CMP10 in `tests/compare_tests.cpp` (ctest `compare`).
- **Mutations caught:**
  - Nodes ignored;
  - legs' queue compared by delay;
  - an unmatched leg left off the CSV line;
  - the blocks written without a Node;
  - the Node's difference reversed.

## 18. S0.8b: a comparison's Node rows in Results → Nodes (D290)

- **The table:** one row per compared Node, then its legs, then its turns. It has 12 columns:
  - level and name;
  - base n and delay, alternative n and delay, difference and ±95 % (s);
  - base and alternative mean queue, difference and ±95 % (m), on legs only.

  There is no LOS column, because a difference has no letter (BATCH §7).
- **The note:**
  - the marker;
  - "alternative minus base" with the seed count and Welch's half-width;
  - the unmatched names, in the comparison tab's wording: Node names bare, leg and turn names as
    `Node / name`.

  While a comparison runs, the note says the rows appear when it finishes. With no Node on either
  side, it gives the no-Node note.
- **Unchanged:** every output, since the change is editor-only.
- **As implemented:**
  - `refreshComparedNodes` in `src/shell/editor_results_nodes.cpp`;
  - the locale key `editorNodeResultsCompared`, which replaces `editorNodeResultsNotCompared`.
- **Row:** N21 (§15).
- **Mutations caught:**
  - the not-compared path kept;
  - base and alternative columns swapped. A comparison with itself let this one survive, so N21
    uses a busier copy;
  - queue cells on turns;
  - unmatched Nodes missing from the note.
