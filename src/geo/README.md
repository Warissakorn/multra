# src/geo

Map coordinates for the desktop's background map: PROJ transforms between WGS84, Web Mercator and
one fixed WGS84 UTM zone per project, and XYZ tile arithmetic. Project coordinates are a
translated and rotated local origin. Contract: [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md).

## Dependencies

Desktop only. Reads `src/project/map_settings.hpp` (the stored `GeoReference`) and PROJ; no Qt.
Core, eval and headless persistence never import this module.

## Files

| File | Holds | Contract | Used by |
|---|---|---|---|
| `projection.*` | `GeoProjection`: geographic, Mercator and world (project) metres both ways; `insideStudy` (100 km) and the wider browsing-only `insideOverview`. Projected grid metres, not survey ground distance | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) | editor, shell |
| `projection_data.*` | `projectionDataDirectory`: finds the PROJ database shipped with the executable, independently of a `--data-dir` content override | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) | shell |
| `tile_grid.*` | XYZ tile maths: `tileBounds`, `tilePoint`, `tileAt`, `tileZoom` (256-pixel tiles) | [MAP_BACKGROUND](../../docs/reference/MAP_BACKGROUND.md) | editor |

## Tests

`tests/map_geo_tests.cpp`, `tests/map_ui_tests.cpp`.
