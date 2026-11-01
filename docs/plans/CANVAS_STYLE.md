# Canvas style — Aimsun-like network drawing, 2026-11-01

The owner chose these in a grill session on 2026-11-01 (Q1–Q12). This file holds the delivery
slices. The decision record holds what each slice decided: slice A is
[D423](../decisions/RECORD.md#d423).

## Owner decisions

| Q | Decision |
|---|---|
| Q1 | **Look.** Aimsun-like style. It is schematic at overview and detailed when zoomed in, through the existing projected-pixel LOD (D200). |
| Q2 | **Order.** A road surface and junction → B labels → C run vehicles → D surface markings → E control and measurement objects. |
| Q3 | **Draw order.** Each object is split into layers: kerb, surface, Connector dividers, Link markings. Junction lines must not tangle. |
| Q4 | **Allowed changes.** Drawing code and canvas tokens in `canvas_style.hpp` only. No schema or project bytes. The single exception is Q10's catalog palettes. |
| Q5 | **Junction holes.** A Node polygon is filled as junction road. With no Node, holes stay as kerbed islands. No computed hull. |
| Q6 | **Connector colour.** A Connector surface uses its type's `linkColor`. `connectorColor` stays for wireframe and emphasis. |
| Q7 | **Connector lines.** Edit view shows each lane's centre path faintly from 8 px per lane (implemented as lane centres, not dividers: most shipped Connectors have one lane and so no divider; D423). They are full on selection, hover or the Connect tool, and hidden in a run. |
| Q8 | **Kerb.** A thin light-grey kerb in slice A. The white inner edge line comes in slice D. |
| Q9 | **Labels.** Labels are placed in screen space: try the other side, then hide lower priority. Selected or hovered labels always show, and every label has a halo. |
| Q10 | **Vehicle colours.** Default per-type colour palettes go in `data/vehicle-appearance/*.json`, which a user palette still overrides. Vehicles get a soft shadow and keep the D199 casing. |
| Q11 | **Markings slice.** White inner edge line, lane arrows derived from Connectors, white stop line. Painting the median is deferred. |
| Q12 | **Who implements.** Claude implements every slice, one commit and one D-row each, then stops for owner review. |

## Common acceptance

Every slice must meet all of these:
- No change to simulation, hits, Ctrl+A, gestures, poses or file bytes. The four shipped projects
  at seed 42 and `--node-analysis` stay byte-identical.
- Every new canvas layer sits below z 11 per level, so the D28 run-layer cache keeps it.
- Both driving sides are tested. Forcing checks come first.
- Before and after images of the shipped projects go in `docs/images/canvas-*`, including grayscale.
- `ctest`, the three checks and Windows CI pass.

## Slice A — road surface and junction (done, D423)

Per level, z = level·100 + 0.5 kerb, 1 surfaces, 1.5 faint Connector lane guides, 2 Link markings.
A Connector being worked on keeps its full edges, dividers and mouth edges at 4.5.
Images: [before](../images/canvas-layers-before/four-leg-run-9px.png),
[after](../images/canvas-layers-after/four-leg-run-9px.png); edit view
[before](../images/canvas-layers-before/four-leg-edit-9px.png),
[after](../images/canvas-layers-after/four-leg-edit-9px.png). Images are from a scratchpad render of the
shipped projects at the named scale after a redraw; `canvas-layers-ui` writes its own with a second argument.

## Slice B — labels (done, D424)

`CanvasLabel` with a Base glyph halo; one screen-space pass per redraw places each label right,
left, above or below its anchor, or hides it; a highlighted Node, or an input's or decision's
selected/hovered origin Link, pins its label. Images:
[before](../images/canvas-labels-before/four-leg-1.5px-th.png),
[after](../images/canvas-labels-after/four-leg-1.5px-th.png).

## Slice B2 — Vistro-like boxed labels (done, D425)

The owner asked for labels like PTV Vistro's, spec first, and answered two questions: which labels
are boxed (input rates/periods and the decision names that sit beside them) and how they look (a
white box in a grey frame). No full Vistro manual was available; the public release notes say only
that Vistro's movement-volume texts keep a fixed size under zoom, and the sibling Visum graphics
offer framed labels and hiding overlaps. The spec below is MULTRA's own design in that spirit.

| Part | Spec |
|---|---|
| Box | Fixed screen size (ignores zoom). Base (white) fill, 1 px Dark (grey) frame, 3 px corners, 4/2 px padding. Text in the Text role: `numericFont()` for inputs, the UI font for decisions. Content as before ("500 veh/h", "500 คัน/ชม.", "4 periods", the decision's name or id). |
| Pinned | While the origin Link is selected or hovered (D424), the frame is 1.5 px in the accent colour and the box always shows. |
| Leader | A 1 px cosmetic Dark line from the anchor (input crossbar end, decision diamond) to the nearest point of the frame, always drawn with its box, never hit (`label-leader`, z just under the box). |
| Placement | D424's pass. A box tries eight directions (E, W, N, S, NE, NW, SE, SW) at 12 px, then at 32 px, and takes the first place whose rectangle (2 px margin) meets no placed label; else it hides with its leader. Pinned first, then input > decision > Node > zone. Node and zone names stay halo text with their four places. |
| Unchanged | A click on a box still opens its input; hits, files and results are untouched. |

Images: [before](../images/canvas-labels-before/four-leg-0.9px-en.png),
[after](../images/canvas-labels-after/four-leg-0.9px-en.png).

## Slice B3 — stacked labels that keep off roads (done, D426)

The owner asked that labels close together line up one under another, never overlap, and not
cover other objects unless they must.

| Part | Rule |
|---|---|
| Cluster | Box labels whose anchors chain within 48 screen px of each other (single linkage) form one unit; a halo label is a unit alone. |
| Column | A unit's boxes stand in one column, 2 px apart: pinned first, then input before decision, then top to bottom by anchor. They align on the side facing the anchors (left edges east of them, right edges west, centres north or south). Each keeps its own leader to its own anchor. |
| Places | E, W, N, S, NE, NW, SE, SW of the anchors' bounding box at 12, 32 and 64 px (a halo label: right, left, above, below). |
| Choice | The first place that meets no placed label and no other drawn object; else the first that meets no placed label (covering an object only when it must); else a pinned unit takes its first place and any other unit hides whole, leaders too. |
| Other objects | Visible items under the box's shape: road surfaces, kerbs, markings, markers, stop lines, signals, conflicts, counters, Node outlines. Not the background, grid, zone washes, feedback, leaders, run items, or the unit's own anchor markers. Queried on the scene index only for the candidates; no new cache (D28). |

Images: [B2](../images/canvas-labels-b2/four-leg-0.9px-en.png),
[B3](../images/canvas-labels-after/four-leg-0.9px-en.png).

## Owner-requested: true-size lane dashes (done, D430)

Dashed lane lines are cut in metres (`laneDash`/`laneGap` per display type, default 3/3) and keep
their length at every zoom; Slice D's markings use the same cut.

## Slice C — run vehicles (next)

- **Palettes.** Default per-type palettes are stored in the catalog files. The existing
  `vehicleDisplayColor` hash picks the colour and draws no simulation random numbers.
- **Shadow.** A soft offset shadow is painted only, so hit shapes are unchanged.
- **Unchanged.** The 14 px and 4×2.5 px LOD floors and the D199 casing stay.

## Slice D — surface markings

- **Edge line.** A white solid inner edge line.
- **Arrows.** Lane arrows come from the Connectors leaving each lane, and are mirrored by `drivingSide`.
- **Stop line.** A white stop line across each lane under the D199 signal bars.

## Slice E — control and measurement objects

To be grilled after slices A–D are reviewed.
