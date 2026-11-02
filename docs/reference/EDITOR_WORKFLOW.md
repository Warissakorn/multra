# History, keyboard editing and rotation

The Network Editor offers the same workflow in English and Thai. These controls extend the
existing authoring commands; they do not change the project format or simulation model.

## Selection and editing feedback

Hover, selection and active tool previews use the shared palette accent (D81/D84), with
different stroke/tint treatment; road display-type colour and markings remain visible. Geometry points are small square
grips, shown only for a single object in Select. Rectangular lane tabs sit directly against
both edges at the start, middle and end; drag a tab outward/inward to add/remove lanes. Tabs
have no count labels. Lane markings are 0.10 m wide, with 3 m dashes and gaps that scale with
zoom. Each stroke of a double marking has the same width.

A plain click on empty space clears selection, Properties and table-owned highlights. Shift
or Ctrl with an empty click preserves the selection; dragging a box adds its objects. Change
of tool clears the previous selection and cancels unfinished gestures. Table selection in an
authoring tool can still show properties, but exposes no geometry/lane grips. Ctrl+Tab cycling and
canvas Delete operate in Select (conflict areas also support Ctrl+Tab in their own tool).

Escape first cancels an unfinished gesture/draft; when idle, it clears selection. With no draft
open, empty-space clicks in any authoring tool clear selection without showing an error. Enter
commits and Escape cancels a multi-click draft.
Plain right/middle drag remains pan; Ctrl+right-click or Ctrl+right-drag creates or changes
in every tool, and a plain left click never authors outside Routes (D116) and Measure/Calibrate,
except that once a multi-click draft (Zone, Node, Queue counter, Section, two-click Connector) has
its first point, plain left clicks place the rest (D306). Shift-click toggles, Ctrl-click adds an unselected object, and
Ctrl-drag of a selected object copies it, retaining the D30 convention.

## History

Use **View → History** or **Ctrl+Shift+H**. The dock lists reachable states from oldest
to newest, including undone states. The current state is bold and labelled; the saved state
has its own marker. Undo/Redo buttons name the operation they will perform.

Select a row and press **Enter**, or double-click it, to restore that state. Browsing with arrow
keys does not change the document. Restoration clears an existing simulation snapshot and
refreshes the canvas, properties, tables and diagnostics. Returning to the saved state clears
the unsaved asterisk. Undo/Redo history retains up to 100 edits; its oldest state may therefore
be later than the original document. Expired states cannot be restored.

Editing after undo creates a new branch and discards the previous redo future. Revision IDs
are not reused. Failed/no-op edits preserve history. Save marks the current state without
clearing history; New/Open/recovery begin a fresh history. The history is not saved in the file.

## Keyboard moves and cancellation

In **Select (S)** with the canvas focused, arrow keys move selected Links/Connectors by the grid
interval with Snap on, or by one metre with Snap off (the default, D428). **Shift** multiplies the step by ten.
Up means increasing world Y; Right means increasing X. Each key step is one undoable edit.
Text/spin fields keep their normal arrow-key behavior. Ctrl/Alt/Meta arrows do not move objects.

The existing group-move rules apply: internal Connectors move with both parent Links; other
Connectors retain their own positions unless selected explicitly. Moving a road away from an
attachment can remove that Connector and its dependent objects in the same undoable edit.
Heads ride their parent roads; head-only selections cannot be nudged independently.

Arrows cannot move objects during a mouse gesture. Escape, tool/selection/level changes and
loss of canvas focus cancel active drags, copies and Ctrl-right creation; a late mouse release
cannot commit the cancelled operation. Multi-click drawing/connection drafts remain available
when simply moving focus between controls between clicks.

Level filtering drops hidden selections and disables their editing controls. Explicit table,
inspector or diagnostic selection reveals a hidden object by switching the filter to all
levels. Invalid or deleted object IDs are not accepted as selection targets.

## Rotate a selection

In **Select (S)**, hold **Alt** and left-drag a selected road to rotate the whole selection.
Alt-dragging an unselected road first selects it. The pivot is the centre of the world-axis
bounds of the affected road surfaces, including internal Connectors. Signal heads do not
change that centre. The canvas shows the pivot, angle and accent outlines before committing.
Keep the pointer away from the pivot: a press or release within eight screen pixels of it
has no stable angle and does not rotate anything. A click or small pointer jitter also does
not rotate. Drag angles ignore the metre grid; hold **Shift** for **15-degree** increments.

Use **Rotate selection** on the toolbar or **Ctrl+Shift+R** to enter an exact angle from
−360 to +360 degrees, to two decimal places. Positive angles turn counter-clockwise in world
coordinates; negative angles turn clockwise. The dialog uses the same selection and pivot as
the gesture. Zero and complete turns leave the document, saved state and redo history intact.
This control also works when a window manager reserves the Alt-drag chord.

Selected Links and explicitly selected Connectors rotate once. A Connector also rotates when
both parent Links are selected; its authored stations, curve points, lane blend, lane widths
and markings are retained. Heads ride their parent roads at their existing stations. A
head-only selection cannot rotate independently. Unrelated selected heads stay on their roads.

Other Connectors keep their world positions under M1.20. After rotation, affected attachments
are re-read; an endpoint still on its parent carriageway snaps to its lane middle. A Connector
that leaves either parent road is removed together with its dependent routes, inputs and
heads. The accent outline previews the rigid transform before this attachment check. **One
Undo restores the entire edit**, including any removed dependants; Redo restores its result.
Rotation clears a compiled run only on a successful change. Save/reopen retains the geometry.
Escape, focus loss, tool/selection/level changes or document replacement cancel the drag;
a later release cannot commit it. Existing copy, move and vertex gestures keep their chords.

## Draw a route and a vehicle input

Routes use an explicit trace gesture (D116, owner request):

| Gesture | Effect |
|---|---|
| `R`, then click a road | Starts an unsaved route at the whole Link or Connector |
| Move over connected roads | Previews the actual path; passing a branch remembers that choice |
| Move back onto an earlier route object | Trims the preview back to that object |
| Click the destination road | Saves the complete preview as one undoable edit |
| `Backspace` | Removes the last object from the trace |
| `Esc`, tool/level change or focus loss | Cancels without storing anything |
| `Ctrl`+right-click | Retains the earlier start/append gesture; `Enter` saves it |
| `V`, then `Ctrl`+right-click a Link | Opens a vehicle input on that Link, with or without a route |
| Right-click a stored route or input | Edit, delete, or show in its table |

An unreachable or equally ambiguous path is rejected. Pass over the desired intermediate
branch to disambiguate it; the gesture does not guess a turn. Hover never mutates the document.
Double-click adds no second commit. Other authoring tools retain D84's Ctrl+right gesture.

Route start/end, vehicle inputs and signal heads share three-pixel cosmetic crossbars with
wider hit regions. Bars intersect the road normal with the actual lane rails. A route's
static blue surface tint follows its compiled spans, including mid-Link arrivals; it has no
straight rubber band, moving dashes or repeated arrows. A plain destination click places the end crossbar at the projected pointer station;
hovering within the same road moves just that crossbar, without rebuilding the route.
The click is re-read even when Qt coalesces its last move. The marker persists as
`definition.routeDestinations[routeId] = { objectId, station }` (schema 30), with Link
stations on the reference polyline and Connector stations on its bundle centreline.
It is editor presentation only: runtime routes and their full compiled tint still
extend to their existing object boundaries. Older routes and Ctrl+right/Enter keep
boundary markers. Backspace drops the preview destination; Escape/tool/level/focus
changes cancel it. Save/Open and Undo retain the committed point. Route deletion
removes it; removing its road or changing the destination road drops it; reversing a Link mirrors its
station; splitting remaps it to the appropriate child, refusing the 0.2 m cut gap.
Shortening past a marker is rejected atomically. Only the selected stored route is tinted.
Inputs mark the served entry lanes (separate bars across non-contiguous positive lane shares),
with a localized rate unit; interval inputs show their number of periods instead of a misleading
instantaneous scalar. Inputs on a hidden level are hidden. Signal bars cover only their controlled
lane. In Edit they are neutral, with a blue selection halo; Run colours the same snapshot bars
from the real signal program. Reset restores the authored bars. Marker stations are saved; cosmetic paint state is not.

## Workspace and screen space

The application opens maximized. The command bar uses one row on wide windows and two rows
below 1180 logical pixels in Compact (1260 in Comfortable, 1340 in Large), keeping playback
and language controls reachable. File, Edit, View,
Simulation, Help and Language menus retain full command names and keyboard shortcuts.
The Simulation bar reads Run/Pause (F5), Step (F6) | Seed, speed | Reset; Reset and Drawing
tools' Delete selected sit last behind a separator (D193).

The Network Objects palette groups the tools as Geometry (Select, Link, Connector, Split),
Control (Signal heads, Conflict areas), Demand (Routes, Vehicle inputs, Zones), Measurement
(Queue counters, Travel-time sections) and Tools (Measure, Calibrate); every tool keeps its
chord. The arrow above the tree (also View → icons only) hides the names; tooltips and
accessible names keep name and chord. Named rows use short tool names with a separate chord
column. Icons-only uses thin category separators and gives the released width to the canvas.
The view is session-only; Reset chooses names at 1500 logical pixels and above, icons below.
A manual choice remains through task switches and window resizing (D432, amending D193).

- **The network view is a dock too (D431).** Its title bar is the validation-marker line: drag it
  to another side, or double-click it (or use View → Float network view) to float it onto another
  screen. It cannot close and keeps its status and message lines. A window resize goes to it.
- **Ctrl+Shift+F** (View → Focus on network) hides the other docks and restores the previous layout
  on the next press, including hidden and floating panels. Opening a panel also leaves focus mode.
- The Objects panel shows one task group at a time, chosen in the combo left of its tabs:
  Network (Links, Connectors, Link types, Street segments, Cross-sections), Control (Signal heads, Signal control, Conflict areas), Demand
  (Routes, Vehicle inputs, Routing decisions, Zones, OD matrices), Measurement (Queue counters,
  Travel-time sections), Results and Problems. A jump to a page (a tool, Show in table, a failed
  check or run) switches to its group; Problems jumps also reopen the panel (D194).
- **View → Workspace → Drawing / Tables / Results** changes the session layout. Drawing is the
  opening layout, with the palette and Properties beside the canvas and Objects hidden. Tables
  opens a bounded Objects area while keeping a readable network preview. Results hides the side panels and gives
  the full dock area to results (a floating network view can remain visible). Navigating to an object page chooses
  Tables; navigating to Results chooses Results. No document, selection or run state is changed.
- **View → Reset panel layout** restores dock positions and the Drawing layout, with the
  Objects panel's Network group selected for the next time it opens.
- **Ctrl+I**, **Ctrl+Shift+O**, **Ctrl+Shift+H** and **Ctrl+Shift+T** toggle Properties, Objects,
  History and the Network Objects palette respectively.
- Properties has a scrollable common section (ID, Name and appearance) above independently
  scrolling tabs. Its content does not force a short window taller.
  Expand **Network and appearance** for drivingSide, level and display type. drivingSide
  applies at once; level and display type wait for their Apply button, below a divider. Labels
  share one column and wrap; a narrow panel puts every field under its label (D195).
- Signal-head table actions appear on the Signal heads tab. Run settings and runnability
  checks are always available in the Simulation menu. **F1** opens gesture/keyboard help.

**View → UI density → Compact / Comfortable / Large** is session-only and shared with the Lab.
Rows and controls use 24 / 28 / 32 logical pixels; label fonts use 11 / 12 / 13 px, numbers
12 / 13 / 14 px and body text 13 / 14 / 15 px. Canvas zoom and stored model values are independent.
The ten-digit unsigned seed has a 104 px field. Initial object columns fit their contents up to
320 px; user-resized widths survive selection, language and data refreshes. Long values retain
normal table scrolling/tooltips. Demand-period columns fit the numeric editor's minimum width.

Results stacks movement and queue tables below 1250 logical pixels of content width. Macro,
Meso and seed-batch commands appear in their own Results tabs; Export/Copy stays common.
The unvalidated marker and complete simulation figures remain visible in focus mode.
Wrapped labels retain height-for-width and a natural hint capped to the current page;
fields reserve their font minimum, so narrow forms wrap and wide forms rejoin their rows.
Explanatory text wraps automatically; intentional paragraph breaks remain.
A wheel never changes a spin-box or closed combo value, even with focus or inside a
later-created dialog. Above a field in a scrollable panel it scrolls that panel instead;
keyboard/arrows and combo choices still edit, an open combo list still scrolls, and
the canvas wheel still zooms. Tooltips use a plain-text popup no wider than 360 logical
pixels, clamped to their owner window and screen. Thai and long unbroken names wrap;
content taller than the window is readable by scrolling the tooltip.
Escape/click/deactivation dismisses it, and so does any scroll of a panel, list or canvas
that encloses the field it describes, since the field has then moved away from it.

Layout examples from the Linux offscreen Qt platform:
[English, 1360×860](../images/workspace-en.png),
[Thai, 1024×768](../images/workspace-th.png), and
[Thai focus mode](../images/workspace-focus.png).

The first plain Route click on a Link also places an opt-in recognition line at its
projected station. Start on the same line to add a Route to its decision. Select-drag
moves the line; [POSITIONED_ROUTING](POSITIONED_ROUTING.md) defines grouping, validation,
passage-time choice and the numerical boundary. Legacy Ctrl+right/Enter remains unchanged.

## Safe form and document refreshes (D432)

Input, routing-decision, speed-decision, signal head/program/controller, run-settings and study-period
forms validate and commit before accepting. A rejected submission keeps the form and staged
values open with a localized explanation. Untouched numeric fields retain stored precision;
opening and confirming a form is not an edit. Study periods expose start/end day as well as clock
time, preserve second-day/fractional values untouched, and infer the next day for an overnight
clock interval on the same chosen day. The stored two-day limit still applies.

A language change preserves pending Properties text, numbers and choices. Empty comma-separated
lane-width entries are invalid for both the hint and Apply. Object names allow 200 Unicode scalar
characters, including Thai. Clicking a table cell editor selects its own row before Remove.
Selecting an input replaces an earlier route-table highlight with that input's route.

New/Open/recovery invalidates list and diagnostics views by document generation, even when the
new document reuses revision numbers and row counts. Export and automation CSV/screenshot writes
refuse the active project and recovery destinations through the common path guard.

Only the active object/Results page contributes its vertical minimum, so a hidden stacked table
does not make another task taller. The common Properties section shrinks to its content when
folded and is bounded when expanded.

Fit reserves screen-pixel space for labels. Placement after redraw, pan and resize keeps visible
labels within the viewport and clear of HUD/map overlays; selected labels retain priority within
that usable space. See [CANVAS_STYLE](../plans/CANVAS_STYLE.md) and
[the remediation evidence](../evidence/ui-audit-fixes.md).
