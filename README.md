# MULTRA

**MULTRA — Multi-Level Traffic Analysis.** A traffic microsimulator with its own **C++20 engine,
link-based network model and Qt 6 Widgets desktop interface**, built to the modelling surface
PTV Vissim users already know and aimed at the movement-level delay and queue output that traffic
impact studies need. The owner chose the name in D284. The repository, the C++ namespace, internal
build targets and the project file's `"format": "TrafficSim"` marker still carry the earlier
working name.

`multra-desktop` opens the Network Editor:
- Links, Connectors and turn pockets, drawn on an aerial image or an OpenStreetMap background.
- Routes, vehicle inputs, routing decisions, fixed-time signal controllers, conflict areas with
  priority and Stop/Yield, queue counters and travel-time sections.
- Zones, OD matrices and capacity-limited parking lots.
- Run, Pause, Step and Reset on the same canvas; Results with per-movement delay, queues, section
  LOS, multi-seed batches with 95 % confidence intervals and scenario comparison, exportable as CSV.
- A Behaviour Lab window for tuning driving behaviour on a test circuit.

The controls are in English and Thai, with a bundled Noto Sans Thai font. Milestone status and
gates are in [`docs/ROADMAP.md`](docs/ROADMAP.md); what is next is in [`docs/NEXT.md`](docs/NEXT.md).

The supplied Link, Connector and Network Editor target specs are retained in
[`docs/specs/`](docs/specs/README.md), with a [code audit](docs/audits/SPEC_AUDIT.md). These documents
are not claims of Vissim parity.

**Not yet validated:** the default longitudinal model is a reduced Wiedemann-inspired prototype;
W74 is selectable and uncalibrated; nothing is a calibrated Vissim equivalent. Movement delay is
completed-trip delay, including source waiting and acceleration — **not HCM control delay** — and
LOS letters come from section delay with the same not-yet-validated marker.

## Build and run

Requires a C++20 compiler, CMake 3.24+, Ninja and nlohmann/json 3.11+.
The desktop additionally requires Qt 6.4+ Widgets/Network and PROJ (Qt Test when tests are enabled).
There is no Node.js, npm, browser, Python or server dependency in the application.

Ubuntu 24.04 development setup:

```bash
sudo apt-get install g++ cmake ninja-build nlohmann-json3-dev qt6-base-dev libproj-dev
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
./build/desktop/bin/multra-desktop
```

Windows instructions for Qt/Visual Studio and dependency paths are in
[`docs/BUILDING.md`](docs/BUILDING.md). Qt must match your compiler and architecture.

Headless build, with **no Qt dependency**:

```bash
cmake --preset headless
cmake --build --preset headless
ctest --preset headless
./build/headless/bin/multra-cli 42
cmake --build build/headless --target check
```

Data is copied beside the executables at build time. Both programs can start from a
different working directory. For a custom data installation use `--data-dir <directory>`;
for another M0 fixture use `--scenario <file.json>`. The desktop accepts `--language th`.
See [`tools/README.md`](tools/README.md) for all CLI and developer-tool commands.

### Prebuilt binaries from CI

`.github/workflows/native.yml` builds and tests every push and pull request.
`.github/workflows/package.yml` produces downloadable builds for manual testing: open
**Actions → Package binaries → Run workflow** (or push a `v*` tag), then download the
`trafficsim-linux-x86_64` or `trafficsim-windows-x64` artifact. Each archive contains
`bin/multra-desktop`, `bin/multra-cli`, the `data/` catalogs and a `RUN.txt`.
The Windows build bundles its Qt runtime; the Linux build needs Qt 6 Widgets installed
(`sudo apt install libqt6widgets6`). These are unsigned test builds, not a release
installer — M7 owns installation.

## Implemented

| Part | Entry point | Behaviour |
|---|---|---|
| Simulation core | `src/core/simulation.hpp` | Fixed stepping, seeded arrivals, prototype or W74 car-following, fixed-time signals with amber, parking lots, conflict-area and merge right-of-way, commitment at waiting lines, mandatory lane changes with cooperative braking. Imports nothing. |
| Network and demand model | `src/model/` | Links, lanes, Connectors, routes, inputs, routing decisions, signal controllers, conflict areas, queue counters, sections, zones, OD matrices and parking lots, compiled into core inputs |
| Project files | `src/project/` | Strict load and save of `*.traffic.json` (reads schemas 1–32; writes 17–32 by feature) and M0 scenario JSON, validation, movement evaluation and its CSV |
| Evaluation | `src/eval/` | Per-movement delay, approach queues, section delay/LOS, discharge and lot reports from one run's event stream |
| Batches | `src/runner/` | Independent seeds in parallel, 95 % CI aggregation and scenario comparison |
| Desktop | `src/shell/`, `src/editor/` | The Network Editor, Run view, Results tab and Behaviour Lab, in English and Thai |
| CLI | `tools/run_simulation.cpp` | One seeded run, `--seeds` batches or `--compare` of a scenario or project; summary JSON, optional events and `--csv` |

Validation against field data is M6 and has not passed; every result carries the not-yet-validated
marker. Read [`docs/reference/SIMULATION.md`](docs/reference/SIMULATION.md) for numerical behaviour.

## Migration evidence

The TypeScript baseline remains in Git at
[`70383db`](https://github.com/Warissakorn/trafficsim/commit/70383db6ab884c718baef97a8ab81292fdc9d1b0).
The C++ tests compare four baseline seeds: all non-movement events and complete state
checkpoints every 10 simulated seconds, with a 1e-7 absolute tolerance on physical values.
Same-build C++ replay compares the full event stream exactly.

`multra-cli 42` (D159 seeding): 180 seconds, **45 completed, 1 active, 0 pending**, 1 safety
clamp, mean completed-trip delay **31.75870154816726 s**. The TS baselines replay under the old
seed mapping (`SeedStart::raw`). This is a regression fixture, not
scientific validation or a performance benchmark. See [`docs/reference/MIGRATION.md`](docs/reference/MIGRATION.md).

## Project map

AI development starts at [AGENTS.md](AGENTS.md); read only the task-specific context
selected by the documentation map.

[`docs/README.md`](docs/README.md) lists every document by purpose. The ones to read first:

| Document | Purpose |
|---|---|
| [`docs/NEXT.md`](docs/NEXT.md) | The one live to-do |
| [`docs/PROBLEM.md`](docs/PROBLEM.md) | Audience, scope and why this owns an engine |
| [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md) | Correctness and working rules |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Modules, dependencies and contracts |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Milestones M0–M12, their status and acceptance gates |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | Session log and the decision record |
| [`AGENTS.md`](AGENTS.md) | Shared working instructions for humans and AI tools |
| [`CLAUDE.md`](CLAUDE.md) | Claude entry point, pointing to the shared instructions |

## Native network editor

Launch `multra-desktop --language th`; the editor is what the application opens. The Network Objects sidebar provides Links, Connectors,
Routes, Vehicle inputs and Signal heads. Ctrl+right-drag creates links and connector
lane ranges; the Objects dock has demand and signal-program editing actions.

Draw a network, add a route and vehicle input, then Run (F5) in the editor. Step (F6),
Reset, seed and playback speed operate on one explicit document revision. Successful
edits invalidate that run. Save/Open preserves geometry, demand, embedded images,
levels and display types; locked recovery copies protect unsaved work.

See [the editor guide](docs/reference/NETWORK_EDITOR.md) for controls and file semantics. M1's acceptance
record is [`docs/plans/M1_ACCEPTANCE.md`](docs/plans/M1_ACCEPTANCE.md). Accepting the editor does not close M0
plausibility or engine validation.

Map → **Map background settings** adds an OSM background using a fixed UTM
reference; the ordinary Canvas gestures pan/zoom it with the roads. See the
[map contract](docs/reference/MAP_BACKGROUND.md) for study extent, accuracy and
cache/network limits.

## License

Copyright (C) 2026 Warissakorn.

MULTRA is free software: you can redistribute it and/or modify it under the terms of the
[GNU General Public License](LICENSE) as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
WARRANTY; see the License for details. The licence covers the program, not the results a user
produces with it.

Commits up to `2053141` (2026-10-20) were published under the MIT License and stay available
under it. Qt, PROJ, nlohmann/json, libthai and the bundled Noto Sans Thai font keep their own
licences; see [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) (D285).
