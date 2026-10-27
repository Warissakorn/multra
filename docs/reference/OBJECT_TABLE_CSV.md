# Authored object table CSV (M1.23.1, D365)

File → Export object table (CSV) exports Links, Connectors, Vehicle inputs, Routes or Routing
decisions to one CSV file. These actions also appear in the command palette. Export reads the
current document, including unsaved edits, without Run, compilation or external catalog access.
It does not alter selection, revision, dirty/save state, History, recovery or simulation results.
Cancellation writes nothing; failed writes use the existing atomic QSaveFile replacement with
no direct-write fallback. The open project and this window's recovery path are protected.

## File convention

- UTF-8 with a BOM for Thai spreadsheet text; records use CRLF. The first record is the header,
  with stable English field names; one subsequent record per object in authored vector order.
  An empty collection (or no definition) gives a header-only file.
- Text is quoted, internal quotes doubled, with commas and embedded newlines preserved. Nested
  arrays/objects are compact JSON inside quoted CSV cells. A CSV reader must parse quoting
  before passing those cells to a JSON reader. This preserves lane, geometry, period and matrix
  order without multiplying one object into many table rows.
- Numbers use locale-independent JSON formatting with full stored precision, rather than the
  two-decimal result-report format. Missing/omitted values are empty cells; an authored empty
  string is a quoted empty string. Booleans are true when the writer emits a true-only flag.
- Coordinates stay in project metres, with no CRS conversion or georeferenced interchange
  claim. Geometry is the authored reference polyline. referenceLength_m is its arclength,
  not a compiled lane/Connector path length. Lane offsets and stations are metres; times seconds;
  vehiclesPerHour is a rate. Blank Connector stations retain the source-end/target-start default.

## Columns (in file order)

| Table | Columns |
|---|---|
| Links | id, name, level, displayType, laneOffset_m, referenceLength_m, laneCount, geometry, lanes, boundaryMarkings, behaviourType, linkType, roadId |
| Connectors | id, name, fromLinkId, fromLaneId, fromStation_m, toLinkId, toLaneId, toStation_m, fromLaneCount, toLaneCount, level, displayType, referenceLength_m, geometry, laneBlend, laneWidths, laneMarkings, laneChangeSide, behaviourType, generated |
| Vehicle inputs | id, routeId, vehicleTypeId, vehiclesPerHour, startTime_s, endTime_s, laneShares, compositionId, routingDecisionId, linkId, volumeFromCounts, intervals |
| Routes | id, segmentIds, destinationObjectId, destinationStation_m |
| Routing decisions | id, name, linkId, position_m, routes, intervals, typeRules |

Persisted fields reuse the existing document/definition serializers on a table-only projection,
which avoids copying images or loading/resolving catalogs. Input scalars reflect the current
model. Counted inputs omit their derived intervals as the project writer does; their flag and
routing-decision counts are exported. Optional Route destination markers are flattened from
the existing map. Runtime segments, expanded routes, resolved vehicle catalogs and simulation
outputs are excluded. CSV is an inspection/interchange export, not a project backup or an
implemented import format. Use Save for a complete project.

## Acceptance

| Row | Evidence |
|---|---|
| OC1 | A 3–4–5 reference polyline exports its known coordinates and length; offset/lane width retain full stored precision |
| OC2 | Thai names with commas, quotes and newlines survive independent CSV parsing; nested arrays/matrices remain readable JSON |
| OC3 | All five tables preserve row order, optional values, attachment defaults, Route destination markers and counted-input conventions |
| OC4 | Empty drafts and missing definitions export headers; global numeric locale does not change bytes |
| OC5 | Desktop menu exports all five tables to Unicode paths without Run; cancellation and failed writes preserve document/save state |
| OC6 | Project/recovery destinations are refused, existing bytes survive; English/Thai menus translate |

Native serialization tests run headless; UI tests run offscreen on Linux and in the existing
Windows native workflow. Platform observations must be reported separately. This slice does not
close M1.23: GeoJSON/PNG, imports/CRS mapping, document workflow, rendering and its full gate remain.
