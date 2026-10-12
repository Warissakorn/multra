# GUI presentation and interaction (M1.29, D151)

The owner's eight-part GUI request applies to the existing Qt editor, dialogs and
Network editor. Preserve D84/D116 gestures, command boundaries and the engine.

| Row | Contract | Automated evidence |
|---|---|---|
| GUI1 | Bundled Noto Sans Thai, native antialiased text; canvas text upright in logical pixels, no scaled text bitmap | Design-system and GUI suites at 1, 1.25, 1.5 and 2x |
| GUI2 | Routes show a dashed destination crossbar at the hovered station and a solid saved line at the final click; hover does not mutate History | Canvas, station-routing UI, project marker tests |
| GUI3 | Forms wrap long rows, labels align left/vertically centred and retain buddies; tool/command shortcuts occupy a separate right column | Dynamic form, tool palette and existing UI suites |
| GUI4 | Input/decision/angle labels use bounded screen-space rectangles, deterministic priority and non-overlapping placement; displaced labels retain object identity | Dense eight-input fixture, picking, stable layout and low zoom |
| GUI5 | Tooltips use plain text, word/character wrapping and owner-window/screen bounds; excessive text has a grapheme-safe ellipsis and retains full accessible text | Dynamic tooltip near edges, Thai/metacharacters, wheel dismissal |
| GUI6 | Wheel never changes spin, combo or slider values, focused or not, with modifiers or pixel/angle deltas; containing panels scroll, open combo lists scroll without committing, canvas zoom remains | Signals, values, keyboard stepping, open list and canvas tests |
| GUI7 | Initial working-name identity has editable SVG, PNGs 16–512 and ICO; Qt, Windows executable and Linux desktop icons use it | Embedded sizes, resource build and installation |
| GUI8 | Cursor comes from the actual gesture state; cancel, release and lost focus clear transient states | Link/Connector, insertion, target validity, cancellation and pan tests |

## Route destination file semantics

Optional schema 30 `routeEndMarkers` maps route IDs to `{objectId, station}`.
Station is in metres along the Link's stored reference geometry or the Connector's
derived centreline. The object must occur in the route; the station must be finite
and within its length. A single-Link positioned route cannot end upstream of its
recognition station. Unknown fields and use before schema 30 fail on load.

The marker is **display only**. Routes still compile complete Links/Connectors;
vehicles do not disappear at this line. The route dialog says this explicitly.
Core types, route expansion, simulation and results semantics are unchanged.
Without markers the previous schema selection and serialized bytes remain intact.

Route creation, positioned decision and endpoint are one History transaction.
Undo/Redo and Save/Open retain the endpoint. Deleting a route or its referenced
road removes its marker; changing the route removes an orphaned marker. Shortening
a road past a marker fails atomically. Splitting transfers a downstream marker and
subtracts the child's start station; a marker within the continuity gap rejects the
split before mutation. Reversing an unreferenced Link maps `station` to `length-station`.

## Cursors and current gestures

| Gesture/state | Cursor |
|---|---|
| Select pointing | Arrow |
| Select Ctrl+right press before drag threshold | Crosshair with insertion glyph |
| Ctrl+right drag on empty space | Crosshair with Link glyph |
| Ctrl+right drag from a lane / valid two-click Connector target | Crosshair with Connector glyph |
| Invalid Connector or upstream Route target | Forbidden |
| Dragging an object/point/bar | Arrow with carrying hand |
| Lane width resize | Directional resize |
| Rotation preview | Rotation glyph |
| Middle/right pan | Closed hand |
| Other idle authoring tools | Crosshair |

The toolbar tool name cannot override `creationTool_`: Draw and Select can create
a Connector when the gesture starts on a lane. Custom cursor pixmaps are rendered
at the current display's device pixel ratio and retain logical hotspots.

## Label and tooltip policy

Labels reflow after camera/viewport changes. Priority and stable object IDs decide
placement; a hovered label gets priority. Light backgrounds and leader lines retain
readability and physical origin. Low zoom or insufficient room suppresses secondary
labels; full values stay in marker tooltips and object tables. A displaced decision
name remains selectable but cannot start dragging its physical bar.

Tooltips do not activate a window or intercept mouse input. Long names, IDs and
parameter text cannot become rich HTML. Panel wheel input dismisses them. Native
keyboard controls and explicit button/text edits remain available.

Automated offscreen DPI checks do not close the owner's physical Windows,
fractional-DPI or multiple-monitor appearance review. See [evidence](../evidence/gui-presentation.md).
