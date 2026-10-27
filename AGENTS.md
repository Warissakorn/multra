# MULTRA

Standing orders for anyone — human or model — working in this repo. Read this first, every
session.

## Session path

These instructions apply to the whole repository, regardless of the AI tool used. The user's
current instruction takes priority over the standing queue; without one, follow NEXT and do not
silently reorder the owner's queue.

1. **[NEXT](docs/NEXT.md)** — the one live to-do.
2. **[The task reading map](docs/README.md#read-by-task)** — the row for your change names the
   contract, acceptance rows and code entry points. Read [ARCHITECTURE](docs/ARCHITECTURE.md)
   before adding a system, [PROBLEM](docs/PROBLEM.md) before proposing a feature and
   [PRINCIPLES](docs/PRINCIPLES.md) before arguing with an existing choice.
3. **The module README** in `src/<module>/` and the decisions the contract cites. Reach into
   PROGRESS or the [decision index](docs/decisions/README.md) only for the entry behind what you
   change; never load the archive wholesale.
4. **Run the checks below** and report unavailable tools or failed checks honestly.

Read [CONNECTOR_PARITY_AUDIT](docs/audits/CONNECTOR_PARITY_AUDIT.md) before touching the Connector:
it separates the supplied spec from never-measured Vissim behaviour.

MULTRA (Multi-Level Traffic Analysis) owns a traffic microsimulation engine for engineering studies
(D38). The owner chose the name in D284, superseding D11 and D201's B1; the licence is
GPL-3.0-or-later (D285). The repository, executables, `multra` namespace, targets and `MULTRA_*`
options carry the name (D287). Only the project file's `"format": "TrafficSim"` marker and the
legacy-storage move (D286) keep the earlier name; never change that marker without a schema step
that still reads it.

## Documentation authority

| Information | Maintained source |
|---|---|
| Next work | [NEXT](docs/NEXT.md), rewritten rather than appended |
| Everything waiting on the owner | [OWNER_SITTING](docs/plans/OWNER_SITTING.md) |
| Milestone scope, status and closing gates | [ROADMAP](docs/ROADMAP.md) — the only status ledger with NEXT |
| Project principles and their reasoning | [PRINCIPLES](docs/PRINCIPLES.md) |
| Module boundaries and interfaces | [ARCHITECTURE](docs/ARCHITECTURE.md) and module READMEs |
| Behaviour and file semantics | The relevant contract in [the documentation map](docs/README.md) |
| Session history and decisions | [PROGRESS](docs/PROGRESS.md) and the [decision record/index](docs/decisions/README.md) |
| Measurements, recipes, reverted ideas | [ENGINEERING_NOTES](docs/reference/ENGINEERING_NOTES.md) |
| Toolchain setup | [BUILDING](docs/BUILDING.md) |

READMEs are navigation, not status. Dated audits, plans and archive entries describe their own
date. A supplied target specification is not proof of implementation or validation. If sources
disagree, inspect the applicable decision, code and tests; record the discrepancy rather than
silently changing an owner gate. Index every new maintained doc in its folder README in the same
commit (`multra-check-docs`).

## Stack

C++20, CMake 3.24+, Qt 6.4+ Widgets for the desktop, and nlohmann/json outside the core.
`core/` depends only on its own headers and the standard C++ library. No Qt, file I/O,
JSON, model types or wall clock may reach it. D15 supersedes the initial D3/D4 stack.
See `docs/BUILDING.md` and `docs/reference/MIGRATION.md`.

## Commands

```bash
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
./build/desktop/bin/multra-desktop
./build/desktop/bin/multra-cli 42
cmake --build build/desktop --target check
```

Use `--preset headless` when Qt is unavailable. Do not claim desktop verification
from a headless-only build.

If either fails on a clean checkout, fixing that comes before any feature work.

## Hard rules

The full list with reasoning is in [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md), except rule 5
below, whose reasoning is D7. The ones that get broken by accident:

1. **`core/` imports nothing.** No UI, no I/O, no framework. The moment it does, the engine
   stops being testable, batchable and portable, and that is the project's most valuable
   property.
2. **Reproducibility is not optional.** Same scenario + seed + engine/toolchain = the same trajectory.
   No wall-clock, no unordered iteration, no thread-dependent floating point in `core/` or
   `eval/`.
3. **One source of truth.** If two places need the same value, the boundary is wrong — move
   it, do not sync it.
4. **Never claim fidelity that has not been measured.** Until M6 passes, every results screen
   carries a "not yet validated" marker.
5. **Content is data, not code.** Vehicle types, behaviour presets, LOS thresholds live in
   `data/`. If adding the 50th one needs a code edit, fix the boundary instead.
6. **Files stay at or under 500 lines.** `multra-check-file-sizes .` fails above 500; split before that.
7. **Never leave the build red.** If it cannot be made green, revert to the last green commit
   and write down what was attempted.
8. **Never close a milestone that has not met its gate.** Merged code is not a passed gate.
   If something must ship incomplete, carve it into a new numbered milestone in
   `docs/ROADMAP.md` in the same session.

## Working rules

- **One system per session.** If it looks like two, it is two.
- **Write the interface first** — the functions other systems will call. That contract is what
  lets a later session build against this without reading its insides.
- **Smallest version that works.** Speculative generality written blind is the most expensive
  thing in this repo.
- **Read D28 before adding another cache** — the editor caches Connector geometry against the
  values it is derived from; the same cache for Link geometry was measured and reverted.
- **Benchmarks are not in `check`:** re-run `tools/*_benchmark.cpp` before repeating any number,
  and a harness that drives Qt pumps the event loop between iterations, or it times Qt's deferred
  work instead of the code (D31).
- **Update `docs/PROGRESS.md` before committing** — what changed and the reasoning behind any
  non-obvious decision, in a short entry; keep about ten entries and move older ones whole to the
  archive. Record new decision IDs and reasoning once in `docs/decisions/RECORD.md`. **What is
  next goes in `docs/NEXT.md` instead**, rewritten rather than appended, so there is one live to-do.
- **Append to `docs/decisions/RECORD.md`, never read it whole** (≈95k tokens, 385k characters):
  add the row with a shell append, and read a cited row by its anchor with `grep -n 'id="dNNN"'`
  or the file's tail.

## Session end

Stop at roughly three-quarters of context, or when the current system is done. In order:
build green (or reverted to green) → `docs/NEXT.md` rewritten specifically enough to need no
questions → session logged in `docs/PROGRESS.md`, decisions with reasons in `docs/decisions/RECORD.md` → commit → tell the user what
changed, in outcomes.

## Layout

```
docs/           NEXT · PROBLEM · PRINCIPLES · ARCHITECTURE · ROADMAP · PROGRESS · README (map)
src/core/       simulation engine — imports nothing
src/model/      network · demand · control data model
src/commands/   every mutation, undoable, one registry
src/project/    load, save, revisions, validation
src/editor/     canvas: tools, gestures, run view
src/shell/      window, inspector, tables, results, layout, palette, i18n
src/eval/       event stream → measurements
src/macro/      static assignment: Macro graph and Frank–Wolfe — imports nothing (M10.1)
src/meso/       Meso graph; the link-transmission engine next — imports nothing (M11)
src/analysis/   analytical junction capacity (HCM 7 Ch. 19) — imports nothing (M13)
src/runner/     multi-seed batches and scenario comparison (M5)
src/lab/        behaviour lab: tuning ring, live run and retune (D165)
src/geo/        Qt-free PROJ transforms and XYZ tile math for the OSM background (desktop only)
src/report/     reserved for impact-study reports (README only; formatting stays in src/project/, D131)
data/           vehicle types · behaviour presets · compositions · priority rules · levels ·
                display types · vehicle appearance · LOS packs · evaluation defaults · map providers ·
                fonts · locales · example projects and scenarios
tools/          CLI, sweeps, benchmarks, checks
tests/
```

## Project-specific rules

<!-- Conventions discovered while building. Add here rather than re-deriving them. -->
- Documentation is written in **English**. UI strings live in translation files only.
- Screen vocabulary follows Vissim ("link", "connector", "conflict area"); **parameter names
  are never translated**, so users can find them in the project file.
- The default car-following is a reduced Wiedemann-inspired prototype; W74 is selectable since
  D138; W99 does not exist. None is validated: never remove the unvalidated marker.
- The previous TypeScript application is retained in Git history, not as a second engine.
- Native tests compare four TS baselines with a 1e-7 physical-value tolerance and exact
  same-build replay, under `SeedStart::raw` (the pre-D159 seed mapping, used nowhere else).
  Never regenerate baseline fixtures to make a failing port pass.
- UI text lives in `data/locales/`. Runtime catalogs are copied beside executables.
- **A test that forces a failure must first assert that the forcing worked**, then assert the
  consequence. Order it the other way and a platform where the setup silently no-ops reports a
  product bug that is not there. Never force one with a platform-specific mechanism — POSIX
  permissions, root behaviour, or `XDG_*` variables, which Windows and macOS ignore.
- A UI suite that builds an `EditorWindow` calls `useTestStorage()` (`tests/ui_test_storage.hpp`)
  after its `QApplication` (D320), never `setTestModeEnabled` alone.
- Qt UI suites run on Linux **and Windows** in `native.yml`. "Verified locally" means Linux
  only; say so, and do not read a green Linux run as cross-platform evidence.
- The owner authorized the full stack migration (D15); it supersedes one-system scheduling
  guidance for that migration only. Existing M0/M1 acceptance gates still apply.
- The owner authorized M1.1–M1.3 together (D16). Project never imports command or Qt types; the shell owns atomic file replacement. Never equate authoring support with runtime support.

## Model invariants to check before editing

- Scenario JSON and editor `*.traffic.json` are deliberately different formats (D19a).
  Read NETWORK_EDITOR's save/recovery contract before changing either loader. Unsupported
  network-object fields must fail on load rather than vanish on save.
- Authored routes name Links and Connectors, never lanes. `buildScenario` expands
  `routeLaneChains`; authoring-only Demand fields compile into ordinary core inputs.
  Keep disconnected routes and report `UNSUPPORTED_ROUTE_TOPOLOGY`; Run refuses them,
  authoring does not. Read the Demand contracts before changing that expansion.
- Do not reopen settled geometry without new measurements: mitered `offsetGeometry`
  stays (D23), and the M1.17 lateral mouth wedge stays reverted in favour of M1.18.
- Consult SIMULATION for results semantics. Completed-trip/whole-route delay includes
  source waiting and acceleration; it is not HCM control delay or validated LOS.
