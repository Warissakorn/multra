# PROGRESS log archive — 2026-10-19

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

## 2026-10-22 — M11.3's node model from Tampère et al. (2011) (D297)

The owner uploaded the paper, and it was read in full.
- **`mesoNodeFlows`** is its §5.1 instance and algorithm: oriented capacity proportional, exact,
  and unique. It reproduces the paper's Table 8.
- **Changed from the contract:** the paper has no instance of conflict constraints (its Eq. 17 is
  provisional). By the owner's choice, priority inside a Node is advised `MESO_PRIORITY_IGNORED`
  instead of guessed.
- **Signals** are explicit green, as the paper's §5.2 allows.
- **Rows:** ME20–ME23 in `mesonode`. Three mutations are caught; two had no effect, and both are
  explained.
- **Still blocked:** the link model needs Yperman (B22).

## 2026-10-22 — M11.3 blocked on its sources; nothing built

The owner asked for M11.3. The contract makes checking §4 against Yperman (2007) and §5 against
Tampère et al. (2011) the first step, but the environment refused every host that holds them
(proxy 403: arxiv.org, lirias.kuleuven.be, ideas.repec.org). The owner chose to wait for the sources,
so no engine code was written. OWNER_SITTING B22 asks for the hosts or the PDFs.

## 2026-10-22 — MA15: Sheffi's Table 5.4 as the Macro benchmark (D294)

The owner supplied Markdown copies of four books, among them Sheffi (1985), which was D272's first
candidate. This unblocked MA15.
- **The example:** Table 5.4, Frank–Wolfe on Figure 5.1. It has three parallel BPR links, t₀ =
  (10, 20, 25) and q = 10.
- **Derived capacities:** the copy's image of book p. 114 is empty, so the capacities were derived.
  Table 5.1's 947 and 137 give c₁ = 2 and c₂ = 4. c₃ = 3 reproduces every printed number, apart from
  two misprints. The owner confirms them against the print (OWNER_SITTING B21).
- **Row:** `macrobenchmark` holds MA15 (a) and (b).
  - (a) Runs capped at 1–5 iterations match the printed flows, times, objective and steps.
  - (b) At the 10⁻⁴ target, the run reaches the equal-time equilibrium in 13 iterations, with
    z = 189.33.
  - The engine was unchanged.
- **Cost:** 80 µs and 0.47 M Ir per run (ENGINEERING_NOTES).
- **The M10 gate stays open:** closing it is the owner's call, and the not-validated marker stays
  until M6.

## 2026-10-22 — M11.3b: routeless inputs and placed decisions as Meso paths (D296)

The owner asked for M11.3b. Meso now takes Micro's own expansion of routeless inputs and placed
decisions: `expandRouteless` after `resolveCatalogs`, so lane splits, slice shares and typed routing
are not copied.
- **Lanes to objects:** `mesoObjectChain` maps lane sections and connector paths back to Links and
  Connectors, then `mesoPath` as in M11.3a.
- **Stubs:** a stub's flow goes to the full routes its vehicles change lanes onto, because the trip
  does not end there.
- **Type parts:** a composition's type parts sum back into its class.
- **The M2.6 template** (four placed decisions) now loads its whole demand, conserved over time.
- **Rows:** ME17–ME19 in `mesodemand`. ME13 now checks the walk's own issues instead of the old
  refusal. Four mutations are caught.

## 2026-10-22 — M11.3a: Micro's routes as Meso paths and demand (D295)

The owner asked for M11.3a after MA15.
- **`src/meso/demand.*`:** paths, classes, turn shares and entry flows, as plain values.
- **`mesoPath`:** a route's Links and Connectors become Meso links and turns. Pieces inside a Node
  collapse to the Node's turn.
- **`mesoDemand`:** expands the document as the Micro compile does (OD matrices, then static
  decisions with their interval and type weights). It keys classes by composition or lone vehicle
  type, with PCU from `modePcu`.
- **Carved out as M11.3b:** placed decisions and routeless inputs walk lanes, not objects, so they
  are refused with `MESO_ROUTELESS_UNSUPPORTED` until then. The M2.6 template uses them.
- **Rows:** ME12–ME16 in `mesodemand`. Five mutations are caught. A turn preference by `via` was
  removed as dead code, since the graph keeps one turn per (in, out) pair.

## 2026-10-21 — M11.2: the Meso graph (D292 answers, D293)

The owner answered the Meso choices "เอาตามแนะนำ": 1a–7a (D292).
- **New module `src/meso/`** (imports nothing): links, turns and nodes, the settings and the step
  limit.
- **`compileMesoNetwork`:** each Node becomes one node with its membership turns and conflicts; the
  pieces inside a Node are left out; other junctions become simple nodes with through and merged
  Connector turns. A refused Node is reported and compiled as if it were not drawn.
- **File:** a link type's `jamDensity` (pcu/km/lane, above the critical density) and
  `mesoSettings{step, slice}` at schema 52, only when set; the shipped projects keep their schema.
- **Tests:** ME1–ME2 in `mesonetwork`. Six mutations are caught, two of them only after the rows
  gained fixtures (two lane Connectors; a refused Node over the junction).

## 2026-10-21 — M11.1: the Meso contract (D291, docs only)

The owner asked whether Meso could start. M11.1 can, because everything it needs exists; only M11.5
(dynamic assignment) waits on M9. [MESO_NETWORK](../reference/MESO_NETWORK.md) fixes the following:
- **The graph:** the Macro graph's Link pieces, with each Node as one Meso node.
- **The link model:** a triangular diagram with Newell's cumulative curves and Yperman's sending and
  receiving volumes.
- **The node model:** Tampère et al. (2011), with signals as capacity and spill-back.
- **Data:** a `jamDensity` per link type, none shipped.
- **Loading:** deterministic.
- **Outputs:** by time slice, with rows ME1–ME11.

Seven owner choices are listed as B20. Sources came from search summaries; Yperman's thesis was not
found, so §4's formulas are checked before M11.3.

## 2026-10-21 — S0.8b: a comparison's Node rows in Results → Nodes (D290); S0 complete

The editor half of D289. MA15 is still blocked (sheffi.mit.edu and arxiv.org refused again).
- **Results → Nodes, for a comparison:** the Node, leg and turn rows of `Comparison::nodes`: two
  sides, difference and ±95 % for delay, plus the legs' queue. There is no letter on a difference.
  Unmatched names go in the note.
- **Tests:** N21 rewritten. A comparison with itself let a column swap survive, so it now compares a
  busier copy; four mutations each fail it.
- **S0 is complete** but S0.4, which moved to S3 (D272).

## 2026-10-21 — S0.8a: Node rows in the scenario comparison (D289)

MA15 is still blocked (sheffi.mit.edu, web.mit.edu and arxiv.org refused), so the S0 item left,
split per AGENTS into the model and output now and the editor table next.
- **Model:** `compareBatches` matches Nodes, then their in-legs and turns, by name; Welch
  differences of delay, and of the legs' mean queue. Unmatched names are listed per level.
- **Output:** JSON `nodes` and two CSV blocks after `network`, only with a Node; every comparison
  without one keeps its bytes. The editor's Copy and Export carry the blocks.
- **Tests:** CMP8–CMP10 in `compare`; five mutations each fail them.
- **Found while checking sources:** Aimsun's manual cites Florian & Hearn (1995), and Visum names
  Mitradjieva, Lindberg et al. (2013) for BFW and Gentile for LUCE. Neither is a benchmark network.
  Sheffi's PDF is also at sheffi.mit.edu.

## 2026-10-21 — S0.7: the Nodes tab lights the Node and its legs (D288); MA15 still blocked

The proxy refused arxiv.org (Boyles et al., *Transportation Network Analysis*, a newer free
candidate for MA15) as well as web.mit.edu, after the owner allowed it. The owner chose an S0 item.
- **Canvas:** selecting a Nodes row lights the outline in the selection colour and draws each leg,
  the Link's span outside the polygon up to its crossing (in filled, out hatched), from
  `nodeMembership`. A refused Node lights its outline only. Clearing, deleting or undoing puts it out.
- **Unchanged:** every output, since the change is editor-only.
- **Tests:** row N22 in `node-ui`; legs never drawn, in/out swapped, the highlight kept after delete
  and a refused Node with legs each fail it.

## 2026-10-21 — Optimization review across all areas; the docs check is 96 % faster

The owner asked for every area (engine, build, docs, design and the rest). Measured on Linux:
an M2.6 hour 541 ms (3.81 G Ir), 10 seeds 1.8 s, the docs check 2.2 s Release / ≈20 s Debug.
- **Done:** `multra-check-docs` caches each document's anchors and builds its regexes once
  (24,976 → 865 M Ir; Release 86 ms; `documentation` 21.1 → 0.6 s). Same errors; self-test and
  a broken real anchor still fail.
- **Package workflow:** the first run since D284/D285/D287 (run 39561211587) is green on Linux
  and Windows. Windows copied every vcpkg `copyright` and found libthai's licence folders (no
  warning); both packages carry `multra-cli`, `multra-desktop`, `proj.db` and the notices.
- **Not worth it now, measured:** `Vehicle` moves per tick 5.8 % and `appendSpans` 7 % of an M2.6
  run; test objects are 60 % of a clean compile, but CI's ccache covers warm builds; slow tests
  are deliberate long runs. No design misfit found; the desktop's start-up was not measured.

## 2026-10-21 — Namespace, targets and CMake options renamed to MULTRA (D287)

The owner renamed the repository to `Warissakorn/multra` and asked for the code to follow.
- **Renamed:** `namespace multra`, every library, test and tool target (`multra_core`,
  `multra-tests`, `multra-check-docs`, …), the `MULTRA_*` macros and CMake options, the presets
  and workflows, and the commands in maintained docs.
- **Compatibility:** `-DTRAFFICSIM_BUILD_DESKTOP=OFF` and the other three old options still apply,
  with a deprecation warning (checked: a configure with the old option builds no desktop target).
- **Kept:** `"format": "TrafficSim"` in project files and D286's legacy folder name.

## 2026-10-20 — Licence: GPL-3.0-or-later, third-party notices in every package (D285)

The owner chose the reviewed recommendation over MIT.
- **Why it could change:** every commit is the owner's, with no outside contributor and no fork.
  Commits up to `2053141` stay MIT.
- **Changed:** `LICENSE` (GPL v3 text) and README's notice. `THIRD-PARTY-NOTICES.md` covers
  Qt, PROJ/EPSG, vcpkg's DLLs, nlohmann/json, libthai/libdatrie, the font and OSM tiles, and
  `licenses/` holds the LGPL texts.
- **Package workflow:** both packages copy the notices and texts. Linux adds PROJ's and
  nlohmann/json's copyright files. Windows adds every vcpkg port's `copyright` and fails if none
  are found; a missing libthai licence folder now warns instead of passing silently.
- **Package run:** green on Linux and Windows on 2026-10-21 (see the entry above).

## 2026-10-20 — The project is named MULTRA (D284)

The owner chose **MULTRA — Multi-Level Traffic Analysis**, after reviews of the licence and of
name candidates (Velk, Vialo, Viaflow, SIMTIA, TRIMOSA and others; collisions in D284's search).
- **Renamed:** `multra-desktop` and `multra-cli`, the application and window names, the CLI's
  help and messages, the CSV/TSV and Macro tab marker, the map User-Agent (its contact URL is
  `github.com/Warissakorn/multra`, the repository's name after the owner renames it), both
  locales, the package workflow and the maintained docs' commands.
- **Kept:** the repository, the `trafficsim` namespace, internal targets and `TRAFFICSIM_*`
  options, and the project file's `"format": "TrafficSim"` marker, so every saved file still
  opens. Dated docs keep the old name.
- **Not checked:** domains, GitHub names and trademarks.
- **Found in review (D286):** the new application name moved Qt's per-user folders, so recovery
  files and map tiles written as TrafficSim would be lost after the upgrade. The first start now
  moves them, never overwriting a folder already under the new name (row `legacy-storage`).

## 2026-10-20 — Run view: vehicle items reused across frames (D283); pass complete

The last item of the optimization pass. Each frame deleted and rebuilt every vehicle item.
- **How:** run vehicles are kept by id and moved; a body is rebuilt only when the type or zoom
  changes, and the items of vehicles gone are removed.
- **Measured:** callgrind over 1,500 Steps 15,517 → 14,716 M Ir (−5.2 %); the wall clock moved
  −2.5 %, inside its spread.
- **Rows (run-presentation):** an item survives while its vehicle runs; none outlives its
  vehicle; a followed outline moves; a reused frame equals a rebuilt one after a zoom. Four of
  five mutants caught; a fill change has no forcing case in the fixture (one display type).
- **The pass, Linux:** a Node adds +14–20 % (was +26–39 %); M2.6 run view 24,609 → 15,355 ms per
  6,000 Steps (−38 %); a session's mandatory reads 32.6 → 22.4 KB.
- **CI fix (PR #178):** the sanitizer's use-after-free was a range-for over `classes[0]` of a
  temporary in MA6's test (now named first); MSVC needed `<nlohmann/json.hpp>` in `node_output.cpp`.
  Windows then failed D281's photo row: its pictures are compared with only pixels more than 2
  levels apart counted (blend rounding), and a failure prints its numbers. Its numbers then showed
  a one-pixel band at the photo's border (≤60 levels on ≈1.2k px), so that row compares the interior.

## 2026-10-20 — Run view: the status line no longer relays out the window (D282)

The fourth item of the optimization pass. The word-wrapped `editorRunInfo` label sat in the central
layout, so its new text every Step laid out the whole window (`QLayout::activate` ≈20 % of a Step).
- **How:** the label lives in a host widget without a layout. A new text refits the label inside
  the host, and the host's height changes only when the wrapped line count does. It looks the same.
- **Measured:** M2.6, 6,000 Steps: 17,642 → 15,750 ms (−11 %; −36 % since the pass began).
- **Rows (scenario-run-ui):** the status is as wide as the run view and shows all its lines; a
  narrower window wraps it onto more; 20 Steps post fewer than 3 layout requests to the window.
  Two mutants caught; dropping the resize refit survived, since a text change refits as well.

## 2026-10-20 — Run view: the static network painted from the background cache (D281)

The third item of the optimization pass. Each Step repainted every road: `QGraphicsPathItem::paint`
was 32 % of the run-view profile.
- **How:** while a run frame is shown, the scene items under the lowest level's vehicles (surfaces,
  markings, Connectors, zones, Nodes, conflict areas, the background image) are hidden and painted
  by `drawBackground`, which QGraphicsView caches. Higher levels, counters and overlays stay live;
  every `redraw()` rebuilds and re-bakes, so a selection or edit is never stale.
- **Measured:** M2.6, 6,000 Steps, Linux RelWithDebInfo offscreen: 24,609 → 17,642 ms (−28 %).
- **Rows (run-presentation):** cached and live pictures match (within 32 levels on under 1 % of
  pixels, the pixmap's rounding); a selection mid-run repaints; a half-transparent background image
  keeps its opacity; leaving the run shows every item. Five mutants caught.
- **Owner check:** Windows paint (OWNER_SITTING E22).

## 2026-10-20 — Docs and AI context: NEXT cut to its open state (D280)

The second item of the optimization pass. A session's mandatory reads were ≈8–11k tokens, and
NEXT was 88 % shipped history that PROGRESS and RECORD already hold.
- **NEXT:** 13,877 → about 2.9 KB. The 2026-10-19 state list moved whole to
  [archive/NEXT-state-2026-10-20.md](NEXT-state-2026-10-20.md); the new state lists only
  what is open, and adds S2 and S0.6, which it lacked.
- **AGENTS:** RECORD is appended to, never read whole (≈70k tokens); the file-size rule now says
  what the check enforces (fails above 500); `src/macro/` is in the layout. NEXT and PROGRESS point
  to AGENTS instead of restating its rules.
- **Stale lines fixed:** ARCHITECTURE's Macro row, ROADMAP's S0 line (S0.6; S0.4 moved to S3),
  read-by-task rows for Macro and study periods, a `src/macro/` README, and the decision index
  (a Macro topic row, D275–D279 summaries, five answered choices no longer "awaiting").

## 2026-10-20 — Node evaluation cost: sections by route, tracks in place (D279)

The owner ran an optimization pass and approved the recommended order; this is its first item.
- **Measured** (Linux, GCC 13.3 RelWithDebInfo, callgrind, seed 42): four-leg with a Node 1,303.6 →
  1,128.8 M Ir (−13.4 %), T-junction with a Node 256.2 → 233.2 M (−9.0 %). A Node now adds +20 %
  and +14 % (was +39 % and +26 %, D264). `SectionAccumulator::observe` 325.8 → 150.9 M.
- **How:** `observe` visits only the sections with a line on the vehicle's route, and advances
  known tracks in place instead of moving every Track into a new vector each tick.
- **Unchanged:** every output byte-identical (four shipped projects and two Node fixtures, run and
  seeds 42–44), the pin and the TS rows.
- **Tests:** two section rows, each killing a mutant that survived the suite (a section with only
  its end line on the new route; a track kept after its vehicle left).

## 2026-10-20 — S0.6: Node rows in Results (D278); MA15 paused

MA15 needs Sheffi (1985) from web.mit.edu, which the proxy still refused after the owner allowed
it, so the owner chose other work ([NODES](../reference/NODES.md) §15).
- **Results → Nodes:**
  - **a run:** the Node, leg and turn rows as the CSV block gives them (vehicles, delay,
    unfinished, queue on legs, travel time on turns, LOS by the Node's control type);
  - **a batch:** n, mean, CI and LOS on the mean;
  - **a comparison:** no rows yet, and it says so.

  The marker comes first.
- **Rows:** N19–N21 (`node-results-ui`). The workspace test and the UI audit list the new tab.
- **Mutations:** seven caught. The batch LOS on mean + CI first survived; N20 now checks every
  row's letter.
- **Unchanged:** every output (editor-only), the pin and the TS rows. Linux offscreen only.

## 2026-10-19 — S2.4: Run Macro in stages, Macro links, link types (D277)

The owner chose §19 (a) and wants more capability later
([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §20).
- **Engine:** a convergence record per iteration (gap and step), and an observer that can cancel
  between iterations.
- **Run Macro…:**
  - asks for the period and runs on its own worker;
  - shows the six stages with their status and the live gap;
  - Cancel stops between iterations;
  - it is not an Undo step.
- **Results → Macro links:** a row per arc with the marker. An edit keeps the rows and marks them
  stale until Undo returns to the result's revision. Export and Copy write the Macro CSV.
- **Run settings:** the target gap and the iteration cap; the defaults write nothing.
- **Network → Link types:** Add, Edit and Delete (a type in use cannot go). The Link page gets a
  Macro type combo.
- **Rows:** MA14 (`macro-ui`), MA18 and the stage and command rows. The workspace and object-group
  tests list the new page.
- **Mutations:** nine caught.
- **Found:** a multi-line note made the Objects dock too tall for the wide-canvas row. It is now one
  paragraph.
- **Unchanged:** Micro outputs byte-identical (only `buildCommit` differs), the pin and the TS rows.
  Linux offscreen only.

## 2026-10-19 — How Visum and Aimsun organise a Macro run (D276, docs only)

The owner asked whether a Macro run has a working procedure, then for a study of other programs
([evidence](../evidence/macro-run-procedures.md)).
- **Visum:** an ordered procedure sequence (generation → distribution → mode choice → skim →
  assignment) in the version file, logged. Its equilibrium assignment balances routes in an inner
  loop and searches for new routes in an outer loop, to a gap or a cap.
- **Aimsun Next:** a Static Assignment Scenario (demand, outputs) and an Experiment (method,
  iteration cap, relative gap), with convergence data retrieved from the output database.
- **TrafficSim:** `runMacro`'s six stages are now named in [MACRO_NETWORK](../reference/MACRO_NETWORK.md)
  §19, with a choice for S2.4 that the owner makes before any code.
- **Caveat:** the help pages could not be opened from the session; the facts come from search
  summaries and are marked so.

## 2026-10-19 — S2.3: Macro run settings, outputs and CLI (D275)

A Macro run now writes its results ([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §18). The editor
surface is split off as S2.4, since it is a second system (a worker job, a Results tab, stale
marking, a link-type editor).
- **Settings (schema 51):** `definition.macroSettings{targetGap, maxIterations}`, written only
  when set. Absent means 10⁻⁴ (D272) and 1000 iterations; bad values are `INVALID_MACRO_SETTING`.
- **Run:** `runMacro(doc, data, period)` names a period as its study-period id or
  `matrix:<id>`, refuses the graph's and demand's issues, and adds `MACRO_NOT_CONVERGED` at the
  cap. The input manifest records every file read.
- **Outputs:** `macroJson` and `macroCsv`, one row per arc with the link type, veh/h per class,
  pcu/h, V/C, free and congested time; both carry the not-validated marker.
- **CLI:** `--macro PERIOD` with `--project` (and `--csv`, `--data-dir` only).
- **Rows:** MA12, MA13 (ctest `macrooutput`, `cli-macro*`). Writing MA12 showed that S2.2's MA11
  twin had no route choice (each entry Link had its own zone connector); S2.3's fixture fans one
  origin out to two routes, and MA11's comment now says so.
- **Mutations:** nine caught. A missing class column first survived (the row checked only its
  prefix); MA13 now compares a whole CSV row.
- **Unchanged:** Micro outputs byte-identical (only `buildCommit` differs), the pin and the TS
  rows. Linux only.

## 2026-10-19 — S2.2: the Macro assignment (D274)

The Macro graph can now be assigned ([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §17). There are
no outputs or editor yet.
- **Engine** (`src/macro/assign.*`, pure):
  - Dijkstra, where on equal distance the lower link index wins;
  - all-or-nothing loading;
  - multi-class Frank–Wolfe on the shared load Σ pcu·v, with an exact bisection line search;
  - §7's relative gap. Stopping at the iteration cap is flagged, and the flows are still
    returned.
- **Rows:** MA7–MA11, MA16, MA17 (ctest `macroassign`). They cover:
  - closed-form and bisected equilibria;
  - an unused route;
  - the gap recomputed from the flows;
  - identical results with Links permuted;
  - a mode ban;
  - mixed PCU.
- **Mutations:** seven caught. An MSA step first survived, because it still converged. Row MA8 now
  pins that two routes reach equilibrium in one exact step.
- **Unchanged:** Micro outputs, the pin and the TS rows. Linux only.

## 2026-10-19 — S2.1: link types and the Macro graph (D273)

The first Macro code ([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §16). There is no assignment
yet.
- **File (schema 50):** a Link's `linkType` and project-owned `definition.linkTypes`
  `{capacityPerLane, freeSpeed, vdfId, modes, source}`. No type ships; `data/vdf/bpr.json` does.
- **Graph:** `compileMacroNetwork` splits Links at Connector stations into capacitated pieces.
  Connectors become fixed-time arcs at the from-Link's speed, and each zone connector gets its
  own centroid node. The new `src/macro/` is a pure module.
- **Demand:** `macroDemand` gives veh/h per class (mode, or composition) and period by connector
  share. `MODE_PCU_MISSING` and `MACRO_NO_PATH` are refused.
- **Rows:** MA1–MA6 (ctest `macronetwork`); nine mutations caught. Micro outputs, the pin and the
  TS rows are unchanged. Linux only.

## 2026-10-19 — The S2 (M10.1) contract: Macro network and assignment (D271, D272, docs only)

The owner chose S2. [MACRO_NETWORK](../reference/MACRO_NETWORK.md) fixes how the drawn network
compiles into a Macro graph and how one study period is assigned. Nothing is implemented.
- **Graph:** Links split at Connector stations become capacitated links. Connectors and zone
  connectors become uncapacitated arcs. Nodes are not read yet.
- **Values:** `Link::linkType` and link types `{capacityPerLane, freeSpeed, vdfId, source}` at
  schema 50, none shipped; BPR 0.15/4 as data (D158).
- **Demand:** pcu/h by connector share for one period; `MODE_PCU_MISSING` is refused.
- **Engine:** a pure `src/macro/` Frank–Wolfe with a named gap formula; non-convergence is
  flagged, never hidden.
- **Rows:** MA1–MA15, planned.
- **Source checks** changed three recommendations:
  - the BPR pair assumes practical capacity, so a type's source must say which capacity it gives;
  - Boyce et al. (2004) argue for a 0.01 % gap;
  - the Sioux Falls data are for academic research only, so the benchmark stays local until the
    licence is confirmed.
- **The owner answered the same day (D272):**
  - one class per mode, with mode bans on link types;
  - a textbook benchmark instead of Sioux Falls;
  - a 0.01 % gap;
  - S0.4 moves to S3;
  - the other choices as recommended.

## 2026-10-19 — S1.2: modes and PCU (D269, D270)

S1 is complete ([STUDY_PERIODS](../reference/STUDY_PERIODS.md) §6, §14).
- **The call (D269):** the owner asked for the best design from the project's own rules. Modes join
  the Demand catalog under its presence rule: `definition.modes` absent is `data/modes/` (`car`,
  `mixed` ship). A project owning its compositions owns its modes once a matrix names one, so it
  still runs with no data directory, and a catalog copy keeps the schema of a project using none.
- **File (schema 49):** a matrix's `modeId` in place of `compositionId` (`OD_TYPES_AMBIGUOUS`),
  owned `definition.modes`, and a vehicle type's `pcu: {value, source}` (`INVALID_PCU`). Each is
  written only when used. No PCU value ships (D267).
- **Run:** a moded matrix runs byte-identical to its composition twin. `UNKNOWN_MODE` and
  `MODE_COMPOSITION_UNKNOWN` are Run refusals. Modes are read only when a matrix names one.
- **Editor:**
  - a Modes tab with the derived PCU ("—" while unknown); owned modes are editable;
  - the OD dialog's Types choice;
  - a PCU row in the type dialog.

  Each commit is one Undo step.
- **Rows:** SP9–SP14 (ctest `modepcu`, `modepcu-ui`); nine mutations caught. Shipped outputs, the
  pin and the TS rows are unchanged. Linux only.

## 2026-10-19 — S1.1: study periods (D268)

OD matrices can now be placed on the study clock ([STUDY_PERIODS](../reference/STUDY_PERIODS.md) §13).
- **File (schema 49):** `studyPeriods[]` in clock seconds, `definition.clockStart`, and a matrix's
  `periodId` in place of its own times. Each is written only when used, so schema-31 and shipped
  files keep their bytes.
- **Run:** `odMatrixWindow` places a period at `start − clockStart`. A placed matrix runs
  byte-identical to its run-second twin, and a window outside the run is `OD_PERIOD_INVALID`.
- **Editor:**
  - a Study periods tab (HH:MM:SS; an end before the start is the next day);
  - the Run settings clock field;
  - the OD dialog's period choice.

  Each commit is one Undo step.
- **Rows:** SP1–SP8 (ctest `studyperiods`, `studyperiod-ui`); six mutations caught. Shipped outputs,
  the pin and the TS rows are unchanged. Linux only.

## 2026-10-19 — The S1 contract: study periods, modes and PCU (D266, D267, docs only)

The owner chose S1 next. [STUDY_PERIODS](../reference/STUDY_PERIODS.md) fills what the workflow left
open:
- **StudyPeriod:** clock periods in a top-level array at schema 49.
- **Placing a period:** the run's `clockStart` places each matrix's period on run time.
- **Matrices:** each takes exactly one of its own times or a `periodId`, and exactly one of a
  composition or a mode. Schema-31 files keep their bytes because nothing is migrated.
- **Modes and PCU:** a `data/modes/` catalog, and an optional cited `pcu` per vehicle type with a
  derived mode PCU. No value ships without a source.
- **Rows:** SP1–SP14, planned. No code changed.
- **The owner accepted every recommendation (D267):** seconds since midnight, `clockStart` on the
  run, no profile or flow basis stored yet, and the PCU field without values.

## 2026-10-19 — Nodes S0.5: Suggest nodes (D265)

The Nodes tab can now draw one `generated` Node per junction no Node covers ([NODES](../reference/NODES.md) §6, §14).
- **The junction rule (D265):** D259 named `conflictGroups`, which are pairs of road objects. A
  junction is now a cluster of conflicts (authored and automatic) joined through their Connectors,
  fed from at least two Links. Its polygon is the padded convex hull of its Connectors and conflict
  points. Overlapping hulls merge; one over an existing Node is dropped.
- **Shipped projects:** one Node each on the four-leg and M2.6 (12 turns; the approach lane adds
  left out) and on the T-junction (4 turns); none on the lane lab. Each membership has no issue.
- **Rows:** N18 (ctest `nodesuggest`) and the `node-ui` action check; six mutations caught.
- **Unchanged:** every run output, the pin and the TS rows. Linux only.

## 2026-10-19 — Nodes S0.3: Micro node evaluation (D264)

Each Node now reports per-turn, per-leg and per-Node delay ([NODES](../reference/NODES.md) §4, §13).
- **How:** one travel-time section per turn (in-leg Link start → out-leg crossing) and one queue
  counter per in-leg at its crossing, measured by the existing accumulators after the authored ones;
  `nodeRows` weights leg and Node delay by vehicles.
- **Output:** JSON `nodes[]`, a CSV block after LOS with its marker line, batch estimates
  `{n, mean, sd, halfWidth95}`, letters with the Node's control type. Nothing without a Node: the
  shipped projects' single and batch outputs are byte-identical.
- **Finding:** vehicles enter from rest, so an in-leg starting at a source includes the start-up
  loss (about 3 s of the T-junction's 3.2 s major-road delay). Choice 1a kept; N14 uses a split
  in-leg.
- **Cost:** no change without a Node; with one, Ir +40 % (four-leg) and +26 % (T-junction).
- **Rows:** N12–N17 (ctest `nodeevaluation`); five mutations caught. Pin and TS rows unchanged.
  Linux only.

## 2026-10-19 — Nodes S0.2: membership (D262, D263)

What a Node holds is now derived from the network inside its polygon ([NODES](../reference/NODES.md) §3, §12).
- **Contract fixed first (D262):** §3 refused any connected Link crossing the boundary twice, which
  refused the shipped T-junction, whose major roads run through as single Links. Legs are now
  boundary crossings: an entry is an in-leg, an exit an out-leg, and a Link's own run through is a
  turn. `NODE_LINK_REENTERS` replaces `NODE_LINK_CROSSES_TWICE`.
- **Model:** `nodeMembership` (`src/model/network/node_membership.*`) gives inside objects, legs,
  turns (checked with `routeLaneChains`), the turn-conflict table and control. Refusals are advisory
  diagnostics at `nodes[i]`, so Run is never blocked.
- **Editor:** the Nodes tab shows control, legs, turns and issues.
- **Rows:** N6–N11 (ctest `nodemembership`); the four-leg gives 4 + 4 legs, 12 turns, signalised;
  the T-junction 3 + 2 legs, 4 turns, unsignalised and 3 conflict rows.
- **Unchanged:** the engine reads no Node; the pin and the TS rows are unchanged. Linux only.
