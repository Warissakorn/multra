# PROGRESS log archive — 2026-09-25 to 2026-09-25

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning. Each section keeps its original file name as anchor.

<a id="progress-2026-09-25-m3.2.7a-b"></a>
## 2026-09-25 — M3.2.7a/b: the T-junction evidence (D66)

### 2026-09-25 — M3.2.7a/b: the T-junction evidence (D66)

M3.2.7 was split:
- **a** — the fixture and the controlled cases;
- **b** — the diagnostic sweep;
- **c** (ROADMAP) — the signal-composition variant and the owner exercise. The exercise needs the
  owner and a Windows build, so it is pending and never inferred.

**Fixture** (`tools/t_junction_network.hpp`), built only through the editor's commands, as the four-leg
fixture is. The committed file is `data/projects/t-junction-priority.traffic.json`, and
`tjunction.committed_file_is_the_builder_output` keeps the two identical.
- **Layout:** a two-way major road with a 1 m median, and one minor approach. The minor road's far
  turn crosses the near major stream (`addCrossingAreas`), then merges into the far one
  (`takeOverMergesOf`, with the minor side yielding). Its near turn only merges.
  - Yield or Stop is set at the lines where the minor road first gives way.
  - The queue counter measures at both of those lines.
- **Variants:** right- and left-hand (a mirror in y, in which the crossing movement is the minor
  RIGHT turn, as the header says), Yield and Stop, and a blocked exit (a permanently red head on the
  far lane past the merge).
- **Demand:** round numbers, with inputs stopping at 900 s in a 1500 s run so demand drains.
- **Geometry fixes found while building it:** the first drawing had no median. The turn's lane then
  touched the near stream right up to its end, so the crossing area ran into the merge. A 1 m median
  and a 20 m join fixed it.

**What the fixture shows** (`tjunction.*`):
- One crossing zone and two merge zones in every variant. On the crossing turn the merge begins past
  the crossing's exit, against a different major stream.
- The sink clearance fits the heavy vehicle (12 m).
- Every variant runs and drains. The two streams are never inside the crossing at once, and the
  major road has **no** safety clamp.
- The mirror gives identical movement and queue rows on the same seed.
- Stop delays both minor movements more than Yield, and leaves the major count unchanged.
- **Blocked exit:** the crossing turn never enters, and eastbound completes exactly as many trips as
  with the exit open. Two guards hold the turn, each enough alone:
  - the crossing and the merge are one chain (the 6.8 m between them is less than the heavy
    vehicle's waiting room);
  - the receiving-space rule.

  Disabling either alone passes the test; disabling both fails it.

**Controlled cases** (`tjunction_controlled.*`, test-owned states on the compiled fixture, no demand):
- **gapTime 4/5/6 s**, with the major 5 s from entry (§2's example): admit, admit, deny, on both
  driving sides.
- **Headway 7 m ± 0.01 m** with time blocking inactive: block, block, pass. **Equality blocks,**
  as in A09.
- **Movement level:** the clearance margin is asserted first (3.8 s from standstill against 5 s).
  The bounds were stated before running:
  - permitted admission within 1.6 s — observed 0.7 s;
  - denied admission no earlier than the major's rear leaving the area, which the run's own trace
    shows at 6.1 s, and within 10 s of the predicted 6.10 s — observed 6.8 s.

  The denied trip arrives later.
- **Mutations**, each failing a named test: the gap `<` → `<=`, the headway `<=` → `<`, and Stop
  service removed.

**Found, not fixed — minor-road safety clamps.** A waiting line closes the tick a major vehicle
enters the gap-time window. A minor driver already too close to stop is then clamped: it arrives at
about 10 m/s, 0.2 m short of the line. There is no commitment or amber rule for a waiting line.
- Seen as 4 clamps in the Yield base run (seed 42), all on the minor approach. The tests assert
  that the major road has none; they do not freeze the minor count.
- This is M3.2.3's admission model, not the fixture, so it is carved into the M3.2.8 ROADMAP row
  rather than tuned away here.

**Sweep metadata first.** `tools/t_junction_sweep.*` records:
- duration, time step, inputs, seeds and rules;
- FNV-1a hashes (carriage returns dropped) of the project file and every catalog the compile
  reads;
- the build.

It was committed as `docs/evidence/m3.2.7-sweep-metadata.json` **before** the sweep ran.
`tjunction.the_archived_sweep_metadata_still_describes_the_fixture` fails if the fixture or a
catalog changes under it.

**Sweep** (b, `docs/evidence/m3.2.7-sweep.*`, 20 runs, run after the metadata commit `b66313b`):
- Every run drained.
- Minor delay and queue rose with gapTime 3 → 5 → 7 s for every seed; the spread between seeds is
  wide.
- **headway 3/7/12 m gave identical rows.** Investigated: no major vehicle within 12 m of an entry
  was ever slower than 10 m/s, so the gap-time window always covered the headway and headway never
  decided a block. The arm exercises nothing at this demand, and a congested variant is left to
  M3.2.7c.
- All 60 clamps in the gap arm were minor vehicles at a waiting line: the carved mechanism.

<a id="progress-2026-09-25-m3.2.6c-counters-editor"></a>
## 2026-09-25, M3.2.6c

### 2026-09-25 — M3.2.6c: queue counters in the editor (D65)

M3.2.6c closes M3.2.6. Counters are now authored, listed, drawn and deleted in the editor.
- **Queue counter tool** (`Q`, `Tool::counter`). Each click adds one line to a draft:
  - a click on a stop line adds a reference to that head;
  - a click on a waiting line adds a reference to that line;
  - anywhere else on a **Link** lane, it adds an explicit point, through the new
    `laneControlPoint`. That function maps the lane-polyline station `nearestHeadSlot` picks onto
    the reference station a `ControlPoint` stores.

  Keys:
  - Enter commits the draft as one counter, one Undo step.
  - Backspace drops the last line; Esc drops the draft.
  - A head clicked twice is one line.
  - A Connector path, or empty ground, is refused.
- **Queue counters tab** (`src/shell/editor_counters.cpp`, index 10), with columns Id, Name, Lines,
  Replaces approach. Its actions:
  - *Add queue counter* over the heads in the selection, which is the keyboard route: select heads
    in the Signal heads table, then add;
  - *Edit* (Name) from Enter or a double-click;
  - *Delete*, which brings the derived row back.
  All of them go through `execute`.
- **Marks:** counter lines are violet dotted bars above stop and waiting lines, taken from the same
  helpers that draw or resolve their objects (`headBar`, `waitingLineBar`).
- **One source for the tab and the report:** `queueRowName` and `replacedApproaches`
  (`project/evaluation.hpp`). `evaluationSpec` now suppresses a derived row through
  `replacedApproaches`.
  - The four-leg and M2.6 reports are **byte-identical** to the previous commit's CLI, and so is a
    project that carries an authored counter.
  - Disconnecting the suppression fails the UI suite.

Tests:
- Headless: the lane click lands on the clicked place on a curve (both sides; lane and reference
  stations differ by more than 0.1 m there), and the tab's helpers agree with the report.
- `queue-counter-ui` (offscreen) on the four-leg template:
  - Esc and a refused click, then three stop lines and a lane place committed with Enter; Undo and
    Redo.
  - Rename through the dialog; the Results tab shows "Counted" in place of the pocket's row, with
    the same row count.
  - Keyboard add, Delete, then Undo of each.
  - Deleting the counter restores the derived rows.
  - Thai headers; Save and reopen.
- Three mutations are caught by this suite: Enter not committing, stop lines not recognised, and
  suppression disconnected.

Other changes:
- The suite's `require` prints before it throws. A failing assertion otherwise aborted in
  `~EditorWindow` (`map::at` while a window with unsaved edits closes during unwind), which hid the
  message. That behaviour is older than this change and is only seen on a failure path.
- Oldest entries (M3.2.2b, M3.2.2c) moved to `docs/archive/` to keep this file under 500 lines.

Test runs: Linux headless 28/28, desktop 45/45 offscreen. Seed 42 is unchanged. Not run on Windows.

<a id="progress-2026-09-25-m3.2.6a-b"></a>
## 2026-09-25, M3.2.6a/b

### 2026-09-25 — M3.2.6a/b: signal positions and queue counters (D64)

M3.2.6 was split: **a** signal positions (A21), **b** queue counters (A22, A23), **c** counter
authoring in the editor (ROADMAP row).

**M3.2.6a** (`tests/signal_position_tests.cpp`): on both driving sides, heads before, exactly on and
after a section cut, on every lane of a three-lane Link, through stretch, split (before, through
and after the head) and copy. Each test checks that three readings of the place agree:
- the command's station;
- the canvas slot (`headSlot`);
- the runtime section point (`rebaseHead`).

A red head past a cut holds traffic at its compiled position. **Nothing needed fixing**: the
behaviour was already right and only unmeasured. Mutating `sectionForStation` so a head on the cut
resolves downstream fails the new test, and an existing merge test.

**M3.2.6b**:
- **Model:** `AuthoredQueueCounter{id, name, lines}` in `Network::queueCounters`, schema 16. Each
  `MeasurementLine` is a head or waiting-line reference, or an explicit `ControlPoint`, never both.
  The fixtures changed only `schemaVersion`.
- **Evaluation:** a counter is now a set of `CounterLine{segmentId, position}`, one mechanism for
  every counter. Derived counters take each head's compiled place. The four-leg and M2.6 template
  CLI reports are **byte-identical** before and after.
- **Resolution:** authored counters resolve through the new `locateControlPoint`. When an authored
  counter measures any of a Link's heads, that Link's derived row is dropped, so there is one row
  per approach (A23).
- **Lifecycle:** `pruneQueueCounters` runs wherever a head, waiting line, Link or Connector is
  removed. A split remaps an explicit point and refuses one on the cut. A lane a counter measures
  on cannot be removed.
- **Copying a Link does not copy its counters**, a deviation from the plan. A copied approach
  would bring a second row with the same name; demand is not copied for the same reason.

Test runs: Linux headless 28/28 (302 unit tests), desktop 44/44 offscreen. Seed 42 is unchanged.

<a id="progress-2026-09-25-m3.2.5b"></a>
## 2026-09-25, M3.2.5b

### 2026-09-25 — M3.2.5b: Stop/Yield in the editor; one waiting line per lane (D63)

**The editor:**
- **Conflict dialog:** a "Control at the waiting line" field — None / Yield / Stop. It is disabled
  while the priority is Undetermined, and is submitted in the same step as the rest of the dialog.
- **Conflict table:** a Control column, the eighth, appended so earlier column indices keep their
  meaning.
- **Canvas:** a waiting line is dashed amber with no control, solid amber for Yield, and solid red
  and heavier for Stop. Item data 2 carries the mode for tests. There is no text on the canvas,
  because it has no locale.

**Owner decision, the open question from M3.2.4b:** `addCrossingAreas` now puts **one line per
lane**, 1 m before the first area that lane meets, shared by all of that lane's areas.
- **Why:** a line per area put the far lane's line inside the near lane's area, so a Stop would
  have halted vehicles mid-crossing.
- **What changes for admission:** nothing. The chain rule already admits the areas behind one line
  together (A15).
- **Scope:** new gestures only. Stored documents are untouched.

With shared lines, `setAreaControl` now sets the control for **every** decided area giving way at
that line. A line cannot be Stop for one area and uncontrolled for the next, which is the D62
rule. A priority flip takes only that area off its old line's control.

**Evidence:**
- `rightofway_editor`:
  - three lines for a 2×1 crossing;
  - B's shared line sits before the first area, with the forcing that the two areas meet B
    1 m and more apart.
- `stop_control_model`: on a two-lane crossing under Stop, more than 20 minor vehicles each stood
  at the shared line.
- `priority-canvas`:
  - Stop set from the dialog, then listed, drawn, undone and redone;
  - the field disabled for Undetermined;
  - the control survives Save and reopen.
- Three mutations each fail their own test:
  - the dialog not submitting the control;
  - the canvas ignoring it;
  - lines back at the last area.

Test runs: Linux headless 28/28 (290 unit tests), desktop 44/44 offscreen. Seed 42 is unchanged.

<a id="progress-2026-09-25-m3.2.5a"></a>
## 2026-09-25 — M3.2.5a: Stop and Yield at the waiting line (D62)

Moved whole from `docs/PROGRESS.md`; the decision log row stays there.

### 2026-09-25 — M3.2.5a: Stop and Yield at the waiting line (D62)

M3.2.5 was split: **a** is the runtime and the model; **b** is the editor (ROADMAP row).

**Core** (`src/core/conflicts.*`, `simulation.cpp`):
- `ConflictZone::control` is `yield` or `stop`. Yield is the existing admission test, unchanged.
- A Stop holds each minor vehicle at its line until it has served it.
- `refreshStops` keeps `SimState::stopService`, at most one entry per vehicle for its next Stop
  line. The entry is carried while it is the same line and made when the vehicle has come to the
  line.

"Come to the line" had to be defined, because the reduced car-following model never reaches zero
behind a line. Measured: 0.04 m/s after 29 s, still creeping. So:
- **the line:** below `kStoppedSpeed` (0.1 m/s), with the front within `stopLineReach`, the gap the
  model keeps behind a standing obstacle at that pace plus 0.1 m;
- **the stop:** the Stop itself brings the vehicle to rest for that tick and one whole tick more.
  This is at most 1 m/s² of ordinary braking, published without a safety-clamp event.

A red head at the same line keeps its own hold, and green removes only that. Nothing composes by
special case: both are holds and the minimum applies.

**Cost:** `stepSimulation` measures 51.39M instructions against 51.24M at the base commit
(callgrind, `trafficsim-engine-benchmark 12 60`, Release). The first version cost +2.2%:
- **+1.02M:** GCC stopped inlining `vector<Vehicle>::push_back` in the grown function. Publishing
  with `push_back(std::move(...))` got it back.
- **+0.5M:** per-vehicle branches. They were moved out of the hot loop, behind `stopZones`.

**Model:**
- `StopControl{id, name, waitingLineId, mode, conflictAreaIds}` (contract §1), in schema 15. It is
  written only when present, so the two fixtures changed only `schemaVersion`, regenerated by
  their tools.
- **Structural issues:**
  - `DUPLICATE_STOP_CONTROL`: a second control on a line, or an area named twice;
  - `INVALID_STOP_CONTROL`: no area;
  - `UNKNOWN_*`.
- **Runtime:** `CONFLICT_STOP_LINE_MISMATCH` when an area does not give way at the control's line.
- The resolver marks the Stop areas' zones.

**Commands:**
- `putStopControl`, `deleteStopControl`.
- `setAreaControl(area, stop | yield | none)`: the mode belongs to the line, so it applies to every
  area the line controls.
- Changing an area's priority clears its control, because it now waits at another line.

**Cascades:**
- Deleting areas or roads prunes controls; `deleteWaitingLine` refuses a controlled line.
- Duplicating copies a control with its line and copied areas.
- A Problems row naming a control opens its area.

**A test slip worth remembering:** a vehicle's `routeIndex` indexes the *canonical* scenario the
state holds (sorted by id), not the scenario passed in. "route-10" sorts before "route-9", and the
first version of the seam test followed major vehicles.

Test runs: Linux headless 28/28 (289 unit tests), desktop 44/44 offscreen. Seed 42 is unchanged.

<a id="progress-2026-09-25-m3.2.4b"></a>
## 2026-09-25 — M3.2.4b: conflict areas on the canvas (D61)

Moved whole from `docs/PROGRESS.md`; the decision log row stays there.

### 2026-09-25 — M3.2.4b: conflict areas on the canvas (D61)

The interaction was written first (`VISSIM_PARITY.md` §2b). Vissim picks conflict areas only
while *Conflict Areas* is the active object type; here the **Conflict area tool** (`A`) is that
type. Only this tool hit-tests areas, so a click at a junction under Select still picks the Link.
With the tool:
- a click selects the area's row;
- a click on the highlighted area, or `P`, cycles its priority: firstYields → secondYields →
  undetermined;
- a drag on a waiting line slides it along its own path;
- `Tab` picks the next area under the last click.

New code:
- Commands (`src/commands/conflict_authoring.cpp`): `cycleConflictPriority`, which keeps the
  name and the rule's numbers, and `moveWaitingLine`.
- Model: `controlPathPolyline`, the polyline a station is measured on. A drag projects onto
  exactly what `waitingLineBar` and the resolver read.
- Canvas (`src/editor/canvas_conflicts.cpp`): `conflictsAt`, `waitingLineAt`, the press and the
  drag, following the head drag. A line wins over an area.
- Readability: the side that gives way (the second while undetermined) is hatched and drawn
  above the solid side, so both show where a crossing's two sides cover the same square.

Evidence: `rightofway_editor.*` (2 new tests) and a new `priority-canvas` suite. It covers:
- Select still picks the Link, with the forcing that the point is inside an area;
- pick, click-cycle and `P`, each one Undo step, and a click on empty space;
- hatch and z order;
- a 5 m line drag along its lane with sideways drift, one step, and a jitter that moves nothing;
- Save with no dialog, then reopen: the same `rightOfWay`, rows, drawn items and "Runs".

Four mutations each fail their own assertion:
- Select hitting areas;
- no hatch;
- an area winning over the line;
- no commit on release.

`Tab` among overlapping areas has no test. Crossing areas never overlap, so it needs a taken-over
merge.

**Seen, not changed:** on a two-lane crossing, the minor side's waiting line for the far lane's
area stands inside the near lane's area. `kCrossingSetback` is measured from each area's own
entry. The resolver chains the two (A15) and the areas run, as the reopen check shows. Whether
the gesture should set the line before the first area is a question for M3.2.5's Stop/Yield
work.

Test runs: Linux headless 28/28, desktop 44/44 offscreen. Windows is CI's. Seed 42 is unchanged
(`1243e5361a7174d1ecc56b67d7c92b12`).

<a id="progress-2026-09-25-m3.2.4a"></a>
## 2026-09-25 — M3.2.4a: the Conflict areas tab (D60)

Moved whole from `docs/PROGRESS.md`; the decision log row stays there.

### 2026-09-25 — M3.2.4a: the Conflict areas tab (D60)

New commands in `src/commands/conflict_authoring.cpp`:
- `addCrossingAreas`: one area per crossing lane pair, extents from `surfaceOverlap`, lines 1 m
  short of entry, rule from the catalog.
- `setConflictControl`, `takeOverMergesOf`, `restoreAutomaticPriorityOf`, `removeConflictArea`.

Model helpers:
- `conflictSideOutline` and `waitingLineBar` draw from the resolver's own strips.
- `mergeSectionsOf` and `mergeSectionOfArea` name a merge by its section.

Shell (`src/shell/editor_priority.cpp`), the tenth objects tab:
- toolbar actions; Enter or double-click opens the dialog;
- a "Run status" column from the resolver;
- row selection highlights the area on the canvas (`src/editor/canvas_conflicts.cpp`);
- Problems rows for right-of-way objects open the tab;
- an `editorRunProtection` note shows with every run.

The validation banner no longer says "no conflict resolution", which stopped being true at
M3.2.3a. It now says conflicts are resolved only where authored, and the not-yet-validated marker
stays. All 49 right-of-way codes have en and th text; a test scrapes them from the sources.

Test runs: Linux headless 28/28, desktop 43/43 offscreen (new `priority-ui` suite). Seed 42 is
unchanged. A screenshot checked the drawing.

<a id="progress-2026-09-25-m3.2.3c"></a>
## M3.2.3c (2026-09-25)

Moved whole from `docs/PROGRESS.md`; the decision log row stays there.

### 2026-09-25 — M3.2.3c: shared receiving space, merges on the solver, hold cycles (D59)

Phase 2 of `stepSimulation` is now `resolveRequests` (`src/core/conflicts.*`). Besides the swept
check, it serves requests crossing their lines behind one standing leader in vehicle-id order, and
caps any the room cannot hold. The leader's id comes from `closestVehicle` through an optional
out-parameter, so `Leader` itself is unchanged. An earlier version that added the id to `Leader`
cost +1.5% of `stepSimulation`; this one measures 51.20M against 51.29M before.

Core validation:
- The route check `CONFLICT_MIXED_ROLES` is replaced by `CONFLICT_HOLD_CYCLE`, a cycle in the
  waits-for graph between zones.
- A zone ending on two feeding segments arbitrates a merge, as a rule does.

The resolver compiles each complete authored merge group to zones instead of `right-of-way/` rules.

Tests changed deliberately, from authored rules to zones: `rightofway.a06`, `a07`, the lane-two
curved take-over, the short-section take-over, three `rightofway_resolution` tests and the
lifecycle `clean` helper. The D58 replay-equality test no longer holds, by design, and is replaced:
a take-over now carries the fallback's line and numbers but also holds the major side.

New tests are in `conflict_chain.*` (3) and `rightofway_runtime.*` (a three-way merge). Disabling
the room reservation or the cycle check each fails its own test. Test runs: Linux headless 28/28
and desktop 42/42 offscreen. Seed 42 is unchanged.

<a id="progress-2026-09-25-m3.2.3b"></a>
## M3.2.3b (2026-09-25)

Moved whole from `docs/PROGRESS.md`; the decision log row stays there.

### 2026-09-25 — M3.2.3b: spans, atomic chains, authored merges run (D58)

`ZoneSide` is now a chain of segment ids. `zoneIncidence` (`src/core/conflicts.*`) resolves each
route's zones once, in route distances (`entryAt`, `exitAt`, `waitAt`, `clearAt`); the run index
and validation share it. The admission functions now read those instead of `partStart`.

The resolver:
- builds each side's chain (`sideChain`);
- drops `UNSUPPORTED_CONFLICT_SPAN` and `UNSUPPORTED_CONFLICT_GROUP`;
- drops `UNSUPPORTED_CONFLICT_RUNTIME` from merge areas, so no code emits it any more.

New core refusals are `CONFLICT_ROUTE_JOINS_INSIDE` and `CONFLICT_MIXED_ROLES`.

Tests changed deliberately, because they pinned the blockers this slice lifts: `rightofway.a04`,
`rightofway.a06`, the `clean`/`noIssues` helpers, and the M3.2.3a refusal test. The last is now
three run tests.

New tests: `tests/conflict_chain_tests.cpp` (4) and `rightofway_runtime.*` (5 in all). Disabling
chaining fails the A15 test; disabling chain-following fails the span test. A taken-over body
merge replays the untouched fallback's seed-42 event stream exactly, and reversing it changes the
stream, so the rule binds. No-zone cost is unchanged: `stepSimulation` measures 51.29M
instructions, as at M3.2.3a. Test runs: Linux headless 28/28 and desktop 42/42 offscreen. Seed 42
is unchanged.

<a id="progress-2026-09-25-m3.2.3a"></a>
## M3.2.3a (2026-09-25)

Moved whole from `docs/PROGRESS.md`; the decision log row (D57) stays there.

### 2026-09-25 — M3.2.3a: one authored crossing runs (D57)

The core gained a `ConflictZone` (`src/core/types.hpp`) and an admission module
(`src/core/conflicts.*`). An authored crossing area with nothing reported against it now
compiles to a zone (`resolveRightOfWay(...).zones`) and runs. It no longer carries
`UNSUPPORTED_CONFLICT_RUNTIME`.

`stepSimulation` now runs in three phases:
1. Compute every candidate move from the snapshot.
2. Cap any minor request whose line crossing coincides with a major front reaching the area in the
   same tick.
3. Publish in vehicle order.

Without zones the arithmetic and event order are the old ones: seed 42 is byte-identical and the
four TS baselines pass.

Named blockers for what this slice does not run:
- `UNSUPPORTED_CONFLICT_SPAN`: an area over a section cut.
- `UNSUPPORTED_CONFLICT_GROUP`: a segment in two areas.
- Merge areas keep `UNSUPPORTED_CONFLICT_RUNTIME`.

Core validation adds `CONFLICT_SINK_TOO_CLOSE` and `CONFLICT_ROUTE_STARTS_PAST_LINE`.

Tests: `tests/conflict_zone_tests.cpp` (11) and `tests/right_of_way_runtime_tests.cpp` (3). With
admission disabled (holds and swept caps stubbed out) six core tests and the gap-time response
test fail. With only the swept check disabled, its own test fails. On the model crossing, the
minor road's mean travel time rises with the rule's gap time, 1 s against 6 s. Test runs: Linux
headless 28/28 and desktop 42/42 offscreen. Seed 42 is unchanged.

Cost with no zones: callgrind on `trafficsim-engine-benchmark 12 60` (Release) puts
`stepSimulation` at 51.29M instructions against 49.29M before, +4.1%. That is the three-phase
split. Wall clock is 0.36–0.37 µs per vehicle-tick against 0.35–0.37, inside the clock's spread.
Two measured attempts to win it back:
- Skipping the zone call for routes with no zone recovered 2.7 points and was kept.
- Publishing inline when there is no zone, through a lambda, cost more (52.05M) and was reverted.
