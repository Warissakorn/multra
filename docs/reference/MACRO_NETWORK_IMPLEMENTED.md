# Macro network — as implemented (S2.1–S2.4)

What each slice of [MACRO_NETWORK](MACRO_NETWORK.md) built, kept apart from the contract so that
file stays under 500 lines. The section numbers are the contract's, so "MACRO_NETWORK §20" still
names a section here.

## 16. S2.1 as implemented (D273)

- **Model.**
  - `Link::linkTypeId` is in `network.hpp`.
  - `LinkType`, `Vdf`, and `AuthoringDefinition::externalLinkTypes`/`linkTypes` are in
    `definition.hpp`.
- **File.** `src/project/link_type_codec.*` holds:
  - `usesLinkTypes` and `rejectLinkTypesBefore50` (`definition.linkTypes`,
    `network.links[i].linkType`);
  - `readLinkTypeIds`;
  - the type and VDF codecs, with exact keys, where a malformed entry is `EDIT_CATALOG_READ`;
  - `resolveLinkTypes`, giving owned types or `data/link-types/` in name order;
  - `resolveVdfs`;
  - `linkTypeIssues`, `vdfIssues`, and `linkTypeDocumentIssues` on commit.

  Fifteen rows' "future schema" example moves to 51.
- **Graph.**
  - `src/macro/graph.*` holds `MacroLink`, `MacroGraph`, `allows` and `reachable`. It is a pure
    module in `check_architecture`'s list.
  - `compileMacroNetwork` (`src/model/demand/macro_compile.*`) builds the graph as §3 describes.
    Every zone connector gets its own centroid node, so a cell keeps Micro's share split and a
    centroid is never a through node.
- **Demand.** `src/project/macro_input.*` holds `macroNetwork`, `macroPeriods` and `macroDemand`.
  - A connector pair with no allowed path is left out. The cell is `MACRO_NO_PATH` only when no
    pair is served, as with Micro's `OD_NO_PATH`.
- **Shipped.**
  - `data/vdf/bpr.json` (α 0.15, β 4). Its source names the 1964 manual as the secondary texts
    report it. The manual itself is not online and has not been read.
  - No link type ships.
- **Rows.** MA1–MA6 are in `tests/macro_network_tests.cpp` (ctest `macronetwork`).
- **Mutations caught:**
  - no schema gate;
  - no station split;
  - a Connector timed at its to-Link's speed;
  - shares or hours ignored;
  - modes ignored;
  - a missing type accepted;
  - capacity not multiplied by lanes;
  - document order leaking into the graph.

## 17. S2.2 as implemented (D274)

- **Engine** (`src/macro/assign.*`, pure):
  - `macroTime`, the BPR time;
  - `shortestTree`, Dijkstra on a `std::set` queue over the arcs a class may use; on equal distance
    the lower link index is the predecessor;
  - `allOrNothing`;
  - `assignMacro(graph, classes, MacroSettings{targetGap, maxIterations})`, returning `classFlows`,
    `load` (pcu/h), `time`, `gap`, `iterations` and `converged`.

  `MacroCell` and `MacroClass` moved to `src/macro/graph.hpp`, so the engine takes them.
- **Loop:**
  1. start from all-or-nothing at free-flow times;
  2. each iteration computes the times at the current load, every class's all-or-nothing target
     and the gap of §7;
  3. it stops at the target or at the cap. Stopping at the cap returns `converged == false` with
     the flows reached;
  4. otherwise one step size moves every class.
- **Defaults** belong to the run settings (S2.3). The engine has none.
- **Found while writing the rows:** with an exact line search, two parallel routes reach
  equilibrium in one step. Row MA8 pins this, and that is what catches a step rule other than the
  line search.
- **Rows.** MA7–MA11, MA16 and MA17 are in `tests/macro_assign_tests.cpp` (ctest `macroassign`).
- **Mutations caught:**
  - ties to the higher index;
  - an MSA step in place of the line search;
  - the gap's sign flipped;
  - PCU dropped from the load;
  - bans ignored;
  - the cap off by one;
  - β ignored.

## 18. S2.3 as implemented (D275)

- **The split.** NEXT's S2.3 held two systems, so the editor surface (Run Macro, the Macro links
  table, stale marking, the link-type editor; row MA14) is S2.4.
- **Settings.** `src/project/macro_settings.*`: `MacroSettings` and `kMacroDefaults` (10⁻⁴, 1000)
  live in `src/macro/graph.hpp`; `AuthoringDefinition::macroSettings` is optional; the command is
  `changeMacroSettings`. The schema chain gains `usesMacroSettings(d)?51`, and fifteen rows'
  "future schema" example moves to 52.
- **Run.** `src/project/macro_run.*`: `macroPeriodName`, and `runMacro` returning `MacroRun{period,
  revision, settings, graph, classes, assignment, advisories, inputManifest}`. `macroDemand` now
  takes the manifest too, so the catalog files it reads are recorded.
- **Outputs.** `src/project/macro_output.*`:
  - JSON: `validated: false`, `measure`, `period`, `revision`, `settings`, `gap{formula, value,
    target}`, `iterations`, `converged`, `advisories`, `classes{id, mode, pcu}`, `links[]` and
    `inputManifest`;
  - each link row: `kind`, `source`, `fromStation`, `toStation`, `linkType` (`MacroLink::linkType`,
    empty off Links), `flow{class: veh/h}`, `load` (pcu/h), `vc` (null when uncapacitated),
    `freeTime`, `time`;
  - CSV: the marker line, `# Period`, `# Relative gap` with the formula, iterations and
    convergence, then a header with one `flow:<class>` column per class.
- **CLI.** `multra-cli --project FILE --macro PERIOD [--csv FILE] [--data-dir DIR]`; JSON on
  stdout, stamped like a batch; a warning on stderr when not converged. The fixture is
  `tests/macro-two-route.traffic.json`.
- **Rows.** MA12 and MA13 in `tests/macro_output_tests.cpp` (ctest `macrooutput`); `cli-macro`,
  `cli-macro-seeds`, `cli-macro-period`.
- **Found while writing the rows:** S2.2's MA11 document gives each entry Link its own zone
  connector, so its flows follow the shares, not route choice. MA11's determinism claim stands;
  its comment is corrected, and S2.3's fixture fans one origin out to two routes.
- **Mutations caught:**
  - the marker dropped;
  - a class column missing (it first survived a prefix check; MA13 now compares a whole row);
  - `MACRO_NOT_CONVERGED` never added;
  - the schema gate off;
  - the VDF parameters ignored;
  - V/C on veh/h;
  - the project's settings ignored;
  - the settings always written;
  - the cap's upper bound unchecked.

## 20. S2.4 as implemented (D277)

- **Engine.**
  - `MacroIteration{iteration, gap, step}`.
  - `MacroAssignment::convergence` holds one entry per gap computed (iterations + 1); the last
    entry's step is 0.
  - `MacroAssignment::cancelled`: `assignMacro`'s optional observer sees each entry once its step
    is known. A false return stops the run before that step is taken, records the step as 0, and
    never counts as converged.
- **Stages.**
  - `MacroStage` and `MacroObserver` are in `macro_run.hpp`. `runMacro` reports period, graph,
    demand and assignment as it enters each, so a refusal names the last stage reported.
  - A cancelled run is not `MACRO_NOT_CONVERGED`.
  - The JSON gains `cancelled` and `convergence[]`. The CSV is unchanged.
- **Commands.**
  - `putLinkType(d, type, installed)` owns the installed catalog first, then adds or replaces;
    a new id gets the prefix `link-type`.
  - `deleteLinkType` refuses with `EDIT_LINK_TYPES_NOT_OWNED`, `EDIT_LINK_TYPE_IN_USE` or
    `EDIT_UNKNOWN_OBJECT`.
  - `setLinkType(d, link, type)`, where nullopt or "" clears the type.
- **Editor** (`src/shell/editor_macro.*`, `editor_link_types.cpp`):
  - **Run Macro…** in the Results toolbar asks for the period, copies the document and runs
    `runMacro` on the panel's own worker. It posts each stage and gap back, and **Cancel Macro**
    stops between iterations. It never calls `execute`, so it adds no Undo step.
  - **Results → Macro links:**
    - one row per arc, with a flow column per class;
    - the note is one paragraph: the marker, then (once a run has started) the six stages and
      their status, the live iteration and gap, the result's summary and the stale notice;
    - Export and Copy on this tab write `macroCsv`.
  - **Stale:** the result keeps the revision it ran on and is marked stale while
    `history_.revision()` differs; Undo back to that revision clears the mark. An edit does not
    discard it, and New, Open and Recovery do. A refused or cancelled run keeps the previous
    result.
  - **Run settings:** the target gap (%) and the iteration cap. An untouched field keeps the stored
    value, and the defaults write no key.
  - **Network → Link types:** a table with Add, Edit and Delete through a dialog (capacity per lane,
    free speed in km/h, VDF, modes, source). The Link page's **Link type (Macro)** combo sets the
    type on every selected Link in one Undo step.
  - **Locales:** the new keys, and messages for the Macro and link-type codes, in English and Thai.
- **Rows.**
  - MA18 is in `macroassign`;
  - the stage and command rows are in `macrooutput`;
  - MA14 is `tests/macro_ui_tests.cpp` (ctest `macro-ui`).
  - The workspace and object-group UI tests list the new page and tab.
- **Mutations caught:**
  - the observer ignored;
  - the record's index off by one;
  - the first record dropped;
  - the stale check inverted;
  - `clearRun` discarding the result;
  - a type in use deleted;
  - Run settings always writing the key;
  - a refusal not marked on its stage;
  - Run Macro made an Undo step.
- **Found while writing the rows:** a multi-line Macro note made the Objects dock taller than the
  wide-canvas row allows. The note became one reflowing paragraph, shown with stages only after a
  run.
