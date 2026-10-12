# Map background contract

OSM is a raster XYZ layer beneath the existing embedded image, roads and vehicles.
The editor's existing QGraphicsView transform owns pan and pointer-centred zoom; the
map has no second camera. This contract is for the owner's 2026-10-12 request.

## Coordinates and scope

Schema 29 adds optional `map`: `providerId`, `opacity`, and `reference` containing
`projectedCrs`, `anchorLongitude`, `anchorLatitude`, `anchorWorldX`, `anchorWorldY`
and `rotationDegrees`. These are required finite values when map is present.
Each world anchor component is limited to ±10,000,000 m so accepted references
remain reachable through Qt's integer scroll range at the Canvas maximum zoom.
WGS84 UTM EPSG:32601–32660 and EPSG:32701–32760 are supported, selected once and
persisted. The geographic anchor must belong to that zone and hemisphere.
Geographic input is longitude, latitude in degrees, normalized explicitly in PROJ.
Project coordinates are projected grid metres, with local numerical origin at the
anchor. Rotation is counter-clockwise from grid east/north into project x/y.
Changing the anchor/rotation moves the background only, never authored geometry.
The first version supports local studies within 100 km of the anchor, not a world GIS.
UTM grid distance is not survey ground distance; map position accuracy is not validated.

`world = anchorWorld + R(rotation) * (projected - projectedAnchor)`.
PROJ is a desktop dependency only. Headless persistence validates the supported CRS
identifier and scalar ranges without needing PROJ; simulation compilation ignores map.

## Lifecycle

Setting/removing map is one atomic History command; bad settings leave the document
and history unchanged. Existing successful-edit Run invalidation also applies to map
commands. Pan, zoom, Go to coordinates and Hide OSM are view state and create no revision.
Old documents without map retain their existing serialized bytes and selected schema.
Pre-29 files containing map are refused, not silently stripped. Tile data and viewport
position are not embedded. Save/Open and recovery preserve reference and appearance.

## Tiles and display

Providers live in `data/map-providers.json`, including URL template, zoom limits,
tile width and visible attribution/link. Production endpoints use HTTPS. Decoded
image cache keys include provider identity and URL; HTTP disk cache keys are URLs.
Requests are asynchronous, deduplicated, bounded to four active requests and 128
visible tiles. Only the current viewport is requested. A bounded timer refreshes the
viewport after pan/zoom/resize; stale replies never install a previous projection.
Network failure never prevents network editing or simulation. The status and credit
are viewport overlays. A hidden/removed layer makes no new requests.

Web Mercator raster tiles are reprojected with an adaptive triangle mesh. Refinement
checks edge/diagonal midpoints against a 0.5 physical-pixel error target, with a
bounded subdivision count; a tile that cannot meet it is omitted and warned.
The target is numerical rendering error at sampled points, not OSM data accuracy.
Triangles share vertices; pixel Y inversion occurs exactly once.
Mercator X is unwrapped around the anchor; XYZ request indices wrap at ±180°.
Crossing the date line within a local study does not request a world-wide tile box.
Tiles are painted in the background, never scene items or part of Fit network bounds.
Tile arrival invalidates background paint only, not road/vehicle scene reconstruction.

## Network and cache

15 s request timeout, 1 MiB encoded and 256–512 pixel tile bounds, 64 MiB decoded LRU,
256 MiB disk cache. PNG decoding checks header dimensions before allocation.
Decoded images use 32-bit premultiplied pixels. The wanted set is additionally
limited by that memory budget (64 tiles at 512 pixels, 128 at 256 pixels).
Cache obeys response freshness and validators; where freshness is absent, store a
7-day expiry. Quoted max-age is accepted; duplicate/invalid explicit freshness is
stale. Corrupt cached bytes lose their validators; a 304 with a different ETag
cannot validate the stored image. Successful 304 validation refreshes Date/Age.
HTTP Date/Expires/Retry-After timestamps, including GMT and legacy forms, are parsed in UTC.
No default no-cache headers. Failures use bounded retry/backoff;
429/503 obey Retry-After, including delays beyond one day. Live OSM sends a stable TrafficSim User-Agent and shows the
provider's credit visibly. No bulk download, offline archive or prefetch feature.
Live policy: https://operations.osmfoundation.org/policies/tiles/ (checked 2026-10-12).

## Acceptance rows

| Row | Required evidence |
|---|---|
| MAP1 | UTM known control point, axis order, inverse and rotated/nonzero anchors |
| MAP2 | Invalid/null/unknown map keys and pre-29 map fail without model/history changes |
| MAP3 | Save/Open and Undo/Redo preserve map; map-free JSON stays identical |
| MAP4 | Tile edges/zoom limits, bounded wanted tiles and high-DPI zoom selection |
| MAP5 | Local fake HTTP: deduplication, cache hits/revalidation, timeout, bad image and 429 |
| MAP6 | Fake raster: orientation, seams, pan/zoom, fit bounds and late reply after replacement |
| MAP7 | Hide/remove cancels requests; network errors keep editor/run usable |
| MAP8 | Same scenario/seed has the same complete simulation output with/without map |
| MAP9 | English/Thai UI, Linux/Windows build and packaged projection/TLS dependencies |
| MAP10 | Owner checks visible alignment and responsiveness on a real local study |

Tests use local fake tiles, never live OSM as an automated fixture. Automated rows
do not close MAP10 or establish engineering/survey accuracy.
