Qt-free M0 loading/output and authoring ProjectDocument codec (reads schemas 1–57). The document persists authoring data and optional background; no compiled runtime network is saved. Empty draft networks are valid. Commands depend on this module, never the reverse. The shell owns QSaveFile atomic replacement.

`input_manifest` optionally records hashes of exact parsed project/catalog/evaluation bytes for CLI discharge studies. Collection is passed explicitly through compilation; loaders keep their legacy path when absent. `input_sha256` has no external dependency. See [measurement/provenance contract](../../docs/reference/DISCHARGE.md).

`evaluation_period` reads, writes and validates the M5.3 evaluation period (schema 22) and its M5.9 cool-down (schema 27, D146), and builds `newProjectDocument`.

`batch_output` formats M5.2 batch reports as JSON and CSV, with the cell formats it shares with `movementCsv` in `csv_format.hpp`. `parseSeedList` (in `load`) reads `--seeds` lists. `parallel_seeds` runs the runner's `runSeed` on worker threads (D152); the runner itself stays thread-free. See [BATCH](../../docs/reference/BATCH.md).

`behaviour_library` reads, writes and validates the M3.3.2a project-owned behaviour library (names/model tags, vehicle classes, link behaviour types, Link/Connector assignment) and the Run refusal of assigned roads until M3.3.2b. See [contract](../../docs/reference/DRIVING_BEHAVIOUR.md#7-implemented-library-and-codec-m332a-d126).

`map_settings`/`map_codec` persist optional schema-29 reference/settings without Qt/PROJ; old map-free bytes stand.

`route_destination` validates and persists editor-only schema-30 destination markers; compilation ignores them.
`zone_codec` persists the optional schema-31 zone layer (zones, zone connectors, OD matrices; M8.2, D154) and its structural checks; compilation does not read it until M8.3.
`od_demand` adds the OD matrices to a copy of the definition as ordinary routes and inputs before compilation, and holds their Run-tier checks (M8.3, D155).
Direct Road zone targets use schema 57 and Macro's compiled direction endpoints; Micro requires *Apply template* to remap them to Links. See [ROADS_IMPLEMENTED §22](../../docs/reference/ROADS_IMPLEMENTED.md#22-road-only-macro-demand-d340). `meso_road_demand` adds Meso's compiled-graph OD paths, resolving shares, modes, periods and PCU from the same catalogs; a matrix uses them only when one of its zones has a Road target, the rest keep Micro's paths (D342). See [MESO_ROAD_DEMAND](../../docs/reference/MESO_ROAD_DEMAND.md).
`parking_compile` turns each authored ParkingLot into the engine's lot after `compileScenario` (M8.4b, D157).

`study_area_run` builds the run-time Micro cut; `study_area_meso` retains composition and single-type classes per Meso slice. Run-time-only type fields pass through `od_demand` to ordinary inputs without changing catalog ownership or stored codecs. See [STUDY_AREA_IMPLEMENTED §22](../../docs/reference/STUDY_AREA_IMPLEMENTED.md#22-single-vehicle-type-meso-cuts-and-cli-coverage-d338).

`macro_run_definition` persists named Macro period/settings definitions at schema 56; `runNamedMacro` executes them without changing the document or its global settings. Results identify the chosen definition; the side file still keeps the latest source for each period. See [MACRO_NETWORK §21](../../docs/reference/MACRO_NETWORK.md#named-macro-runs).
