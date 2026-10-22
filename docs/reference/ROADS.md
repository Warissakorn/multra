# Roads, cross-section templates and Nodes from crossings (S3)

Written 2026-10-22 (D303) as the S3.1 contract, docs only. The owner answered its seven choices
the same day, "as recommended": 1a, 2a, 3a, 4a, 5a, 6b, 7a (D304), so §3–§9 state the chosen
options. S3.2 is implemented (D304, §14), and so is S3.3 (D305, §15). It fixes:
- the Road object, and how it relates to the directed Links Micro already runs;
- cross-section templates and *Apply template*;
- Nodes from Road crossings;
- S0.4's coarse Node values (moved here by D272, [NODES](NODES.md) §5);
- seam checks;
- link types whose values are cited from HCM 7th edition.

Rule 4 applies: every result carries the not-validated marker. Cited link-type values are inputs,
not validation.

## 1. Scope

- **In S3:**
  - the Road, the cross-section template catalog, *Apply template*, *Join opposite Links into a
    Road*, *Assign template*, and detaching a Link from its template;
  - Nodes from crossings, with *Merge nodes*;
  - the S0.4 coarse values with refine and derive-back, the lifecycle and the two-leg merge;
  - seam checks;
  - an HCM-cited link-type catalog;
  - the Macro and Meso compilation of Roads and coarse Nodes.
- **Out:**
  - imports (CSV, then GeoJSON and OSM files, DXF on demand) and control points: M10 (WORKFLOW §10);
  - the readiness panel, *Fill defaults* and Node proposals: S4;
  - Micro on part of the network and the run-time cut: S5;
  - turn pockets and approach widening belong to the Node (WORKFLOW §4), so they are not a
    cross-section field.

### Already decided (cited, not asked again)

| Item | Decision | Where |
|---|---|---|
| Road and template objects | Road = centreline + `crossSectionId` + overrides; templates in `data/cross-sections/` with directions, lanes per direction, widths, median, shoulders, `linkType`, source | WORKFLOW §2 (D175) |
| Template role | A creation tool first: *Apply template* rewrites both directions in one Undo step; a hand-edited Link is detached and shown as such | WORKFLOW §4, answer 4 |
| Crossings | Nodes from Road crossings on the same level, each confirmed junction or overpass; a split updates Routes, ZoneConnectors and OD in one Undo step; *Merge nodes* | WORKFLOW §4 |
| Snap tolerance | ~8 px while drawing, 1–2 m on import, both adjustable | WORKFLOW §7, answer 8 |
| Coarse values | On a Node without detail only: turns differing from the default, control, major/minor, stage signal; priority by `linkType` hierarchy; refine deletes them, deleting detail derives them back, each in one Command | WORKFLOW §3, answer 9; NODES §5; D260, D272 |
| Seams | Lane counts offer *use Link / use Node*; speed jumps warn; severity depends on the level run; automatic adjustment only to defaults, inside the Command that caused it | WORKFLOW §6 |
| Values | No value ships without a source | WORKFLOW §1.4; D267, D272 (2a) |
| Thai cross-sections | Department of Highways and Department of Rural Roads standards; the editions are open | WORKFLOW §7; OWNER_SITTING B12 |
| Micro and Roads | A Road Micro cannot run is left out of its run and reported as a seam | WORKFLOW §11 |

## 2. Today (measured from the code on 2026-10-22)

- **`Link`** (`src/model/network/network.hpp:17`):
  - one direction, with N `Lane{id, width}`;
  - a reference polyline, `laneOffset`, `level`, `displayType`, `boundaryMarkings` and
    `linkTypeId` (schema 50, read only by Macro and Meso);
  - no speed, no modes and no reference to an opposite Link. Direction is the polyline's order.
- **Opposite Links:** `oppositeLink` (`src/commands/network_commands.cpp:110`) generates a
  reversed, offset copy (`2·width + 2·gap`). Nothing links the pair afterwards.
- **`Node{id, name, polygon, generated}`** references nothing. Membership is derived
  (`nodeMembership`), and *Suggest nodes* draws hulls around conflict clusters (`nodeSuggestions`).
- **Schema:**
  - the version written follows a presence chain (`src/project/document.cpp:149`), from 48 (nodes)
    to 52 (Meso fields);
  - the reader accepts up to 52 (`document.cpp:191`);
  - an older file using a newer key is refused with `EDIT_UNSUPPORTED_FIELD`
    (`rejectLinkTypesBefore50` and the like).
- **Link types** (`src/model/demand/definition.hpp:40`): `{id, name, capacityPerLane, freeSpeed,
  vdfId, modes, source, jamDensity?}`, with no rank. `resolveLinkTypes`
  (`src/project/link_type_codec.cpp:72`) reads the project's types, or else `data/link-types/`,
  which does not exist: none ships (D273).
- **Missing:** a Road, a template, a hierarchy and any importer.
- **Reusable:**
  - geometry: `offsetGeometry`, `laneGeometry`, `replaceLaneBundle`, `trimSelfIntersections`;
  - edits: `addLink`, `splitLink` (remaps routes and heads), `oppositeLink`, `reanchorConnectors`;
  - Nodes: `suggestNodes`, `nodeMembership`;
  - commands: `History::execute`, one named Undo step.

## 3. The Road (choice 1a)

```json
{ "id": "r1", "name": "Rama IV", "centreline": [[0,0],[400,0]], "crossSectionId": "urban-4-divided",
  "level": 0, "overrides": { "lanesForward": 3 }, "links": ["r1-f", "r1-b"] }
```

- **Stored in `network.roads`, at schema 53:**
  - the centreline, digitised in the forward direction;
  - `crossSectionId` and `level`;
  - `overrides`: any template field except `source`;
  - `links`: the ids of its generated Links, forward first, empty until applied.
- **Generated Links stay ordinary Links**, so Micro reads them unchanged and every existing project
  keeps its bytes. Each carries an optional `roadId` (schema 53), and the reverse direction is
  reversed.
- **Apply template** (one Undo step):
  - each direction's Link sits at its offset from the centreline: half the median plus the lanes;
  - lane widths come from the template, `linkTypeId` from it, and `boundaryMarkings` are default.
  - Re-applying replaces the Links in place (`replaceLaneBundle`). Connectors are reanchored, and
    routes keep their Link ids.
- **Detached:** a Link whose lanes or geometry are edited by hand loses `roadId` in that Command,
  and the Road drops it from `links`. The editor shows the Road as partly detached; nothing is
  regenerated silently.
- **Join opposite Links into a Road** (Micro-first studies): two Links of opposite direction on the
  same level, roughly parallel within a tolerance, become one Road. Its centreline is the mean of
  their centrelines, it has no template (`crossSectionId` absent), and they are its `links`.
- **Assign template** sets `crossSectionId` on several Roads in one Undo step without generating
  Links. Generating is a separate *Apply*, so a Micro study is never rewritten by an assignment.
- **Refusals:**
  - a centreline with fewer than two distinct points is `ROAD_GEOMETRY_INVALID`;
  - an unknown template is `ROAD_CROSS_SECTION_UNKNOWN`;
  - a `links` entry naming a missing Link, or a Link whose `roadId` disagrees, is
    `ROAD_LINK_MISMATCH`.

## 4. Cross-section templates (choice 2a)

```json
{ "id": "urban-4-divided", "name": "Urban, 2+2 lanes, divided", "directions": 2,
  "lanesForward": 2, "lanesBackward": 2, "laneWidth": 3.25, "median": 2.0, "shoulder": 0.5,
  "linkType": "urban-arterial", "source": "…" }
```

- **Where they live:** `data/cross-sections/`, under the Demand catalog's presence rule (D269), as
  link types are. A project-owned template is written at schema 53.
- **Fields:**
  - `directions` (1 or 2);
  - lanes per direction (`lanesForward`, and `lanesBackward` when two-way);
  - `laneWidth`, or `laneWidths` per lane, m;
  - `median` and `shoulder`, m, 0 allowed;
  - `linkType`, optional, naming a link type;
  - `source`, a non-empty citation.
- **Errors:** a bad field is `INVALID_CROSS_SECTION` at its path; a malformed file is
  `EDIT_CATALOG_READ`.
- **`shoulder` is dropped (D305)** until something reads it; neither Micro nor *Apply* uses it.
- **Shipped:** none (2a). The Thai editions are open (B12). The editor offers project-owned
  templates, and a study states its own source.

## 5. Nodes from crossings (choice 5a)

- **Find crossings** is a command on demand, not on every edit. It lists every point where two
  Roads' centrelines cross, or an endpoint lies within the snap tolerance, on the same `level`.
  Roads on different levels never meet (WORKFLOW §4).
- **The batch dialog** shows each crossing, with *junction* as the default and *overpass* as the
  alternative. Confirming applies every choice in **one Undo step**. For each junction:
  - each Road is split at the crossing. The Road is split, and its generated Links are split by
    `splitLink`, which remaps routes and heads; ZoneConnectors and OD keep their zone ids;
  - a Node is created whose polygon is a disc of the cross-section's half-width plus a margin.
    It is `generated` until edited, as *Suggest nodes* (S0.5) makes them.
- **An overpass** is remembered as a pair of Road ids in `network.overpasses` (schema 53), so the
  next *Find crossings* does not ask again.
- **Merge nodes** (for roundabouts) joins selected Nodes into one polygon, the hull of theirs, in
  one Undo step. A Node with detail inside is merged only if the result passes `nodeMembership`;
  otherwise it is refused with that check's issues.

## 6. S0.4 coarse values (choices 3a and 4a)

```json
{ "id": "n1", "polygon": [...], "coarse": { "control": "signalised", "major": ["r1", "r1~2"],
  "bannedTurns": [["r2", "r1~2"]], "signal": { "cycle": 90, "stages": [ { "green": 40, "turns": [...] } ] } } }
```

- **Stored only on a Node without detail** (no Link or Connector inside its polygon), at
  schema 53:
  - `control`: `priority`, `signalised` or `allWayStop`;
  - `major`: the leg Roads that are major, absent meaning by hierarchy;
  - `bannedTurns`: the in→out Road pairs not allowed, beyond the U-turns that are never allowed;
  - `signal`: `{cycle, stages[{green, turns}]}`, required when `control` is `signalised`.
  - Defaults are absent fields (NODES §5).
- **Legs** are the Roads that end at the Node, named by Road id and direction.
- **Priority by hierarchy (4a):** link types gain an optional integer `rank`, where a higher rank
  is major.
  - The legs whose type has the highest rank are major.
  - A tie, or a leg without a rank, leaves major and minor undetermined. Macro and Meso then
    report `NODE_PRIORITY_UNDETERMINED` as an advisory, not a refusal, because neither yet
    reads priority (D297, WORKFLOW answer 10).
- **Readers, so no field is stored before its reader:**
  - Meso compiles a coarse Node as a Meso node with every allowed turn. A `signalised` Node gives
    each turn its stages' g/C, `Σ green / cycle` over the stages that serve it (the averaged green
    of D299). Turns have zero free time (D301), since there is no geometry.
  - Macro compiles the allowed turns as connector arcs. Banned turns are absent.
- **Refine and derive back** (NODES §5): drawing the first Link or Connector inside a coarse Node
  deletes `coarse` in that Command. Deleting the last detail first derives `coarse` from it:
  - control from heads, or priority;
  - major from the right-of-way rules;
  - banned turns from the missing chains;
  - a signal from the program's groups mapped to stages.
- **Lifecycle:**
  - a dead-end Node stays;
  - a two-leg Node with no `coarse` and identical cross-sections on both legs is merged away,
    joining the two Roads, by the Command that made it so;
  - a Node with detail or `coarse` is never removed, only reported.
- **Refusals:**
  - `coarse` on a Node with detail is `NODE_COARSE_WITH_DETAIL`;
  - a `bannedTurns` or `major` entry naming no leg is `NODE_COARSE_UNKNOWN_LEG`;
  - a signal without stages, or a green total above the cycle, is `NODE_COARSE_SIGNAL_INVALID`.

## 7. Seam checks

- **Run on demand** (and later by S4's readiness panel). Each is one issue at the object's path,
  and its severity depends on the level being run (WORKFLOW §6):

  | Check | Micro | Meso, Macro |
  |---|---|---|
  | A Road without generated Links | `SEAM_ROAD_NOT_GENERATED`: advisory; left out of the run | Compiled from the Road (§9) |
  | A generated Link's lane count differs from the Node's leg | `SEAM_LANE_COUNT`, offering *use Link* or *use Node*, one Undo step | Advisory |
  | A free-speed jump above 20 km/h between consecutive links | Warning only | Warning only |
  | A Link detached from its Road | Advisory | Advisory |

- **No silent adjustment:** a seam fix changes only defaults, inside the Command the user chose.
- The 20 km/h threshold is a display choice, not a model value, and can be changed in settings.

## 8. HCM-cited link types (choice 6b)

Read in the owner's Markdown copy of HCM 7th edition (`Warissakorn/highway-capacity-manual-7edition-markdown`
@ `74f4bc3`), with page numbers of the source PDF.

| Value | HCM | Page |
|---|---|---|
| Basic freeway capacity: `c = 2,200 + 10(FFS − 50)`, `c ≤ 2,400` pc/h/ln, 55 ≤ FFS ≤ 75 mi/h; tabulated 2,250 / 2,300 / 2,350 / 2,400 / 2,400 at FFS 55 / 60 / 65 / 70 / 75 | Exhibit 12-4, Exhibit 12-6 | 818–819, 822 |
| Multilane highway capacity: `c = 1,900 + 20(FFS − 45)`, `c ≤ 2,300` pc/h/ln, 45 ≤ FFS ≤ 70 mi/h; tabulated 1,900 / 2,000 / 2,100 / 2,200 / 2,300 / 2,300 at FFS 45 … 70 (65 and 70 extrapolated, not field data) | Exhibit 12-4, Exhibit 12-6 | 818–819, 822 |
| Base saturation flow at a signalised intersection: 1,900 pc/h/ln (metro population ≥ 250,000), otherwise 1,750 | Exhibit 19-11 | 1430 |
| Jam density: default 190 pc/mi/ln, range 150–270 | Exhibit 10-7 | 657 |
| Urban street base free-flow speed: computed from the speed limit and segment inputs, no single value | Chapter 18, §3 | — |

- **The capacity question.** BPR's 0.15 and 4 were fitted to the 1964 *practical* capacity, and
  `capacityPerLane` is the capacity the link's VDF uses (MACRO_NETWORK §4). HCM's values are base
  (ultimate) capacities and saturation flows. They suit Meso's `q_max` and Micro checks, but not
  BPR.
- **Option 6b (recommended):** link types gain `capacityBasis`, `practical` or `ultimate`, and VDFs
  gain the basis they were fitted to (`bpr` is `practical`).
  - Macro refuses a link whose type's basis differs from its VDF's, with
    `MACRO_CAPACITY_BASIS_MISMATCH`.
  - Meso reads any basis.
  - The shipped types are `ultimate`.
- **Shipped under 6b**, in `data/link-types/`, each citing its exhibit:
  - `freeway-55` … `freeway-75` and `multilane-45` … `multilane-70`;
  - each has the tabulated capacity, `freeSpeed` the FFS converted exactly (1 mi/h = 0.44704 m/s),
    `jamDensity` 190 pc/mi/ln = 118.06 pc/km/ln, `vdfId` `bpr` and `capacityBasis` `ultimate`;
  - Macro therefore refuses them until an `ultimate` VDF is cited, as intended.
- **Not shipped:** urban street types. Their free speed is site-specific (Ch. 18), so the editor
  offers a preset with 1,900 or 1,750 pc/h/ln and the jam density, where the user enters the speed
  and the source becomes "HCM 7 Exhibit 19-11 and Exhibit 10-7; speed: user".
- **Units:** pc against PCU. HCM's passenger-car equivalents are taken as PCU, as D272 shares
  capacity in PCU. Base conditions assume no heavy vehicles, and the source says so.

## 9. Compile seams

- **Macro and Meso** (`compileMacroNetwork`, `compileMesoNetwork`):
  - a Road with generated Links compiles from those Links, as today;
  - a Road without them compiles as one directed link per direction from its template's lanes,
    `linkType` and the centreline length, split at its Nodes.
- **A coarse Node** compiles as in §6. A detailed Node compiles as today (D293, S2).
- **Micro** runs Links only. A Road without Links is `SEAM_ROAD_NOT_GENERATED`; its demand is
  refused at the route as today, so nothing is silently dropped.
- **Byte identity:** a project without Roads, templates, `roadId`, overpasses, `coarse`, `rank` or
  `capacityBasis` keeps its schema and bytes (row RD1).

## 10. Sources

- **HCM 7th edition**, read in the owner's copy (§8), Exhibits 10-7, 12-4, 12-6 and 19-11. Its
  values are national US norms for base conditions (Exhibit 12-4's text).
- **Thai standards** (Department of Highways, Department of Rural Roads): not read. The editions
  are open (B12).
- **MULTI_LEVEL_WORKFLOW** §2–§7 and §9–§11, NODES §5 and MACRO_NETWORK §4, for the decisions
  cited in §1.

## 11. Owner choices (answered 2026-10-22, D304: 1a–5a, 6b, 7a)

1. **The Road's storage.**
   - **(a) Road plus its generated Links, stored, linked by `roadId` (recommended).** Micro is
     unchanged, and *Join opposite Links* reuses existing studies.
   - (b) A centreline-only Road, from which Links are derived on every load. Simpler storage, but
     every hand edit to a Micro Link would have to be an override.
2. **Templates shipped.**
   - **(a) None until B12 names the Thai editions (recommended).**
   - (b) Generic templates at HCM's 12 ft base lane width.
3. **Coarse values' readers.**
   - **(a) Meso and Macro both read them in S3 (recommended),** so no field is stored before its
     reader.
   - (b) Meso only, with Macro later.
4. **Priority hierarchy.**
   - **(a) An optional `rank` on link types; ties give `NODE_PRIORITY_UNDETERMINED`
     (recommended).**
   - (b) A rank on templates. It needs a template on every Road.
5. **Crossings.**
   - **(a) Find crossings on demand, with a batch dialog and one Undo step (recommended).**
   - (b) Detection on every edit, with a prompt each time.
6. **HCM link types.**
   - **(b) Ship the freeway and multilane types as `ultimate`, with a capacity basis that Macro
     checks (recommended).**
   - (a) Ship them for every level with BPR, ignoring the basis. MACRO_NETWORK §4 warns against
     this.
   - (c) Ship none, and offer every HCM value as an editor preset.
7. **Slice order.**
   - **(a) S3.2 HCM catalog and `capacityBasis`/`rank` → S3.3 Road and templates (codec, *Apply*,
     detach) → S3.4 *Join opposite Links* and *Assign template* → S3.5 crossings, Nodes and *Merge
     nodes* → S3.6 coarse values and their compilation → S3.7 seam checks (recommended).** It
     starts with the smallest piece.
   - (b) The Road first, and the catalog last.

## 12. Slices

| Slice | Content | Rows |
|---|---|---|
| S3.1 | This contract. **Written (D303)** | — |
| S3.2 | `data/link-types/` from HCM; `capacityBasis` on types and VDFs. **Implemented (D304)**. `rank` moved to S3.6 with its reader (D304) | RD1 (basis), RD2, RD3 |
| S3.3 | `network.roads`, `data/cross-sections/`, `roadId`; *Apply template*; detach. **Implemented (D305)**: model, codec and commands | RD1, RD5–RD9 |
| S3.3b | The editor surface (D305): a Road tool, a Cross-sections tab, *Apply template*, and the detached display | written with S3.3b |
| S3.4 | *Join opposite Links into a Road*; *Assign template* | RD10, RD11 |
| S3.5 | *Find crossings*, overpasses, splits, generated Nodes, *Merge nodes* | RD12–RD15 |
| S3.6 | `coarse`, refine and derive back, lifecycle; the link-type `rank`; Macro and Meso compile coarse Nodes and Roads | RD4, RD16–RD20 |
| S3.7 | Seam checks | RD21, RD22 |

## 13. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| RD1 | Byte identity | Every shipped project and fixture keeps its schema and bytes; a Road, template, `roadId`, overpass, `coarse`, `rank` or `capacityBasis` makes the file 53, and each key below 53 is `EDIT_UNSUPPORTED_FIELD` at its path |
| RD2 | HCM catalog | Every shipped type parses; capacities equal Exhibit 12-4's table at each FFS; `freeSpeed` = FFS × 0.44704; `jamDensity` 118.06 ± 0.01; each `source` names its exhibit and page |
| RD3 | Capacity basis | Macro on a shipped `ultimate` type with `bpr` gives `MACRO_CAPACITY_BASIS_MISMATCH` at the link; a project-owned `practical` type runs as before; Meso runs on both |
| RD4 | Rank | Two legs of rank 2 and two of rank 1 give the rank-2 legs as major; a tie gives `NODE_PRIORITY_UNDETERMINED` |
| RD5 | Template codec | A project-owned template round-trips at 53; each bad field gives `INVALID_CROSS_SECTION` at it; absent means the installed directory |
| RD6 | Apply template | A two-way 2+2 template on a straight centreline gives two Links, reversed, at offsets ±(median/2 + lane widths), with the template's widths and type, in one Undo step; Undo restores the bytes |
| RD7 | Re-apply | Changing the template's lanes and re-applying keeps the Link ids, reanchors Connectors and keeps routes valid |
| RD8 | Detach | Editing one generated Link's lanes clears its `roadId` and drops it from the Road's `links` in the same Command |
| RD9 | Road refusals | `ROAD_GEOMETRY_INVALID`, `ROAD_CROSS_SECTION_UNKNOWN` and `ROAD_LINK_MISMATCH`, each at its path |
| RD10 | Join opposite | Two opposite Links made by `oppositeLink` join into one Road whose centreline lies midway; non-parallel or same-direction Links are refused |
| RD11 | Assign template | Assigning to three Roads is one Undo step and generates no Link |
| RD12 | Find crossings | A grid of two Roads by two on one level gives four crossings; one Road on level 1 gives none with it |
| RD13 | Junction split | Confirming one junction splits both Roads and their Links, remaps a route across it, creates one `generated` Node, all in one Undo step |
| RD14 | Overpass | An overpass is stored and not offered again; Undo removes it |
| RD15 | Merge nodes | Four generated Nodes of a roundabout merge into one hull; a merge whose result fails `nodeMembership` is refused with its issues |
| RD16 | Coarse codec | `coarse` round-trips at 53; on a Node with detail it is `NODE_COARSE_WITH_DETAIL`; unknown legs and bad signals give their codes |
| RD17 | Refine | Drawing a Connector inside a coarse Node deletes `coarse` in that Command; Undo restores it |
| RD18 | Derive back | Deleting the four-leg fixture's detail derives `control` `signalised`, its stages' greens and its banned turns, in that Command |
| RD19 | Meso and Macro from coarse | A coarse signalised four-leg compiles in Meso with 12 turns and their stages' g/C, and in Macro with 12 connector arcs; a banned turn is absent from both |
| RD20 | Roads without Links | A Road network without generated Links compiles in Macro and Meso with one link per direction split at Nodes; Micro reports `SEAM_ROAD_NOT_GENERATED` and refuses its routes |
| RD21 | Lane-count seam | A 3-lane Link into a 2-lane leg gives `SEAM_LANE_COUNT`; *use Link* and *use Node* each fix it in one Undo step |
| RD22 | Speed seam | A 100 → 60 km/h jump warns at every level and refuses none |

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
- **Not yet:** the editor (S3.3b). Roads are authored by command only.
