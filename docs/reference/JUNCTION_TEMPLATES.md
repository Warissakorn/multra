# Junction templates and the HCM study workflow (M14, D386, D387)

Contract before code (D145). Written 2026-10-28 as a junction builder (D386), then
rewritten the same day at the owner's direction (D387, "เอาตามแนะนำ"): **no builder tool**. The
engineer draws Street segments; where they cross, *Find crossings* makes a Node, and a **junction template**
on that Node says which lanes serve which turns, the pockets and the control, from which the
Connectors, right-of-way and signals are generated. Where two Street segments meet end to end with different
cross-sections, a **transition template** joins them. The export is [UTDF_EXPORT](UTDF_EXPORT.md).
SR and J1 are implemented; J2's interfaces and geometry are described in §6 (D391, D392).
J3a adds the template catalog and stored recipes (D393–D395); fill and editor remain later slices.
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

- **`kind`:** `junction` or `transition` (§7); J3a loads junction entries. Transition structure/loader support is J5.
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
later entry's `.id`. The installed [12 structural templates](../../data/junction-templates/README.md)
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
  A manual choice clears `generated`; Detach drops only `junction` and also keeps `coarse`.
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
  `SEAM_JUNCTION_DIFFERS`, with *Apply again* (replaces `generated` objects) and *Detach* (drops
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
     for `allWayStop`.
  5. **Signals:** one controller named after the Node, a group per NEMA phase numbered by that
     phase (§8), timing from the Node's values, a head at the start of every Connector path.
- **Apply again** replaces only objects still `generated` and refuses, naming them, when a person
  has edited one (`JUNCTION_EDITED`). Undo restores every byte.

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

- **Today:** Macro and Meso join free ends within 2 m (STREET_SEGMENTS_IMPLEMENTED §19); Micro gets no
  Connector. *Find crossings* lists only places of three legs or more, and the two-leg merge
  (§20) joins only Street segments with the same template.
- **Proposed:** *Find crossings* also lists two Street segments meeting end to start whose templates or
  overrides differ, as a **Transition** row. Confirm makes a two-leg Node with a transition
  template; the same-template case keeps today's merge.
- **A transition template** (`kind: "transition"`) states, per direction:
  - `align`: `kerb` or `far`, the side on which lanes line up; lanes are added or dropped on the
    other side. **Chosen in the template, no project default** (D387);
  - the taper length is a site value on the Node (`junction.legs.<road>.taper`), required.
- **Apply** cuts the legs, adds the taper as Connectors from the dropped lanes merging into the
  last aligned lane (or the added lanes diverging from it), and one Connector per aligned lane.
  F4's merge areas apply. No HCM; UTDF exports the Node as a plain node between segments.

## 8. NEMA numbering of signal groups

UTDF and both target programs work in NEMA phases 1–8 (two rings, a barrier). MULTRA keeps its
model: **a group's number is its NEMA phase**, so no second signal model exists (rule 3). A
hand-made controller is renumbered in the existing dialog. `nemaCheck(controller, layout)`
(pure) names failures (`NEMA_*`), which reach UTDF as refusals; Micro and HCM do not read them:
- each lane's heads carry one protected group, and at most one permitted group;
- groups 1–4 are ring 1 and 5–8 ring 2, each ring in order within the cycle;
- both rings cross the barrier together (between 1–2/5–6 and 3–4/7–8);
- all-red per group is the gap from its amber's end to the next group of its ring.

| Preset | Ring 1 | Ring 2 | Far turns |
|---|---|---|---|
| P1 Two-phase | 2 \| 4 | 6 \| 8 | permitted, with the opposing through |
| P2 Protected far turns on the major | 1, 2 \| 4 | 5, 6 \| 8 | major leading protected; minor permitted |
| P3 Protected far turns, all legs | 1, 2 \| 3, 4 | 5, 6 \| 7, 8 | protected leading |
| P4 Split phasing on the minor | 2 \| 4, 8 sequential | 6 \| — | each minor approach alone |

Phases 2 and 6 serve the major, 4 and 8 the minor; for left-hand traffic the far turn takes the
left-turn numbers. U0 confirms what both programs read (UTDF_EXPORT §6).

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
| J3 | **J3a done (D393–D396):** catalog, loader, `Node::junction`, directions, JT5; stop for review. J3b: `junction` fill, pockets, cuts, Connectors, right-of-way, JT6–JT8; stop for review (§3–§5) | 2 |
| J4 | Signals from the template: presets, NEMA numbering, timing, `nemaCheck` (§8) | 1 |
| J5 | Transitions (§7) | 1 |
| J6 | Template editor, *Find crossings* column, *Junction template…*, `SEAM_JUNCTION_DIFFERS` | 1 |
| J7 | Analysis panel and results (§9) | 1–2 |
| U1–U3 | UTDF writer and round trip (UTDF_EXPORT §7) | 3–4 |

U0 waits on the owner, and its answers can change J4 and U1. About 13–16
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
4. **JT-4 All-way stop.**
   - **(a) Generated; Micro refuses it as F4 does (`FILL_NODE_ALL_WAY_STOP`), HCM waits for A5,
     UTDF exports it (recommended).**
   - (b) Not offered until Micro and A5 support it.
5. **JT-5 Yellow and all-red.**
   - **(a) Required per Node, no default (recommended):** no cited default exists in the project.
   - (b) 3 s and 1 s as editable defaults, recorded as unsourced.

## 13. Acceptance rows (planned)

| Row | Slice | Asserts |
|---|---|---|
| JT1 | SR | A street-segment document saves at 59 with only the new keys and reads back equal; the same document as a 58 file reads to it; each version refuses the other's keys at their paths; a 58 file's malformed entry is named by its own key; owned templates alone stay at 53 (`tests/street_segment_schema_tests.cpp`, **passing**) |
| JT2 | J1 | A Link's `speedLimit` round-trips at 60; absent keeps the shipped projects' bytes; 59 refuses a Link speed and 60 an approach speed; a 58 file's approach speeds move to their Links with the same queues, and a stray or conflicting one is refused; Node analysis reads the in-leg's; a template speed reaches generated Links; a split keeps it (`tests/link_speed_tests.cpp`, **passing**) |
| JT3 | J2 | Kerb/far pockets on both `drivingSide`s preserve original lane footprints, routes and heads (including taper/boundary heads); storage/taper read back over a bend and a later split; hand-drawn 1→2 remains ambiguous; a pocket stops at a Node-owned Connector even straight through to the next junction; generated/refine, atomic refusals, save/reopen and one Undo/Redo (`tests/junction_layout_tests.cpp`, **passing**) |
| JT4 | J2 | `approachLayout` names all 12 four-leg movements on both `drivingSide`s; shared lane sets/widths, duplicate directions, compass ties, overrides and 45°/135° boundaries (`tests/junction_layout_tests.cpp`, **passing**) |
| JT5 | J3a | Every invalid template in §3 is refused on load with its path (installed/owned); catalog ordering/ownership, schema-61 floors and pre-version field paths, partial recipe/coarse round-trip, exact Macro/Meso compile parity for a coarse signalised Node with a banned turn before/after choice and Detach (both driving sides), detail refine, direction/split and one Undo/Redo (`tests/junction_template_tests.cpp`, `tests/junction_recipe_tests.cpp`, `tests/junction_command_tests.cpp`; **passing**) |
| JT6 | J3 | Two crossing Street segments, Find crossings with a template, Apply: `approachLayout` gives the template's lanes and the Node's storage back exactly, for left- and right-hand traffic |
| JT7 | J3 | Each missing site value is listed with its code and not applicable; Undo restores every byte; *Apply again* after a person's edit is `JUNCTION_EDITED` |
| JT8 | J3 | The applied junction runs in Micro (seed 42) with no refusal, and NODE_ANALYSIS finds every lane group |
| JT9 | J4 | Each preset passes `nemaCheck`; a broken barrier, ring order or head group is named; HCM g/C per lane group equals the Node's greens over the cycle |
| JT10 | J5 | A 3-lane Street segment ending into a 2-lane Street segment gives a Transition row; Apply with `align: kerb` drops the far lane over the taper; the same-template pair still merges |
| JT11 | J6 | Editing a generated Connector gives `SEAM_JUNCTION_DIFFERS`; *Apply again* and *Detach* each fix it in one Undo step |
| JT12 | J7 | The panel writes `Node::analysis` in one Undo step; *Analyse* matches `--node-analysis` byte for byte |
