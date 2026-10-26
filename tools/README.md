# Native tools guide

All executable project tools are C++20. Run build commands from the repository root.
See `../docs/BUILDING.md` for Windows/Qt setup; no Node.js or Python is needed.

## Setup and quick reference

```bash
cmake --preset headless
cmake --build --preset headless
cmake --build build/headless --target check
```

| Source | Executable | Purpose |
|---|---|---|
| `project_schema.cpp` | `multra-project-schema` | Print a partial formal project envelope/background and Network/Link/Lane schema with exact integer bounds; `--help` explains its limits |
| `run_simulation.cpp` | `multra-cli` | Run a seeded M0 scenario without a window |
| `desktop_control.cpp` | `multra-control` | Send a JSON command to a visible desktop's local automation endpoint; desktop build only |
| `check_architecture.cpp` | `multra-check-architecture` | Check core/eval include boundaries and forbidden nondeterministic APIs |
| `check_file_sizes.cpp` | `multra-check-file-sizes` | Enforce the source-file line budget |
| `check_docs.cpp` | `multra-check-docs` | Check maintained Markdown local links, anchors and folder indexes; supplied spec parts are excluded |
| `node_detail_benchmark.cpp` | `multra-node-detail-benchmark` | Time F4 for unconnected four-arm Nodes; `16 output.json` prints three timings and saves the first document for byte comparison; not in `check` |
| `validate_benchmark.cpp` | `multra-validate-benchmark` | Time `validateDocument`, which every editor command runs (D140); prints, never asserts; not in `check` |

The executables are under `build/headless/bin/` or `build/desktop/bin/` for the Ninja
presets. On Windows add `.exe`. Multi-configuration generators add the configuration
name after `bin/`.

## Simulation runner

```bash
./build/headless/bin/multra-cli
./build/headless/bin/multra-cli 42
./build/headless/bin/multra-cli --seed 123
./build/headless/bin/multra-cli --scenario data/scenarios/crossing.json --data-dir data
./build/headless/bin/multra-cli 42 --events run-42.jsonl
./build/headless/bin/multra-cli --project data/projects/four-leg-signalised.traffic.json --seeds 42-51 --csv four-leg-10.csv
./build/headless/bin/multra-cli --help
```

| Argument | Meaning | Default |
|---|---|---|
| Positional seed or `--seed N` | Unsigned integer 0–4294967295; specify once | 42 |
| `--scenario FILE` | M0 authoring network plus scenario definition | `data/scenarios/crossing.json` beside executable |
| `--data-dir DIR` | Vehicle/behaviour catalogs; desktop also reads locales here | Data beside executable, then working directory |
| `--events FILE` | Stream all events as JSON Lines | No trajectory file |
| `--help` | Usage without loading assets or running a simulation | — |
| `--project FILE --seeds LIST --compare ALT` | Scenario comparison (M5.8a): ALT runs over the same seeds; alternative minus base per movement, section, approach and the network, Welch 95 % CI, unmatched rows listed; refused when the evaluation periods differ; `--csv` writes the table. See [BATCH](../docs/reference/BATCH.md) §7 | Off |
| `--project FILE --macro PERIOD` | Static Macro assignment for a study period or `matrix:ID`, using project settings; accepts `--csv`. OD targets can be Links or directed Link-less Roads (schema 57, D340) | Off |
| `--project FILE --macro-run ID` | Select a saved period/settings definition; JSON/CSV identify it. Mutually exclusive with `--macro`. See [MACRO_NETWORK §21](../docs/reference/MACRO_NETWORK.md#named-macro-runs) | Off |
| `--project FILE --meso` | Full-run Meso loading; accepts `--csv`. Direct Road targets opt OD into minimum free-flow-time paths, identified in JSON/CSV. See [MESO_ROAD_DEMAND](../docs/reference/MESO_ROAD_DEMAND.md) | Off |
| `--threads N` | With `--seeds`: at most N seeds at once (D152); results identical for every N | One per hardware thread |
| `--project FILE --seeds LIST` | Multi-seed batch (`42-51`, `1,5,9`): n, mean, SD and 95 % CI per movement/approach, per-seed accounting, overloaded seeds flagged and kept, `buildCommit`; `--csv` writes the batch table. See [BATCH](../docs/reference/BATCH.md) | Off |
| `--project FILE --discharge` | Unvalidated lane/cycle queue headways and startup estimates | Off |
| `--discharge-start S`, `--discharge-end S`, `--discharge-warmup S` | Declared measurement window and warmup; requires `--discharge` | Full run, zero warmup |
| `--discharge-steady-first N`, `--discharge-steady-last N`, `--discharge-startup-last N` | Original one-based crossing ranks | 3, 5, 2 |
| `--discharge-type ID` | Repeatable selection of original follower gaps by vehicle type | All types |

See [the discharge contract](../docs/reference/DISCHARGE.md) for raw crossings,
unavailable results, selection semantics and captured parsed-byte input hashes. `discharge_options.hpp` owns CLI
syntax; scientific validation and estimation stay in `eval/discharge.*`.

Stdout contains a JSON diagnostic; errors go to stderr with a nonzero exit code.
Invalid/overflow/fractional seeds are rejected. Missing files, wrong JSON field types,
unknown references, off-grid timing and unsupported topology fail before stepping.
`--events` requires a new filename and will not overwrite an existing file.

The loaded scenario can contain explicit `definition.vehicleTypes` and
`definition.behaviours` arrays. Otherwise every `.json` file in the corresponding
catalog directory is loaded in filename order. Adding the 50th vehicle type needs no
C++ edit. The authoring fixture format is documented in `../docs/reference/SIMULATION.md`.

Seed 42 outputs 45 completed trips, 1 active, 0 pending, mean delay
31.75870154816726 seconds and 1 safety clamp after 180 simulated seconds (D159 seeding).
This is an unvalidated completed-trip diagnostic, **not HCM control delay or LOS**.

| Output field | Interpretation |
|---|---|
| `validation` | Always `not-yet-validated` in M0 |
| `engineVersion` | Native engine generation; store with report runs |
| `compiler` | Compiler identifier/version used to build the executable |
| `seed`, `time` | Input seed and final simulated time |
| `completed` | Vehicles whose front reached the route sink |
| `active` | Vehicles still in the network |
| `pending` | Sampled arrivals still waiting outside source lanes |
| `meanTravelTime` | Mean in-network time for completed trips, or null |
| `meanDelay` | In-network time plus source wait minus desired-speed reference, or null |
| `safetyClamps` | Numerical caps that prevented longitudinal overlap/stop-line crossing |

Event files may become large. Without `--events`, movement events are filtered at the
sink and no trajectory history is retained. The state still contains latest-step events.

## Architecture and size checks

```bash
./build/headless/bin/multra-check-architecture .
./build/headless/bin/multra-check-architecture --self-test
./build/headless/bin/multra-check-file-sizes .
```

The architecture checker permits only literal same-directory headers and a reviewed
standard-library include list in core/eval; eval additionally reads the core event
contract. It rejects Qt, I/O, random-device/clock/unordered APIs, macro includes and
module imports. Negative fixtures ensure obvious forbidden imports fail. This is a
restricted source scanner, not a C++ AST parser; source review covers nonstandard
preprocessor forms. Do not expand the allowlist to hide a boundary violation.

The size checker scans C++ headers/sources, Markdown and CMake, skips build/output and
hidden directories, and fails above 500 lines.
Exit 0 means pass, 1 means a violation; command misuse returns 2.

## Tests and daily workflow

```bash
cmake --build --preset headless
ctest --preset headless
ctest --test-dir build/headless -R reference --output-on-failure
./build/headless/bin/multra-tests core
cmake --build build/headless --target check
```

CTest covers core/network semantics, strict project input, four saved TS baselines,
architecture checks and CLI boot/errors. A desktop build adds controls, translation,
full-run agreement and failed-load preservation checks using Qt's offscreen platform.
Tests use explicit checks and remain active in Release builds.

Before committing: native build green, appropriate CTest suites green, source checks
green, `docs/PROGRESS.md` updated. Automated regression does not replace owner M0 review.

| Symptom | Action |
|---|---|
| CMake cannot find nlohmann/json | Install its development package or set `MULTRA_JSON_INCLUDE_DIR` |
| CMake cannot find Qt Widgets | Select the correct kit with `CMAKE_PREFIX_PATH`, or use the headless preset |
| Windows reports missing Qt DLLs | Add the matching Qt `bin` to PATH or run `windeployqt` |
| Cannot locate data | Keep `data/` beside executable or pass `--data-dir` |
| Baseline mismatch | Inspect the first differing event/checkpoint; do not regenerate fixtures to hide it |
| `UNSUPPORTED_MERGE` | M0 cannot arbitrate merging streams; right-of-way remains M3 |

## Editor regression coverage

The `editor` native test group covers ProjectDocument and command transactions without
Qt. The `editor-ui` CTest entry exercises the Qt editor through mouse/keyboard actions,
including saving, images and calibration. The architecture guard additionally rejects
project imports of commands/editor/shell/render and Qt, with negative fixtures.
The `connectors` native group covers lane mappings, editable curves, endpoint
maintenance, atomic failures and route/input preservation. The `connector-ui` CTest
entry exercises lane-end gestures, curve-point editing, ID/Properties controls,
reference-safe deletion/Undo, persistence and English/Thai feedback. Both UI entries
require a desktop build; headless results do not verify them.
See [NETWORK_EDITOR.md](../docs/reference/NETWORK_EDITOR.md) for the editing workflow and limits.

`multra-m26-study <output>` regenerates `data/projects/m2.6-study-template.traffic.json` from
`m26_study_network.hpp` — the M2.6 study template with placeholder volumes. `m26study` in
`multra-tests` fails when the two differ.

`multra-lane-change-fixture <output>` regenerates `data/projects/lane-change-lab.traffic.json`
from `lane_change_network.hpp` (D98). It has four independent scenes, each isolating one
lane-change situation:

| Scene | What it isolates |
|---|---|
| Overtaking | a two-lane road |
| Three lanes | a three-lane road |
| Lane drop | the D96 case |
| Diverge | a three-lane road splitting to a one-lane and a two-lane exit |

`lanelab` fails when the file and the builder differ, and pins what each scene compiles to.

`multra-lane-change-sweep <repo root> [--project FILE] [--seeds 42-51]
[--thresholds absent,0.25,0.5,1.0] [--accepted 1]` runs the project (the lab by default) for every
seed and variant in one process:

- **Variants.** For each variant it sets the two D95 fields on the compiled scenario's behaviours.
  No catalog copy is needed.
- **Output.** Markdown tables:
  - per variant: delay, mandatory and discretionary changes, quick repeats by kind, and safety
    clamps;
  - per movement: changes, and Δdelay paired by seed against the first variant.
- **A55.** Run it with `--project data/projects/four-leg-signalised.traffic.json` (or the M2.6
  template) and `--seeds 42-81`.

It is development evidence, not part of `check`.

### T-junction clamp diagnosis (D105)

`multra-t-junction-clamps --trace <out.jsonl> <repo root>` records seeds 42/43 at
headways 3/7/12 m; `--trace-all` records seeds 42–81. `--sweep` writes checked per-run
movement/queue figures and trajectory digests for those 40 seeds at each headway.
Each run verifies full copied-state replay, vehicle accounting, shared-segment body
separation, swept crossing safety and explicit reporting of excessive braking.
Development evidence, not an M5 runner or M6 validation; see
[`docs/evidence/m3.2.8a-clamps.md`](../docs/evidence/m3.2.8a-clamps.md).

## Reaction-time discharge evidence (BA38)

```
cmake --build --preset headless --target multra-reaction-discharge-sweep
./build/headless/bin/multra-reaction-discharge-sweep \
  data/projects/four-leg-signalised.traffic.json \
  data/driver-behaviour/presets/w74-sumo-defaults.json data \
  build/headless/compile_commands.json out/ba38-new
```

The output directory must not exist and its parent must exist. This is a developer evidence
experiment, not part of `check`. It requires source files beside the compiler-command entry;
CMake exports `compile_commands.json` under the Ninja presets. Roads with behaviour assignments
and parking lots are refused: the experiment targets the unassigned four-leg fixture.

- Prototype and the supplied W74 preset × `reactionTime` 0 / 0.5 / 1 s × seeds 42–81,
  fixed dt 0.1: **240 experimental runs**. All other fields stay unchanged within each model.
  Preset models can differ in amber handling; compare reaction times within a model.
- **80 absent-key controls:** each zero-delay run is compared against an absent-key run on
  every tick, including vehicles/FIFOs, queues, events, RNG and counters. Every experimental
  run also checks up to 32 ticks of copied-state replay from the middle of the run and vehicle
  accounting. Replay includes live FIFOs when present, reported explicitly.
- `runs.jsonl` retains full per-seed discharge cycles, raw crossings, explicit unavailable
  reasons/nulls, completed/active/pending counts and clamps. `cycles.csv` exports the same
  cycle estimates; blank numbers mean unavailable. Startup lost time retains its sign.
- `summary.json` reports per-seed means and per-head/lane mean/SD across available seed means,
  with sample counts and unavailable reasons. Seeds have equal weight; missing values are never
  zero. Different availability can change the population: inspect counts and matched seed rows.
- Eight resolved `*.traffic.json` snapshots (including absent controls) retain all variant
  settings. `metadata.json` contains their hashes, the exact-byte parsed-input manifest, compiler
  command/flags, build commit, tool source hashes and output hashes. It is written **last** with
  `complete=true`; a directory without that marker is an incomplete experiment.

The defaults remain the existing discharge specification: whole demand-duration window,
zero warmup, steady ranks 3–5 and startup ranks 1–2, end-of-tick timestamps and half-open greens.
These are unvalidated sensitivity measurements, not calibrated capacity, PCU or HCM control delay.
