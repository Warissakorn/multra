# UI presentation proposal — phase 1, 2026-10-15

**Owner accepted the recommended direction on 2026-10-15:** “ทำตามแนะนำ”. B16=A, B17=the
five task groups; B18 logic/state ideas remain deferred. This specifies later slices, not
current behaviour. Read the [source audit](../audits/UI_PRESENTATION_AUDIT.md) and
[delivery/acceptance plan](UI_PRESENTATION_SLICES.md). Existing semantics remain in
[EDITOR_WORKFLOW](../reference/EDITOR_WORKFLOW.md). No colour palette or theme redesign is proposed.

## 1. Layout A: task groups inside the existing Objects dock (recommended)

Keep editorPaletteDock, editorInspectorDock, editorObjectsDock, editorHistoryDock,
editorObjectTabs and editorResultsTabs. Do not rename, replace or duplicate these objects.
Use a task combo above the existing page strip; display only pages of the chosen task with
QTabBar visibility. Use page identities, not visual positions, for all jumps and translations.
A group jump always exposes its owning dock, exits focus through the existing mechanism,
selects the page and frames the same object as before. No second selection model.

| Task group | Existing pages, proposed order |
|---|---|
| Network | Links; Connectors |
| Control | Signal heads; Signal control; Conflict areas |
| Demand | Routes; Vehicle inputs; Routing decisions; Zones; OD matrices |
| Measurement | Queue counters; Travel-time sections |
| Results | Results, retaining all four inner pages |
| Problems | Problems |

Show all task names in one compact combo at both sizes. Do not hide Problems behind a run-only
mode. Keep current actions reachable from menus and Ctrl+K. The selector does not create a
workspace preset, a remembered category, a badge count or an automatic document mode.

**1360×860, logical pixels, schematic:**

```text
+-----------------------------------------------------------------------+
| File Edit View Map Simulation Help Language                            |
| File/Undo | Fit Rotate Snap grid | Run/Pause Step Reset seed speed | UI |
+----------------+--------------------------------------+---------------+
| Network Objects| Not yet validated                    | Properties    |
| Geometry       | Existing run summary (wraps)         | ID / Name     |
| Control        |                                      | Appearance >  |
| Demand         |                                      | Behaviour     |
| Measurement    |             NETWORK CANVAS           | Links/Conn/Bg |
| Tools          |                                      | scrollable    |
| level          | map status top-left                  | label / value |
| background     | credit bottom-left; HUD bottom-right | Apply groups  |
+----------------+--------------------------------------+---------------+
| Objects and problems: [Task: Network v]                                |
| [Links] [Connectors]                                                   |
| Existing page actions | separator | destructive actions                |
| Existing read-only table                                              |
+-----------------------------------------------------------------------+
| existing coordinates                                                  |
+-----------------------------------------------------------------------+
```

**1024×768, logical pixels, schematic:**

```text
+---------------------------------------------------------------+
| File Edit View Map Simulation Help Language                    |
| File/Undo | Fit Rotate Snap grid                               |
| Run/Pause Step Reset seed speed | panels/focus | language       |
+-----------+-----------------------------------+---------------+
| Tools     | Not yet validated                 | Properties    |
| icon+name | run summary (wraps)               | ID / Name     |
| or icons  |                                   | wrapped forms |
|           | CANVAS >=400 x 250                | scroll tabs   |
| level     | map credit          HUD           |               |
+-----------+-----------------------------------+---------------+
| Objects: [Task v] [pages with elision/scroll buttons]            |
| actions / overflow | table with horizontal scroll              |
+---------------------------------------------------------------+
| coordinates                                                   |
+---------------------------------------------------------------+
```

Sizing budget to test, not an achieved screenshot: retain approximately 180 palette, 310
Properties, 185 bottom dock at wide size. At 1024, permit palette about 160 and Properties
about 280 subject to font minimums; prefer table scroll/toolbar overflow over widening the
window. Cap additional group-selector height to one 24 px row plus 4 px spacing. Existing
scope/run text wrapping consumes variable height: measure actual viewport rather than subtracting
nominal dock sizes. If it falls below 400×250, reduce bottom chrome or defer A; never shrink
fonts, hide the marker or weaken the test. Default icon+name remains keyboard accessible.

Focus mode keeps the validation marker, summary and Simulation toolbar. Save/restore must keep
custom hidden, floating and moved docks; resetting exits focus and restores defaults without
resetting the document or run. Group choice is transient and unaffected by focus toggles.
No persistent layout migration or new saved state format in this slice.

**Alternative B:** six task docks, tabified in the existing bottom region with one raised at a
time. Network palette and Properties remain on the sides. Results retains its inner tabs;
Zones retains its horizontal tables. This allows detachment but needs new dock names and
explicit migration/compatibility for editorObjectsDock and editorObjectTabs; that is a higher
risk than A. Owner must approve that compatibility design before B implementation.

## 2. Palette and toolbar

Proposed groups, preserving the hidden combo protocol and every shortcut:

| Group | Mode / tool / chord |
|---|---|
| Geometry | 0 Select S; 1 Link L; 5 Connector C; 2 Split X |
| Control | 8 Signal heads H; 9 Conflict areas A |
| Demand | 6 Routes R; 7 Vehicle inputs V; 12 Zones Z |
| Measurement | 10 Queue counters Q; 11 Travel-time sections T |
| Tools | 3 Measure M; 4 Calibrate K |

Provide a transient icon+name / icons-only toggle in the palette. No remembered collapse state.
Icons-only shows a checked/outlined active tool and keyboard focus independently of hover;
its tooltip and accessible name expose the existing name + shortcut. Preserve tree-to-level,
background, canvas, ID, Name focus order. Group names and toggle labels need both locales.

Group Run/Pause, Step, Reset, seed and speed on editorRunToolbar. Text+icon Run/Pause keeps
its dynamic verb, and the existing run summary supplies Edit/Run/paused/complete information;
if a visible mode label is added, derive it from current run state, never an independent mode.
Keep this group in focus mode. Preserve F5, F6, Space, Esc, + and - contexts.
Use existing 1180 breakpoint, menu fallbacks and bounded overflow for narrow layouts.

Separate delete/remove from Add/Edit and view controls with a separator and trailing position.
Retain confirmations, enabled rules and Undo semantics. Primary buttons and fields remain
24 px in normal, hover, pressed, checked, focused, invalid and disabled states; toolbar 32.
Do not alter token tests or use a border change that shifts text. No shadow or animation.
State language (D190), the same on toolbars, buttons, menus, tabs and list/table rows: hover =
Midlight fill only; pressed = Mid; checked/selected = Mid plus an accent marker on one edge
(underline on buttons and tabs, left bar on rows); only keyboard focus draws an accent frame.
No tooltip over the empty canvas or panel backgrounds; help text belongs in the F1 dialog.

## 3. Typography, values and long content

One family: bundled Noto Sans Thai, real 400 and 600. Pixel sizes are existing tokens.

| Size | Proposed role | Weight and use |
|---|---|---|
| 11 | Metadata/group/header | 600 group/table headers; 400 secondary metadata; Thai untracked |
| 12 | Numeric content | 400 tabular digits; table figures, spin fields, run statistics, Lab chart |
| 13 | Body/action | 400 labels, menus, tool names, dialogs, inspector; 600 primary action |
| 14 | Section heading | 600 Results/dialog/Lab section headings only, not every form label |
| 18 | Dialog/window content title | 600 command palette and major in-content title; avoid dock crowding |

Dock titles are 13/600 and instruction hints (palette, status bar) 11/400 (D190); Lab group
boxes are the 14/600 section heading. Use token values, not literal duplicate fonts. No Thai tracking, forced uppercase Thai or
font below 11 px. Do not add 14 px headings to small-table toolbars if their height would grow.
Thai vowels/tone marks must fit both regular and 600 cuts at 100/150/200%; a change to line-box
metrics requires a decision and test update, not a padding guess (D103).

UI numeric strings go through formatValue()/QLocale with explicit decimal precision and
right alignment; integers use the existing locale's integer formatting policy. Preserve the
window's QLocale::c(), Latin digits and existing precision per field/report. Language switching
changes labels only. Units are in translated table headers; spin suffixes remain where they
already explain an editable field. No UI formatting changes to project JSON, CSV or clipboard
export. Null and unavailable results retain existing empty/text semantics; never turn them to 0.

Tables and palette use right elision and a full-text tooltip for elided names/IDs; tooltips
use the D151 bounded plain-text popup, including long Thai and unbroken IDs. Forms/help use
ReflowLabel wrapping, preserve label buddies and height-for-width. Numeric cells never wrap
one value onto multiple lines; allow horizontal scrolling when columns exceed the viewport.

## 4. Monochrome icon shape catalogue

Retain 16 logical px, 1.5 px stroke, 20-unit drawing grid, 1×/2×/3× rasters and role colours.
These are shape proposals; compare at actual 16 px before adopting. Slice a adopted connector,
route, reset and focus as [D192](../decisions/RECORD.md#d192) records; the rest stay proposals.
All 30 current enum entries:

| Icon | Shape / intended distinction |
|---|---|
| document | Page outline, folded corner and two text strokes |
| open | Folder with raised tab |
| save | Disk outline and inset label |
| undo / redo | Opposite hooked arrows, retaining directional pair |
| fit | Four inward corners with centre cross |
| focus | Panel outline with sides withdrawn; no centre cross |
| finish | Check mark |
| rotate | Curved arrow around a small tilted rectangle |
| reset | Circular return arrow with a small origin tick; no tilted rectangle |
| remove | Bin with lid and two interior strokes |
| select | Pointer arrow |
| link | Parallel road rails, dashed divider |
| connector | Two short road ends joined by a curved twin-rail neck; no arrow |
| route | Single trace with start dot and destination arrow; no twin rails |
| input | Entry arrow crossing a road bar, replacing generic vehicle silhouette |
| signal | Vertical signal housing, three separated lamp rings |
| split | Road rails with a transverse cut and separation tick |
| measure | Ruler outline with unequal graduation strokes |
| image | Image rectangle with mountain line and sun; retained for Calibrate |
| run | Solid forward triangle |
| pause | Two vertical bars |
| step | Forward triangle and end bar |
| inspector | Panel with right-side fields |
| objects | Table grid |
| grid | Even grid intersections |
| conflict | Crossed road pair with hatched overlap centre |
| counter | Single road bar with three short tally ticks |
| section | Two endpoint bars joined by bracket/time span |
| zone | Closed asymmetric polygon |

Toolbar/menu entries for an existing object reuse its palette glyph; do not copy painter paths
into each table. Do not invent a controller/OD icon by misusing Signal head/Zone. Future enum
additions are part of slice a only if needed and reviewed. Non-object actions keep verb glyphs.
Render a monochrome contact sheet of all glyphs and inspect duplicate pairs; image inequality
alone is insufficient evidence of recognition.

## 5. Canvas hierarchy, HUD and LOD

Preserve the per-level road/vehicle order listed in the audit; do not renumber the scene into
the supplied spec's global 0–100 layers. Within each level: road surface → markings →
interaction feedback → control bands → head bars → vehicles/measurements → Run head casing.
Demand and interactive drafts remain above ordinary roads; handles remain available only under
the current tool/selection rules. Screen HUD is painted after scene content, ignores hit
queries and cannot intercept pan/zoom/draw gestures.

HUD: bottom-right inside viewport, 8 px inset and opaque Base backing; scale bar uses
1/2/5×10^n metres, 80–160 logical px where space allows, plus px/m zoom (no invented “100%”
physical scale). North uses a distinct arrow plus N only with valid existing map georeference;
use projection/local north direction, then the current view transform including heading-up.
If a trustworthy direction is unavailable, omit the arrow. Do not add a new survey computation
or claim grid north equals true north. Map attribution bottom-left and map status top-left
have priority; HUD must avoid their rectangles, scope marker and long translated text. In the
smallest viewport shorten the scale label/stack HUD, never suppress attribution/validation.

Proposed LOD is based on projected screen detail, not the supplied spec's undefined zoom “×”.

| Projected detail | Proposed paint policy |
|---|---|
| Lane width <3 logical px | Suppress unreadable minor decoration only; retain road/route/counter/head and selected handles |
| Lane width >=3 px | Existing lane markings and control geometry; avoid overlapping optional name labels (a 1e-9 relative rounding counts as 3 px, D205) |
| Vehicle length <14 px | Retain existing minimum-body symbol (4×2.5 px) |
| Vehicle length >=14 px | Existing windshield/cab detail |
| All scales | Existing grid >=8 px minor, 10× major; essential control, selection, validation and measurement cues remain |

These thresholds are proposals to validate, not performance claims. Automatic LOD must not
turn on Ctrl+A or change objectShape/hit targets. No new persistent cache or spatial index;
measure before proposing either and read D28. Check positive/negative scales and rotated views.

State distinctions independent of hue: hover = thin continuous outline; selected = heavier
outline + direction marks; primary = grips under existing Select rules; draft = dashed outline
and endpoint glyph; disabled = stable muted boundary + existing disabled interaction, no hover
promise; run vehicle = neutral edge casing over photos. R/A/G signal cues need an additional
pattern/glyph derived from the current phase, while retaining semantic roles and crossbar hit
geometry. Avoid dense text on every vehicle/head at overview zoom; measure legibility first.

## 6. Properties and table recipe

Keep ID/Name pinned above three scrolling pages. Use common label column alignment within
related forms; wrap long labels at narrow width. Group geometry, lanes, attachments, background
transform and appearance with 14 px headings only where space permits. Separate immediate
settings (e.g. drivingSide) from explicit Apply sections with an 8 px gap and a divider.
Do not change commit triggers or rename parameters. Multiple selection still edits the current
primary object; multi-object shared values are a separate proposal.

Table recipe: row token 24, numeric right/text left, header units, consistent 4/8 px gaps,
trailing destructive section, full-text tooltips, horizontal scroll and existing read-only/edit
policy. Apply the presentation helper to late-created dialogs and Lab as well as startup tables.
Zones/connectors remain horizontal 3:2. Results retains report notes, overloaded/unavailable
flags and all four pages. Lab keeps its permanent marker and live-retune/Apply boundaries.

## 7. Exclusions and decision gate

No changes to src/core, src/model, src/commands, input formats, numerical results, data-owned
road/vehicle colours, gesture chords, action keys or objectName values. No new hex literals,
themes, caches, dependencies, persisted workspace modes or preference storage.
The six state/logic feature ideas are separate owner selections in B18, with files and rationale
in the slice plan. B16 selected A and B17 selected the proposed groups. No new D-row until a visible
change is actually approved and implemented. Each subsequent session implements one slice.
