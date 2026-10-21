# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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
  [archive/NEXT-state-2026-10-20.md](archive/NEXT-state-2026-10-20.md); the new state lists only
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
it, so the owner chose other work ([NODES](reference/NODES.md) §15).
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

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
