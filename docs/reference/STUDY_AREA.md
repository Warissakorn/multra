# The study area, the run-time cut and stale marking (S5)

Written 2026-10-24 (D326) as the S5.1 contract, docs only. The owner answered its six choices in
chat the same day (§12): S5-1c, S5-2a, S5-3c, S5-4a, S5-5a, S5-6a, five slices. Two of them depart
from the recommendations:
- the study area is a set of Nodes, not a polygon;
- both cuts, from Macro and from Meso, are in S5.

S5.2a is implemented (D327, §16); the rest is not. The contract fixes, for WORKFLOW §5 steps 3, 4 and 7–8
([MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md)):
- which part of the network a Micro run keeps;
- where its boundary flows come from;
- when a result is stale;
- how an export says so.

It replaces M12's stored subarea cut (D175, [MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md)
§3.6).

Rule 4 applies: every result of every level carries the not-validated marker. A cut moves a
coarser model's flows to a finer one; it does not make either one right.

## 1. Scope

- **In S5:**
  - the study area as a set of Nodes;
  - Micro on the part of the network that set keeps, with gates at its open ends;
  - typed gate flows;
  - the Macro cut (path flows from Frank–Wolfe) and the Meso cut (path counts at the gates);
  - Macro and Meso results saved beside the project with their provenance;
  - stale marking by SHA-256;
  - the export's marking line;
  - readiness warnings for `generated` objects inside the area.
- **Out:**
  - calibration against counts, ODME and the cross-level consistency report: M10, the ODME
    milestone and M12's remaining report (§13 marks what S5 already records for it);
  - a Meso benchmark (M11 gate);
  - Hybrid Meso–Micro, where flows feed back during a run (WORKFLOW §9 #14);
  - demand on a Road-only arm: zone connectors attach to Links only (booked in NEXT).
- **Already decided, not asked again:**
  - one document, cut at run time and never stored as a second file (D175, hard rule 3);
  - a boundary flow comes from a coarser result or is typed, and the result records which
    (WORKFLOW §9 #11);
  - coarser results are saved to file with provenance; a cache comes only if a cut is measured
    slow (WORKFLOW §8).

## 2. Today (measured from the code on D325's commit)

| Item | Today | Where |
|---|---|---|
| Micro's extent | The whole network of Links. A Road without Links is left out and reported (`SEAM_ROAD_NOT_GENERATED`) | `src/model/network/compile.cpp`, `src/model/demand/seam_checks.*` |
| Node contents | Derived on every read: the Links and Connectors inside it, its in- and out-legs, turns, conflicts and control | `nodeMembership`, `src/model/network/node_membership.hpp` |
| Macro result | `MacroAssignment` keeps link flows per class and the gap, but **no path flows** | `src/macro/assign.hpp` |
| Macro and Meso links | Each carries its `source` Link (or Connector) id and its stations on it | `src/macro/graph.hpp`, `src/meso/graph.hpp` |
| Meso result | Per-link cumulative curves, and per (class, path) `demand`, `entered` and `arrived`. Nothing per path at an inner link | `src/meso/loading.hpp` |
| Stale | `macroStale()` / `mesoStale()` compare an in-memory `revision`; nothing survives a reopen | `src/shell/editor_macro.cpp:38`, `editor_meso.cpp:33` |
| Hashes | `inputSha256` of exact bytes; `InputManifest` records the files a run read | `src/project/input_manifest.hpp` |
| Export marker | The first CSV line is `# MULTRA - not yet validated. …` | `src/project/batch_output.cpp:97` and the Macro and Meso CSVs |
| OD to Micro | A period's matrix becomes Micro inputs and routes through zone connectors (M8.3) | `src/project/od_demand.*` |

## 3. The study area (S5-1c)

- **Stored** in the zone layer as `"studyArea": {"nodes": ["<node id>", …], …}`, written only
  when the list is not empty, at schema 55. It is absent in every file today, so every shipped
  file keeps its bytes (SA1). Below 55 the key is `EDIT_UNSUPPORTED_FIELD`.
- **Refusals** at its path:
  - `STUDY_AREA_NODE_UNKNOWN`: an id that names no Node;
  - `STUDY_AREA_NODE_DUPLICATE`: an id listed twice.
- **Commands keep it whole:**
  - deleting a Node removes it from the list in the same Command;
  - *Merge nodes* replaces the merged ids with the result's id in the same Command;
  - a Command that empties the list removes `studyArea`.
- **Editing:** the Nodes tab gains a *Study area* column; ticking a row is one Undo step. The
  canvas outlines the area's Nodes. No new dialog: this is a column, not a dialog (D319).
- **No study area** means Micro runs the whole network, exactly as today (SA1).

## 4. The Micro cut (S5-2a)

- **The kept set:**
  - for each Node of the area, the Links and Connectors `nodeMembership` puts inside it, and the
    Links of its in-legs and out-legs;
  - every Link and Connector on a lane chain that joins an out-leg of one area Node to an in-leg
    of another area Node without passing through a Node outside the area (the mid-block pieces
    between two area junctions);
  - the Connectors whose two ends lie on kept Links.

  No Link is clipped: a kept Link runs whole, and geometry is never made at run time.
- **Refusals for a Micro run with a study area**, before compile, at `studyArea.nodes[i]`:
  - `MICRO_AREA_NODE_COARSE`: an area Node has no detail (*Fill defaults* F4, or drawing, first);
  - `MICRO_AREA_NODE_INVALID`: an area Node has membership issues (its codes follow);
  - `MICRO_AREA_EMPTY`: the kept set has no Link.
- **Gates:** where a lane chain enters the kept set, its first kept Link's start is an *entry
  gate*; where it leaves, its last kept Link's end is an *exit gate*. A gate is named by its Link,
  `gate:<link id>:in` or `gate:<link id>:out`. A zone connector on a kept Link stays a zone inside
  the area; gates and these zones are the run's origins and destinations.
- **Demand in an area run** is only the gate flows (§5–§7) between those origins and
  destinations. The document's vehicle inputs, routing decisions, authored routes and OD are not
  run; each one touching a dropped Link is listed once as the advisory `MICRO_AREA_INPUT_IGNORED`.
  This way the boundary flow has one owner. Routes inside the area come from the existing OD →
  routes expansion (M8.3) between the run's origins and destinations.
- **Leaving and coming back:** a path that leaves the kept set and enters it again makes two
  gate-to-gate trips, one per stay inside. Each kept stretch is a trip of its own.
- **Run-time copy:** the cut builds a document copy, holding only the kept set, run-time zones
  for the gates and the gate matrices, and compiles that copy. The copy is never saved and adds no
  Undo step. The Micro result's object ids are the original ones, so Results and the canvas need
  no mapping.

## 5. Typed gate flows

- **Stored** in `studyArea.gateFlows`: rows of
  `{"periodId", "from", "to", "compositionId" | "modeId", "vehiclesPerHour"}`. `from` and `to`
  are gate names or zone ids. It is used when `studyArea.flowSource` is `"typed"`.
- **Refusals:**
  - `STUDY_AREA_GATE_UNKNOWN`: a gate the kept set does not make, checked at run time, since gates
    are derived;
  - `STUDY_AREA_PERIOD_UNKNOWN`: an unknown period;
  - the matrix codes M8 already uses for a bad cell.
- **Editing:** an OD table for the gates, reusing the OD table's widget (M8.6b) with gate rows and
  columns.

## 6. The Macro cut (S5-3c, slice S5.3)

- **Path flows.** Frank–Wolfe moves the load as x^{k+1} = (1 − λ_k) x^k + λ_k y^k, where y^k is
  iteration k's all-or-nothing load (`assignMacro`, `src/macro/assign.hpp`). Keeping each
  all-or-nothing path set with its weight decomposes the final link flows exactly:
  - iteration j's paths carry weight λ_j Π_{i>j} (1 − λ_i);
  - the initial all-or-nothing's paths carry Π_i (1 − λ_i).

  `MacroAssignment` gains, per class and OD cell, the distinct paths (lists of Macro link
  indices) with their summed veh/h. The sum of the path flows over a link equals `classFlows` on
  it to rounding (SA7, a hand-computed two-route fixture). The memory this adds is measured on the
  M2.6 template's Macro run before S5.3 ships (rule: measure first).
- **The cut.** For the period of each Micro interval:
  - map each Macro link of `kind` link or connector to its `source`; a link is kept when its
    source is in the kept set;
  - each path's kept stretches (§4) give gate-to-gate (or zone) trips with that path's veh/h.

  Trips that share both ends and a class are summed into one gate matrix per period and class.
  Units stay veh/h per class (Macro's `classFlows`), so no PCU conversion is involved.
- **Periods:** a Micro interval takes the Macro run of the study period that covers it (S1's run
  clock). An interval with no saved Macro run for its period refuses the run with
  `MICRO_AREA_PERIOD_NOT_RUN` at the period.
- **A run that is not converged** may be cut. The cut carries Macro's `MACRO_NOT_CONVERGED`
  forward as an advisory, with the gap.

## 7. The Meso cut (S5-3c, slice S5.4)

- **Path counts at the gates.** `loadMeso` gains, for a set of watched links, the cumulative
  count per (class, path) at the link's entrance and exit. It is computed by FIFO on that link:
  a path's vehicles leave a link in the order they entered, so the path's share of the entrance
  curve maps to the exit curve by the time the link's aggregate curves give. The watched links
  are the kept set's entry-gate links and exit-gate links, so runs without a study area keep
  their bytes (SA1). Yperman 2007's link model is FIFO per link, which is what the mapping
  assumes. The section is to be cited from the owner's copy in S5.4 before code (BOOKS).
- **The cut.** Each kept stretch of a path gives a trip from its entry gate to its exit gate.
  Per Meso output slice, the trip's volume is the path's count increase at the entry gate's link
  entrance. PCU is turned back into vehicles by the class's PCU (Meso loads PCU, D300).
- **Time profile:** the gate matrices come one interval per Meso output slice (`mesoSettings`),
  so Micro's arrivals follow the slices. Whether to merge slices into coarser intervals is open
  (§13); no rule is invented here.
- **A cancelled Meso run** (`cancelled`) cannot be cut: `MICRO_AREA_SOURCE_INCOMPLETE`.

## 8. Saved results and provenance (S5-5a, slice S5.5)

- **Side files:** *Run Macro* writes `<project>.macro.json` (one entry per period, replacing that
  period's previous one), and *Run Meso* writes `<project>.meso.json`. The project file keeps its
  bytes.
- **Each side file records:**
  - `"format"` and its own schema;
  - `projectSha256`: `inputSha256` of the project bytes as `saveDocument` would write them, so an
    unsaved edit counts;
  - the run's settings, its gap and convergence (Macro) or its step and horizon (Meso);
  - the path flows (Macro) or the watched path counts (Meso);
  - the input manifest.
- **A Micro result from a cut** records:
  - `flowSource`: `macro`, `meso` or `typed`;
  - the source file's SHA-256 and period (or periods);
  - its own `projectSha256`.

  Its JSON output and CSV header carry them.
- **Untrusted input:** a side file is only read, never run. A malformed one is refused with
  `MICRO_AREA_SOURCE_INVALID` at its path, as any input file is. Its paths must name existing
  Macro or Meso links: the graph is compiled again from the document and compared.

## 9. Stale (S5-5a)

A result is **stale** when either of these holds:
1. its `projectSha256` differs from the current document's;
2. for a cut result, the source side file's SHA-256 differs from the one the result recorded,
   or the side file itself is stale by (1).

Both checks work after a reopen, which the in-memory `revision` cannot do. `macroStale()` and
`mesoStale()` move to rule (1); the revision stays only to skip re-hashing between edits. A Micro
run from a stale source runs, with the advisory `MICRO_AREA_SOURCE_STALE`: the study decides,
nothing is refused silently.

## 10. `generated` in the area (S5-4a)

- **Micro readiness** (the Problems tab, READINESS §3) lists one warning
  `MICRO_GENERATED_IN_AREA` per area Node holding `generated` objects, at `nodes[i]`, with the
  count by kind (Connectors, right-of-way objects, heads, controller). The run still starts.
- With no study area, the rule reads the whole network as the area, since Micro runs all of it.
- Selecting the row lights the Node, as the Nodes tab does (D288).

## 11. Export marking (S5-6a)

- A CSV export, and the TSV copy, gain a second header line right after the not-validated
  marker:
  `# MULTRA - generated objects in the study area: <n>; stale inputs: <list or none>.`
  It is written only when n > 0 or a list is not empty. Every other export keeps its bytes (SA1).
- JSON outputs gain `"marking": {"generatedInArea": n, "stale": [...]}` under the same rule.
- This covers single-run, batch, Macro, Meso and comparison exports. A comparison lists each
  side's marking.

## 12. Owner choices (answered 2026-10-24, D326)

1. **S5-1 What the study area is.**
   - (a) One polygon (recommended).
   - (b) Several named polygons.
   - **(c) A set of selected Nodes: chosen.** No geometry test; the Nodes already know their
     contents, legs and turns.
2. **S5-2 What Micro keeps.**
   - **(a) Every Link touching the area, with gates at the open ends, no clipping: chosen.** With
     Nodes (§4), "touching" means the Nodes' interiors, legs and the pieces between area Nodes.
   - (b) Links wholly inside.
   - (c) Clip at the boundary.
3. **S5-3 Where boundary flows come from.**
   - (a) A Macro path cut or typed (recommended).
   - (b) Typed only first.
   - **(c) The Macro cut, the Meso cut and typed flows, all in S5: chosen.**
4. **S5-4 `generated` in the area.**
   - **(a) Warning rows by Node; the run starts: chosen.**
   - (b) A refusal.
   - (c) One summary row.
5. **S5-5 Stale and storage.**
   - **(a) SHA-256 of the project bytes and the source, with Macro and Meso side files: chosen.**
   - (b) Revision only, in memory.
6. **S5-6 Export.**
   - **(a) A second header line only when it applies: chosen.**
   - (b) Refuse export.
7. **Slices: five (chosen)**, as in §14.

## 13. Open items (each needs a source or an answer before its slice)

- **Meso slices to Micro intervals** (S5.4): one interval per slice is the reading taken; merging
  them would need a rule from a source.
- **Rounding** of a cut gate matrix into Micro vehicles: M8.3's existing rule applies, unchanged.
  S5.3 checks that it does.
- **Gates on a Road-only arm:** a kept leg must be a Link (§4). An area Node with a Link-less arm
  gives `MICRO_AREA_NODE_COARSE`, or `SEAM_ROAD_NOT_GENERATED` for that arm.
- **M12's consistency report:** S5 records, per gate, the flow it was fed. Comparing that with
  Micro's measured throughput is M12's, not S5's.

## 14. Slices

| Slice | Content | Rows |
|---|---|---|
| S5.1 | This contract (docs only) | — |
| S5.2 | `studyArea` at schema 55 (nodes, `flowSource`, `gateFlows`), the Nodes tab column and canvas outline, the kept set, gates, the run-time copy, typed gate flows, the area refusals. **S5.2a implemented** (D327, §16); S5.2b, the gate-flow table and the outline, is next | SA1–SA6 |
| S5.3 | Frank–Wolfe path flows, the Macro cut by period | SA7–SA9 |
| S5.4 | Watched path counts in `loadMeso`, the Meso cut by slice | SA10, SA11 |
| S5.5 | Side files, stale by SHA-256, `MICRO_AREA_SOURCE_STALE`, the marking line, `MICRO_GENERATED_IN_AREA` | SA12–SA15 |

## 15. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| SA1 | Byte identity | Every shipped project and fixture keeps its schema and bytes; Micro, Meso and Macro outputs of a file without `studyArea` are byte-identical to D325's; `studyArea` below 55 is `EDIT_UNSUPPORTED_FIELD` |
| SA2 | Codec | `studyArea` with nodes, `flowSource` and `gateFlows` round-trips at 55; an unknown or repeated Node gives its code at its path; an empty list is not written |
| SA3 | Commands | Deleting an area Node, merging two area Nodes, and ticking the column each keep the list whole in one Undo step; Undo restores the bytes |
| SA4 | Kept set | Two signalised four-arm Nodes joined by a mid-block Link split by a Connector: the area {A} keeps A's interior and its eight leg Links; {A, B} adds the mid-block piece; a Link beyond B is not kept |
| SA5 | Gates and typed run | With {A} and typed gate flows, Micro runs on the kept set only, with entries at the four in-legs and exits at the four out-legs; vehicles appear only at gates; the document's inputs give `MICRO_AREA_INPUT_IGNORED` once each; seed 42 gives the same bytes twice |
| SA6 | Area refusals | A coarse Node in the area gives `MICRO_AREA_NODE_COARSE`; a Node with `NODE_LINK_REENTERS` gives `MICRO_AREA_NODE_INVALID` with it; an unknown gate gives `STUDY_AREA_GATE_UNKNOWN` |
| SA7 | Path decomposition | A two-route Frank–Wolfe fixture, computed by hand: the summed path flows equal `classFlows` on every link to 1e-9, and the route split equals the fixture's |
| SA8 | Macro cut | On SA4's network with an outer zone pair whose paths cross A, the gate matrix equals the paths' flows through A's legs; a path leaving and re-entering {A, B} gives two trips |
| SA9 | Macro periods | Two periods with different matrices give two gate matrices; an interval whose period has no Macro run gives `MICRO_AREA_PERIOD_NOT_RUN`; a non-converged source passes its advisory on |
| SA10 | Meso FIFO counts | On a corridor whose upstream merge mixes two paths, a watched link's per-path exit counts sum to its aggregate exit curve, and each path's entrance-to-exit order is FIFO |
| SA11 | Meso cut | The gate matrices per slice sum to the watched entrance counts in vehicles (PCU ÷ class PCU); a cancelled run gives `MICRO_AREA_SOURCE_INCOMPLETE` |
| SA12 | Side files | Run Macro and Run Meso write their side files; the project file's bytes do not change; reopening reads them back equal |
| SA13 | Stale after reopen | Editing a Link, saving and reopening shows the Macro result stale; restoring the bytes shows it fresh; a Micro cut result is stale when its side file changes |
| SA14 | Marking line | An export with one F4 Node in the area carries the second header line with its count; with no generated object and nothing stale the bytes equal D325's |
| SA15 | Generated warnings | An F4 Node in the area gives one `MICRO_GENERATED_IN_AREA` with its counts; editing every generated object clears the row; the run starts in both cases |

## 16. S5.2a as implemented (D327)

- **Model and codec** (`src/model/demand/zones.hpp`, `src/project/study_area_codec.*`):
  - `ZoneLayer::studyArea` holds `nodes`, `flowSource` and `gateFlows`;
  - `"studyArea"` is written at the root only when it names a Node, which makes the file 55. Below
    55 the key is `EDIT_UNSUPPORTED_FIELD`;
  - a malformed value is `EDIT_STUDY_AREA_INVALID`: an empty list, a `flowSource` other than
    `"typed"` (S5.3 and S5.4 add theirs), or a gate flow not exactly
    `{periodId, from, to, compositionId | modeId, vehiclesPerHour}`;
  - `studyAreaIssues` gives `STUDY_AREA_NODE_UNKNOWN`, `_NODE_DUPLICATE` and `_PERIOD_UNKNOWN`,
    plus M8's `OD_COMPOSITION_MISSING` and `OD_CELL_INVALID` on a flow.
- **Commands:**
  - `setStudyAreaNodes`; an empty list removes the area with its flows;
  - `mergeNodes` keeps its result in the area when any merged Node was in it;
  - `settleStudyArea` in `History::execute`, after `settleNodes`, drops ids naming no Node, so a
    deleted Node, a lifecycle merge and Undo all keep the list whole.
- **The kept set** (`src/model/network/study_area.*`, pure):
  - `studyAreaCut` computes §4's kept set and its entry and exit gates. A Node without detail is
    `MICRO_AREA_NODE_COARSE`, checked before its membership, since such a Node always reads
    `NODE_NO_TURN`. Then come `MICRO_AREA_NODE_INVALID` with the membership codes, and
    `MICRO_AREA_EMPTY`;
  - `cutNetwork` keeps the kept objects and the right-of-way standing on them. A Connector path is
    named by its Connector alone (`ControlPathRef` leaves `linkId` empty).
- **The run** (`src/project/study_area_run.*`):
  - `studyAreaDocument` builds §4's run-time copy: gate zones `gate:<link>:in|out` with
    connectors `…/c`, and one matrix `gate-flows/<k>` per (period, composition or mode);
  - `compileDocument` and `runDiagnostics` (`src/project/run.cpp`) use the copy whenever the area
    names a Node, so the editor's Run, batch, compare, readiness and the CLI need no change. The
    advisories `MICRO_AREA_INPUT_IGNORED` come after the copy's own rows.
- **Readings taken:**
  - every input, route, routing decision, route destination and OD matrix of the document is
    listed as not run, not only those on dropped Links, since none of them runs in an area run;
  - speed decisions and areas on kept Links stay, as network control.
- **Editor:** the Nodes tab's eighth column, *Study area*, is a check box; each tick or untick is
  one Undo step. The gate-flow table and the canvas outline are S5.2b.
- **F4 fix:** READINESS §14, amended. A leg keeps its far end, so a split arm runs.
- **Rows:**
  - SA1–SA6 in `studyarea`, on two F4 junctions joined by a Road split twice; seed 42 runs twice
    with the same bytes;
  - SA3's editor part in `node-ui`.
