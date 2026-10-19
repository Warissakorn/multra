# Nodes: a polygon over a junction (S0, M5.1)

Written 2026-10-19 (D259). The owner accepted every §7 recommendation the same day (D260); slices
S0.1, S0.2 and S0.3 are implemented (D261, D263, D264, §8); D262 amended §3. Stage S0
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
  they land with S2 (§5, slice S0.4).
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

## 5. Coarse values and lifecycle (specified now; slice S0.4 lands with S2)

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
- **Suggest nodes** (S0.5) draws one `generated` polygon around each conflict cluster
  (`conflictGroups`, `src/model/network/right_of_way.hpp`), skipping clusters already inside a Node.
  It is one Undo step for the whole set.

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
| S0.4 | Coarse values, refine and derive back, lifecycle (with S2) | written with S2 |
| S0.5 | Suggest nodes | N18 |

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
| N18 | Suggest nodes | One `generated` polygon per conflict cluster not already inside a Node; one Undo step |

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
- **Editor.** The Nodes tab adds control, legs (in / out), turns and issues. Selecting a row does not
  yet highlight its legs (§6).
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
  - The editor's Results tables and the batch comparison do not show Node rows yet; the editor's CSV
    copy carries the block.
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
