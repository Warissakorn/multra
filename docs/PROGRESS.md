# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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
(dynamic assignment) waits on M9. [MESO_NETWORK](reference/MESO_NETWORK.md) fixes the following:
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

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
