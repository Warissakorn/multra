# Study area — as implemented (S5.2a–S5.5b)

What each slice of [STUDY_AREA](STUDY_AREA.md) built, kept apart from the contract so that file
stays under 500 lines. The section numbers are the contract's, so "STUDY_AREA §20" still names a
section here.

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
  - SA1–SA6 in `studyarea`, on two F4 junctions joined by a Street segment split twice; seed 42 runs twice
    with the same bytes;
  - SA3's editor part in `node-ui`.

## 17. S5.2b as implemented (D328)

- **`studyAreaEnds`** (`src/project/study_area_run.*`): the names a gate flow may use.
  - The origins are the entry gates in cut order, then the zones with an origin connector on a
    kept Link.
  - The destinations are listed the same way, from the exit gates.
  - The run's `STUDY_AREA_GATE_UNKNOWN` check reads the same lists, so the dialog and the run
    cannot disagree.
- **Commands** (`node_commands.*`):
  - `setGateFlows` replaces one (period, composition or mode) key and keeps the others in order;
  - `setStudyAreaGateFlows` sets the whole list.

  Both drop cells of 0.
- **Dialog** (`src/shell/gate_flow_dialog.*`, a free function, D319): a period, Composition/Mode
  and a grid of origins × destinations in veh/h.
  - Typed values are kept per key, so switching period or types and back loses nothing.
  - The first stored flow decides what opens.
  - Its size is capped to the screen (`fitToScreen`, D318).
- **Entry:** *Gate flows…* on the Nodes tab is enabled when the area runs (`studyAreaEnds` does not
  throw) and a study period exists. Accepting it is one Undo step. A gate reads as its Street segment's name
  (else the Link's name, else its id), with "in" or "out".
- **Canvas:** an area Node gets a light fill (`ok` at alpha 40) under its usual outline. Selection
  and the dotted outline of a `generated` Node still read.
- **Rows:**
  - SA16 in `studyarea` (`studyAreaEnds`, the commands);
  - in `node-ui`: the dialog, the fill, and the action disabled on an area that cannot run.

  S5.2 is complete.

## 18. S5.3 as implemented (D329)

- **Path flows** (`src/macro/assign.*`):
  - `assignMacro(…, keepPaths)` and `allOrNothing(…, paths)` record each cell's path in travel
    order;
  - `MacroAssignment::paths` holds, per class, the final load's paths sorted by (cell, links),
    weighted as in §6;
  - without `keepPaths` the field stays empty and every other byte is unchanged (SA7 compares the
    two runs).
- **The cut** (`src/project/study_area_macro.*`):
  - `cutTrips` splits a path into its kept stretches. A stretch starts at an inside zone when a
    centroid arc of an origin zone precedes it, else at its first kept Link's entry gate, and ends
    the same way;
  - `macroGateFlows` runs `runMacro(…, keepPaths = true)` for each study period wholly inside
    [clockStart, clockStart + duration] and sums veh/h per (period, class, from, to);
  - a class names its mode, or else the composition after `composition:`.
- **Source of the Macro result (owner, D329):** Macro runs at compile, once per such period, until
  S5.5 reads a fresh side file instead. Frank–Wolfe has no seed, so the CLI, batch and the editor
  give the same bytes (SA9 runs seed 42 twice).
- **Codes:**
  - refused: `MICRO_AREA_PERIOD_NOT_RUN` at `studyPeriods` when no period lies inside the run;
  - advised: `MICRO_AREA_PERIOD_OUTSIDE` (a period only partly inside), `MACRO_NOT_CONVERGED`, and
    `MICRO_AREA_CUT_UNMATCHED` (a stretch whose end is no gate or inside zone), each at
    `studyPeriods[i]`;
  - Macro's own refusals pass through, and a catalog error is a `catalogs` row in readiness.
- **`flowSource` `"macro"`** parses at 55. Typed `gateFlows` stay stored and unused under it.
- **Editor:** the gate-flow dialog's *Source* (Typed here / Cut from Macro) disables the grid
  under Macro and keeps the typed values. *Gate flows…* no longer needs a period to open.
- **Measured** (Debug build, GCC 13, Linux, one run each):
  - a synthetic 12×12 two-way BPR-4 grid, with 1,892 OD cells over its 44 boundary nodes, reaches
    gap 1e-4 in 187 iterations: 1.51 s without paths and 2.60 s with them (+72 %). It keeps 27,089
    paths and 434,817 link entries, about 3.5 MB;
  - MA15's Sheffi network shows no difference (3 paths).

  Only a Micro run cut from Macro keeps paths. The M2.6 template has no zones, so it cannot run
  Macro (§6's measurement was read on these two instead).
- **Not covered:** `MACRO_NOT_CONVERGED`'s pass-through is not exercised. The corridor has one path
  per cell, so its gap is 0 from the start. The code path is a single `advise` call.
- **Rows:** SA7 (MA8's pair by hand; three routes for the sum), SA8 (the corridor; a synthetic
  re-entry), SA9 (two periods, outside, none inside, the run twice), and the dialog's Source in
  `node-ui`.

## 19. S5.4 as implemented (D330)

- **Source read** in the owner's copy of Yperman 2007 (`ecd6bfe`, attached and not vendored):
  - §3.2, in the route-count rule and under "FIFO conditions": a link's downstream route counts
    inherit the upstream composition through the inverse cumulative function, which needs FIFO
    on every link. LTM ensures this, though not strictly: vehicles that entered earlier may leave
    in the same step;
  - Ch. 5 at Eq. (5.2): disaggregate sending flows need the same link-FIFO and CFL conditions.
- **Watched counts** (`src/meso/loading.*`): `loadMeso(…, watch)` copies, for each loaded path
  and each watched link on it, the path's cumulative PCU entering the link. These are the route
  counts the loading already keeps.
  - `runMeso(…, watchLinks)` watches the Meso links whose `source` is listed.
  - With no watch nothing is added (SA10 compares the curves).
- **The cut** (`src/project/study_area_meso.*`): `mesoGateFlows` runs Meso with the kept Links
  watched and cuts each loaded path's kept stretches.
  - The origin is the stretch's first Link's entry gate. An OD route starting the stretch at its
    own first Link takes the one listed origin zone with a connector there instead. The
    destination is chosen the same way.
  - Per output slice, the vehicles are the entrance count's increase on the stretch's first link
    ÷ the class PCU.
  - Each (slice, composition, from, to) gives one gate flow with `start`/`end` and no period. The
    run-time copy builds one matrix with its own times per slice and composition.
- **Owner (D330):**
  - Meso runs at compile over the whole Micro run until S5.5's side files (no seed, D292);
  - one Micro interval per Meso slice.
- **Codes:**
  - `MICRO_AREA_SOURCE_INCOMPLETE` (a cancelled loading; at compile there is no observer, so it
    is a guard);
  - originally `MICRO_AREA_CLASS_UNSUPPORTED` at `classes[i]` for a `type:` class; §22 (D338)
    adds that cut. Unknown class prefixes still get this advisory;
  - `MICRO_AREA_CUT_UNMATCHED` at `studyArea`.
- **Editor:** the gate-flow dialog's Source gains *Cut from Meso*.
- **Rows:**
  - SA10: on each watched link the paths' counts sum to its `up` curve, never fall, and leave the
    loading unchanged;
  - SA11: 100 trips over 300 s slices; all 100 vehicles cut, west to the middle exit gate with
    {A} and west to east with {A, B}; the run twice at seed 42;
  - the Source in `node-ui`.

## 20. S5.5a as implemented (D331)

S5.5 is split like S5.2: S5.5a is §8–§9 here; S5.5b is §10–§11 (rows SA14, SA15).

- **Owner (D331):**
  - a side file holds the cut source only (Macro path flows, Meso watched counts) plus its
    provenance. A reopen shows one line on the Macro or Meso tab, fresh or stale, not the table;
  - a side file whose network no longer matches the document is not used. Macro or Meso runs at
    compile as in D329/D330, and the run gets `MICRO_AREA_SOURCE_STALE`;
  - Run Macro keeps paths only when the study area cuts Macro, and Run Meso watches the kept
    Links only when it cuts Meso.
- **The files** (`src/project/side_files.*`):
  - `<project>` less its extension, then `.macro.json` / `.meso.json`, with `format` and `schema` 1;
  - Macro: one entry per period, replacing that period's. Each entry holds `projectSha256`, the
    settings, gap, iterations and convergence, the classes, the graph's arcs as
    `[kind, source, fromStation]`, the paths (when kept) and the input manifest;
  - Meso: `projectSha256`, step and slice, duration, the Meso links as arcs, each class's paths
    (source, links), `watchedLinks`, the watched counts and the input manifest;
  - `projectSha256` is `inputSha256` of `documentJson` indented by 2 plus a newline, the bytes
    saving writes, so an unsaved edit counts.
- **Untrusted:** `parseMacroSide` / `parseMesoSide` check types, ranges and every index against
  the file's own arrays. The first bad value is `MICRO_AREA_SOURCE_INVALID` at `macroSide…` /
  `mesoSide…` plus its JSON path; bytes that are not JSON are refused at `macroSide` / `mesoSide`.
  Before a stored index is used, the graph is compiled again from the document and its arcs are
  compared.
- **The cut** (`StudyAreaSources` through `compileDocument`, `runDiagnostics`, `readiness` and
  `studyAreaDocument`):
  - Macro: per period, an entry with paths whose arcs equal the graph is cut in place of the run;
  - Meso: a file whose links equal the graph and which watched every kept Link is cut with its
    own slice and duration;
  - `MICRO_AREA_SOURCE_STALE` (advisory, at `studyArea.flowSource`) when a used entry's
    `projectSha256` is not the document's, or when a file was there but could not be used;
  - a missing file, or a missing period, runs at compile with no advisory (D329/D330 unchanged).
- **Provenance:** `RunSnapshot::studyArea` records `flowSource`, the side file's SHA-256 when used,
  the Macro periods taken from it, the document's `projectSha256` and whether it was stale.
  Writing it into the Micro JSON and CSV moves to S5.5b with the marking line, so every export
  keeps its bytes here.
- **Editor** (`src/shell/editor_side_files.cpp`):
  - a finished Run Macro or Run Meso of a saved project writes its side file. A write failure
    shows `EDIT_SIDE_FILE_WRITE` and the result stays;
  - opening a project reads its side files. A Save as to another path reads that path's (none are
    copied);
  - every Micro run, batch, comparison and readiness check hands them to the cut;
  - `macroStale()` / `mesoStale()` now follow §9 rule 1: the result's `projectSha256` against the
    document's, hashed once per revision. Undo back to the same bytes reads fresh, as before.
- **Rows:**
  - SA12 (`studyarea`): the hash is of the saved bytes; Macro paths and Meso watched counts read
    back equal; a second period joins and keeps the first;
  - SA13 (`studyarea`): a fresh file gives the at-compile flows to 1e-9 with no advisory; other
    bytes are stale; another graph, or no paths, runs at compile and is stale; a bad index and
    non-JSON bytes are refused at their paths; the snapshot names the file; Meso without the kept
    Links watched runs at compile, stale;
  - `macro-ui` / `meso-ui`: the side file is written and the project's bytes are unchanged; a
    reopen reads it fresh, an edit saved reads it stale, and the original bytes read it fresh
    again. Both suites now open their fixture from a temporary copy.
- **Not tested here:** an added Link is covered by a changed stored arc, not by editing the
  network.

## 21. S5.5b as implemented (D332)

- **Counting** (`src/project/export_marking.*`):
  - `generatedInNode` reads the Node through `nodeMembership`: generated Connectors inside;
    waiting lines and conflict areas inside, plus the priority rules and stop controls naming
    them; heads inside; and the generated controllers those heads show;
  - `generatedInArea` sums the study area's Nodes. With no study area it counts every generated
    object of the network and definition, since Micro runs all of it (§10, applied to the
    marking too).
- **Readiness** (§10):
  - at Micro, one `MICRO_GENERATED_IN_AREA` warning at `nodes[i]` per area Node (every Node
    without an area) with a non-zero count. `ReadinessRow::counts` carries Connectors,
    right-of-way objects, heads and controllers, and the Problems tab puts them in the message;
  - the run still starts: the rule is in `readiness`, not `runDiagnostics`;
  - a Problems row naming a Node now selects it on the Nodes tab, which lights it (D288). Seam
    rows at `nodes[i]` gain this too.
- **Marking** (§11):
  - `markedCsv` inserts after the not-validated line
    `# MULTRA - generated objects in the study area: <n>; stale inputs: <list or none>.`, only
    when n > 0 or something is stale;
  - then, for a run over a study area,
    `# MULTRA - study area flows: <source>; source <sha256 | run at compile | typed>; project <sha256>.`
    (§8);
  - JSON gains `marking` and `studyArea` under the same rules;
  - a comparison writes each side's lines, prefixed `base:` and `alternative:`;
  - the stale tokens are `macroSide` / `mesoSide` (the cut's side file, D331) and `project` (a
    Macro or Meso result on show that was made for other bytes);
  - the formatters are unchanged. Every export without generated objects, staleness or a study
    area keeps its bytes (SA1). The TSV copy is the marked CSV through `csvToTsv`.
- **Callers:**
  - the editor's Export and Copy for single, batch, comparison, Macro and Meso. Each result keeps
    the marking of the snapshot it was compiled from;
  - the CLI's `--csv` and JSON output for the same five. The CLI now reads the side files beside
    each project, as the editor does;
  - a comparison's alternative project is now cut from its own side files, not the open
    project's (a S5.5a slip found here).
- **Rows:**
  - SA14 (`studyarea`): F4's priority junction A counts Connectors and right-of-way objects; the
    two Nodes share out every flag; the line's exact bytes; stale tokens and the flows line;
    JSON; both comparison sides; nothing marked keeps the CSV and JSON bytes;
  - SA15 (`studyarea`): two rows with no area, one at A's `nodes[i]` with its counts with {A},
    none once the flags are cleared, and the typed area run starts in both cases;
  - `node-ui`: the row shows "1 Connectors, 0 right-of-way objects…" and selecting it lights the
    Node.
- The CLI wiring is exercised by §22's separate-process test (SA18).
- A full result table restored on reopen remains outside D331's cut-source-only choice.

## 22. Single-vehicle-type Meso cuts and CLI coverage (D338)

- `mesoGateFlows` accepts both `composition:` and `type:` classes, keeping the complete class id
  in its aggregation key. The existing entrance-count difference divided by class PCU still
  determines each slice's vehicles; no loading or engine rule changes.
- `GateFlow::vehicleTypeId` and `OdMatrix::vehicleTypeId` are run-time-only fields. The cut groups
  matrices by type as well as period/window, composition and mode; `withOdDemand` passes the type
  directly to its ordinary `VehicleInput`. Authored matrices and typed gate flows still require a
  composition or mode, and their codecs and schema 55 are unchanged.
- Catalog ownership stays unchanged. In particular, a project with its own reduced vehicle-type
  library can consume `car-only` from the external catalog without validating unused compositions
  against that smaller library. No synthetic composition is added.
- SA17 (`studyarea`, `study_area_vehicle_type_tests.cpp`) checks non-unit PCU, mixed classes,
  per-type counts, catalog/source preservation, seeded replay and fresh side-file parity. External
  composition bytes remain in the captured input manifest. Unknown class prefixes still advise.
- SA18 (`cli-study-area`) generates a synthetic project with a direct heavy-vehicle input, then
  runs the real CLI for a single seed, batch and comparison. It checks absent, fresh and stale
  Meso side files, exact hashes, JSON/CSV marking and equal simulation outcomes for equal counts.
