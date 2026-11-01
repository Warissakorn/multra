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

## Slice B — labels (next)

- **Placement.** One screen-space placement pass for input rates, decision and Node labels. A label
  that collides tries the other side of its anchor, then yields to a higher-priority one.
  Selected and hovered labels always show.
- **Halo.** Every label gets a Base-colour halo.
- **Tests.** Tests check no overlap at the shipped fit zooms, both locales, and rotated views.

## Slice C — run vehicles

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
