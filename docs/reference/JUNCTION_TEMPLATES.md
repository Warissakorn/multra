# Junction templates and the HCM study workflow (M14, D386, D387)

Contract before code (D145). Written 2026-10-28 as a junction builder (D386), then
rewritten the same day at the owner's direction (D387, "เอาตามแนะนำ"): **no builder tool**. The
engineer draws Street segments; where they cross, *Find crossings* makes a Node, and a **junction template**
on that Node says which lanes serve which turns, the pockets and the control, from which the
Connectors, right-of-way and signals are generated. Where two Street segments meet end to end with different
cross-sections, a **transition template** joins them. The export is [UTDF_EXPORT](UTDF_EXPORT.md).
SR and J1 are implemented; J2's interfaces and geometry are described in §6 (D391, D392).
J3a adds the template catalog and stored recipes (D393–D395). J3b is implemented
(D398–D402): the two-way-stop fill, named refusals and legacy-fill parity below. All-way-stop
generation is deferred by the owner until A5 or U1 needs it (D399).
J4 generates P1–P4 signals from required site timing and checks their NEMA layout (§8, D404).
J4T adds the protected three-stage T and measured T template cuts (D416).
The editor recipe surfaces are implemented in J6.
The owner choices are answered
(§12: D387, D388; J2 clarification on 2026-10-29).

**Naming.** D387 renamed Road everywhere, file format included, before any M14 code (slice SR,
§10). "Segment" was taken by the engine's `Segment` and its messages, so the owner chose **Street
segment** (HCM's *urban street segment*), D389; SR is done.

Rule 4 applies: every analysis result keeps the not-validated marker.

## 1. Why templates, not a builder

- The chain already exists: Street segment → *Find crossings* (STREET_SEGMENTS_IMPLEMENTED §18) → *Coarse values…*
  (§20) → F4 ([READINESS](READINESS.md) §13). Its one gap for HCM is lanes: F4 shares an in-leg's
  lanes kerb to far (§13.3), so a pocket, an exclusive turn lane or a shared far-through lane
  cannot be stated, and NODE_ANALYSIS lane groups and UTDF's Lanes table need exactly that.
- A template is **data** (rule 5), reused by every junction of its kind, edited once, and applied
  the way a cross-section template is (*Apply template*, D305). One way of working for both.
- ROADMAP M1's "junctions are not something the user places" holds: the Node already exists, and
  the template writes ordinary `generated` objects (READINESS §13.6), which every tool then edits.

## 2. What is reused

| Need | Existing piece |
|---|---|
| Junction places, overpasses, splits | `findCrossings`, `applyCrossings`, `splitStreetSegment` (STREET_SEGMENTS_IMPLEMENTED §18) |
| Major and minor legs | `Node::coarse.major`, else `majorByRank` (§19); `NODE_PRIORITY_UNDETERMINED` otherwise |
| Leg cuts at the stop line | `nodeLegCuts` (READINESS §13.2) |
| Connector curve, conflict areas, HCM Ch. 20 ranks, Stop on minor arms | F4 (§13.3–§13.4) |
| Controller, groups, heads | `SignalController`, `SignalGroup{number, greenStart, greenEnd, amber}` (D359, D360), F4's heads (§13.5) |
| Pockets | `addTurnPocket`, kerb or far with a widening taper (J2); legacy `splitLink` keeps its constant-width far-side addition |
| One Undo step, `generated`, refine | `History::execute`, `settleGenerated`, `settleNodes` |
| Offer and apply | *Fill defaults…* (`fillOffers`, `applyFills`), as F3 and F4 |
| Lane groups and HCM | `src/analysis/` (NODE_ANALYSIS A1–A3), `Node::analysis` (schema 58) |

## 3. The junction template (data)

Installed in `data/junction-templates/`, one file each, and project-owned copies like link types.
A template states **structure, not site numbers**: lane assignments, which lanes are pockets, the
control and the phasing preset. Storage, taper and timing are site values, entered per Node (§4),
so a shipped template needs no source beyond its own description (WORKFLOW §1.4).

```json
{ "id": "signal-4-major-far-pockets", "name": "Four legs, signalised, far-turn pockets on the major",
  "kind": "junction", "legs": 4, "control": "signalised", "phasing": "P2",
  "roles": {
    "major": [ { "movements": "F", "pocket": true }, { "movements": "T" }, { "movements": "TK" } ],
    "minor": [ { "movements": "FT" }, { "movements": "TK" } ] } }
```

- **`kind`:** `junction` or `transition` (§7); J3a loads junction entries; J5 adds transition structure/loader support (§7).
- **`legs`:** 3 or 4 (§12 JT-2).
- **`control`:** `signalised` (pretimed), `twoWayStop` or `allWayStop`.
- **Roles, not compass directions**, because leg angles differ at every site:
  - four legs: `major` and `minor`, each applied to both legs of that role;
  - three legs: `major-far` (the major leg whose far turn enters the stem), `major-kerb` and
    `minor`.
- **Lane rows** run **far side to kerb** (`drivingSide` decides which is far). `movements` is a
  contiguous subset of far turn `F`, through `T`, kerb turn `K`; `pocket: true` makes the lane a
  short added lane rather than a full one; `side` (`far` or `kerb`, default by movement: `F` far,
  `K` kerb) says where the pocket is added.
- **Refused on load** (`JUNCTION_TEMPLATE_INVALID` at its path): a lane serving nothing,
  non-contiguous movements, lanes whose movements cross (a lane's movements must not lie farther
  than those of a lane on its far side), a role missing for its leg count, a pocket serving `T`.

The template editor (J6) is a table per role with the geometric names beside each row, so "far
turn" reads *right* in Thailand and *left* in the United States.

**J3a catalog/loader (D393).** `resolveJunctionTemplates` (`src/project/junction_template_codec.*`)
reads installed JSON files in filename order, with paths `catalog/junction-templates/<file>`;
malformed JSON names the file, and invalid fields name their full path. No cache is retained.
`definition.junctionTemplates` owns the whole catalog when present, including an empty array;
when absent it resolves installed entries, as Link types do. Duplicate ids are refused at the
later entry's `.id`. The installed [13 junction and two transition templates](../../data/junction-templates/README.md)
cover P1–P4 four-leg and two-way stop three-/four-leg, each with and without major far-turn pockets.
No storage, taper, cycle or timing numbers ship. Signalised entries require P1–P4; other controls
carry no phasing. P2 major / P3 all roles use separate `F` rows for protected far turns (D396),
full-width when not a pocket; `TK` remains shared. Rows and role shapes share the same validation on load and Command commit.

## 4. On the Node: the choice and its site values

`Node::junction`, schema 61 (J3a, D393–D395), written only when set:

```json
"junction": { "templateId": "signal-4-major-far-pockets", "major": ["r2", "r3"],
  "legs": { "r2": { "storage": [60], "taper": [20] }, "r3": { "storage": [45], "taper": [20] } },
  "signal": { "cycle": 100, "phases": [ { "number": 2, "green": 38, "yellow": 3, "allRed": 2 } ] } }
```

- **Legs** are named by Street segment id, as coarse values name them (B29 (b) may change the key).
  `storage` and `taper` list one value per pocket of that leg's role, far to kerb, in metres.
- **Major and asymmetric lanes (D393):** optional `major` names Street segment ids, falling back
  to `majorByRank` when empty. Optional `junction.legs.<id>.lanes` replaces that leg's role rows,
  with the same schema/validation and far-to-kerb order as §3. The Command `setNodeJunction`
  carries `coarse.major` when no major is supplied and keeps the existing `coarse` unchanged.
  B36/D412 keeps current `generated` for same-template site-only edits; another template,
  Detach and polygon edits clear it, never setting it back. Detach also keeps `coarse`.
- **Signal:** the cycle and, per NEMA phase the preset uses, green, yellow and all-red. All are
  required to apply, with no default (§12 JT-5). **D395:** incomplete recipes save what is entered:
  scalar fields are absent when empty, while `storage`/`taper` use `null` for an unentered pocket
  slot. Entered lengths/cycle/green are finite and > 0; yellow/all-red are finite and >= 0;
  phase numbers are unique integers 1–8. Counts/applicability are J3b's, ring/barrier timing J4's.
  J2's pocket command still requires storage/taper >= 0.2 m. A positive smaller value can be
  kept in a draft, but cannot generate a pocket.
- **Directions (D394):** independent `Node.approachDirections` maps in-leg Link ids to `N`, `E`,
  `S`, `W`, including Nodes without a template. Keys must name existing Links. `approachLayout`
  reads it; a per-call override wins
  for that key. A split carries the key downstream; deleting the Link removes it. It changes
  labels only, not angles. Recipe leg ids and major follow only the moved end of a split Street
  segment, as coarse values do; B29 (b)'s cross-level key remains undecided.
- **Codec (D393):** the three new fields (`definition.junctionTemplates`, `Node.junction`,
  `Node.approachDirections`) select 61 only when used. Older files still read; presence before
  61, even `null`, is `EDIT_UNSUPPORTED_FIELD` at that field. Malformed recipes/directions
  are `EDIT_NODE_INVALID` at their paths. Coarse and recipe may coexist; no unrelated version
  floor moves. Unknown template/leg references can remain in a draft for J3b's missing-piece offer;
  values are never discarded on load.
- **The recipe, not the truth.** As a Street segment keeps its `crossSectionId` after its Links exist, the
  Node keeps its template after the detail exists and after detail removal; the two-leg lifecycle
  does not erase a recipe. What HCM and UTDF read is the drawing, through
  `approachLayout` (§6). A drawing that no longer matches the template is the advisory
  `SEAM_JUNCTION_DIFFERS`, with explicit *Apply again* (D410, lists edited detail before replacement) and *Detach* (drops
  `junction`), as `SEAM_LANE_COUNT` offers its two fixes (STREET_SEGMENTS_IMPLEMENTED §21).
- **D393 review:** `coarse` and `junction` coexist before detail. Choosing or detaching a template
  does not change the Macro/Meso compile: control, banned turns and stages still come from
  `coarse`. When the J3b fill creates detail, the existing `settleNodes` refine rule clears
  `coarse` in that same Command, with the recipe retained. Undo restores both. The rule refusing
  coarse values on a Node that already has detail is unchanged (STREET_SEGMENTS_IMPLEMENTED §20).

## 5. Choosing and applying

- **In *Find crossings*:** each Junction row gains a *Template* column (none by default), so
  Confirm creates the Node with its template in one Undo step. End-to-end rows (§7) are new.
- **On the Nodes tab:** *Junction template…* sets or changes the template and the site values of
  the selected Node, one Undo step.
- **Apply** is the fill kind `junction` in *Fill defaults…*, after F3 (the Street segments' Links must exist).
  It is offered when the Node has a template, all its legs are Link legs, the leg count matches,
  the major legs are determined, and every site value is entered; otherwise its row names the
  missing piece (`JUNCTION_STORAGE_MISSING`, `JUNCTION_TIMING_MISSING`, `JUNCTION_LEG_COUNT`, …).
  Applying, in one Command, every object `generated`:
  1. **Pockets (D391):** the in-leg ends storage + taper before the intended stop line; a widening
     Connector spans the taper, and a downstream Link carries the full-width lane over the
     storage. The side is far or kerb; Thailand's exclusive left-turn lanes are kerb-side.
  2. **Cuts** by `nodeLegCuts`.
  3. **Connectors:** one per (lane row, movement), adjacent rows with the same movement merged into
     one multi-lane Connector; the far turn lands on the out-leg's far lanes, the others from the
     kerb (§13.3's out-end rule). This replaces §13.3's share when a template is present.
  4. **Right-of-way:** F4's areas and ranks; Stop on the minor arms for `twoWayStop`, on every arm
     for `allWayStop` when generation is implemented (deferred, D399).
  5. **Signals:** one controller named after the Node, a group per NEMA phase numbered by that
     phase (§8), timing from the Node's values, a head at the start of every Connector path.
- **Ordinary Fill's Apply again** replaces only objects still `generated` and refuses, naming them, when a person
  has edited one (`JUNCTION_EDITED`). Undo restores every byte.
  D410 separately authorizes the seam's explicit replacement list ([J6 contract](JUNCTION_TEMPLATE_EDITOR.md)).

**J3b scope confirmed by the owner (D397).** Signalised templates were listed but disabled in
J3b; J4 (D404) now applies their required site timing and ordinary generated controllers/heads
(§8). Missing cycle/phase values still get their own
`JUNCTION_TIMING_MISSING` paths. JT8 uses the two-way-stop fill for Micro, then the existing
manual signal Commands only in the test to check HCM lane groups. It does not execute a preset.
The number of non-pocket rows must equal the in-leg's full lane count; otherwise
`JUNCTION_LANE_COUNT` lists existing → required counts. The owner changes the cross-section or
per-leg lane override before Apply; the fill never resizes those full lanes implicitly.

**J3b implementation (D398–D402).** `junctionFillStatus` and
`generateJunction` are per-call plans and Commands in `src/commands/junction_fill*`. F3 comes
first. The `junction` offer lists every missing storage/taper slot and timing component with
its code and path, extra/inapplicable site values, unknown legs/templates, unresolved major
arms and geometry refusals. F4's lane sharing is not offered on a Node carrying a recipe.
The planner measures cuts with the final widths, then creates the physical taper/storage and
movement ranges. Full lanes keep their footprints. The final storage Link keeps the leg's
Street segment association; detached upstream pieces and the new Links carry `generated`
(schema 62), as do cuts and Connectors. Generated Links enter S5's warnings/export marking. Recipe arm ids stay unchanged; Link-based inputs follow
the downstream end through the existing split rules. No applied layout/receipt is stored.
Re-apply walks generated pocket predecessors, stopping at a junction, reconstructs the base
leg from its cross-section, and refuses named manual detail/attachments instead of deleting
it. A fill transaction preserves generated ownership during re-generation; manual Commands
refine it. The existing `settleNodes` transition alone clears coarse after detail appears. Ordinary F4 still keeps a leg end when no other surface meets it ([READINESS](READINESS.md) §13.2).
T template fills use the measuring-copy correction below; a remaining zero through-Connector
gap still refuses with `EDIT_CONNECTOR_GAP`, without inventing a cut distance.
**All-way stop (D399, B31 answered).** The fill stays listed but disabled with
`FILL_NODE_ALL_WAY_STOP`; D388 JT-4a's generation waits until A5 or U1 needs it.
StopControl is unchanged and schema 63 is not used. Signal generation is implemented in J4 (§8).

**T template cuts (D416, supersedes D400 for templates).** The private full-width measuring
copy extends the stem to the far major pavement edge before the existing overlap cuts.
Only cuts on the original Link are published, with stations mapped back before reference
checks. No distance default or F4 change; [details/JT19](JUNCTION_SIGNALS.md#t-template-cut-measurement-jt19-b37d416).

**J6a correction (D415).** Ordinary Fill and corridor rebuild support three exit lanes. Exact
endpoint station conversion fixes a one-rounding-unit conflict overrun; D413's refusal is
removed. Authored extents and the core validator are unchanged; JT16/JT17 cover both sides.

**Street segment Apply template after a fill (D401, corrected by D402).** A Street segment
holding junction-generated Links (`Link.generated`) refuses with `JUNCTION_STREET_SEGMENT_DETAIL`
before changing a lane, including after recipe detach. Choosing a recipe on existing F4 detail
does not establish junction-fill ownership: F3 → F4 → recipe → change laneWidth → Apply template
keeps its pre-J3b behaviour on both driving sides (JT14).
If Undo is available, return before the junction fill and Apply the cross-section there.
The recovery also works after reopen, with no Undo: **delete the junction's generated Links,
Apply the Street segment template, then fill the junction again** (JT15).
**Deleting Links also removes objects attached to them**, including stationed measurements;
place those again after the new fill. An ordinary junction re-apply also refuses with that code
if its base lanes/widths/reference geometry no longer match the cross-section; it cannot silently
move or resize the detached upstream Link.
**Corridor limitation (D403):** a Link between two filled junctions is generated for both. Deleting it
in this recovery also removes the neighbouring junction's leg; the recovery is not local to the
selected junction. Account for both ends and attached objects before rebuilding a corridor.

**Work order (D402): fill the junction before placing stationed measurements.** In particular,
place queue-counter measurement lines and travel-time sections after the cut/pocket geometry is
settled. Cuts and re-apply refuse stationed data they cannot keep in place; deleting the Links
in the recovery above removes their attached objects.

**J6 combined action (D402, D403, D410): change cross-section through junctions.**
The Command/preview and UI discover the complete shared-Link component, list its Nodes/detail,
restore every end and regenerate the corridor in one Undo, preserving references or refusing all.
Use this action before the deletion recovery; its guards and acceptance are in the
[J6 contract](JUNCTION_TEMPLATE_EDITOR.md). The catalog and Node recipe forms are implemented in J6.

**Existing references (D401).** Retained in-leg heads, route ids and Link-based Node inputs
follow the pocket and Undo restores the complete JSON, revision included. A cut that would
truncate or rebase a person's stationed data refuses with `JUNCTION_EDITED` naming it.
Re-apply preserves zone destinations, refuses a new origin or routing target on a removed
pocket, and refuses conflicting direction keys (including two pocket pieces without a base
override) before collapsing them. No reference is guessed
or dropped. The historical F3/F4 implementations at `33a856a5` are compared in the same build
against the fill dialog's accepted `generatedCommand=true` path (JT13, D400).

## 6. The lane layout, derived and never stored (D272)

`approachLayout(network, node, overrides = {})` (pure, `src/model/network/approach_layout.*`)
reads, per in-leg:
- each lane's movements, from the Connectors leaving it;
- each pocket's storage, from its start to the stop line, and taper;
- the in-leg's bearing, from its last segment before the cut.

It serves NODE_ANALYSIS's lane groups (replacing its derivation only if byte-identical on
NA1–NA23), UTDF's Lanes table, and `SEAM_JUNCTION_DIFFERS`. **Movement names:** an approach's
direction is its travel bearing's nearest of N/E/S/W (`JUNCTION_DIRECTION_AMBIGUOUS` when two
in-legs share one or a bearing ties two directions). **D392, owner chose 2A:** absolute turn angle
< 45° is T; 45° to < 135° is L or R; >= 135° is refused as `JUNCTION_UTURN`. Names are NBL/NBT/NBR,
EBL/EBT/EBR, SBL/SBT/SBR and WBL/WBT/WBR. L/R are geometric, independent of driving side.

**J2 interface and geometry (D391, owner chose 1A):**
- `addTurnPocket(document, link, storage, taper, side)` (`pocket_commands.hpp`, implemented in
  `split_link.cpp`) runs inside one `History::execute`. The current Link end is the intended stop
  line. The upstream id stays; the returned downstream Link adds its lane at index 0 for kerb,
  at the end for far. Original lanes keep their footprints. The outer continuity Connector
  widens 1→2, and every new taper/continuity Connector is `generated`; a hand edit refines it.
- Storage and taper must each be at least 0.2 m, leaving more than 0.2 m upstream (the existing
  minimum section length). Attachments/controls within the taper are refused as in a split;
  signal heads project onto the original lane's new owner/path. Routes keep ids and order, with
  the downstream Link inserted. For an implied Link-to-Link step, only `addTurnPocket`'s
  still-`generated` 1→2 Connector continues the ordinary lane without counting its zero-source-width
  surplus path. A hand-drawn or refined widening remains ambiguous, as before J2; so do two
  ordinary bridges. Explicit Connector selection is unchanged (D391, J2 review).
- Layout lanes are in Link order, **kerb to far**, with lane id, width, stop-line station, the
  membership turn indices and optional pocket `{side, storage, taper, connector}`. Template lane
  rows run the other way (§3). A pocket starts at a Connector path with zero source width;
  ordinary aligned predecessors are walked across a later split, stopping at multiple incoming
  paths, a junction turn, or **any Connector with a run inside a Node**, including a straight-through
  movement (D391, J2 review). A pocket is never carried through one junction to the next.
  The taper's reference spine is recovered by removing the stored
  first-lane endpoint offsets with `laneBlend`; length is not measured along the sloping lane.
- Bearings are degrees clockwise from north, at the first departure/stop line. Refusals retain
  membership but return no approaches or movement names. Overrides are a per-call map of this
  Node's in-leg Link ids to compass directions; J3a persists the per-Node choice (§4), J6 edits it.
  An override changes the label only, never the turn angle or the U-turn refusal.
- No new stored fields: existing geometry, lane counts, `laneChangeSide`, `laneBlend` and
  `generated` suffice. J3a separately uses schema 61 for recipes/catalog ownership/directions (§4).
  The legacy split tool's 0.2 m span and far-side
  constant-width addition keep their existing behaviour. J2 adds no UI. HCM's derivation stays
  unchanged; replacing it requires the NA1–NA23 byte-parity check above.

## 7. Transitions: two Street segments end to end with different cross-sections

- **Before J5:** Macro and Meso join free ends within 2 m (STREET_SEGMENTS_IMPLEMENTED §19); Micro gets no
  Connector. *Find crossings* lists only places of three legs or more, and the two-leg merge
  (§20) joins only Street segments with the same template.
- **J5:** *Find crossings* also lists two Street segments meeting end to start whose templates or
  overrides differ, as a **Transition** row. Confirm makes a two-leg Node with a transition
  template; the same-template case keeps today's merge.
- **A transition template** (`kind: "transition"`) states, per direction:
  - `align`: `kerb` or `far`, the side on which lanes line up; lanes are added or dropped on the
    other side. **Chosen in the template, no project default** (D387);
  - the taper length is a site value on the Node (`junction.legs.<road>.taper`), required.
- **Apply** cuts the legs, adds the taper as Connectors from the dropped lanes merging into the
  last aligned lane (or the added lanes diverging from it), and one Connector per aligned lane.
  F4's merge areas apply. No HCM; UTDF exports the Node as a plain node between segments.

**B34 answered (D406).** Put the entire entered taper in the incoming arm, before the join,
measured along that Street segment's centreline. Keep the existing alignment; refuse a Link
that is too short. D407 supersedes the original Node-space refusal for a still-generated Node:
the fill enlarges its polygon; a person's polygon still refuses. Forward follows the end-to-start
centrelines; backward is the reverse. `junction.legs.<id>.taper[0]` serves that incoming arm.
The installed set begins with two templates, kerb/kerb and far/far; owned entries can mix them:
`{ "id": "transition-kerb", "name": "Kerb aligned", "kind": "transition", "legs": 2,
"directions": { "forward": { "align": "kerb" }, "backward": { "align": "kerb" } } }`.
There are no control, phasing or lane-role fields on a transition. New stored transition
structure selects schema 63 and is refused before it at its field path; existing recipe fields
keep their earlier floors when no new structure is stored. No duplicate Node kind is stored.
Find crossings gets a required selector only for Transition rows in J5, initially unselected;
J6 also implements general junction selection and the recipe editor. JT10 is **passing**.

**J5 implementation.** The existing junction fill dispatches transition entries to a per-call
plan; no applied layout is stored. It preserves full lane widths/ids, marks the program's cut
Links and new Connectors/merge controls generated, and reuses the generated-only restoration
and named reference guards from §5. Aligned lanes are drawn first; extra lanes retain the
existing merge drawing-order/default rules. The entered taper is the cut retreat on the
incoming centreline, not the Connector's curved arc length. Re-apply and Undo cover the entire
document. F3 is required; missing taper, inadequate Link/edited-Node space, unmatched directions and
inapplicable site fields refuse without mutation. One-way matching arms need only their
incoming taper. Equal-section free ends are not new Transition rows, and the existing
same-template two-leg lifecycle still merges when a Node becomes eligible (STREET_SEGMENTS_IMPLEMENTED §20). No HCM
transition method or UTDF writer is added; existing node analysis keeps its unsignalised refusal.

**J5 review (D407).** A Node from Find crossings that is still `generated` needs no manual polygon
edit: Fill enlarges it to the convex hull of its original polygon and both tapers' cut points,
Connector spines and painted rails, padded by the widest attached lane as for suggested Nodes.
This keeps the cuts inside and the external Link ends outside; inadequate arm space still refuses.
The polygon remains generated. Preview changes nothing; polygon, Links, Connectors and controls
apply in the same Command and one Undo. If `Node.generated` is false, no polygon is enlarged:
`TRANSITION_TAPER_OUTSIDE` names `network.nodes[index].polygon`; enlarge that polygon or shorten
the entered taper. Recipe selection/refine keeps its existing ownership rules. No new field or schema.

**Capture guard (D408).** After enlargement, every Link leg must already have been a leg of the
Node, with the same entry/exit direction. A newly captured leg refuses with
`TRANSITION_POLYGON_CAPTURES` at that Link: shorten the taper or draw the polygon by hand.
Refusal changes neither document nor History. The existing neighbouring-Node `NODE_OVERLAP`
refusal still applies. A side road that remains outside the grown polygon does not block Fill.
**J6 carry-over (D409).** The private plan checks the grown polygon against every other Node
before offering Apply. `NODE_OVERLAP` names the neighbouring Node and disables the Fill row;
preview and a refused Command leave the document/History unchanged.

## 8. NEMA numbering of signal groups

The complete signal contract is [JUNCTION_SIGNALS](JUNCTION_SIGNALS.md): four-leg P1–P4
(D404) and the protected three-stage T (B33/D405, B37/D416). Site timing remains required;
2/6 start at zero, the leading far ends the preceding cycle, and unused T phases are omitted.
T uses existing P2 plus legs=3, with two full lanes per arm; no new schema or timing default.

## 9. Posted speed and the analysis panel

- **Posted speed (J1, done, D390):** booked by D379, confirmed by D380, shaped by D381.
  - One optional `speedLimit` (km/h, finite, > 0) on the Link, schema 60, never a cap.
  - A cross-section template's optional `speedLimit` is copied onto the Links *Apply template*
    generates; a template without one leaves a Link's own.
  - The inspector edits it on every selected Link in one Undo step; the object CSV has
    `speedLimit_kmh`.
  - NODE_ANALYSIS reads the in-leg Link's. A 58–59 approach row's speed moves to its Link on read,
    and a stray or conflicting one is refused.
  - `LinkType.freeSpeed` stays independent (Macro, Meso). Micro does not read it yet; D381's
    "Micro default" is its own slice. UTDF's Links and Lanes will read it.
- **Analysis panel (J7, NODE_ANALYSIS A9's first half):** the Nodes tab's *Analysis…* edits
  `Node::analysis` per turn (named by direction and L/T/R from §6) in one Command; *Analyse* runs
  `src/analysis/` on the UI thread and fills Results → *Node analysis* (lane group v, s, c, X,
  g/C, d, LOS, back of queue; approach and intersection d and LOS; refusals with locale strings,
  D383), with CSV through `csv_format.hpp` and the marker line.

## 10. Slices

| Slice | Scope | Sessions |
|---|---|---|
| SR | **Done (D389).** Road renamed Street segment: schema 59 writes `streetSegments`, `streetSegmentId` and a zone connector's `streetSegmentId`/`streetSegmentDirection`, and still reads `roads`/`roadId`/`roadDirection` below 59, each version refusing the other's keys; codes `ROAD_*` became `STREET_SEGMENT_*` (`EDIT_STREET_SEGMENT_*`, `SEAM_STREET_SEGMENT_*`, …); types, files, commands, tests, locale keys and strings (en "Street segment", th "ช่วงถนน"); maintained docs. RECORD, PROGRESS history and archives keep "Road" | 1 |
| U0 | UTDF probe with the owner's Synchro 12 and Vistro 2026 (UTDF_EXPORT §8) | 1 + owner |
| J1 | **Done (D390).** Posted speed on the Link (§9), schema 60 | 1 |
| J2 | **Done (D391, D392).** Kerb-side pocket; `approachLayout` and movement names (§6) | 1 |
| J3 | **J3a done (D393–D396):** catalog, loader, `Node::junction`, directions, JT5; stop for review. **J3b done (D398–D402):** `junction` fill, pockets, cuts, Connectors, right-of-way, JT6–JT8/JT13–JT15; all-way-stop generation deferred (D399); stop for review (§3–§5) | 2 |
| J4 | **Done (D404, JT9 passing).** Signals from the template: P1–P4, NEMA numbering, required site timing and `nemaCheck` (§8); stop for review | 1 |
| J5 | **Approved at d5e2bbe (D406–D408, JT10 passing).** Upstream transitions and the Transition-only selector (§7) | 1 |
| J6 | **Approved at 3ec3d39/09b0689 (D409–D414):** template catalog/Node forms, crossing column, seam/Readiness, explicit repair and corridor UI. B35/B36, compact previews; JT11/JT16 **passing**; [detailed contract/status](JUNCTION_TEMPLATE_EDITOR.md) | 1 |
| J6a | **Implemented, awaiting review (D415):** fix endpoint conversion for ordinary Fill/corridor exit lanes; remove D413's refusal. JT17 **passing** | 1 |
| J7 | Analysis panel and results (§9) | 1–2 |
| J4T | **Implemented, awaiting review** (D405/D416): protected signalised T, compass NEMA subset, template T cuts, JT18/JT19; separate review | 1 |
| U1–U3 | UTDF writer and round trip (UTDF_EXPORT §7) | 3–4 |

U0 waits on the owner, and its answers can change J4 and U1. About 14–17
sessions in all; one slice per session, each starting with its rows.

## 11. What stays out

Roundabouts, actuated control and coordination (NODE_ANALYSIS A7, A8); channelised free kerb turns
(slip lanes) and bus bays; five or more legs; pedestrians (ROADMAP "Later"); Thai cross-section
templates, which wait for the owner's standard and edition (OWNER_SITTING B12).

## 12. Owner choices

**Answered 2026-10-28 (D387, "เอาตามแนะนำ"):** templates on Nodes from crossing Street segments instead of a
builder; the full rename before M14 code (to Street segment, D389); transitions align per template.

**Answered 2026-10-28 (D388, "B30 เอาตามแนะนำ"): JT-1a to JT-5a**, so §3–§9 state the chosen options:
1. **JT-1 Shipped junction templates.**
   - **(a) A small structural set (P1–P4 four-leg, two-way stop three- and four-leg, with and
     without far-turn pockets), carrying no site numbers (recommended).**
   - (b) None: every project makes its own.
2. **JT-2 Legs.**
   - **(a) 3 or 4 legs, directions by nearest compass point with a per-Node override (recommended).**
   - (b) Up to 8 legs with diagonal directions now.
3. **JT-3 Asymmetric junctions.**
   - **(a) Roles apply to both legs of a role; a per-leg lane override on the Node covers a site
     whose two major legs differ (recommended).**
   - (b) A template states every leg separately (by position), which makes templates site-shaped.
4. **JT-4 All-way stop (target; generation deferred by B31, D399).**
   - **(a) Generated; Micro refuses it as F4 does (`FILL_NODE_ALL_WAY_STOP`), HCM waits for A5,
     UTDF exports it (recommended).**
   - (b) Not offered until Micro and A5 support it.
   **B31, 2026-10-29 (D399):** keep the fill listed but disabled; generation waits until A5 or
   U1 needs it. StopControl stays unchanged; no schema 63.
5. **JT-5 Yellow and all-red.**
   - **(a) Required per Node, no default (recommended):** no cited default exists in the project.
   - (b) 3 s and 1 s as editable defaults, recorded as unsourced.

## 13. Acceptance rows (implemented and planned)

| Row | Slice | Asserts |
|---|---|---|
| JT1 | SR | A street-segment document saves at 59 with only the new keys and reads back equal; the same document as a 58 file reads to it; each version refuses the other's keys at their paths; a 58 file's malformed entry is named by its own key; owned templates alone stay at 53 (`tests/street_segment_schema_tests.cpp`, **passing**) |
| JT2 | J1 | A Link's `speedLimit` round-trips at 60; absent keeps the shipped projects' bytes; 59 refuses a Link speed and 60 an approach speed; a 58 file's approach speeds move to their Links with the same queues, and a stray or conflicting one is refused; Node analysis reads the in-leg's; a template speed reaches generated Links; a split keeps it (`tests/link_speed_tests.cpp`, **passing**) |
| JT3 | J2 | Kerb/far pockets on both `drivingSide`s preserve original lane footprints, routes and heads (including taper/boundary heads); storage/taper read back over a bend and a later split; hand-drawn 1→2 remains ambiguous; a pocket stops at a Node-owned Connector even straight through to the next junction; generated/refine, atomic refusals, save/reopen and one Undo/Redo (`tests/junction_layout_tests.cpp`, **passing**) |
| JT4 | J2 | `approachLayout` names all 12 four-leg movements on both `drivingSide`s; shared lane sets/widths, duplicate directions, compass ties, overrides and 45°/135° boundaries (`tests/junction_layout_tests.cpp`, **passing**) |
| JT5 | J3a | Every invalid template in §3 is refused on load with its path (installed/owned); catalog ordering/ownership, schema-61 floors and pre-version field paths, partial recipe/coarse round-trip, exact Macro/Meso compile parity for a coarse signalised Node with a banned turn before/after choice and Detach (both driving sides), detail refine, direction/split and one Undo/Redo (`tests/junction_template_tests.cpp`, `tests/junction_recipe_tests.cpp`, `tests/junction_command_tests.cpp`; **passing**) |
| JT6 | J3 | Two crossing Street segments, Find crossings with a template, Apply: `approachLayout` gives the template's lanes and the Node's storage back exactly, for left- and right-hand traffic; four-leg far/kerb pockets and skew three-leg roles (`tests/junction_fill_tests.cpp`, `tests/junction_fill_completion_tests.cpp`, **passing**) |
| JT7 | J3 | Each missing site value is listed with its code and not applicable; Undo restores every byte; *Apply again* after a person's edit is `JUNCTION_EDITED`; full-lane counts, schema-62 paths, manual refine/copy/split, generated replacement, S5 marking, rank fallback, named cut/reference refusals, guarded Street segment Apply template and Undo → cross-section → fill recovery (`tests/junction_fill_tests.cpp`, `tests/junction_fill_completion_tests.cpp`, **passing**; all-way-stop generation deferred) |
| JT8 | J3 | The applied two-way-stop junction runs in Micro (seed 42) with no refusal on both driving sides. Existing manual signal Commands, used only in this test (D397), let NODE_ANALYSIS find all 10 lane groups and 12 turns (`tests/junction_fill_run_tests.cpp`, **passing**); template signals are covered separately by JT9 |
| JT9 | J4 | P1–P4 on both driving sides, both major axes and pocket variants: exact entered timing, clearance/wait split, missing/grid/cycle refusals, broken barrier/ring/head and protected-lane codes; compiled HCM g/C, P3 protected result and Micro seed 42; generated/refine and safe replacement with full JSON Undo/Redo (`tests/junction_signal*_tests.cpp`, **passing**, 9 cases) |
| JT10 | J5 / J6 carry-over | 3→2 Transition row, upstream taper and kerb/far/mixed mapping on both driving sides; Find crossings → applyCrossings → F3 → 15/30/60 m tapers → Fill enlarges the generated polygon to contain Connector rails, with unchanged preview, save/reopen, re-apply and one exact JSON Undo/Redo; an edited polygon refuses atomically at its polygon path. At 60 m in left-hand traffic, a side road (-40,112) → (-40,12), mirrored for right-hand traffic, would add two Link legs: refuse at each captured Link without document/History/save-state changes; y=25 stays outside and fills with one Undo. Neighbour overlap keeps NODE_OVERLAP; its preview disables the offer, names the neighbour, and direct generation also refuses before publication, on both driving sides. Schema-63/older-field guards; missing/bounds/unused-site refusals; same-template lifecycle merge; generated replacement/refine, stationed-head protection; ordinary Micro compilation and existing HCM refusal. Required selector, Cancel and one Undo driven in Qt (`tests/transition_tests.cpp`, `tests/transition_reference_tests.cpp`, `tests/transition_ui_tests.cpp`; **passing**, 14 native cases plus UI on both sides) |
| JT11 | J6 | Catalog Add/Edit/role validation and owned capture; Node template/major/lanes/directions/pocket/timing forms preserve drafts, missing versus entered zero, Cancel and one Undo. B36 keeps ownership only for site edits, never resurrects it; taper entry still allows generated polygon enlargement. Refined detail, catalog/major/timing/pocket/transition changes and missing controls give advisory `SEAM_JUNCTION_DIFFERS` at all levels; explicit preview/Apply again/Detach preserve references or refuse. Optional crossing choice has no site defaults. Native/Qt both sides, forms in both locales **passing** ([J6 detail](JUNCTION_TEMPLATE_EDITOR.md)) |
| JT12 | J7 | The panel writes `Node::analysis` in one Undo step; *Analyse* matches `--node-analysis` byte for byte |
| JT13 | J3b | F3 and F4 (priority/signalised with a banned turn), both driving sides, and Street segment re-apply after F4 produce the same complete document as the frozen `33a856a5` implementations under the old/manual History path; compare against `generatedCommand=true`, including IDs, revision and flags (`tests/junction_fill_parity_tests.cpp`, **passing**) |
| JT14 | J3b review | F3 → F4 on a coarse Node → choose a recipe → change laneWidth → Apply template matches the complete pre-J3b document and one Undo/Redo on both driving sides, with and without recipe pockets (`tests/junction_fill_completion_tests.cpp`, **passing**) |
| JT15 | J3b review | Reopen without Undo, delete the junction's generated Links, Apply the changed cross-section and fill again: `checkLayout` passes on both driving sides, recipe survives, attached travel-time measurement is removed, and every Command has complete JSON Undo/Redo (`tests/junction_fill_completion_tests.cpp`, **passing**) |
| JT16 | J6 / J6a | Shared Links/pockets rebuild both ends and the complete three-junction chain on both sides; retained references survive reopen, edited neighbour refuses atomically, with preview/Cancel and full JSON Undo/Redo. Compact 3-Node/446-object preview exposes all ids on request. East exit 3 outgoing lanes rebuilds and validates (D415); 3 incoming lanes keeps `JUNCTION_LANE_COUNT`. Native/Qt **passing** (`tests/junction_editor_tests.cpp`, `tests/junction_ui_tests.cpp`) |
| JT17 | J6a | Exact polyline endpoint conversion; single-junction 3-lane-exit controls at x=0 and x=200; ordinary History Fill of -200/0/200 corridor and atomic cross-section rebuild, both driving sides, with valid whole document, unchanged preview, seam parity, save/reopen and exact JSON Undo/Redo. D413 refusal removed (`tests/junction_corridor_conflict_tests.cpp`, **passing**, 9 cases) |
| JT18 | J4T | Protected three-stage T, both sides/all stem directions, literal U0 compass check, required site timing/omitted phases, NEMA/refusal guards, HCM/Micro, drafts/Node form, generated/refine and full Undo/reopen ([contract](JUNCTION_SIGNALS.md); **passing**) |
| JT19 | J4T | T template full-width measuring extension, both sides, signals/two-way stop, original footprints/reference guards, legacy F4/JT13 unchanged and one Undo ([contract](JUNCTION_SIGNALS.md); **passing**) |
