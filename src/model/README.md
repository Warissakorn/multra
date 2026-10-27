C++ authoring model and scenario compilation in network/. M0 JSON fixture/catalog loading lives in ../project/.

`network/connector_attachment.hpp` shares the existing 0.01 m lane-attachment measurement
between network validation and optional shell diagnostics; it adds no JSON or UI dependency.

`automaticConflicts` owns a `ConnectorSurfaceCache` for one immutable calculation.
Overlap classification and mouth/display polygons reuse it; ordinary geometry queries remain
uncached. Destroy it before any edit; no revision or persistent invalidation key is used.
