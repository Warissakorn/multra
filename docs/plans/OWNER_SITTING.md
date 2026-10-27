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
`multra-desktop`. Screen at 100 %, then 125/150/200 % where an item says so.

---

## A — Gates (first: a fail here changes what comes next)

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
21. **MA15's capacities and the M10.1 gate** (listed 2026-10-22, D294). Open Sheffi (1985), book
   p. 114, Figure 5.1, in print or a PDF. The Markdown copy's image of that page is empty, and the
   capacities c = (2, 4, 3) were derived from Tables 5.1 and 5.4
   ([evidence](../evidence/macro-sheffi-benchmark.md)). Does the figure print 2, 4 and 3? If yes,
   does MA15 (Table 5.4 reproduced, equilibrium within the 10⁻⁴ gap) meet the M10 gate's benchmark
   clause? *Owner:*
22. **M11.3 sources** (listed 2026-10-22). The LTM engine waits until §4 and §5 of
   [MESO_NETWORK](../reference/MESO_NETWORK.md) are checked against Yperman (2007, KU Leuven thesis)
   and Tampère et al. (2011, TR-B 45(1) 289–309). The environment refused every host that holds them
   (proxy 403). Either allow `arxiv.org`, `lirias.kuleuven.be` (and `sciencedirect.com` if possible)
   under the environment's Network access → Allowed domains
   (https://code.claude.com/docs/en/cloud-environments#network-access), or upload the PDFs
   (optionally Boyles et al., arXiv 2502.05182, too). Which? *Owner:*
   **2026-10-22:** the owner uploaded Tampère et al. (2011); the node model is in (D297). Still needed:
   Yperman (2007), or Boyles et al. as a citable secondary source for the LTM link model.
   **2026-10-22:** the owner supplied Yperman (2007) as a Markdown repository; the link model and the
   engine are in (D298). Boyles et al. are not needed. Only the host question remains, for later sources.
25. **M13 A3's choices** (listed 2026-10-27). Read [NODE_ANALYSIS §12](../reference/NODE_ANALYSIS.md#12-outlines-of-later-slices)
   A3 and answer its five questions by letter: (1) scope, (2) recognising protected-permitted
   operation, (3) opposing flow, (4) pedestrians and bicycles, (5) acceptance; each lists (a, recommended)
   first. A3 starts only when the owner asks; its contract and formulas follow these answers. Which
   letters? *Owner:*
Answered 2026-10-23: **B23**, owner: “เอาตามแนะนำ” (“as recommended”), [D315](../decisions/RECORD.md#d315):
QM (c), M1-d with M1-b, M2-c with M2-b, not-blind cases as verification only, Boyles et al. 2025
supplied as a Markdown repository or with `arxiv.org` allowed. Numbers are not reused.

Answered 2026-10-23 (in chat, AskUserQuestion): **S4's four choices**, as recommended,
[D313](../decisions/RECORD.md#d313): S4-1a, S4-2a, S4-3a, S4-4a ([READINESS](../reference/READINESS.md) §8).

Answered 2026-10-22: **B24**, owner: “เอาตามแนะนำ” (“as recommended”), [D304](../decisions/RECORD.md#d304):
S3's choices 1a, 2a, 3a, 4a, 5a, 6b, 7a ([ROADS](../reference/ROADS.md) §11). Numbers are not reused.

Answered 2026-10-21: **B20**, owner: “เอาตามแนะนำ” (“as recommended”), [D292](../decisions/RECORD.md#d292):
Meso choices 1a–7a ([MESO_NETWORK](../reference/MESO_NETWORK.md) §11). Numbers are not reused.

Answered 2026-10-17: **B19**, owner: “ช่วยตัดสินใจ” (“help decide”), decided on that delegation in
[D214](../decisions/RECORD.md#d214): the [tiers](../reference/BEHAVIOUR_PARAMETERS.md) stand, the 50 km/h
lateral reference moves to fixed, and tier B is ordered by effect on Thai study outputs (kerb bias,
commitment deceleration, merge deceleration, speed limits, …). Numbers are not reused.

Answered 2026-10-16: **B1, B2, B3, B5, B6, B8, B12, B13**, owner: “ปิดตามข้อเสนอ” (“close as
proposed”), [D201](../decisions/RECORD.md#d201). Keep `TrafficSim` until a release; D95/D101 stay off
and reopen only by route (i) in M3.4; the `level` skip is a discrepancy with M3_CONTRACT, queued as
a fix in NEXT; the standstill cap joins M3.2.8a.1; more routing stations wait for a study that needs
them; `four-leg-signalised` gets a 300 s cool-down (done 2026-10-25, D337); B12 is asked when multi-level S1
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
13. **D333/D334 vehicle library and colours** (listed 2026-10-24). On Windows at high DPI, run a
   project with the seven bundled types at working and overview zoom; edit fixed and mixed type
   colours, restart, and check the Behaviour Lab's appearance. Is each type recognisable and are
   the colours kept? The type parameters stay illustrative until a study calibrates them. *Owner:*

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
   Set a reference on a real study, trace a known road, pan/click without a camera jump, zoom out
   to the overview, choose a country with and without a province, interrupt and restore the
   connection and retry, then save/reopen, on a packaged build (its TLS/PROJ dependencies). Are
   alignment and responsiveness good enough for tracing? Linux local-server tests and automated
   checks do not close survey accuracy. *Owner:*

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
   the run feel quicker than before (Linux: −38 % per Step with D282 and D283)? Does the status line above
   the canvas still show all its lines at 100–200 %? *Owner:*
23. **D306 E1 tool pointers and clicks** (listed 2026-10-22). At 100/150/200 % on Windows, use
   each drawing tool: is its pointer glyph clear, and do plain clicks after a draft's first point
   add points without starting anything else? *Owner:*

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
4. **D318 dialogs keep their buttons on screen** (listed 2026-10-23,
   [dialog-fit](../evidence/dialog-fit.md)). At 1366×768 and at 150 %, open the dialogs whose rows
   can grow. Are OK/Cancel always reachable without moving the window? *Owner:*

## G — Object interchange

1. **M1.23.1 object table CSV** (listed 2026-10-27, D365). On packaged Windows, open a project,
   make an unsaved name edit containing Thai text, a comma and quotes, then use File → Export
   object table (CSV) for Links, Connectors, inputs, routes and decisions. Do Unicode filenames
   and nested JSON cells open correctly in the study's spreadsheet/CSV reader? Check an empty
   draft, Cancel, a failed destination and refusal to overwrite this window's project/recovery;
   confirm no dirty/revision/Undo or Run state changes. Does a name starting with `=` open as
   text with one leading `'` (D368)? *Owner:*

2. **M1.23.2 network PNG** (listed 2026-10-27, D367). On packaged Windows, export a whole
   network and current view with Thai filenames at 100/150/200% display scaling. Check chosen
   pixel dimensions, active level/wireframe, image rotation/opacity, background exclusion and
   loaded online tiles with readable credit; missing tiles must not trigger export downloads.
   Cancel at both dialogs and confirm no project/camera/History/save changes. *Owner:*

## H — Desktop automation on Windows

1. **D335/D336 desktop automation** (listed 2026-10-25,
   [DESKTOP_AUTOMATION](../reference/DESKTOP_AUTOMATION.md)). On a newly packaged build, run its
   PowerShell workflow: control the visible editor, watch playback, patch and Undo a Link,
   save/reopen and capture a screenshot. Is a duplicate endpoint refused, and do Unicode file
   paths work? From a second Windows account, does a pipe of the same name give
   `UNTRUSTED_SERVER`? *Owner:*
2. **D344–D350 AI control** (listed 2026-10-26). Enable and disable the live endpoint, reconnect;
   guarded New and Quit, with the full reply read before the normal exit; after a crash, deferred
   recovery list/restore/discard; map set/status/retry with real provider access. Do live-owner
   drafts and unsaved work survive every refusal, and are focus/menu/pointer busy reasons right?
   An AI disconnect must stay distinct from quit or cancel. Linux offscreen is not Windows
   evidence. *Owner:*
