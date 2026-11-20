# PRIORITY_RULES — a stop line with conflict markers, without a conflict area (draft, D604)

**Draft, not implemented.** The owner's request of 2026-11-20 (verbatim): "เครื่องมือเส้นหยุดสามารถสร้างได้โดยไม่ต้องใช้
Conflict area ก็ได้ ให้สร้างพื้นที่ให้ทางเหมือน vissim", then the choice "ข. Priority rule แบบ Vissim" over
taking over the automatic candidates. Written before code; rows PR1–PR10 fail first. Section 8 holds
the owner's choices; no slice starts before they are answered.

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
editor gesture and a display. The engine changes only if §8 asks for more than gap and headway.

## 3. Model

- A **priority rule** is one stop line (a `WaitingLine`, a `ControlPoint` on a lane path) and one
  or more **markers**, each a `ControlPoint` on any lane path with its own `gapTime` (s, > 0) and
  `headway` (m, ≥ 0).
- It compiles to one runtime `PriorityRule` per marker, all sharing the line's yield position, so a
  vehicle at the line waits while **any** marker blocks (Vissim's rule with several markers).
- The rule applies to every vehicle whose route passes the line. A marker no route reaches is a
  Problems row, not an error; a marker on the line's own path, or upstream of the line on it, is
  refused (it would hold a vehicle for itself).
- A line can serve conflict areas (STOP_LINES) and rules at once; both hold, and the nearer hold
  wins as for heads and zones today. A Stop control on the line (`StopMode::stop`) also requires the
  full stop before the rule's gap test, as for areas (§8 Q4).

## 4. Schema (the next step after 79)

- `rightOfWay.priorityRules[i]` gains a second, exclusive form: `{id, name?, waitingLineId,
  markers: [{point, gapTime, headway}, …]}` where `point` is the existing ControlPoint object.
  Exactly one of `conflictAreaId` (the existing form) or `waitingLineId` + nonempty `markers`.
- Refused below the new step at its path (`EDIT_UNSUPPORTED_FIELD`), as every later key; files without the
  form keep their bytes and version. The writer's version choice adds the floor.
- Lifecycle: a marker or line follows its path like every ControlPoint (split, drag, lane edits,
  delete cascades in one Undo step; `right_of_way_lifecycle`).

## 5. Editor

- **Stop line tool (`W`).** Ctrl+right-click on a lane with no area downstream now places the line
  and starts a rule instead of being refused; with a line picked, Ctrl+right-click on another lane
  adds a marker there with §8 Q5's default `gapTime`/`headway`. A marker drags along its path; Delete removes the picked marker, and the
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
| PR9 | Every shipped project, seeds 42–51, is byte-identical (no file uses the form) |
| PR10 | Same seed, same run: two runs of a rule fixture give identical events |

## 7. Out of scope

Calibrated gap times (M6). Vissim's per-marker maximum speed, vehicle-class filters and "look
beyond red signals" unless §8 asks for them. Converting existing conflict areas to rules.

## 8. The owner's choices (recommended first)

1. **Where it lives:** (a) a second exclusive form in the existing `priorityRules` array, at the next schema step;
   (b) a new root array `priorityRuleSets`.
2. **Conditions per marker:** (a) gap time and headway only, as the engine has; (b) also Vissim's
   maximum speed (a major vehicle slower than it does not count), an engine change.
3. **Which vehicles stop:** (a) every vehicle crossing the line; (b) per vehicle class, as Vissim.
4. **Stop at the line:** (a) a Stop control on the line applies to the rule too (full stop, then the
   gap test), Yield is the rule alone; (b) rules are always Yield-like.
5. **Default numbers for a new marker:** (a) the shipped give-way default `data/priority-rules/default.json`,
   3.0 s and 7.0 m (not calibrated), editable per marker; (b) other numbers the owner names.

## 9. Slices

1. The schema step, codec and compile to `PriorityRule` (PR1, PR2, PR6, PR9), with the runtime rows PR3–PR5
   and PR10 on a hand-built crossing; no UI.
2. The Stop line tool's gestures, lifecycle and Undo (PR7, PR8).
3. Display and the table rows; the owner's look on Windows.
