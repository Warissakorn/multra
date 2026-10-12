# OSM implementation review — 2026-10-12

Scope: the owner's detailed review of [PR #138](https://github.com/Warissakorn/trafficsim/pull/138),
starting at `d0eb5c6`. Contract: [MAP_BACKGROUND](../reference/MAP_BACKGROUND.md),
decision [D150](../decisions/RECORD.md#d150). This review does not close MAP10 or an engine gate.

## Findings and corrections

| Finding | Trigger and previous result | Correction and evidence |
|---|---|---|
| Corrupt disk cache | Fresh metadata/ETag plus broken PNG sent a validator, receiving a useless 304 before another download | Decode before trusting cached metadata; discard bad bytes and validators. `--review-corrupt` failed before the fix, now one unconditional request |
| Decoded memory budget | A 16-bit PNG decoded to 64-bit pixels, exceeding the assumed four bytes/pixel; 512-pixel wanted sets could exceed the 64 MiB LRU | Normalize to 32-bit premultiplied pixels; bound wanted sets to 64/128 tiles by actual raster size. `--review-png` failed before the fix; `--review-budget` loads 64 of 80 candidate tiles without request churn |
| Fit/Frame refresh | Fit changed scale while the centre remained fixed; scrollbar callbacks did not refresh the tile level | Schedule map refresh after Fit/Frame. `--review-fit` failed before the fix and now requests the new level |
| Date-line crossing | A 300 m view near 180° formed a nearly world-wide Mercator bounding box | Unwrap Mercator X about the reference and wrap HTTP X indices. `--review-wrap` failed before the fix; checks now cover both hemispheres, rotation and latitude 83° |
| Unreachable anchor | Schema accepted a 1e9 m anchor beyond Qt's integer scroll range | Shared ±1e7 m bound in validation and dialog. `--review-anchor` failed before the fix; accepted boundary navigates within 1 m of the anchor |
| Quoted freshness | `max-age="0"` incorrectly received the seven-day fallback | Parse quoted max-age, combine Cache-Control fields and treat duplicate/invalid explicit freshness as stale. `--review-freshness` failed before the fix; `--review-policy` adds deterministic age/directive cases |
| Wrong 304 validator | A 304 returned a different ETag and reused bytes it had not validated | Match the stored/requested validator before accepting bytes; reject and fetch again on mismatch. `--review-validator` failed before the fix; policy tests also pin weak-tag matching and refreshed Date/Age |
| Retry-After truncation | Source review found delays above one day clamped to 86400 s | Respect the full requested deadline and saturate overflow; deterministic two-day/overflow cases in `--review-policy` |
| HTTP date parsing | Qt 6.4 rejected the standard GMT suffix when passed to its RFC2822 parser; Date/Expires/Retry-After dates were ignored | Normalize HTTP dates to numeric UTC offsets and handle legacy RFC850/asctime forms. The Expires/Age policy test failed before this fix; GMT and legacy-date cases now pin the behavior |
| Custom content directory | Startup searched only the selected content directory for proj.db, hiding the database shipped beside the executable | Prefer a content override database, then the installed database. `--review-bootstrap` verifies absence, installed fallback and explicit override |

Tile candidate multiplication also uses 64-bit arithmetic before deciding whether to
zoom out, preventing an overflowing product from bypassing the visible request budget.
HTTP cache responsibilities now live in `editor/map_http_cache`; database lookup in
the Qt-free `geo/projection_data`. No core/eval dependency changed.

The first seven focused failure cases above were run against the previous code and
failed at the intended assertions. Corrupt-cache and 16-bit fixtures additionally
assert that their exceptional setup actually took effect. Other corrections have
deterministic regression coverage; the HTTP-date case also failed before its parser fix.

## Verification

Full desktop build/check passed 120/120 CTest tests with GCC 13.3, Qt 6.4.2 and
PROJ 9.4.0; architecture, file-size and documentation checks passed. The map-ui suite
includes local HTTP fixtures, rendering, settings, undo/redo, save/reopen and Thai UI.
The Qt/PROJ-free headless build and full check passed 82/82 CTest tests.
Install staging contains an identical, valid proj.db and the provider catalog;
projection control-point tests passed using that installed database.

Some local runs encountered BAD_COMMAND after executable permissions changed in
this sandbox. The final desktop run used a local permission-restoring launcher,
configured through CMake's emulator property: each test executes the same native
binary with the same arguments and assertions. This local launcher is not shipped.
No automated test accesses live OSM. Invalid-PNG fixtures intentionally produce libpng
diagnostics; these are expected negative cases.

The original implementation commit `d0eb5c6` passed all five native CI jobs:
Linux desktop, Linux headless, Linux release, Windows desktop and Windows core,
[run 38198105807](https://github.com/Warissakorn/trafficsim/actions/runs/38198105807).
That result does not substitute for CI on the review fixes.

Latest full local checks and the review commit's CI status are recorded in the PR.
The [NEXT](../NEXT.md) review queue remains authoritative for pending work.

## Limits

MAP10 remains an owner check on a real local study: visible alignment, road tracing,
pan/zoom responsiveness and save/reopen. Offscreen fixture rendering and projection
control points establish numerical/software behavior; they do not validate OSM data
accuracy, survey ground distances, real server availability or TLS packaging on a
fresh end-user machine. The scope remains one fixed UTM reference within 100 km.

HTTP reference used in review: [RFC 9111](https://www.rfc-editor.org/rfc/rfc9111.html),
especially freshness directives and updating stored responses after validation.
