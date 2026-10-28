# VISSIM PARITY — how the network editor differs from the surface it is modelled on

The point of D1 is a tool that behaves like the modelling surface its audience already knows.
This file measures how far the native editor is from that, **feature by feature and gesture by
gesture**, so the gap is a list of decisions rather than a feeling.

**Which parts are current.** §1a and §2 are the current state (§1a measured by
`trafficsim-gesture-walkthrough`). The 2026-09-14 pre-completion §1, §3 and §6 described an editor
that no longer exists and moved to [the archive](../archive/VISSIM_PARITY-2026-09-14-sections.md).
Everything M1.1–M1.10 built is listed in [`../reference/NETWORK_EDITOR.md`](../reference/NETWORK_EDITOR.md).

**Owner correction (2026-09-15, M1.12):** Ctrl+click adds selection; Ctrl+drag already
selected objects duplicates them. Lane handles work on both sides without recentering existing
lanes. Outer road markings and internal dividers replace lane-centre dashes. Independent
Connector and Signal head copies require valid attachments.

**How to read it.** A gap is not automatically work. `../PROBLEM.md` owns scope and `../ROADMAP.md`
owns sequence; this file only tells the truth about the distance and proposes where each item
belongs. The gaps the owner accepted are carved into **M1.8, M1.9 and M1.10** in
[`../ROADMAP.md`](../ROADMAP.md) — anything else here is recorded, not booked.

This file describes authoring. It makes no claim about simulation fidelity; the
not-yet-validated marker (D5, hard rule 4) stands over everything below.

---

## 1a. The same rows, measured (2026-09-22, M1.27.3)

The archived §1 is the 2026-09-14 assessment. This is what `trafficsim-gesture-walkthrough`
observes on the current build: it replays each reflex against a real drawing and reports what
the **document** did, so a row is a measurement, not a reading of the code. Run it with
`QT_QPA_PLATFORM=offscreen trafficsim-gesture-walkthrough data`.

| Vissim reflex | Today | |
|---|---|---|
| `Ctrl`+right-drag on empty space | Draws a Link; the Link Data dialog takes lane count and widths | transfers |
| `Ctrl`+right-drag from a lane to a lane | Creates a Connector; the dialog defaults to the **whole carriageway**, so a 3-lane movement is one drag, not three | transfers |
| `Ctrl`+right-click on a Link | Inserts a curve point | transfers |
| `Alt`+left-drag on the selection | Rotates it (M1.22.2) | transfers |
| Drag the selection | Moves all of it (M1.16) | transfers |
| Right-drag | Pans | transfers |
| `Tab` | Cycled the objects under the pointer when measured; since `ed74268` (2026-09-29) plain `Tab` moves focus and `Ctrl`+`Tab` cycles (`EditorCanvas::cycleOverlap`, `canvas_input.cpp`) | transfers (not re-measured since the chord changed) |
| Corner drag points on a Connector | Change the lane count at either end | transfers¹ |
| `Ctrl`+left-click on the selection | **Nothing happens** | dead end |

¹ The only row the harness does not replay; the `editor-gestures` suite covers it instead.

**Counted cost, native path,** where one input is one click, drag, keystroke or dialog field:
draw a 3-lane Link 4 · connect two 3-lane Links 3 · add a curve point 3 · duplicate 2 · rotate 2.

**The one dead end, and the owner's ruling.** §1 and §2 both call `Ctrl`+left-click a *collision*
— the chord that duplicates in Vissim extending the selection here. That is only true of an
object that is **not** already selected, which is not the case a Vissim user's hand is in: on a
selected object it does nothing at all. Offered the Vissim verb, **the owner ruled on 2026-09-22
that it stays a dead end** (D30). Nothing is lost: `Shift`+click already extends a selection and
`Ctrl`+*drag* already duplicates. It is the last reflex that does not transfer, deliberately.

---

## 2. Keyboard

Vissim users work with one hand on the keyboard. **Rewritten 2026-09-22 (M1.27.3)** — the
2026-09-14 inventory described a much thinner editor and had become wrong in five rows.

### What the editor binds today

| Action | Chord | Source |
|---|---|---|
| New / Open / Save / Save As | `Ctrl+N` / `Ctrl+O` / `Ctrl+S` / `Ctrl+Shift+S` | `editor_window.cpp` |
| Undo / Redo | `Ctrl+Z` / platform default **and `Ctrl+Y`** on every platform | `editor_palette.cpp` |
| Choose a tool | `S` select · `L` link · `C` connector · `R` route · `V` input · `H` head · `A` conflict area · `X` split · `M` measure · `K` calibrate · `Q` queue counter | `editor_palette.cpp` |
| Run · step · stop | `F5` · `F6` or `Space` · `Esc` | `editor_run.cpp` |
| Background image | `Ctrl+B` | `editor_palette.cpp` |
| Wireframe (simple link display): centre lines only, vehicles stay in their lanes | `Ctrl+A` | `editor_palette.cpp`, D100 |
| Properties · Objects and problems · History docks | `Ctrl+I` · `Ctrl+Shift+O` · `Ctrl+Shift+H` | `editor_inspector.cpp`, `editor_tables.cpp`, `editor_history.cpp` |
| Fit network | `F` | `editor_window.cpp` |
| Cycle the objects under the pointer (with the Conflict area tool: the areas); plain `Tab` moves focus | `Ctrl+Tab` | `canvas_input.cpp` |
| Command palette · Network Objects dock · focus on network | `Ctrl+K` · `Ctrl+Shift+T` · `Ctrl+Shift+F` | `editor_workspace.cpp` |
| Keyboard and mouse shortcuts | platform Help key (`F1` on Windows/Linux) | `editor_workspace.cpp` |
| Run faster · slower | `+` · `-` | `editor_run.cpp` |
| Conflict area tool · cycle the selected area's priority | `A` · `P` | `editor_palette.cpp`, `editor_priority.cpp` |
| Delete the selection · delete a geometry point | `Delete` · `Ctrl+Delete` | `canvas_input.cpp` |
| Nudge the selection by one grid step (`Shift` ×10) | arrow keys | `canvas_input.cpp` |
| Finish the Link or route being drawn · drop its last segment | `Enter` · `Backspace` | `canvas_input.cpp` |
| Cancel the current gesture | `Esc` | `canvas_input.cpp` |

### Collisions with Vissim

Not "missing" — **bound to something else**. A wrong action is worse than an absent one, so
these come before the gaps. Three of the six rows this table used to carry are gone because the
editor was changed to match: `Ctrl+B` is the background image, the Objects dock moved to
`Ctrl+Shift+O`, `Delete` deletes the selection, and `Ctrl+Y` redoes on Linux too.

| Chord | Vissim ✔ | Here | Severity |
|---|---|---|---|
| `Ctrl` + left-click | **Duplicate** the selection | **Nothing** on an already-selected object; adds an unselected one | Settled, not open — measured a dead end and ruled deliberate on 2026-09-22 (D30, §1a) |
| `Esc` | **Stop the simulation** | Stops the run **and** cancels the drawing gesture, in that order | Low — M1.24 made both meanings live in one window, and doing both is the safe reading |

**Owner correction (2026-09-22).** An earlier revision listed `Ctrl+N` as Vissim's *simple
network display* toggle and called it a Medium collision with New. **It is not: the display
toggle is `Ctrl+A`.** `Ctrl+N` = New collides with nothing. `Ctrl+A` is the wireframe toggle
since D100 (2026-10-02); the supplied `docs/specs/network-editor` text that reads it as Select all
is superseded by this correction and left as supplied.

### Absent, and worth taking

| Vissim ✔ | Purpose | Where it belongs |
|---|---|---|
| `Ctrl+C` / `Ctrl+V` | Copy / paste network objects | Not booked — needs an ID-allocation policy for pasted objects |
| `Ctrl+Q` | Quick mode (draw less, simulate faster) | Not booked — revisit when a run is slow enough to need it |

`Ctrl+D` (3D), `Ctrl+U` (time format), `Ctrl+T` and the 3D navigation keys (`K` `I` `J` `L`
`Q` `A`) are out of scope: there is no 3D mode and no wall-clock display to toggle.

---

## 2b. Creation gestures for the objects we do not have yet

Recorded now because M1.5.1 is the **next** milestone and it authors two of these. Adopting the
gesture at the same time as the model is far cheaper than retrofitting it.

| Object | Vissim gesture ✔ | Status here |
|---|---|---|
| **Vehicle routes (static)** | `Ctrl`+right-click on the link/connector at the routing decision, then left-click the destination section | Typed since **M1.5.1**; the gesture is **M1.25**; **M1.26** makes the route name Links and Connectors, so it covers every lane of the carriageway the way Vissim's does, and narrowing a Connector no longer invalidates it. Still absent: the decision as a positioned object, relative flows, per-interval volumes and a lane-specific route — all M2.1 |
| **Nodes** | Right-click-drag a polygon over the junction, double-click the first point to close | **Implemented since S0 (D261–D290; updated 2026-10-27).** Node tool `N`: Ctrl+right-click the first vertex, plain clicks the next, Enter commits ([NODES](../reference/NODES.md) §6); also *Suggest nodes* and *Merge nodes*. Members, legs and turns are derived; per-turn, leg and Node delay, queue and letters in single-run, batch and comparison outputs; coarse values on Nodes without detail (ROADS §6); HCM 7 analysis (M13, CLI only). The gesture differs from Vissim's drag. |
| **Signal controllers** | `Signal Control > Signal Controllers` table, right-click → **Add…**, then **Edit signal groups** | **Fixed time since M2.7b:** Signal control tab → Add signal controller, a groups table with green start/end and amber, a timing-bar diagram, 2-/4-phase templates; a head shows a signal group. No actuated control, intergreen matrix or conflict check — **M4** |
| **Conflict areas** | Generated automatically wherever links/connectors overlap, *passive* until the author sets a status; picked only while *Conflict Areas* is the active object type (from the manual, not measured here) | **M3.2.4c (D68):** the same. Every at-grade overlap of two roads is a passive area and every merge shows its derived priority, both derived from the drawing and never stored. With the Conflict area tool (`A`) a click on one sets it (a turning Connector gives way to a Link) — or `P`/Enter on its row; a click on a set area selects it, again or `P` cycles firstYields → secondYields → undetermined; Delete returns a crossing to passive. A waiting line drags along its lane. *Add crossing areas* still sets every lane pair of two roads at once |
| **Queue counters** | Placed as their own object type with a click on a link (from the manual, not measured here) | **M3.2.6c (D65):** the Queue counter tool (`Q`). Each click adds a line to one counter: a stop line references its head, a waiting line references itself, anywhere else on a Link lane is a point; Enter creates it, Backspace drops a line, Esc cancels. Keyboard: *Add queue counter* over heads selected in the Signal heads table. A counter over an approach's heads replaces that approach's derived row. No point on a Connector path |
| **Parking lots** | **Car Park Creator** generates bays and their connectors from a drawn area | Absent. Booked as **M8** (D145) — a lot as a zone with capacity, gate and dwell time; bay geometry is not in its first scope |

The pattern worth extracting: in Vissim **one chord creates everything**, and the object type
comes from the sidebar, not from the gesture. That is the actual argument for the sidebar in
M1.9 — it is not decoration, it is what makes a single creation chord unambiguous.

---

## 4. Network objects: what exists

The entire authoring model is five types (`src/model/network/network.hpp`):

```
Link { id, geometry, lanes[] }        Lane { id, width }
Connector { id, from, to, geometry }  NetworkSignalHead { id, lane, position, programId }
Network { id, drivingSide, links[], connectors[], signalHeads[] }
```

Commands over them (`src/commands/*.hpp`): add/delete link, change geometry, change lane
widths, delete many as one transaction, change driving side, opposite carriageway, split (with
or without a downstream lane), add/delete connector, change connector geometry or endpoints,
reset curve, reanchor.

**Present in Vissim, absent here** (as of this dated assessment; conflict areas, priority rules,
stop signs and queue counters have existed since M3.2 — see §2b)**:** conflict areas · priority rules · stop signs · reduced
speed areas · desired speed decisions · vehicle routes as first-class objects · parking lots ·
pedestrian areas and links · public transport stops and lines · data collection points ·
queue counters · travel time sections · nodes · levels · display types · link behaviour types.

Two of these are already booked and should not be re-litigated here:

- **Vehicle routes and inputs** exist only as untyped JSON inside the project document — they
  have no table and no commands. `../ROADMAP.md` M1.5.1 owns this and says so plainly.
- **Conflict areas** are the whole reason D1 exists (`../PROBLEM.md` §2). They belong with
  right-of-way in the engine, not with the editor; authoring them before the core can honour
  them would be a fidelity claim the code cannot support.

The rest are genuinely out of scope until the milestone that needs them. A network object with
no engine behind it is a drawing, and drawing it invites a user to believe it is modelled.

---

## 5. Running, and what comes out — the widest gap

**In Vissim the simulation runs in the network editor.** You draw, you press play, and vehicles
move over the network you just drew, in the same window, at a speed you control. That single
property is most of why the tool feels like one tool.

Today:

- The editor "does not simulate edited projects yet" (`docs/NETWORK_EDITOR.md:3`).
- The simulation window was a **separate** `MainWindow` that loaded M0 scenario JSON.
  **Superseded by M1.24:** it is removed and the editor opens both file kinds; `loadScenario`
  (`src/project/load.cpp`) survives for `multra-cli`.
- Until this session, that window could not even *open* a project file: it read
  `definition` unguarded, and an editor project legitimately saves `"definition": null`, so it
  failed with a raw `json.exception.type_error.304`. It now names the file kind and offers to
  open the drawing in the editor instead (§7).

So the editor's Run story is not "rough" — it is **absent**, and the bug that prompted this
review is a direct symptom of the split. A user who draws a network and looks for the play
button finds a different window that rejects their file.

**What an in-editor Run actually requires**, in dependency order:

1. **A demand authoring model** — routes, vehicle inputs, signal programs as typed objects with
   undoable commands. This is M1.5.1 and nothing runs without it.
2. **Catalog resolution before Run** — vehicle types and driver behaviours live in `data/`, not
   in the project, so a project must resolve them at run time and say what is missing. M1.7.
3. **Revision → run snapshot** — compile one explicit document revision, so a run is always
   attributable to a revision and edits during a run cannot change it underneath. M1.7.
4. **A vehicle layer over the editor canvas** plus Run/Pause/Step/Reset and a speed control.
   This is the only genuinely new surface, and it is M1.8.

Steps 1–3 already exist on the roadmap; only step 4 needed booking. Hence M1.8 is the *surface*
and M1.7 stays the *plumbing* — see `../ROADMAP.md`.

**Final output.** Vissim ends in evaluation: node results, movement delay, queue lengths, LOS.
Here `src/eval/` produces a completed-trip mean delay, shown on the editor's run status since
M1.24 (the simulation window that used to show it is gone). The
per-movement delay and LOS tables that a traffic impact study actually needs are M2 onwards and
are not attempted. **Whatever is built, the not-yet-validated marker stays** until M6 (D5).

---

## 7. Two file kinds — the trap this review started from

Worth stating on its own, because it is a UX defect rather than a missing feature, and it is
fixed as of this session.

The project produces **two** JSON kinds that used to be indistinguishable to the user:

| | M0 scenario | Editor project |
|---|---|---|
| Typical name | `data/scenarios/crossing.json` | `network.traffic.json` |
| Root keys | `network`, `definition` | `format`, `schemaVersion`, `nextId`, `revision`, `network`, `definition`, `background` |
| `definition` | Required, complete | **Null** for any network drawn from scratch — that is correct, not corrupt |
| Opened by | The network editor, or `multra-cli` (M1.24; formerly the simulation window) | The network editor |
| Runs? | Yes | No — it has no demand yet (§5) |

Both matched the same `*.json` filter, and the editor's default save name is
`network.traffic.json`. Picking a project in the simulation window therefore produced a parser
exception about a null, which described the JSON accurately and the user's situation not at all.

Now: `loadScenario` classifies the file before reading any field and fails with a named,
translated code (`SCENARIO_IS_PROJECT`, `SCENARIO_NO_DEFINITION`, `SCENARIO_NO_NETWORK`,
`SCENARIO_NOT_JSON_OBJECT`, `SCENARIO_FILE_READ`), and the dialogs default to
`*.traffic.json` for projects. The **Open in Network Editor** hand-off the simulation window
offered is moot since M1.24: the editor opens both kinds itself.
See `docs/reference/NETWORK_EDITOR.md` §"Save, recovery and formats" (the file-kind table).

The two kinds were **not** merged into one schema, deliberately. A single format would make
every drawing look runnable, which is the fidelity claim hard rule 4 exists to prevent. They
converge when M1.8 gives a project a real Run — not before, and by adding demand to projects,
not by blurring the formats.

## Dated follow-ups

The 2026-09-15 to 2026-09-21 follow-ups (reported editor failures, intermediate points, the
Connector audited end to end, the snapping audit, the withdrawn wedge) are in
[`../archive/VISSIM_PARITY-2026-09-15--2026-09-21-follow-ups.md`](../archive/VISSIM_PARITY-2026-09-15--2026-09-21-follow-ups.md).
