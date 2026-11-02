# src/shell

The Qt Widgets desktop application. The network editor window is the only editing window; the
Behaviour Lab is a separate tool window. The shell owns the document, History, the Run view, files
and recovery, dialogs, the Results tabs and the optional local automation server. Text comes from
`data/locales/` (English and Thai). Contracts: [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md),
[EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md),
[DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md).

## Dependencies

Everything below it: `src/editor/`, `src/commands/`, `src/project/`, `src/model/`, `src/core/`,
`src/eval/`, `src/runner/`, `src/lab/`, `src/geo/`. It shares the `multra_shell` target with
`src/editor/` ([its dependencies](../editor/README.md#dependencies)); no other `src/` module includes the
shell. Every file the shell writes is
replaced atomically (`QSaveFile`).

## Files

`editor_window.*` declares `EditorWindow`; its members are split by concern:

| Area | Sources |
|---|---|
| Window, workspace, tables, inspector | `editor_window.cpp`, `editor_workspace.cpp`, `editor_tables.cpp`, `editor_inspector.cpp`, `editor_palette.cpp` (command palette), `editor_commands.cpp`, `editor_history.cpp`, `editor_input.cpp`, `editor_modes.cpp` |
| Files and recovery | `editor_files.cpp`, `editor_recovery.cpp`, `editor_side_files.cpp`, `editor_storage.cpp` |
| Drawing | `editor_creation.cpp`, `editor_connectors.cpp`, `editor_rotation.cpp`, `editor_cross_sections.cpp`, `editor_link_types.cpp`, `editor_appearance.cpp`, `editor_diagnostics.cpp` |
| Control | `editor_priority.cpp`, `editor_control.cpp`, `editor_signal.cpp` |
| Demand | `editor_demand.cpp`, `editor_demand_preview.cpp`, `editor_routing.cpp`, `editor_decision.cpp`, `editor_catalog.cpp`, `editor_periods.cpp`, `editor_speed_areas.cpp`, `editor_speed_decisions.cpp`, `editor_behaviour.cpp`, `editor_zones.cpp` (Zones tab), `editor_od.cpp` (OD matrices tab) |
| Street segments and Nodes | `editor_street_segments.cpp`, `editor_crossings.cpp`, `editor_coarse.cpp`, `editor_seams.cpp`, `editor_fill.cpp`, `editor_proposals.cpp`, `editor_nodes.cpp` |
| Evaluation objects | `editor_counters.cpp`, `editor_sections.cpp` |
| Runs and Results | `editor_run.cpp` (Run/Pause/Step/Reset, seed, speed), `editor_batch.cpp` (N seeds on one worker thread), `editor_compare.cpp` (the open project against a chosen file), `editor_macro.cpp`, `editor_meso.cpp`, `editor_results.cpp`, `editor_results_nodes.cpp` |
| Map, lab, automation | `editor_map.cpp`, `editor_lab.cpp`, `editor_automation.cpp`, `editor_automation_lifecycle.cpp` |

The other headers:

| File | Holds | Contract |
|---|---|---|
| `editor_window.*` | `EditorWindow` (sources above) | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `editor_style.*` | The 20-unit toolbar and cursor glyphs; the editor QSS, palette roles only | [UI_REDESIGN_AUDIT](../../docs/audits/UI_REDESIGN_AUDIT.md) |
| `editor_interaction.*` | Application-wide input behaviour installed once, covering modal dialogs | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) |
| `message_label.*` | The one-row, closable message line under the canvas | [D429](../../docs/decisions/RECORD.md#d429) |
| `editor_storage.*` | Atomic replacement and exact Save bytes shared with analysis snapshots; refuses an export over the active project or recovery file | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `legacy_storage.*` | The one-time move of per-user folders from the earlier application name | [D286](../../docs/decisions/RECORD.md#d286) |
| `path.hpp` | `nativePath` and `displayPath`: Unicode-safe path conversion | none |
| `form_values.*` | Precision-preserving staged values, language-switch drafts and validation before modal acceptance | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) |
| `table_layout.*` | Bounded initial columns, stable user widths and active-tab minima | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) |
| `workspace_layout.*` | Palette width transfer with explicit preset Properties size and deferred Qt bounds | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) |
| `presentation.*` | Session UI density and translated menu, shared by editor and Lab | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) |
| `dialog_fit.*` | Keeps a tall dialog's buttons on screen | [EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md) |
| `junction_preview_dialog.*` | Affected Node/detail list and named refusals before explicit seam/corridor replacement | [JUNCTION_TEMPLATE_EDITOR](../../docs/reference/JUNCTION_TEMPLATE_EDITOR.md) |
| `junction_polygon_dialog.*` | Original/proposed junction polygon outlines, named refusals and explicit Fit + Fill confirmation | [JUNCTION_TEMPLATES §5](../../docs/reference/JUNCTION_TEMPLATES.md) |
| `node_analysis_dialog.*` | Staged turn/approach/settings inputs, missing/zero distinction, stored-codec validation and explicit stale-row clearing | [NODE_ANALYSIS_EDITOR](../../docs/reference/NODE_ANALYSIS_EDITOR.md) |
| `node_analysis_fill_dialog.*` | Explicit window/HV selection and sourced whole-set before/after preview, staged until the Analysis form confirms | [NODE_ANALYSIS_MODEL](../../docs/reference/NODE_ANALYSIS_MODEL.md) |
| `node_analysis_manual_dialog.*` | Explicit per-Node manual-count window preview/confirmation, shared comparison guards and no document mutation | [NODE_ANALYSIS_MODEL](../../docs/reference/NODE_ANALYSIS_MODEL.md) |
| `node_analysis_panel.*` | Self-owned analytical Results page, snapshot provenance, CLI-identical exports and captured-batch Micro comparison | [NODE_ANALYSIS_EDITOR](../../docs/reference/NODE_ANALYSIS_EDITOR.md) |
| `junction_cross_section_dialog.*` | Staged cross-section id/override form; blank values inherit, Cancel mutates nothing | [JUNCTION_TEMPLATE_EDITOR](../../docs/reference/JUNCTION_TEMPLATE_EDITOR.md) |
| `junction_form_widgets.*` | Far-to-kerb lane rows, geometric labels and optional site-number fields | [JUNCTION_TEMPLATE_EDITOR](../../docs/reference/JUNCTION_TEMPLATE_EDITOR.md) |
| `junction_catalog_dialog.*` | Staged structural template/role editor and project catalog capture | [JUNCTION_TEMPLATE_EDITOR](../../docs/reference/JUNCTION_TEMPLATE_EDITOR.md) |
| `junction_recipe_dialog.*`, `junction_site_dialogs.cpp` | Node recipe, independent directions, per-leg overrides/pockets and NEMA timing drafts | [JUNCTION_TEMPLATE_EDITOR](../../docs/reference/JUNCTION_TEMPLATE_EDITOR.md) |
| `editor_batch_cells.*` | The cells the batch and comparison tables share | [BATCH](../../docs/reference/BATCH.md) |
| `editor_macro.*` | The Macro run: one worker over a copy of the document, stages and gap posted back | [MACRO_NETWORK §19, §20](../../docs/reference/MACRO_NETWORK.md) |
| `editor_meso.*` | The Meso run, as the Macro one | [MESO_NETWORK §7, §20](../../docs/reference/MESO_NETWORK.md) |
| `macro_run_dialog.*` | Named Macro run definitions, committed as one Undo step | [MACRO_NETWORK](../../docs/reference/MACRO_NETWORK.md#named-macro-runs) |
| `run_identity.hpp` | Metadata of the GUI single run, apart from engine state | [DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md) |
| `signal_timing_view.*` | A controller's timing bars across one cycle, from the run's colour rule | [SIMULATION](../../docs/reference/SIMULATION.md) |
| `behaviour_library_dialog.*` | The driving-behaviour library dialog, editing a staged copy | [DRIVING_BEHAVIOUR §9](../../docs/reference/DRIVING_BEHAVIOUR.md) |
| `behaviour_editor.*` | One behaviour's name, model and keys | [W74](../../docs/reference/W74.md), [DRIVING_BEHAVIOUR](../../docs/reference/DRIVING_BEHAVIOUR.md) |
| `behaviour_lab_window.*` | Simulation → Behaviour Lab: non-modal, its own `EditorCanvas` following one vehicle; Apply writes the tuned vehicle back through `execute` | [BEHAVIOUR_LAB §3](../../docs/reference/BEHAVIOUR_LAB.md) |
| `behaviour_lab_panel.*` | The lab's three-tab parameter panel over one `LabSettings` | [BEHAVIOUR_LAB §3](../../docs/reference/BEHAVIOUR_LAB.md) |
| `behaviour_lab_telemetry.*` | The followed vehicle's last 60 s as speed and gap strips | [BEHAVIOUR_LAB §3](../../docs/reference/BEHAVIOUR_LAB.md) |
| `demand_catalog_dialog.*` | Vehicle type and composition dialogs | [DEMAND_CATALOGS](../../docs/reference/DEMAND_CATALOGS.md) |
| `demand_period_editor.*` | Demand period rows, returned for the caller's History command | [DEMAND_TIME_TYPES](../../docs/reference/DEMAND_TIME_TYPES.md) |
| `demand_type_rules_dialog.*` | A routing decision's per-type matrices over its targets and periods | [DEMAND_TIME_TYPES](../../docs/reference/DEMAND_TIME_TYPES.md) |
| `od_lot_dialogs.*` | The OD matrix grid and parking-lot dialogs | [ZONES_AND_PARKING §9](../../docs/reference/ZONES_AND_PARKING.md) |
| `street_segment_zone_dialog.*` | Direct Street segment targets for Macro and Meso (Street segments without generated Links only) | [STREET_SEGMENTS_IMPLEMENTED §22](../../docs/reference/STREET_SEGMENTS_IMPLEMENTED.md#22-street-segment-only-macro-demand-d340), [MESO_STREET_SEGMENT_DEMAND](../../docs/reference/MESO_STREET_SEGMENT_DEMAND.md) |
| `gate_flow_dialog.*` | A study area's typed gate flows for one period and composition | [STUDY_AREA §5](../../docs/reference/STUDY_AREA.md) |
| `map_settings_dialog.*` | Map reference and appearance (staged) and navigation | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `map_location_picker.*` | Searchable country and province shortcuts that fill the coordinate fields | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `vehicle_color_dialog.*` | Per-type fixed or mixed body colours, saved as application-local preferences shared with the lab | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md#vehicle-colour-preferences-d334) |
| `object_export.*` | File → Export object table (CSV); the formatter is `src/project/object_csv` | [OBJECT_TABLE_CSV](../../docs/reference/OBJECT_TABLE_CSV.md) |
| `image_export.*` | File → Export network image (PNG); the canvas renders | [NETWORK_IMAGE](../../docs/reference/NETWORK_IMAGE.md) |
| `automation_server.*` | Optional local JSONL control, one object per line, dispatched on the GUI thread | [DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md) |
| `automation_endpoint.*` | The protocol bound and endpoint naming shared with the `multra-control` client | [DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md) |
| `automation_error.hpp` | `CommandError`: an automation error code with details | [DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md) |
| `automation_document.*` | Patches, observed structural discovery and dry validation through the codec and History; no claimed complete schema | [DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md) |
| `automation_geometry.*` | Previews the Connector command on a copy; enriches rejected attachment errors | [DESKTOP_AUTOMATION](../../docs/reference/DESKTOP_AUTOMATION.md) |

## Tests

The `tests/*_ui_tests.cpp` executables and the automation tests (`tests/automation_*_tests.cpp`,
`tests/editor_automation_tests.cpp`, `tests/desktop_automation_process_tests.cpp`), registered in
`src/shell/CMakeLists.txt`; Street segment/transition UI test targets are in `street_segment_tests.cmake`.
