Native QGraphicsView editor canvas. Reads a const ProjectDocument; gesture callbacks submit commands through the shell. Local previews never mutate the model. Pan, zoom, grid snap, selection, drawing, point/link dragging, insert/remove points, split picking and two-point measurement/calibration are implemented. Connector tools add lane-end picking, curve previews, connector selection and interior-point editing with locked endpoints.

OSM raster map paint and asynchronous tile client are in `map_*` and `canvas_map.cpp`; see [map contract](../../docs/reference/MAP_BACKGROUND.md).
`map_http_cache` owns response freshness, validation and Retry-After handling.

`canvas_cursor` resolves one cursor from the actual gesture and current display DPI.
`canvas_labels` places upright vector labels without collisions in the viewport.
`canvas_demand` previews exact display endpoints; marker metadata never changes runtime routes.
