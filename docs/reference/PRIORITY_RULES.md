# PRIORITY_RULES — a stop line with conflict markers, without a conflict area (draft, D604)

**Slices 1, 1b and 2 implemented (D614–D616, 2026-11-21: schema 83, codec, compile, engine, Stop at the line,
the Stop line tool's marker gestures and lifecycle); slice 3 (display, table, owner's look) open. The owner answered §8 on 2026-11-20 (D606).** The owner's request of 2026-11-20 (verbatim): "เครื่องมือเส้นหยุดสามารถสร้างได้โดยไม่ต้องใช้
Conflict area ก็ได้ ให้สร้างพื้นที่ให้ทางเหมือน vissim", then the choice "ข. Priority rule แบบ Vissim" over
taking over the automatic candidates. Written before code; rows PR1–PR10 fail first. Section 8 holds
the owner's choices, answered as Vissim does it (D606).

## 1. The problem

The Stop line tool (D595, [STOP_LINES](STOP_LINES.md) §5) refuses a line with no conflict area
downstream on its path. A give-way that is not an overlap of two drawn lanes cannot be authored at
all: a right turn across an opposing stream whose lanes do not touch on the drawing, a driveway
that waits for a stream further along, a minor road that must look at two streams, one of them far
off. Vissim models these with a *priority rule*: a stop line on the minor path and one or more
*conflict markers* on major paths, each with a minimum gap time and a minimum headway.

## 2. What already exists

- **The runtime is there.** `PriorityRule` (`src/core/types.hpp`) is exactly one stop line and one
  marker: `yieldSegmentId`/`yieldPosition`, `conflictSegmentId`/`conflictPosition`, `gapTime` (s),
  `headway` (m). `stepSimulation` (`src/core/simulation.cpp`, "Priority rules") holds a vehicle at
  the line as a red head does while a major vehicle on the marker's segment is over the marker, is
  within `headway` short of it, or reaches it in less than `gapTime` at its current speed; a
  standing major vehicle beyond the headway does not block; a driver who cannot stop at the line
  goes unless the marker is occupied (M3.2.8a, D219). Validation, route incidence (`routeRules`) and
  join anticipation (D255) read them.
- **Only merges use it.** The rules come from `resolveRightOfWay`'s automatic merge ranking
  (`derivedPriorityRules`); the file's `priorityRules` rows (`AuthoredPriorityRule`) only carry an
  area's `gapTime`/`headway` by `conflictAreaId` and compile to a conflict zone, not to a rule.

So this feature is mostly authoring: a file form, a compile step to the existing `PriorityRule`, an
editor gesture and a display. The engine changes for D606's two Vissim conditions only: a maximum
speed on the headway test and vehicle-class filters (§3).

## 3. Model

- A **priority rule** is one stop line (a `WaitingLine`, a `ControlPoint` on a lane path) with an
  optional list of vehicle classes, and one or more **markers**. A marker is a `ControlPoint` on any
  lane path with its own `gapTime` (s, > 0), `headway` (m, ≥ 0), `maxSpeed` (km/h, > 0) and optional
  list of vehicle classes.
- **Who waits:** a vehicle whose route passes the line and whose type is in one of the line's classes
  (no list: every type). Other types pass the line as if it were not there; a different class needs a
  second rule at the same line (Vissim).
- **Who blocks, per marker:** a major vehicle on the marker's segment whose type is in the marker's
  classes (no list: every type), and that (i) is over the marker, or (ii) is within `headway` short of
  it **and** its speed is at or below `maxSpeed`, or (iii) reaches it in less than `gapTime` at its
  current speed. A standing major vehicle beyond the headway does not block (unchanged). With
  `maxSpeed` at its ceiling the headway test is today's.
- It compiles to one runtime `PriorityRule` per marker, all sharing the line's yield position, so a
  vehicle at the line waits while **any** marker blocks: it passes only when every marker's
  conditions hold (Vissim's rule with several markers).
- A marker no route reaches is a Problems row, not an error (with the table, slice 3); a marker on the
  line's own lane path is refused, `PRIORITY_MARKER_ON_OWN_PATH` (it would hold a vehicle for itself).
  Slice 1 checks the same lane path only, not other paths upstream on a route (D614). A class id that names
  no vehicle class is refused at its path.
- A line can serve conflict areas (STOP_LINES) and rules at once; both hold, and the nearer hold
  wins as for heads and zones today. A Stop control on the line (`StopMode::stop`) requires the full
  stop before the rule's gap test, as for areas (Q4 (a)); Yield is the rule alone. **Built in slice 1b (D615):** a
  Stop control on a marker rule's line may name no area (`conflictAreaIds: []`, from 83); it makes each
  of the rule's runtime rules a Stop, served by the engine's stop service as a Stop zone's line is.
- **Derived merge rules are unchanged:** no class lists, `maxSpeed` at its ceiling, so every shipped
  project and seed keeps its bytes (PR9).

## 4. Schema 83 (`priorityMarkerVersion`, D614)

- `rightOfWay.priorityRules[i]` gains a second, exclusive form: `{id, name?, waitingLineId,
  vehicleClassIds?, markers: [{point, gapTime, headway, maxSpeed?, vehicleClassIds?}, …]}` where
  `point` is the existing ControlPoint object and an absent `vehicleClassIds` means every type.
  `maxSpeed` is km/h; absent is the ceiling, no limit (D614).
  Exactly one of `conflictAreaId` (the existing form) or `waitingLineId` + nonempty `markers`; a
  rule carrying keys of both is `INVALID_PRIORITY_RULE` at the key.
- Compiled per marker to a runtime `PriorityRule` `right-of-way/<id>#<k>`; classes become type ids in
  the project layer (`priority_rule_classes.cpp`), an unknown class is `UNKNOWN_VEHICLE_CLASS` at its path.
- Refused below the new step at its path (`EDIT_UNSUPPORTED_FIELD`), as every later key; files without the
  form keep their bytes and version. The writer's version choice adds the floor.
- Lifecycle: a marker or line follows its path like every ControlPoint (split, drag, lane edits,
  delete cascades in one Undo step; `right_of_way_lifecycle`).

## 5. Editor

- **Stop line tool (`W`).** Ctrl+right-click on a lane with no area downstream now places the line
  and starts a rule instead of being refused; with a line picked, Ctrl+right-click on another lane
  adds a marker there with Vissim's defaults: `gapTime` 3.0 s, `headway` 5.0 m and `maxSpeed` at its
  ceiling (no speed limit on the headway test; Vissim's example uses 10–20 km/h to keep a junction
  clear, entered per marker). A marker drags along its path; Delete removes the picked marker, and the
  rule with its last marker. Each gesture is one Undo step.
- **Display.** Shown under the right-of-way tools only (D603): the line as today, each marker as a
  short green bar across its lane with its headway as a faint band upstream on the major lane, and
  a thin line from the stop line to each marker while the rule is picked.
- **Table.** The Conflict areas tab lists rules below the areas: line, markers, gap time, headway,
  routes held; a row's dialog edits each marker's numbers (D587's one-open rule).

## 6. Acceptance rows (failure first)

| Row | Statement |
|---|---|
| PR1 | A file with the new form round-trips byte for byte at the new step and is refused below it at its path |
| PR2 | Each marker compiles to one runtime `PriorityRule` with the line's yield position and its own segment and position (checked against hand-computed stations) |
| PR3 | A minor vehicle facing a major vehicle `gapTime` + 0.1 s from the marker goes; at `gapTime` − 0.1 s it is held at the line (the forcing: both cases are reached) |
| PR4 | A standing major vehicle within `headway` of the marker holds the minor one; beyond it, it does not |
| PR5 | Two markers on two streams: a vehicle on either alone holds the line |
| PR6 | A marker on the line's own path, or upstream of it, is refused with its path; nothing is stored |
| PR7 | Placing a line with no area downstream, then a marker, is one Undo step each; Delete of the last marker removes the rule |
| PR8 | Split, drag and lane edits carry the line and markers; a delete cascades in one Undo step |
| PR9 | Every shipped project, seeds 42–51, is byte-identical (no file uses the form); since item 51 the one exception is `priority-rule-lab`, built for the owner's look |
| PR10 | Same seed, same run: two runs of a rule fixture give identical events |
| PR11 | `maxSpeed`: a major vehicle inside the headway at 30 km/h holds the line with `maxSpeed` 50 and does not with `maxSpeed` 20 (its gap time is set beyond `gapTime`, so only the headway test decides) |
| PR12 | Classes: with the line's classes set to cars only, a heavy vehicle passes the line while a car facing the same major vehicle is held; with a marker's classes set to heavy only, a car on the major road does not hold the line and a heavy vehicle does |
| PR13 | A class id that names no vehicle class is refused at its path; nothing is stored |
| PR14 | A Stop on the line: with an empty major road a minor vehicle stands at the line before it crosses; with Yield it drives through (the forcing) |
| PR15 | After the stop, a major vehicle within the headway still holds it; it goes once the major road clears |
| PR16 | A Stop control with no area on a marker line round-trips at 83 and compiles every marker rule to a Stop (Yield: none); on a line no marker rule names it is still `INVALID_STOP_CONTROL` |

## 7. Out of scope

Calibrated gap times (M6). Vissim's "look beyond red signals". Converting existing conflict areas to rules.

## 8. The owner's choices — answered 2026-11-20: "1ก 2ข 3ข 4ก 5ข เหมือน vissim" (D606)

1. **Where it lives:** (a) a second exclusive form in the existing `priorityRules` array, at the next schema step;
   (b) a new root array `priorityRuleSets`.
2. **Conditions per marker:** (a) gap time and headway only, as the engine has; (b) also Vissim's
   maximum speed, which applies to the headway (clearance) test only: a major vehicle inside the
   headway counts only when its speed is at or below it (PTV's example uses 10–20 km/h to keep a
   junction clear of queues). An engine change: today every vehicle inside the headway counts.
3. **Which vehicles stop:** (a) every vehicle crossing the line; (b) per vehicle class, as Vissim.
4. **Stop at the line:** (a) a Stop control on the line applies to the rule too (full stop, then the
   gap test), Yield is the rule alone; (b) rules are always Yield-like.
5. **Default numbers for a new marker:** (a) the shipped give-way default `data/priority-rules/default.json`,
   3.0 s and 7.0 m (not calibrated), editable per marker; (b) Vissim's defaults, 3.0 s and 5.0 m.

**Answers:** 1 (a), 2 (b), 3 (b), 4 (a), 5 (b). §3–§6 are written to them.

## 9. Slices

1. **Done (D614):** the schema step, codec and compile to `PriorityRule`, and the engine's `maxSpeed` and class
   filters (PR1–PR6, PR9–PR13) on a hand-built crossing; no UI. Tests: `tests/priority_rule_tests.cpp`.
   **1b done (D615):** the Stop at the line (PR14–PR16).
2. **Done (D616):** the Stop line tool's gestures, lifecycle and Undo (PR7, PR8):
   `priority_marker_commands.cpp`, markers drawn as green bars (`canvas_markers.cpp`); tests
   `priority_marker` and `stop-line-ui`. A copy keeps a rule when its line is copied, with the markers
   whose roads were copied; a study-area cut does the same.
3. Display and the table rows; the owner's look on Windows: [OWNER_SITTING_WINDOWS item 51](../plans/OWNER_SITTING_WINDOWS.md),
   on `data/projects/priority-rule-lab.traffic.json` (Open example → Priority rule lab).

**How Vissim does each (PTV Vissim help, "Creating priority rules" and "Attributes of priority rules",
versions 2020–2024; read 2026-11-20):** a priority rule is its own network object, one red stop line and
one or more green conflict markers, not part of a conflict area (Q1). Each marker has a minimum gap time
(the time the first upstream vehicle needs to reach the marker at its current speed; default 3.0 s), a
minimum clearance (the distance from the marker to the next vehicle upstream; default 5.0 m) and a
maximum speed for the clearance test (Q2). Vehicle classes are set on the stop line, applying to all its
markers, and on each marker; other classes get a second rule at the same position (Q3). A vehicle passes
without stopping only when every marker's conditions hold (§3). Stop at the line (Q4) is not checked in
that help; Vissim models a stop sign as a separate object, which this session has not verified.
