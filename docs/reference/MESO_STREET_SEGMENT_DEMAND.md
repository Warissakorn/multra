# Meso OD demand on Street segments (D341, D342)

The next Street segment-demand slice after [STREET_SEGMENTS_IMPLEMENTED §22](STREET_SEGMENTS_IMPLEMENTED.md#22-street-segment-only-macro-demand-d340)
(D340), booked in NEXT. Uses the existing schema-57 Street segment zone targets. No schema, LTM engine,
milestone gate or model-parameter change; all results remain **not yet validated**.

## Scope and compatibility

- **Per matrix (D342, superseding D341's whole-document switch):** an OD matrix takes the graph
  rule when one of its `zoneIds` has a direct Street segment zone connector (`mesoGraphOdMatrices`), and
  then for all its pairs, Link-to-Link included. This covers mixed Link/Street segment networks without
  inventing a second editable network or pretending a Street segment is a Micro Link.
- Every other matrix keeps the existing Micro-chain OD expansion (D295/D296), run on
  `withoutGraphOd`: the document less the graph-rule matrices and every Street segment connector. Their
  zones have no Street segment connector, so their shares, path weights, ordering and refusals are as
  before. A Street segment connector in a zone no matrix names changes no result. Without a graph-rule
  matrix, legacy CSV is byte identical; JSON is byte identical apart from the normal
  `buildCommit` change. Converting a zone's last Street segment target with Apply template returns its
  matrices to the legacy rule; the result's rule marker says whether any matrix used the graph.
- Authored routes, routing decisions and routeless inputs keep their existing expansion in
  either case. Graph OD is appended to those classes, without replacing their paths.
- Micro still requires Link targets or a study-area cut. Macro keeps D340's assignment.
- Parking-lot dynamics remain Micro's. A lot whose zone a graph-rule matrix names refuses
  explicitly with `MESO_STREET_SEGMENT_OD_PARKING_UNSUPPORTED`, rather than inventing departures or
  ignoring capacity; other lots stay with the legacy expansion.

## Endpoints and paths

`mesoZoneEndpoint` and `mesoShortestOdPaths` live in `src/model/demand/meso_street_segment_paths.*`.
They read the compiled Meso graph, with no Qt, I/O, random numbers or persistent cache.

- Origin attaches to the first compiled link of the selected source and travel direction;
  destination to its last. Street segment direction is along/against the centreline, as D340 defines it.
  A Link travels along its reference line. A Node's trimming uses the compiled boundary.
- An unavailable Street segment direction gives `MESO_ZONE_STREET_SEGMENT_UNAVAILABLE` at
  `zoneConnectors[i].streetSegmentDirection`; no compiled Link gives `MESO_ZONE_LINK_UNAVAILABLE` at
  `.linkId`. Every target of a zone a graph-rule matrix names is checked, even when that zone has
  no positive cell; other connectors are the legacy expansion's.
- Routes minimize **free-flow travel time**: sum of `length/freeSpeed` over their links plus
  `freeTime` over their turns. Only compiled turns are traversable, so banned turns stay absent.
  A named matrix mode can use only links admitting that mode; composition-only matrices use all
  modes, as Macro does. Signal green and congestion affect loading, not this fixed path choice.
- Dijkstra computes reverse distances; iterative traversal enumerates all simple shortest paths
  in turn-index order. Ties are times equal within a relative `kMesoOdTieTolerance` (1e-12, D342): the
  same link times summed in another order differ by an ulp, and an exact comparison gave one of
  two equal detours all the demand (about 18 % of 10 m-step detour pairs at 50 km/h).
  Every tied path gets an equal share. More than **4096** paths gives `MESO_OD_PATH_LIMIT` at the
  cell and blocks Run, with no partial enumeration used.
- This is the first fixed-path rule for Street segment OD; no Micro drivable chain exists for a Street segment.
  Minimum time avoids making route choice depend on how many stations a Link was split into.
  M9/M11.5 remain the later assignment loop; this is not congestion-responsive assignment.

## Flows, classes and refusals

`mesoStreetSegmentOdDemand` (`src/project/meso_street_segment_demand.*`) receives the already resolved catalog and
modes from `mesoDemand`. It leaves the source document and catalog ownership unchanged.

- Each matrix uses its own run window, or its StudyPeriod less `clockStart`, via
  `odMatrixWindow`. Unknown periods and windows outside `[0, definition.duration]` refuse with
  `UNKNOWN_PERIOD` / `OD_PERIOD_INVALID`. All graph-rule matrices are checked, including
  zero-demand ones.
- A cell contains vehicle trips; its hourly flow is `trips / windowHours`. It is split by both
  normalized connector shares and equally over tied paths. Normalization scales by the maximum
  share before summing, so finite large shares do not overflow the denominator.
- Every positive connector pair must have a permitted path. No connectors gives
  `OD_ZONE_NO_CONNECTOR`; no path gives `MESO_OD_NO_PATH`, both at the cell. Any such issue blocks
  loading; shares are never silently redistributed or discarded. Non-finite hourly flow gives
  `OD_CELL_INVALID`.
- Composition and mode references must exist (`OD_COMPOSITION_UNKNOWN`, `UNKNOWN_MODE`,
  `MODE_COMPOSITION_UNKNOWN`). Classes remain `composition:<id>` or `type:<id>` with the same PCU
  calculation as existing Meso inputs; missing PCU gives `MODE_PCU_MISSING`. Runtime-only type
  matrices keep their type. Different modes sharing a composition can still have different paths.
- Path source ids encode matrix, origin/destination zones, both connectors and path index as a
  JSON tuple after `od:road:` (the existing `od:` namespace preserves study-area origin zones).
  Delimiters/newlines in ids cannot collide. Authored paths are
  appended separately, so an authored id cannot overwrite graph OD or vice versa.
- Classes and paths are sorted by id/source, links and turns; flow intervals retain matrix
  windows. Same inputs give the same paths, loading and export bytes.

## Results and editor

- `MesoRun::odPathRule` is `minimum-free-flow-time` when any matrix took the graph rule, empty
  otherwise.
  Only nonempty values add `odPathRule` to JSON and an OD-path-rule comment to CSV. The editor's
  Meso note explains the rule in English/Thai. Legacy output bytes keep their existing shape.
- The existing Street segment connector dialog describes Macro and Meso support, with Apply template
  required for Micro. Run Meso uses the existing worker, progress, Cancel, tables and side file.
  Project bytes and original input hashes remain unchanged by execution.
- The study-area cut still reads the actual Meso paths/counts. No full result-table restoration
  or new side-file schema is introduced (D331).

## Acceptance

| Row | Case | Asserts |
|---|---|---|
| MR1 | Graph paths | Forward/backward endpoints; minimum time over link/turn times; mode filtering; all equal-time alternatives; one-link paths; no loops; long routes without recursion; explicit path limit |
| MR2 | Flow and time | Exact directional trips, shares and PCU; large shares; StudyPeriod/clock windows and overlapping matrices; replay and source preservation |
| MR3 | Mixed demand | Link/Street segment pairs, Link pairs through Street segments, authored/routeless paths and graph OD coexist without replacement; mode-specific paths share a composition safely |
| MR4 | Refusals | One-way backward, banned/unreachable/mode-restricted pairs, missing endpoints/references/PCU, invalid periods and parking lots; valid forcing checked first |
| MR5 | Loading and provenance | Demand/arrival/storage conservation, class identity, unchanged source bytes/ownership and manifests; result rule marker and unchanged legacy outputs |
| MR6 | Editor and CLI | Real Meso worker and CLI run Street segment OD, export JSON/CSV and original hashes; Micro still refuses; persistence and English/Thai rule text |
| MR7 | Per-matrix rule and ties (D342) | A Street segment connector in an unnamed zone leaves the Link-only demand and parking unchanged; naming the zone opts the matrix in; equal detours in another order share, a metre's difference does not |

Verification date, platform and results are in PROGRESS. No automated run closes an owner
Windows/high-DPI appearance gate or a published Meso benchmark gate.
