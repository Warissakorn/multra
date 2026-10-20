# Owner sitting — everything that waits on the owner, in one pass

The owner's answer to the 2026-10-09 review's R4 (D143): clear the queue in **one sitting** with
this sheet, on the owner's Windows machine, and carve out whatever the sitting does not reach.
This is the only list of owner items; [NEXT](../NEXT.md) points here instead of keeping a copy
(hard rule 3). A session prepares the build and prints this sheet; it records the owner's words
verbatim and fills in **no verdict itself**.

**Rules.**
- Each item names what to open, what to do, and its failure condition as a yes/no question.
  Write the owner's answer on its *Owner:* line with the date, then move the item's result to its
  ROADMAP/PROGRESS line and delete it here.
- **Carve-out (R4 b).** After the sitting, any item still unanswered and listed for more than 14
  days becomes a numbered carve-out in [ROADMAP](../ROADMAP.md) under its milestone (rule 2), and
  leaves this sheet. A look is never closed by a session (only a new D-row could change that).
- *Listed* is the date of the decision or merge that created the item.

Build for the sitting: `cmake --preset desktop`, `cmake --build --preset desktop`, then
`trafficsim-desktop`. Screen at 100 %, then 125/150/200 % where an item says so.

---

## A — Gates (first: a fail here changes what comes next)

1. **M0 plausibility** (listed 2026-09-11). Open `data/scenarios/crossing.json`, Run. Do vehicles
   accelerate, queue at red and discharge at green in a way an engineer recognises? A *no*
   triggers D13 (fix or replace car-following) before W74/W99 or M6. Observation only, not
   calibration. *Owner:*
2. **M3.2.7d T-junction exercise** (listed 2026-09-26). Fill the sheet in
   [M3_ACCEPTANCE](M3_ACCEPTANCE.md) §3 on `data/projects/t-junction-priority.traffic.json`. Does
   minor-road delay respond to gap time as an engineer expects? It is what M3.2 closes on (R7).
   *Owner:*
3. **M5.7 rehearsal** (listed 2026-10-08). The PROBLEM §4 sentence on a real aerial image and the
   M2.6 template with real volumes (B4), Run seeds 42-51, Copy into the report. Record file,
   commit, table and the image size against the 32 MiB cap; labelled partial until M6. Did any
   step need a workaround, hand-edited JSON or outside help? *Owner:*

## B — Decisions in chat (one line each)

4. **The M2.6 template** (listed 2026-09-25). `data/projects/m2.6-study-template.traffic.json`
   has placeholder volumes, a guessed timing-to-approach mapping and no aerial image. Supply the
   real study's counts, timing sheet and image for M5.7 (A3)? *Owner:*
9. **M6 benchmark choice (Q4)** (listed 2026-10-10, M6.0). Read the
   [option sheet](../evidence/m6-benchmark-options.md) and answer its §7 by letter: Q4's kind of
   benchmark, a primary and secondary option for S1 (saturation flow) and S2 (delay), and for U
   (minor-movement capacity) the option and whether U-e is booked as the R7 calibration path. The sheet holds no engine numbers; the tolerance comes after this
   answer and before any comparable run. Which options? *Owner:*

Answered 2026-10-17: **B19**, owner: “ช่วยตัดสินใจ” (“help decide”), decided on that delegation in
[D214](../decisions/RECORD.md#d214): the [tiers](../reference/BEHAVIOUR_PARAMETERS.md) stand, the 50 km/h
lateral reference moves to fixed, and tier B is ordered by effect on Thai study outputs (kerb bias,
commitment deceleration, merge deceleration, speed limits, …). Numbers are not reused.

Answered 2026-10-16: **B1, B2, B3, B5, B6, B8, B12, B13**, owner: “ปิดตามข้อเสนอ” (“close as
proposed”), [D201](../decisions/RECORD.md#d201). Keep `TrafficSim` until a release; D95/D101 stay off
and reopen only by route (i) in M3.4; the `level` skip is a discrepancy with M3_CONTRACT, queued as
a fix in NEXT; the standstill cap joins M3.2.8a.1; more routing stations wait for a study that needs
them; `four-leg-signalised` gets a 300 s cool-down (queued in NEXT); B12 is asked when multi-level S1
starts; the `w74` preset stays out of the catalog until M4.2 is revisited. Numbers are not reused.

Answered 2026-10-15: **UI B16/B17**, owner: “ทำตามแนะนำ” (“follow the recommendation”).
Proceed with layout A (task selector within the existing Objects dock) and the five proposed tool
groups, after slice 0. B18's six logic/state ideas stay separate and deferred; the reply does not
request those features. The [proposal](UI_PRESENTATION_SPEC.md) and
[slice plan](UI_PRESENTATION_SLICES.md) record the chosen direction; no visible UI change is made
by slice 0. Numbers B16–B18 are not reused.

Answered 2026-10-15: the fourteen multi-level workflow questions, as recommended
([D175](../decisions/RECORD.md#d175), [workflow](MULTI_LEVEL_WORKFLOW.md) §9); B14 and B15, moved out
([D178](../decisions/RECORD.md#d178): order accepted, keep-left default, standard motorcycle values for now).
Answered 2026-10-14 and moved out: B7 and B10 ([D158](../decisions/RECORD.md#d158)), B11
([D159](../decisions/RECORD.md#d159)). Numbers are not reused.

## C — Run view (the 125 % screen)

1. **D141 Run seeds** (listed 2026-10-09). On the M2.6 template: Results → Run seeds, Cancel part
   way, run again, Copy into Excel/Word, Export; Thai strings. Does the window stay responsive,
   and does the pasted table land column by column? *Owner:*
2. **D104 Results → CSV** (listed 2026-10-04). Run to the end, Export results (CSV). Does it open
   in a spreadsheet, marker line first? Greyed before the end? *Owner:*
3. **Rear-axle turning, phases 1–3** (listed 2026-10-05). Car and heavy vehicle on a curved
   Connector, a short Connector and an internal Link join; lane-change guidance at low zoom. Does
   the rear axle cut inside the curve with continuous heading, a stopped changer hold its body,
   and a second change continue the first? The D102 lane-change display (C4, merged here by D201): in
   `data/projects/lane-change-lab.traffic.json`, Lane drop scene, does a changer ease across and
   hold when stopped? *Owner:*
7. **D149 Compare with…** (listed 2026-10-10). Save a copy of the M2.6 template with one input
   raised; open the original, Results → Compare with… the copy (Seeds 42-51), Cancel part way, run
   again, Copy into Excel/Word, Export; Thai strings. Do the table and note read as alternative
   minus base, and does the pasted table land column by column? *Owner:*
8. **M8.6a zones** (listed 2026-10-14, D161). On a copy of four-leg: press Z, Ctrl+right-click
   four corners beside the West approach, Enter; in the Zones tab rename it, select the West
   approach Link and add an origin connector, the East exit and add a destination connector,
   change a share, Undo/Redo; Thai strings. Is the polygon easy to draw and the connector line
   readable at normal zoom? *Owner:*
9. **M8.6b OD table and lot** (listed 2026-10-14, D162). With two zones and their connectors:
   OD matrices → Add, type a few trips, untick and re-tick a zone; Zones → Parking lot… on one
   zone (capacity 3, a short dwell); Run to the end and open Results → Parking lots; Thai strings.
   Does the grid read as origin rows × destination columns, and do the lot figures make sense to an
   engineer? *Owner:*
10. **Behaviour Lab** (listed 2026-10-14, D165/D166). Simulation → *Behaviour Lab…*; run at 1×–5×;
   change `additiveSafetyDistance` and watch the gap chart move without a reset; change lanes or
   `straightLength` (restarts); click another vehicle, then *Follow tuned vehicle*; *Apply to
   project*, then Undo; switch to Thai. On the default Urban circuit judge the layout, the X2/X3
   left turns and the heading-up camera (toggle *Heading up*); change `green` (live) and *layout* →
   Oval (restarts). Is it usable for tuning, and is the camera comfortable? *Owner:*
11. **Review fixes D167–D174** (listed 2026-10-14). Retune the Lab length, Apply, Undo; select a
   lane suffix during a lane-change blend; Cancel a batch and restart it at once. Does anything
   look wrong or stall? *Owner:*
12. **MSVC Release timing** (listed 2026-10-14, D163/D164). On P-cores, the M2.6 CLI run: the
   compile figure (≈116 ms before D163) and the wall time (before D164). Record both numbers; a
   session cannot measure this machine. *Owner:*

## D — Right-of-way on the drawing

2. **D109/D110 conflict controls, Bézier coverage** (listed 2026-10-05). Curved Connector after an
   interior drag; a 3 × 3 crossing: nine lane-pair band pairs, one row, group edit/Delete, Undo;
   two separated intersections stay two rows. *Owner:*
3. **D114 edited Connector lane centres** (listed 2026-10-06). Curved 3 → 3 Connector and both
   taper sides; drag an interior point, rerun. Do vehicles, heads, waiting bars and coverage follow
   the edited lanes? Zero points, Reset straight, widths, save/reopen, Undo/Redo. *Owner:*
4. **D115/D117/D118 overlap display** (listed 2026-10-06). Oblique crossing, a Connector landing
   mid-Link, two Connectors leaving one lane, both traffic sides, obtuse/curved mouths, a taper.
   Do bands follow each lane and leave rails readable? Do Crossing/Merge edit together and
   Branching stay read-only; separated sites stay separate? *Owner:*

## E — Drawing and gestures (100/150/200 %)

2. **Canvas grid** (listed 2026-10-03). At 125 %, too faint? Fix colour, not width. *Owner:*
5. **D96 route overlay** (listed 2026-10-02). Route 2-lane Link → 1-lane Connector → 2-lane Link,
   end-mounted and mid-body. Both lanes of both Links shown, nothing upstream of a mid-body
   arrival? *Owner:*
6. **D100 Ctrl+A wireframe** (listed 2026-10-02). Is the line readable on the grid? Wanting
   vehicles on the line is the failure. *Owner:*
8. **D116 road crossbars and Route trace** (listed 2026-10-06). Curved/multi-lane roads,
   Connector-mounted heads; trace a branch, click the destination, Undo/Redo, Backspace, cancel,
   save/reopen. Signal Run bars stay at the stop position and change colour; Reset restores. *Owner:*
9. **D119 positioned routing** (listed 2026-10-06). Click a mid-Link source, hover to an exit, add
   a second Route from the line, edit weights/time/type rules, run routeless traffic. Recognition
   at the line, Undo/Redo, line drag, save/reopen, legacy files. Is the one-timestep recognition
   boundary acceptable for a study? *Owner:*
10. **D151 GUI interaction** (listed 2026-10-13, [EDITOR_WORKFLOW](../reference/EDITOR_WORKFLOW.md)).
   Create a Route to the middle of a curved road, move the pointer within that road, save/reopen
   and Undo; narrow/widen Properties in Thai and English; hover long descriptions near each window
   edge; wheel over focused and unfocused settings in panels and dialogs. Do settings stay unchanged
   while panels scroll and canvas zoom works? The destination marker is presentation only. *Owner:*
11. **MAP10 OSM background** (listed 2026-10-12, D150, [MAP_BACKGROUND](../reference/MAP_BACKGROUND.md)).
   Set a reference on a real study, trace a known road, pan/zoom, save/reopen. Are alignment and
   responsiveness good enough for tracing? No automated check closes survey accuracy. *Owner:*

13–15. **The current UI: D190 quiet canvas, D192 icons, D193 palette** (listed 2026-10-15/16;
   merged by D201, E12 dropped because slices a/b changed the workspace it asked to see unchanged).
   Compare the before/after images of [D190](../images/ui-states-after/workspace-th.png),
   [D192](../images/ui-icons-after/contact-sheet.png) and [D193](../images/ui-palette-after/workspace-en.png)
   (their *-before* folders beside them), then work in the current build at 100/150/200 % on
   Windows, in English and Thai: rest on the empty canvas, Objects panel and a conflict area (no
   tooltip; F1 shows the help); find the active tool from its underline and keyboard focus with Tab;
   tell Connect lanes from Routes, Rotate from Reset and Fit from Focus without the tooltip; read the
   five palette groups, then View → icons only (blank group rows acceptable? extra row above the tree
   worth it?); on the Simulation bar, is Reset behind its separator clear and Run (F5) unclipped?
   Linux offscreen evidence does not close this. *Owner:*

16. **D194 grouped Objects navigation** (listed 2026-10-16). Compare
   [before](../images/ui-palette-after/workspace-en.png) and
   [after](../images/ui-groups-after/workspace-en.png), then the
   [English](../images/ui-groups-after/objects-demand-en.png) and
   [Thai](../images/ui-groups-after/objects-demand-th.png) Demand group at 1024×768. Use the combo
   left of the Objects tabs: do the six groups hold the pages you expect, and is Network the right
   default? Press A, Q or Z, Show in table on a route, and Check runnability: does the panel switch
   group where you look? At 100/150/200 % on Windows, is the combo text unclipped and the first tab
   uncovered in both languages? Linux offscreen evidence does not close this. *Owner:*
17. **D195 Properties presentation** (listed 2026-10-16). Compare
   [before](../images/ui-properties-before/properties-en-1360.png) and
   [after](../images/ui-properties-after/properties-en-1360.png), then Thai at
   [1024](../images/ui-properties-after/properties-th-1024.png) and a
   [200 px floating panel](../images/ui-properties-after/properties-th-narrow.png). Are wrapped
   labels in one 120 px column better than the old wide column? Is the divider between drivingSide
   and the level/display Apply clear? Two open findings need your call: at 1024×768 with
   *Network and appearance* open the top section is squeezed (it was before too); should that part
   scroll, which unpins ID/Name? Should long Apply buttons wrap or shorten at narrow widths? At
   100/150/200 % on Windows, is any Thai label clipped? Linux offscreen evidence does not close this. *Owner:*
18. **D196 table presentation** (listed 2026-10-16). Open Demand catalog, Behaviour library and
   an OD matrix: do their tables now match the Objects tables ([dialog](../images/ui-tables-after/tables-dialog.png))
   and do the OD grids keep their taller editing rows? Rest on a cut-off name or route chain: is
   the full-text popup ([example](../images/ui-tables-after/tables-tooltip.png)) helpful or in the
   way, and is its height right? At 100/150/200 % on Windows, are figures right-aligned and
   unclipped? Linux offscreen evidence does not close this. *Owner:*
19. **D197 canvas HUD** (listed 2026-10-16). Look at the bottom-right of the canvas
   ([rotated georeference](../images/ui-hud-after/hud-rotated.png),
   [Thai 1024](../images/ui-hud-after/workspace-th.png)). Zoom with the wheel: does the bar keep
   round 1/2/5 m values and the px/m readout help? With a map background set, does the arrow point
   to true north, and turn with *rotationDegrees* and the Lab's heading-up? It is hidden without a
   map provider: is that right, or should a plain georeference show it too? Is bottom-right the
   right corner, clear of the credit at every size? At 100/150/200 % on Windows, is the text
   unclipped? Linux offscreen evidence does not close this. *Owner:* 2026-10-16, north without a
   map provider: “ควรแสดง” (show it), done as [D198](../decisions/RECORD.md#d198). The other
   questions are open.
20. **D199 run casings and signal shapes** (listed 2026-10-16). Run a project over a map photo and
   look at the [grayscale frame](../images/ui-run-after/run-grayscale.png): can you still see every
   vehicle's edge, and tell red (full bar), amber (thin middle) and green (open middle) apart without
   colour? Click a vehicle in the Lab: is it still picked where it was? The casing costs about 9–16 %
   of Debug paint time on Linux; does a Windows run feel slower? *Owner:*
21. **D200 lane markings at overview zoom** (listed 2026-10-16). Zoom out until a lane is under 3 px
   wide: lane lines and Connector edges stop drawing; roads, heads, selection and handles stay.
   Compare [at 3 px](../images/ui-lod-after/crossing-at-3px.png) and
   [just under](../images/ui-lod-after/crossing-under-3px.png). Do you miss anything at that zoom?
   Does zooming out on a large network feel quicker on Windows? *Owner:*
22. **D281 static run layer** (listed 2026-10-20). Run M2.6 on Windows, pan and zoom while it runs,
   select a Link mid-run. Do the roads, markings and Connectors look as they do in Edit, and does
   the run feel quicker than before (Linux: −28 % per Step)? *Owner:*

Carved out 2026-10-16 by [D201](../decisions/RECORD.md#d201) (R4 b applied before the sitting, owner's
choice): C5, C6, D1, E1, E3, E4 and E7, the looks listed more than 14 days earlier, are now
[M1.29](../ROADMAP.md#m129--owner-looks-carved-out-of-the-sitting-d201) and
[M3.2.10](../ROADMAP.md#m3210--owner-looks-carved-out-of-the-sitting-d201). They stay unchecked and close
nothing. Numbers are not reused.

## F — Dialogs: demand and behaviour

1. **D142 volume from turning counts** (listed 2026-10-09). Type a 15-minute count sheet into a
   decision, tick the input's checkbox; total and rows right; edit a count and Undo; save/reopen;
   run. *Owner:*
2. **Demand slices 1–6** (listed 2026-10-05). Catalog periods and type overrides in the desktop;
   the stacked PRs reviewed in order (#106, #107, then time/type). *Owner:*
3. **D128 behaviour library** (listed 2026-10-07). Capture the catalogs, duplicate a behaviour,
   heavy-vehicle class, an urban type with an override on several roads, effective list,
   Undo/Redo, delete with replacement, Thai, save/reopen, run; also a `w74` run. *Owner:*
