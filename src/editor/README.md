Native QGraphicsView editor canvas. Reads a const ProjectDocument; gesture callbacks submit commands through the shell. Local previews never mutate the model. Pan, zoom, grid snap, selection, drawing, point/link dragging, insert/remove points, split picking and two-point measurement/calibration are implemented. Connector tools add lane-end picking, curve previews, connector selection and interior-point editing with locked endpoints.

OSM raster map paint and asynchronous tile client are in `map_*` and `canvas_map.cpp`; see [map contract](../../docs/reference/MAP_BACKGROUND.md).
The screen HUD (scale bar, px/m, north from the map georeference) is `canvas_hud.cpp`, placed by `canvas_map.cpp` (D197).
Run vehicles are drawn at lane centre + `Vehicle::lateral`, to the left of the body's heading, and a lane-change record flagged `lateral` is never slid (`canvas_run.cpp`, LATERAL §10, D203).
`map_http_cache` owns response freshness, validation and Retry-After handling.
`map_locations` validates the bundled country/region catalog for reference and navigation choices.
