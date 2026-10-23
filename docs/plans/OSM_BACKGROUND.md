# OSM background implementation

Owner request 2026-10-12: implement a pannable, zoomable OSM background in the existing
native editor. Contract and acceptance rows: [MAP_BACKGROUND](../reference/MAP_BACKGROUND.md).

1. Optional schema-29 map settings, finite/range validation and atomic History command.
2. Qt-free desktop projection module using PROJ; known UTM controls and XYZ tile math.
3. Qt Network tile client: viewport-only queue, HTTP freshness/validators and bounded caches.
4. Adaptive reprojection paint in QGraphicsView background; one camera, no tile scene items.
5. Localized settings and navigation dialog; visible status/attribution; packaging and CI.
6. Local fixture tests, complete native checks and owner visual review.

The owner's 2026-10-23 request adds wider overview zoom, stable panning and a bundled
country/province picker; the current contract above defines their bounds and semantics.

Keep engine/eval and existing project bytes unchanged. No online address search, automatic road import,
offline tile packages, terrain, satellite layer or world-scale navigation in this version.
Cache defaults are initial resource budgets, not performance measurements. Runtime costs
must be measured with event-loop pumping on a real network before any speed-up claim.
