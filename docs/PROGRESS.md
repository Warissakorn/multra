# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-12 — M6 tolerance set (D516); `EditorWindow` steps 2–6 (D517–D524)

- **D516:** owner answered B47 with (a): S1 ±5 %, S2 ±15 %, U ±15 % of the reference, on the mean
  over seeds 42–81, CI shown beside it; U's `gapTime = t_c − τ` confirmed. Recorded in the option
  sheet's §6, ROADMAP M6.0 and OWNER_SITTING. No run made, no gate closed.
- **D517:** the single run's state is `RunSession` (`editor_run_state.hpp`); the three run getters
  moved out of line; header 488 → 469 lines; no behaviour change.
- **D518:** recovery/autosave state is `RecoverySession` (`editor_recovery_state.hpp`); header
  469 → 466 lines; no behaviour change.
- **D519:** automation state is `AutomationHost` (`editor_automation_state.hpp`); header 466 → 460
  lines; no behaviour change.
- **D520:** the thirteen object tabs' table/revision pairs are one `ObjectTab` each
  (`object_tab.hpp`); header 460 → 455 lines; no behaviour change.
- **D521:** object tabs become panels with an `ObjectTabContext`; Study periods
  (`StudyPeriodsPanel`), then Modes (`ModesPanel`) OD
  matrices (`OdPanel`), speed decisions, reduced-speed areas, queue counters,
  travel-time sections, Link types,
  Zones, Cross-sections and Street segments (on the `ObjectTabPanel` base).
- **D522:** a selection hub (`ObjectSelection`) the panels subscribe to, after a look at how Vissim,
  Aimsun and SUMO share one selection across views; the counter and section panels follow a
  cleared canvas selection through it.
- **D523:** a written contract ([OBJECT_PANELS_NODES_CONFLICTS](plans/OBJECT_PANELS_NODES_CONFLICTS.md)), then
  Nodes (`NodesPanel`, hooks `NodesHooks`) and Conflict areas (`ConflictsPanel`, `ConflictsHooks`, with
  the automatic areas) as panels: all 13 object tabs are panels; header 412 → 398 lines.
- **D524:** the Macro and Meso runs' functions (build, start, discard, refresh) moved onto `MacroPanel`
  and `MesoPanel` (`macro_panel.*`, `meso_panel.*`), with `MacroHooks`/`MesoHooks`; the window keeps
  the side files and read-only getters; header 398 → 396 lines.
- **Verified on Linux:** desktop build, ctest 321/321 after each step; `check` after D517–D520.

## 2026-11-12 — D505's approved items: streams slice 4, pocket example, EditorWindow step 1, M6 §6 (D512–D515)

- **D512 (INPUT_STREAMS §8 slice 4, choice 2):** W74 traits and red runners hash
  `traitIdentity(id, origin, ordinal)`: the id without an origin key (all of `shared`, hand-placed
  vehicles), the input's or lot's key and ordinal otherwise. `samplePending` sets the key before
  hashing. Rows IS10–IS11, forced under `shared` and by putting the id back (both fail then).
- **D513:** `signalised-node-analysis` uses `signal-4-p3-major-far-pockets` (45 m storage, 15 m
  taper on the major): 10 lane groups, highest ratio 0.79, none F; greens and volumes unchanged.
  `nodeexample` and `example-menu-ui` count 10.
- **D514 (`EditorWindow` step 1):** the batch/compare worker's state and widgets are
  `BatchSession` (`editor_batch_state.hpp`); header 500 → 488 lines; no behaviour change.
- **D515:** the M6 option sheet's §6 holds the run protocol for D505's sources; book values marked
  *to read*. Tolerance and U's `t_c` translation: OWNER_SITTING B47. No comparable run was made.
- **Verified on Linux (Qt 6.4, GCC 13):** desktop build, full ctest 321/321 and `check` (docs,
  file sizes, architecture). **Not done:** headless build; a callgrind measure of slice 4; a
  separate byte comparison of `shared` outputs against the previous build (the suite's frozen
  baselines and exact CLI rows are the evidence).

## 2026-11-12 — Random streams per input in the project (D510); NEXT and OWNER_SITTING slimmed

- **D510 (INPUT_STREAMS §8 slice 3):** `definition.randomStreams` at schema 74
  (`random_streams_codec.*`), written only as `perInput`, so a file that keeps the single stream
  keeps its version and bytes; older files naming the key are refused. `newProjectDocument` sets
  `perInput` (D505 1 b); shipped projects are not regenerated. *Run settings → Random streams*
  lists `shared`/`perInput`, one Undo step. The mapping rides on `MovementReport`, so run, batch and
  comparison JSON/CSV state `perInput` (nothing under `shared`, IS1), `aggregate` refuses mixed
  runs, `compareBatches` mixed batches, and `requireSameRandomStreams` two projects before any run
  (CLI `--compare`, desktop Compare page, new locale key `editorCompareStreams`).
- **Rows:** `inputstreamsproject` (codec, schema floor, Undo, IS8 output and refusals, each output
  row forced by first asserting the two batches ran different mappings) and a Run settings step in
  `study-period-ui`. `cooldown`'s new-project schema expectation moves 27 → 74.
- **Docs pass (optimize-program, docs dimension):** NEXT held 32 lines of shipped work against its
  own rule (D319) and owner checks that OWNER_SITTING lacked. Finished slices are one-line pointers;
  OWNER_SITTING split at its 500-line limit (sections C–I to `plans/OWNER_SITTING_WINDOWS.md`,
  item names unchanged) and gained B46, C14–C16, E35, E36; the container build note moved to
  BUILDING. NEXT ≈6,400 → ≈4,450 tokens (chars ÷ 4), 349 → 250 lines.
- **Build pass (D511):** a schema step dirtied 542 objects through `document.hpp`; `kLatestSchema`
  has its own header and a step now dirties 35 (66 s). A signal head's look dirtied 181 through
  `canvas.hpp`; `road_crossbar_geometry.hpp` holds what the canvas needs, and it dirties 10 (13 s).
  The UI executables' reused PCH was refused in the shared build (85 warnings); `ui_pch.cmake`
  gives them one, 986 → 852 CPU-s. `check` 321/321 after each step.
- **Verified on Linux:** desktop build, `check` (321/321, docs, file sizes, architecture), and the
  real CLI: a `perInput` copy of the T-junction writes `"randomStreams": "perInput"`, the shipped
  file writes none, and `--compare` between them is refused. Headless not rebuilt this session.

## 2026-11-11 — Route marks: bright ends, a tint between them, hover previews (D509)

- **D509d (follow-up, 2026-11-12):** owner: "เปลี่ยนระยะ offset เป็น 0.6m". The tint's inset is
  0.6 m (was D509c's 0.8 m), still at most a quarter of the narrowest lane. Images:
  `docs/images/route-tint-d509d/` (`before.png` is D509c).
- **D509c (follow-up, 2026-11-12):** owner: "ตกลงทั้ง 2 ข้อ" to a 50 % yellow and a wider inset.
  The tint is 50 % stored and 35 % preview (2.25:1 on the road; lane marks 2.3:1 through it), and
  its own inset is 0.8 m, at most a quarter of the narrowest lane (was the conflict area's 0.5 m and
  a fifth), so a grey edge shows between it and the selection outline. Only the selected route is
  tinted, as before. Images: `docs/images/route-tint-d509c/` (`before.png` is D509b).
- **D509b (follow-up, 2026-11-12):** owner: "สีเหลืองเพิ่มความทึบ" after the violet, yellow and white
  tints were compared on the road. `routeTintHex` is `#FACC15`, 59 % stored and 43 % preview (was
  violet at 39 % and 27 %): 2.5:1 on the road `#5A5D61` against violet's 1.2:1, and the lane marks
  still read through it at 2.1:1. White was not taken (it greys the road and is the lane-mark and
  car colour). Images: `docs/images/route-tint-d509b/` (`before.png` is the violet tint).
- **D509a (follow-up):** owner: "บาร์ปลายทางยังมีกรอบสีขาวอยู่". The moving bars lost their Base
  casing too; no route end has a white frame now.

- **Owner:** the destination bar should show while pointing; the tint should not look like the
  selection blue and should be inset like a conflict area; the tint past the destination is cut;
  the end colours were too dark (measured 1.0:1 on the road) and option 3 (brighter canvas tones)
  was chosen.
- **D509:** `canvas_route_overlay.cpp` (moved out of `canvas_demand.cpp`) draws a route's violet
  tint between its green origin and red destination bars, inset 0.5 m (at most a fifth of the
  narrowest lane) from the road's outer edges once the Link/Connector seams are closed. Hover sets
  `routeHover_`: before a route starts a green bar shows the origin; with a draft the red bar and
  the tint's end follow the pointer, re-cut in place (`retintRoutePreview`) along one road. New
  tokens `routeStartHex`, `routeEndHex`, `routeTintHex`; the tint colour was my recommendation
  and is one token to change.
- **Images:** `docs/images/route-tint-d509/` (hover origin, hover destination, stored route, the
  inset at a Link/Connector join; `before.png` is D508's stored route).
- **Rows:** `station-routing-ui` hovers the origin and destination bars, checks their colours, that
  the tint item moves in place and stops at the hovered bar.

## 2026-11-11 — Ctrl+right previews the route's ends; thin end bars (D508)

- **Owner:** a correct preview of the origin and destination while Ctrl + the button is held, movable
  until release; and end bars shaped like the other canvas objects. Chose the inputs' thin bar and
  an origin at the release point.
- **D508:** `EditorCanvas::applyRoutePress` re-applies a press snapshot on each move, so the green
  origin or red destination bar sits where the release will put it; `finishRoutePress` places it.
  An origin mid-Link opens or joins that Link's routing decision at the station (within 10 px of
  the start it stays a whole-Link route); a destination near its far end keeps the boundary marker.
  Route ends lose D435's width and casing, sit above a placed decision's bar, and a draft that is
  only its origin no longer shows a red bar at the origin's far end.
- **Images:** `docs/images/route-press-d508/` (held origin, held destination, the stored route, and
  the earlier wide ends for comparison).
- **Rows:** `station-routing-ui` holds, slides and releases an origin and a destination and checks
  the stored station, destination and thin bar; `demand-ui` presses its whole-Link route at the
  Link's first metre.

## 2026-11-11 — Pick the origin once, then several destinations (D507)

- **Owner:** "ต้องการให้เลือกต้นทางครั้งเดียว และเลือกปลายทางได้หลายอัน" (pick the origin once, then
  several destinations).
- **D507:** after the Route tool saves a traced destination, `EditorCanvas::commitRouteDraft` keeps
  the start object and station, so the next click is another destination from the same origin and
  joins the same routing decision. A click back on the origin's line does nothing; the same
  destination by the same roads is refused (`EDIT_ROUTE_DUPLICATE`, en/th); Esc or Backspace on the
  bare origin ends it. The status hint says the origin stays. `Ctrl`+right/`Enter` is unchanged.
- **Rows:** `station-routing-ui` now draws two destinations from one origin click and checks the
  no-op click, the refused duplicate, Esc and Backspace; `road-crossbar-ui` expects the bare origin
  after a commit. Verified: Linux, desktop ctest 312/312 and `check`; Windows is the owner's.

## 2026-11-11 — Random streams per input in the engine (D506)

- **Owner:** "ทำตามคำแนะนำ" after D505: the recommendations, in their order. B9 and B43 needed no
  code; B44 (streams before the M6 runs) is the first that did. Branch protection and B45 stay the
  owner's; the D500 move and the `EditorWindow` split wait for their own sessions.
- **D506 ([INPUT_STREAMS](reference/INPUT_STREAMS.md) §4, §8 slices 1–2):** `Scenario::randomStreams`,
  `shared` by default. Under `perInput` each input and each lot draws from its own xorshift32 state
  seeded from the run seed and an FNV-1a key of its id; a routing draw is hashed from the seed, the
  vehicle's origin key and ordinal, and the decision's id (its id, not its index, so an added
  decision moves no other). A vehicle carries `streamKey` and `ordinal`; under `shared` they and
  the new state fields stay zero. A `perInput` checkpoint also writes the stream states.
- **Moved to slice 3:** the codec key and IS8's summary/CSV field and `--compare` refusal. No run
  outside the tests can be `perInput` until the project sets it, and an unconditional field would
  change every existing output's bytes.
- **Rows:** IS1–IS7 and IS6b (a lot's draws) in `inputstreams`. Each `perInput` row runs again
  under `shared` and expects the check to fail there, which is its failure-first proof.
- **Split:** `types.hpp` was at 500 lines; the event structs moved whole to `event_types.hpp`
  (no rebuild change measured: `types.hpp` includes it).
- **IS9 (callgrind, Release, `multra-engine-benchmark 12 300`, seed 42):** `shared` 909.8 M →
  914.0 M instructions (+0.46 %, the larger `PendingVehicle`), same 553 trips. `perInput` against
  `shared` per vehicle-tick: 2408 vs 2419 (−0.4 %); the two runs are different traffic (551 and 553
  trips), so the whole-run totals are not compared. The benchmark takes `shared|perInput`.
- **Verified:** Linux, GCC 13.3: headless ctest 208/208 and `check`; `multra-cli 42`, its event
  file and a four-leg 42–45 batch CSV byte-identical to the base commit. Desktop not built.
- **CI:** the Canonical archives stopped answering the Linux runners (`azure.archive` did not
  resolve; `archive.ubuntu.com` timed out on every package); the apt step hung ~47 min. apt's
  mirror list now tries kernel.org and MIT first, then the Canonical ones, security included, with
  short timeouts and a 15-min step limit (`native.yml`). Windows jobs passed throughout.

## 2026-11-11 — The owner's answers to the open questions (D505)

- **Owner:** asked for a recommendation on each open question, then took all of them. Documents
  only; no code, example or baseline changed, and no new work was started.
- **M6 benchmark sources (was B9):** Q4 (d); S1-a, S1-d second; S2-b below saturation, S2-a
  across it, S2-c first; U-a after U-c, U-e booked for R7. Sources only: no gate closes and the
  tolerance is still written before any comparable run ([ROADMAP](ROADMAP.md) M6.0).
- **Random streams (was B44):** `perInput` for new documents at a schema step, traits re-keyed by
  input, paired comparison as a later slice, before the M6 runs
  ([INPUT_STREAMS](reference/INPUT_STREAMS.md) §7).
- **Micro's M6 gate (was B43):** may close before M10 and M11.
- **Approved, not started:** the D500 example on the pocket template; `EditorWindow` split in
  small steps. **Waits:** `src/project/`. **Not wanted:** a start page; tool names at 1366 px.
- **Still the owner's:** B45 on Windows, and requiring `Native C++` on GitHub.
- **Verified:** Linux: headless `check`.

## 2026-11-11 — File → Open recent (D504)

- **Owner:** asked for the rest of the UI review's queue: recent files and a start page.
- **D504:** the last eight projects opened or saved, newest first, in `recent-files.json` in the
  application's local data folder (as `vehicle-colors.json` is), never in a project. Remembered
  on *Open*, on a save (`saveFile`, so an automation save too), on `--scenario` and when opened
  from the menu; not an example (it opens untitled) and not a recovery draft. A file that is gone
  is listed disabled; a damaged list reads as empty. An empty drawing's status line names the
  menu once the list holds a file (`recent_files.*`, free functions; no new window member).
- **Not built: the start page.** A panel over the empty canvas would sit where a drawing starts;
  its look is the owner's choice and is an Ask in NEXT. Both things it would offer are in File.
- **Tests:** new `recent-files-ui` (failed before the menu existed): order, no repeats, the cap,
  a missing file, an example leaves the list alone, a second window reads it, Thai, Clear list,
  a damaged file.
- **Verified:** Linux, GCC 13.3: desktop ctest and headless `check`. The folder on Windows and the
  menu's look there are the owner's (B45). Images (composited from the two menus):
  [en](images/open-recent-d504/open-recent-en.png), [th](images/open-recent-d504/open-recent-th.png).

## 2026-11-11 — The palette names its tools on a 1440 px window (D503)

- **Owner:** chose this from the UI review's remaining items: at 1440 × 900 the palette was 16
  icons with no names.
- **Cause:** the palette went icons-only automatically below 1500 px of window width, so a
  maximised window on a 1366 or 1440 px laptop never showed a tool's name.
- **D503:** the threshold is 1440 px, one constant (`namedPaletteMinWindowWidth`) in place of
  three literals. A person's own toggle still stops the automatic switch.
- **Why not lower (the plan said 1280):** with names, a 1366 px window leaves the canvas 810 px,
  and `ui-audit-layout` (D432's audit) requires 900 there; at 1280 that row and
  `properties-layout-ui` failed. They were left as they are and the threshold raised instead.
  At 1440 the canvas is 884 px with names (1066 with icons). 1366 px windows keep the rail.
- **Tests:** new `palette-width-ui` (failed at the old threshold: "Wrong palette view at 1440 px");
  `workspace-ui` reads the constant instead of the literal 1500.
- **Verified:** Linux, GCC 13.3, desktop ctest 318/318 and headless `check`. Thai captions fit at
  1440 ([en](images/palette-d503/palette-1440-en.png), [th](images/palette-d503/palette-1440-th.png)).
  Windows and high-DPI are the owner's (B45).

## 2026-11-10 — Process review: stated facts checked, ROADMAP tables, a user guide (D502)

- **Owner:** asked for the rest of the queue: the process review's three proposals.
- **`multra-check-docs` checks facts:** a decision id in any maintained document must be a row of a
  decision record (`D18` names the family D18a–D18d); no document cites a schema above
  `kLatestSchema`, read from `src/project/document.hpp`; `data/projects/README.md` lists exactly
  the shipped projects. Negative fixtures for each. On its first run it flagged nothing wrong in
  the documents, and then this session's own `D502` until the row existed.
- **ROADMAP:** M10, M11, M13 and M14 are one table row per slice (contract, implemented, open)
  with scope and done-condition above; their paragraphs are whole in
  [the archive](archive/ROADMAP-implemented-2026-11-10.md). To stay at 500 lines the M1 and M2
  implemented-slice tables and the S1–S5 stage lines went there too, M0.1 became a paragraph of
  M0, and M8's follow-up line joined its paragraph. No scope, gate or status changed.
- **[USER_GUIDE](USER_GUIDE.md):** the Micro sentence on the four-leg example in five steps, with
  where LOS letters and the HCM table come from. Every quoted control was checked against
  `data/locales/en.json`. English only, no pictures.
- **Not done:** milestone status statements are not checked against code (no mechanical source
  for "implemented"); branch protection is the owner's on GitHub.
- **Verified:** Linux, GCC 13.3: headless `check`; desktop `documentation` rows.

## 2026-11-10 — A routing decision reaches a pocket lane (D501)

- **Owner:** chose this defect, found by D500, as the next work.
- **Cause:** `routeLaneFamily` took the first lane path leaving a lane at a named Connector and
  stopped. Fill's pocket taper is 1 → 2: first path to the ordinary lane, second into the pocket.
  A route still reached the pocket by D459's joined chain and a lane change; a decision's family
  is built without joining, so every chain to the far turn was a stub, the family was empty and
  `entryDecision` skipped the destination with no issue.
- **D501:** without joining, a chain forks over every path leaving its lane, and each first-Link
  lane keeps its first full chain, else its first stub. With joining (routes, compile, Node
  membership) nothing changes. An entry decision's destination that no lane serves now adds the
  existing advisory `ROUTING_DECISION_UNREACHABLE`.
- **Tests:** `pocketdecision`, 4 rows on the P3 pocket template, both driving sides; three failed
  before the fix (a destination at 1/2 instead of 1/3, a turn with no vehicle, no advisory), the
  fourth pins that a route's family is as before.
- **Unchanged results:** CLI output of the eight shipped projects and `crossing.json` is
  byte-identical before and after (build metadata lines aside); no fixture or baseline moved.
- **Not done:** the D500 example stays on the template without pockets; moving it is an Ask in NEXT.
  A decision's destination that needs a lane change on a later Link with no fan-out is still not
  served; it is now named instead of dropped.
- **Verified:** Linux, GCC 13.3: desktop ctest and headless `check`.
