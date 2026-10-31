# src/editor

The Qt network canvas (a native `QGraphicsView`) and everything it paints: the drawing, Run
vehicles, the background map and the HUD. The canvas reads a const `ProjectDocument`; gesture
callbacks hand edits to the shell, which submits them as commands. Local previews never mutate
the model. Contracts: [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md),
[EDITOR_WORKFLOW](../../docs/reference/EDITOR_WORKFLOW.md),
[VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md),
[MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md).

## Dependencies

Qt Widgets and Network. Built into one target with `src/shell/`, `multra_shell`
([ARCHITECTURE](../../docs/ARCHITECTURE.md)), so no boundary is checked between them. Reads
`src/model/`, `src/core/`, `src/geo/` and `src/project/` headers; `canvas_cursors.cpp` also reads
`../shell/editor_style.hpp`, `map_provider.cpp` and `map_locations.cpp` read `../shell/path.hpp`,
and `canvas_connectors.cpp` reads `../commands/connector_commands.hpp` for its previews.

## Files

| File | Holds | Contract |
|---|---|---|
| `canvas.*` | `EditorCanvas`: pan, zoom, grid snap, selection, drawing, dragging, points, split picking, measurement and calibration, Connector tools. Split by concern into `canvas_*.cpp`: `input`, `select`, `spatial`, `cache`, `cursors`, `feedback`, `lanes`, `connectors`, `conflicts`, `heads`, `counters`, `sections`, `decisions`, `demand`, `zones`, `streetSegments`, `rotation`, `display`, `map`, `run` (Run vehicles at lane centre + `Vehicle::lateral`) | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md), [VISSIM_PARITY](../../docs/audits/VISSIM_PARITY.md) |
| `canvas_style.*` | Canvas colours as functions (built after `QApplication`) | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `ui_design_tokens.*` | The only place a hex colour literal may appear; read back by `QPalette` role | [UI_REDESIGN_AUDIT](../../docs/audits/UI_REDESIGN_AUDIT.md) |
| `canvas_hud.*` | Screen HUD: scale bar, zoom in px/m, north arrow when the georeference gives one; transparent to the mouse | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `canvas_image.*` | Bounded PNG export from an isolated authoring canvas (max 8192 px per edge, 32 Mpx), following the live display settings | [NETWORK_IMAGE](../../docs/reference/NETWORK_IMAGE.md) |
| `road_crossbar.*` | A Run signal state's shape at a station, readable without hue | [NETWORK_EDITOR](../../docs/reference/NETWORK_EDITOR.md) |
| `vehicle_pose.*` | Display-only pose from the front-bumper station on the whole route | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md) |
| `rear_axle_pose.*` | Rear-axle reference for a prescribed front-bumper track, no lateral slip | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md) |
| `lane_change_pose.*` | A lane change's display journey, including overlapping changes and settling | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md), [LANE_CHANGE_DURATION](../../docs/reference/LANE_CHANGE_DURATION.md) |
| `vehicle_shape.*` | Plan-view silhouettes and interior decoration, separate from the solid hit shape | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md#plan-view-appearance-d333) |
| `run_vehicle_item.*` | `RunVehicleItem`: one reusable scene item per Run vehicle | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md) |
| `vehicle_color.hpp` | Fixed or mixed body colours by stable vehicle or type id, never from engine random state | [VEHICLE_POSE](../../docs/reference/VEHICLE_POSE.md) |
| `map_provider.*` | Tile providers loaded from data, with URL template and attribution | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `map_tile_client.*` | Asynchronous tile requests; callbacks on the owning UI thread | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `map_http_cache.*` | Response freshness, validation and Retry-After | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `map_layer.*` | `MapLayer`: load state, tile meshes (`map_mesh.cpp`) and painting; hidden or unavailable layers make no requests | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |
| `map_locations.*` | The bundled country and region catalog for choosing a reference; not boundaries | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) |

## Tests

The Qt `tests/*_ui_tests.cpp` executables, each registered in `src/shell/CMakeLists.txt`;
`tests/map_*_tests.cpp`; and the pose tests `tests/vehicle_pose_tests.cpp`,
`tests/rear_axle_pose_tests.cpp`, `tests/lane_change_pose_tests.cpp`.
