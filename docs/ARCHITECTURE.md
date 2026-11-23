# ARCHITECTURE — MULTRA

**Stack: C++20, CMake, Qt 6 Widgets** (D15 superseded the TypeScript stack). This file maps
modules, boundaries and interfaces; milestone status and gates live only in [ROADMAP](ROADMAP.md).

Project-owned Demand catalogs (schema 18) store types, behavior snapshots, composition
weights and authoring names. `resolveDemandCatalog` supplies the UI and compiler;
`putDemandCatalog` captures ownership atomically through History. Names/compositions
stay outside core; only the expanded input rates and existing vehicle contracts run.
Schema 20 adds opt-in physical routing stations. `station_routing` derives shared-prefix
route alternatives and clips lateral awareness spans; `core/routing` selects once at the
recognition tick using passage-time/type weights. See [POSITIONED_ROUTING](reference/POSITIONED_ROUTING.md)
for the conservative tick boundary and limits. Schema 19 adds composition periods and complete type routing matrices.
`demand_time_types` supplies validation and expansion before conditioned routing;
placed decisions key route families by type. Scheduled demand time selects legacy weights,
including downstream decisions without `position`; source queues never reselect. Legacy compilation
order and schema-17/18 bytes stay unchanged without the new fields.

## Module dependencies

Each module may include only the modules to its right; `multra-check-architecture` fails the
build on any other include (D494). Qt is allowed only in `editor` and `shell`, nlohmann/json from
`project` upward.

| Module | May include | Qt | JSON |
|---|---|---|---|
| `core`, `macro`, `meso`, `analysis` | nothing (standard library only) | no | no |
| `eval` | `core/types.hpp` | no | no |
| `runner` | `core`, `eval` | no | no |
| `model` | `core`, `macro`, `meso`, `analysis` | no | no |
| `project` | `model`, the engines, `eval`, `runner` | no | yes |
| `commands` | `project`, `model`, `core` | no | yes |
| `lab` | `commands`, `project`, `model`, `core` | no | yes |
| `geo` | `project` (map settings), PROJ | no | yes |
| `editor` | `commands`, `geo`, `project`, `model`, `core` | yes | yes |
| `shell` | every module | yes | yes |

## Boundaries

The authoring network is the source of truth. `compileScenario` derives a runtime scenario;
it is never persisted as a second editable network. `createSimulation` copies and
canonicalizes that scenario into `std::shared_ptr<const Scenario>`.

D108 source insertion checks ordinary following/integration from rest against snapshot
leader clearance before removing a queued arrival. The same sampled record waits until
its first step fits the unchanged buffer; later safety phases remain authoritative.

The engine cannot access the authoring model, JSON, Qt, files, wall clocks or threads.
Each step receives a const state and returns a new value; vehicle/input/event vectors
are independent copies. Old snapshots remain intact. The `SimState&&` overloads (D91) take the
previous state over instead; hot loops step with `std::move`, consuming it. States are C++ values, not objects
with JavaScript-style deep-freeze; callers must treat published states as snapshots.

Accepted lane changes capture a snapshot-owned `LaneChangeTrace` before station remapping.
Only the Qt-free Run pose target reads it, reconstructing a spatial front guide with
continuous rear rolling, including overlapping changes and stopped vehicles. Engine
occupancy still changes instantly; scalar following/conflict/evaluation contracts stay
unchanged. See [VEHICLE_POSE.md](reference/VEHICLE_POSE.md).

| CMake target | Location | Dependencies | Status |
|---|---|---|---|
| `multra_vehicle_pose` | `src/editor/vehicle_pose.*`, `rear_axle_pose.*`, `lane_change_pose.*` | Model geometry, core route/type contracts; no Qt | Route front sampler, spatial rear-axle tracks and composed lane-change guides, independently testable headless |
| `multra_macro`, `multra_meso`, `multra_analysis` | `src/macro/`, `src/meso/`, `src/analysis/` | Standard C++ library only; none links another (D494) | The Macro four-step steps and assignment, the Meso link-transmission loading and the HCM 7 Node method; see Remaining systems |
| `multra_core` | `src/core/` | Standard C++ library only | M0 engine implemented; crossing admission (`conflicts.*`, M3.2.3a); mandatory lane changes and cooperation (`lanes.*`, M3.2.8b) — spans and dead ends arrive as data, the core never sees a lane; parking lots (`parking.*`, M8.4a) — gate routes and exits arrive as data, the core never sees a zone |
| `multra_model` | `src/model/network/`, `src/model/demand/` | Core contracts/validation; the Macro, Meso and analysis targets its compilers fill | M0 authoring model and compiler implemented; fixed-time Signal Controllers compiled to core programs (`signal_control.*`, M2.7b), with pure template timing/NEMA checks (`nema.*`, M14); authored right-of-way controls (`control.hpp`, `right_of_way.*`, M3.2.2a); lane families and lateral spans (`routeLaneFamily` in `routing.cpp`, `lane_family.cpp`, M3.2.8b) |
| `multra_eval` | `src/eval/` | Core events and states | Completed-trip diagnostic; per-movement delay/travel time and approach queues for one run (M2.5); M3.2.8c diagnostics (lane changes, segment times, stop-line discharge, arrival phases, dead-end waits); queue discharge/startup observer with type selection, shared-prefix routing, proven lateral/source passage reconstruction and rank-scoped remap invalidation; same-tick source-sink types come from core's pure `upcomingArrivals` replay |
| `multra_runner` | `src/runner/` | Core and eval only | Multi-seed batches and their seed-ordered aggregate (M5.2, [BATCH](reference/BATCH.md)); scenario comparison, alternative minus base with a Welch 95 % CI (`compare.*`, M5.8a, BATCH §7) |
| `multra_project` | `src/project/` | Model, evaluation types, runner, nlohmann/json | M0 loading/output; the authoring codec (reads project schemas 1 to the latest, `kLatestSchema` in `document.hpp`); evaluation spec and report output, single-run, batch and comparison (`batch_output.*`, `comparison_output.*`, the D152 seed scheduler `parallel_seeds.*`, sharing `csv_format.hpp` and the LOS pack/formatting in `los_output.*`); revision run snapshots; optional captured-byte input manifests/SHA-256 |
| `multra_lab` | `src/lab/` | Commands, project, model, core; no Qt, editor or shell (check_architecture) | Behaviour Lab (D165, [BEHAVIOUR_LAB](reference/BEHAVIOUR_LAB.md)): `LabSettings`, the stadium ring built with the editor's commands, `LabRun` (circuit marking, vehicle placement, live retune or restart) and focus telemetry; drawn by the shell's `BehaviourLabWindow` |
| `multra_commands` | `src/commands/` | Project document | Atomic named edits, Undo/Redo, network, demand, control and appearance operations |
| `multra_geo` | `src/geo/` | Project map settings, PROJ; desktop only, no Qt | Fixed WGS84 UTM reference and XYZ tile math ([MAP_BACKGROUND](reference/MAP_BACKGROUND.md)) |
| `multra_shell` | `src/shell/`, `src/editor/` | Commands, Geo, Qt Widgets/Network | The native editor — the application's only editing window since M1.24; the Behaviour Lab (D165) is a second, non-modal tool window that never edits the document except through Apply |
| `multra-cli` | `tools/run_simulation.cpp` | Project/core/eval | Headless runner: single seed, JSONL export, `--project` movement report and CSV, `--seeds` batches with 95 % CI and `buildCommit`, `--compare` scenario differences (M5.8a), and the M3.2.8c diagnostic flags (`--lane-changes`, `--segment-times`, `--stop-lines`, `--arrival-phases`, `--wait-causes`), plus `--discharge` controls and captured input hashes |
| `multra-desktop` | `src/shell/main.cpp` | Shell | Native desktop entry point; opens the editor |
| `multra-control` | `tools/desktop_control.cpp` | Qt Core/Network | Sends one JSON request to an opt-in local desktop automation session (D335) |

Connector widths and runtime paths share topology-only `connectorLanePairs`; the surface
never calls `connectorPaths`. D114 derives each runtime lane's interior vertices from adjacent
final painted-rail midpoints, and terminal vertices from named Link lane attachments.
The compiler, canvas, heads and controls read the same path and metre length. Stored control
stations map by corresponding leg fractions; conflict coverage uses the same rails and authored
cross-sections. The cubic remains the default drawing generator. Core sees lengths/stations only.

The Run view keeps the front bumper at route distance `d`, then derives a rigid body's
rear-axle pose with a prescribed-bumper-track/no-slip model. `RearAxlePath` integrates
heading by spatial RK4, independent of ticks and paint order; the rear axle is the
scene item's origin. Optional type `axles` data has nominal old-file defaults and does
not affect traffic following or reservations. Derived paths are shared per route/type,
cleared on Run-network replacement or a different immutable Scenario snapshot. Minimum
visible size does not alter physical axle dimensions. Lane changes compose continuous
front guides and use the same rolling solution; their cache keys include each vehicle's
complete physical-change/routing-remap traces, selected route and type. Between-lanes engine occupancy remains unmodelled.
See [VEHICLE_POSE.md](reference/VEHICLE_POSE.md) for the equation, defaults, numerical budget and limits.

Qt and JSON are not linked into the core. Set `MULTRA_BUILD_DESKTOP=OFF` to build
and test the engine, model and CLI on a machine without Qt.

## Contracts

```cpp
// core/simulation.hpp
SimState createSimulation(const Scenario&, std::uint32_t seed);
SimState stepSimulation(const SimState&);
SimState stepSimulation(const SimState&, double dt); // Must equal scenario.timeStep.
SimState stepSimulation(SimState&&);                 // D91: takes the previous state over
SimState stepSimulation(SimState&&, double dt);
SimState runSimulation(const Scenario&, std::uint32_t seed,
                       const EventSink& sink = {}, bool includeMovementEvents = true);
// D165 (BEHAVIOUR_LAB.md): opt-in; a scenario that never calls them runs exactly as before.
// Route::circuit marks a route that closes on itself; its vehicles never arrive.
SimState retuneSimulation(SimState&&, const Scenario&, const RetuneOptions& = {}); // same shape only
SimState placeVehicle(SimState&&, const VehiclePlacement&);

// model/network/network.hpp
Scenario compileScenario(const Network&, const ScenarioDefinition&);
std::vector<ValidationIssue> validateNetwork(const Network&);

// model/network/right_of_way.hpp (M3.2.2a): the one resolver compile and diagnostics share
RightOfWayResolution resolveRightOfWay(const Network&, const RuntimeSections&, const PriorityDefaults&);
// ... and what a crossing area's extents are checked against (M3.2.2c, conflict_coverage.cpp)
SurfaceOverlap surfaceOverlap(const Network&, const ControlPathRef&, const ControlPathRef&);
// ... and one route's chains per entry lane, full or stub, with the spans between them (M3.2.8b)
std::vector<FamilyChain> routeLaneFamily(const Network&, const std::vector<std::string>& objectIds,
                                         std::vector<std::string>* ambiguous = nullptr);
void appendLaneChanges(const Network&, const RuntimeSections&, const std::vector<FamilyRoute>&,
                       std::vector<LaneChangeSpan>&, std::vector<RouteDeadEnd>&);

// project/load.hpp
LoadedScenario loadScenario(const std::filesystem::path& file,
                            const std::filesystem::path& dataDirectory);
```

`runSimulation` invokes a synchronous `std::function<void(const SimEvent&)>` sink and
returns the final state. The core owns no output stream. The callback decides whether to
accumulate statistics, write a file or discard events. `SimEvent` is a `std::variant` of
eight typed event structs (`RoutingEvent` appended last, D119). States retain only the latest tick's events.

The desktop uses a Qt timer to schedule fixed steps. Playback time never enters the
engine. The M0 workload runs on the UI thread; playback credit is capped per callback.
A future worker handoff must retain snapshots and deterministic step order. Independent
batch seeds run in parallel since D152 (`project/parallel_seeds.*`; each seed is one
single-threaded run, so results do not depend on the thread count).

`EditorCanvas` is the only surface that draws a run. It reads model geometry and runtime
snapshots and owns no edits, signals, arrival generation or simulation timer. `src/render/`
held a second, simpler `NetworkView` for the M0 harness window; M1.24 removed both. The
current renderer is a QGraphicsView/QPainter diagnostic, not a performance-tested production
renderer.

## Editor boundary

`ProjectDocument` owns the authoring network, the optional typed `AuthoringDefinition`, the
background, the revision and the ID counter. Every mutation is a named command committed through
`History`, which publishes a candidate only after it validates; a failed edit never mutates the
published document. Project never imports commands or Qt types: a boundary check and negative
fixtures enforce this, and the shell owns atomic file replacement (D16).

`EditorCanvas` renders a const document and sends gesture callbacks; drag previews are transient
and one release submits one command. `EditorWindow` is the only editing window: it composes the
native actions, inspector, translation, save prompts and its own compiled Run revision. Qt stays
out of project, model and core. Lane paths, surfaces, mouths and conflict bands are derived from
the document and never persisted as a second editable object. Each contract owns its part:

| Behaviour and its code seams | Contract |
|---|---|
| Drawing, Check and Run (`compileDocument`), save, recovery and file formats | [NETWORK_EDITOR](reference/NETWORK_EDITOR.md) |
| Connector ranges, end handles, attachment stations, lane bundles and runtime sections | [NETWORK_EDITOR_CONNECTORS](reference/NETWORK_EDITOR_CONNECTORS.md#code-seams) |
| Connector axis, P1–P4 mouths, the surface and runtime lane centres | [CONNECTOR_FOUR_POINT_MOUTH](reference/CONNECTOR_FOUR_POINT_MOUTH.md) |
| Link geometry actions and shared markings | [AUTHORING_EXTENSIONS](reference/AUTHORING_EXTENSIONS.md) |
| Conflict-area overlaps, groups and bands | [M3_CONTRACT](reference/M3_CONTRACT.md#1-units-ownership-and-references) |
| History, keyboard moves, rotation, route crossbars and form refreshes | [EDITOR_WORKFLOW](reference/EDITOR_WORKFLOW.md) |
| Live automation through `History` over a local socket | [DESKTOP_AUTOMATION](reference/DESKTOP_AUTOMATION.md) |

## Remaining systems

M3.3 proposes one effective behavior resolver for compiled segment/type assignments;
[DRIVING_BEHAVIOUR](reference/DRIVING_BEHAVIOUR.md) defines class/default/legacy
precedence and tick selection. M3.3.2a stores the project-owned library and road
assignments (schema 21, `project/behaviour_library`, `commands/behaviour_commands`);
M3.3.2b compiles them into `segmentBehaviours` and every consumer selects through
`effectiveBehaviour` (front segment, legacy type-only when unassigned). Model state
and lateral occupancy are later slices.

M3's right-of-way seam ([M3_CONTRACT.md](reference/M3_CONTRACT.md)) has landed in slices M3.2.2a–M3.2.6:
authored waiting lines, conflict areas, priority rules (gap time/headway), Stop/Yield controls and
queue counters are project-codec objects; `resolveRightOfWay` resolves them once into runtime
conflict zones and route incidence, admitted by `core/conflicts.*`. Derived merges still run on
M3.1 `PriorityRule`s (D59).

| System | Location | Required boundary |
|---|---|---|
| Extended commands | `src/commands/` | Multi-selection and future object edits use the same transaction path |
| Extended demand/control | `src/model/demand/` | M1 typed routes/inputs/fixed-time programs exist; M2 added intervals, compositions, routing decisions with per-interval turning proportions (M2.1.1–M2.4) and fixed-time Signal Controllers (M2.7b); partial/dynamic routing (M2.1) and actuated control (M4) remain |
| Movement evaluation | `src/eval/` | One-run delay and queues exist (M2.5); multi-seed means are M5.2's runner; travel-time sections are `sections.*` (M5.4), owned by `MovementAccumulator`; LOS letters and groups are pure `los.*` (M5.5) |
| Batch runner | `src/runner/` | Implemented (M5.2; comparison M5.8a, D148): independent seeds, deterministic aggregation; editor use is M5.6 (batch) and M5.8b (comparison, D149) |
| Reports | `src/report/` | Reserved for the M5.5/M5.6 LOS report table; data formatting stays in `src/project/` (D131) |
| Meso engine | `src/meso/` | Implemented through M11.4b, gate open (D293–D301, [contract](reference/MESO_NETWORK.md)): imports nothing; `compileMesoNetwork` (`src/model/demand/meso_compile.*`) derives its links, nodes and turns from the drawn network and its Nodes. The link and node models, `loadMeso` and `runMeso` (`src/project/meso_run.*`) are the link-transmission engine; loading is deterministic with no seed (D292) |
| Analytical junction capacity | `src/analysis/` | A1 implemented (M13, D370–D372, [contract](reference/NODE_ANALYSIS.md)): imports nothing; HCM 7 Ch. 19 for pretimed protected exclusive lane groups. `compileNodeAnalysis` (`src/model/demand/node_analysis_compile.*`) derives lane groups from a Node's Micro detail and programs; `src/project/node_analysis_run.*` analyses every Node. Analytical, not simulated |
| Macro four-step model | `src/macro/` | Implemented through M10.4, gate open (D273–D277, D470–D485): imports nothing; `compileMacroNetwork` (`src/model/demand/macro_compile.*`) derives its graph and `assignMacro` runs multi-class Frank–Wolfe; `src/project/macro_run.*` runs one study period. `trip_generation.*`, `trip_distribution.*` and `mode_choice.*` are the other three steps, run singly or by `src/project/procedure_run.*`; their parameters are project-owned sets or `data/` |
| Zones, OD and parking | `src/model/demand/`, `src/project/od_demand.*`, `src/core/parking.*` | Implemented (M8, [contract](reference/ZONES_AND_PARKING.md)): Zone, ZoneConnector, OdMatrix and ParkingLot are authoring objects shared by every level; OD compiles to ordinary inputs; never a second editable network. Node, Street segment and StudyPeriod are implemented too ([NODES](reference/NODES.md), [STREET_SEGMENTS](reference/STREET_SEGMENTS.md), [STUDY_AREA](reference/STUDY_AREA.md); [D175 workflow](plans/MULTI_LEVEL_WORKFLOW.md)) |

Adding a command must never teach `project/` its implementation. The corresponding boundary check now runs with the M1 document/command implementation.

## Data and enforcement

- `data/scenarios/`: M0 authoring fixtures.
- `data/vehicle-types/`, `data/driver-behaviour/`: all JSON catalog files are loaded in
  filename order. Adding a new entry needs no C++ change. Explicit catalogs inside a
  scenario definition override the directory catalogs for that definition.
- `data/levels/`, `data/display-types/`: ordered levels and named styles loaded by
  `loadDisplayCatalog`. Model objects store IDs/levels; rendering owns their appearance.
  These values never alter simulation topology or conflict handling.
- `data/vehicle-colors.json`: optional stock swatches. Shell colour preferences override
  type colours in application-local storage; the canvas selects a stable per-vehicle colour
  without consuming engine random values ([display contract](reference/VEHICLE_POSE.md#vehicle-colour-preferences-d334)).
- `data/compositions/` (M2.3), `data/priority-rules/` (gap time and headway for derived rules),
  `data/evaluation/queue-counter.json` (queue-counter conditions), `data/los/hcm.json` (LOS bounds, M5.5), `data/vehicle-appearance/`
  (display only, D97), `data/fonts/` (the bundled UI face) and `data/projects/` (shipped example
  projects that tests and the CLI run; [purpose of each](../data/projects/README.md)).
- `data/locales/`: English key source and Thai UI text, copied with runtime data.
- `data/junction-templates/`: structural junction recipes ([catalog](../data/junction-templates/README.md)); site values live on Nodes (schema 61), never in shipped recipes.
- `data/los/`: LOS packs (`hcm.json`, M5.5) — jurisdiction-specific data, never compiled constants.

`multra-check-architecture` permits literal local headers and reviewed standard
headers in core, eval, runner, macro, meso and analysis. It rejects Qt, I/O, unordered containers,
wall-clock/RNG headers, macro includes and module imports, with negative fixtures. For project,
lab, model, commands, geo and editor it checks the table under Module dependencies. This is a restricted source
check, not a full C++ parser; review remains responsible for unconventional preprocessor
constructs. CMake target dependencies provide an additional compile/link boundary.

No engine-abstraction layer, plugin framework or second persisted model is introduced.

OSM backgrounds use one Canvas camera. Qt Network and PROJ live outside core/eval;
headless map persistence is Qt/PROJ-free. Tiles are viewport paint, not scene items,
and are bounded by HTTP/disk and decoded-image caches. Provider definitions live in
`data/map-providers.json`; only map reference/settings enter schema 29.
