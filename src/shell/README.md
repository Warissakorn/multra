Qt Widgets desktop: the network editor window, the application's only editing window since M1.24 (the Behaviour Lab, D165, is a separate tool window) — inspector, tables, Results (single run, N seeds on one worker thread — `editor_batch.cpp`, D141 — or the open project compared with a chosen file on that worker — `editor_compare.cpp`, D149), Run/Pause/Step/Reset, seed, playback speed, open/save/recovery and English/Thai UI from data/locales/.

`automation_server.*` accepts optional local JSONL control; `editor_automation.cpp` routes
commands through the same document, History and Run view. `multra-control` is its compiled
client. See [desktop automation](../../docs/reference/DESKTOP_AUTOMATION.md) for the protocol.

`editor_zones.cpp` (M8.6a, D161) is the Zones tab: zones drawn with the Zone tool (Z) in `../editor/canvas_zones.cpp`, their connectors added to the Link selected on the canvas. `editor_od.cpp` and `od_lot_dialogs.cpp` (M8.6b, D162) are the OD matrices tab, its grid dialog and the lot dialog; Results → Parking lots is in `editor_results.cpp`.

`editor_lab.cpp` opens Simulation → Behaviour Lab (D165): `behaviour_lab_window.*` (a second, non-modal window drawing the lab run on its own `EditorCanvas`, which follows one vehicle), `behaviour_lab_panel.*` and `behaviour_lab_telemetry.*`; Apply writes the tuned vehicle back through `execute`.

`editor_map.cpp` supplies localized map settings, navigation and visibility actions.
Its dialogs are free functions in `map_settings_dialog`; `map_location_picker` adds searchable
country and optional province/state choices while preserving manual coordinate entry.

`vehicle_color_dialog` edits per-type fixed or mixed body colours from View, and atomically saves
application-local display preferences shared by the editor and Behaviour Lab. See
[vehicle colour contract](../../docs/reference/VEHICLE_POSE.md#vehicle-colour-preferences-d334).

`macro_run_dialog` edits saved Macro run definitions as one Undo step and selects a named definition or project settings for the existing staged worker. Definitions are stored with the project; result tables remain session-local. See [MACRO_NETWORK §21](../../docs/reference/MACRO_NETWORK.md#named-macro-runs).
