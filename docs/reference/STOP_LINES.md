# Stop lines: derived waiting positions and a Stop line tool (M3.2.12)

Written 2026-11-19 (D591) as a draft; the owner said go ("ทำ STOP_LINES ต่อได้เลย"). **Implemented:
slices 1–2 (D594), slice 3 (D595).** No gate closes with it, and rule 4 applies: nothing here is
validated. It follows M3.2.4e (merge
admission over the physical mouth) because both decide where a yielding vehicle stops.

## 1. The problem

The owner's T-junction exercise, attempt 1 (OWNER_SITTING A2):
- The merge waiting lines sit at the D50/D59 one-metre admission strip at each Connector's end,
  on the joined lane's centre line, so the minor vehicle waits inside the lane it joins, and the
  major lines are downstream of where the turn first overlaps their lane.
- Every lane-pair conflict area has its own stored waiting line. A 3 × 3 crossing draws a dozen
  lines the author never placed, which confuses the picture where areas are complex.

The owner proposed (2026-11-19): use the conflict area's default for its waiting line and hide
it, and add a separate tool for stop lines, so the author sees only the lines they placed.

## 2. The owner's choices (2026-11-19, "ตามแนะนำ")

| Question | Choice |
|---|---|
| Is a derived line hidden entirely? | No: it shows faintly while its area is hovered or selected under the Conflict area tool, and nowhere else |
| Where is the derived line? | The vehicle's front stops `frontStopDistance` before the first point where its path's surface overlaps the other path's surface (M3.2.4e's physical overlap). One value per project, beside `gapTime` and `headway` in the priority defaults; default 1.0 m |
| Which areas does a placed stop line control? | Every conflict area downstream on its path, up to the end of the junction (the next Link past the Connector chain), unless another placed line comes first. The Conflict areas table shows which line each area uses |

Already done (D590): conflict areas and plain waiting lines show only under the Conflict area tool
(lines also under the Queue counter tool, which snaps to them). A line carrying Stop or Yield
always shows.

## 3. Model

- `ConflictSide.waitingLineId` becomes optional. Absent: the side uses its **derived line**,
  computed from the drawing whenever it is needed, never stored (hard rule 3).
- A **stop line** is today's `WaitingLine` that the author placed, with an optional `StopControl`
  (`yield` or `stop`). Its coverage (§2) is derived, not stored; a side that names a line keeps
  naming it (an explicit choice wins over coverage).
- The derived line needs the physical overlap of M3.2.4e. Until that slice lands, nothing in this
  contract is switched on: hiding the D50/D59 position would hide the defect the owner found.
- Runtime: `zoneHold` reads a line position per side from the compiled scenario; compilation fills
  it from the named line, the covering stop line or the derived line, in that order. The core does
  not learn which it was.

## 4. Schema and migration

One schema step. A stored line is kept as a stop line when a `StopControl` or a queue counter
names it, or when its station differs from the position the editor would have placed it at
(D50/D59: crossing entry − 1 m, merge entry) by more than 0.01 m (the author dragged it).
Every other line is dropped and its sides use the derived line. Shipped projects and the
T-junction fixture regenerate through their builders in the same slice, with a recorded decision
(the minor road's Stop/Yield lines stay as stop lines; the merge and major lines go).

## 5. Editor

- **Stop line tool** (new, in the tool picker after Conflict area): click a lane to place a line;
  drag it along its path; Delete removes it; its row is in the Conflict areas tab with its mode and
  the areas it covers. Placing a line on a path with no conflict area downstream is refused with a
  message, not stored.
- **Conflict area tool:** hovering or selecting an area shows its derived lines faintly (dashed,
  ink colour) and placed lines solid. A derived line cannot be dragged; dragging one offers to place
  a stop line there instead.

## 6. Acceptance rows (failure first)

| Row | Check |
|---|---|
| SL1 | On the T-junction, every derived merge line is upstream of the joined lane's edge: the minor vehicle's front at rest is outside the major lane's surface (fails today: `wait-14`, `wait-18`) |
| SL2 | The major lines are upstream of the first overlap of the turn with their lane (fails today: `wait-13`, `wait-17`) |
| SL3 | A derived line moves when the Connector is reshaped; nothing in the file changes |
| SL4 | A placed stop line covers the downstream areas of its path and the table names them; a second line upstream of the first covers nothing the first already covers |
| SL5 | Migration keeps every line named by a Stop/Yield or a counter, and every dragged line, and drops the rest; a re-save is byte-stable |
| SL6 | Same seed, same build: a project whose lines all migrated to derived lines runs identically to one with the same positions stored |
| SL7 | Under the Select tool no derived line, plain line or area is drawn; under the Conflict area tool a derived line shows only for the hovered or selected area |
| SL8 | `frontStopDistance` outside (0, 10] m is refused at load with its path |

## 7. Out of scope

The line-approach law (a held vehicle creeps for about 20 s and stops 2–3 m short; release jumps
to the type's maximum): its own contract, as LINE_APPROACH. Calibrated stop distances: M6.

## 8. Slices

1. M3.2.4e physical merge admission (done: [MERGE_MOUTH](MERGE_MOUTH.md), D593), then the derived line and
   `frontStopDistance` (SL1–SL3, SL6, SL8).
2. Schema step and migration, shipped projects regenerated (SL5).
3. Stop line tool and coverage (SL4, SL7).

## 9. As implemented (D594), and where it differs from the draft

- **Done:** derived lines (`derivedWaitingStation`, `waitingStation`), `frontStopDistance`, schema 79,
  `deriveStopLines` (the §4 rule) and the regenerated T-junction. Every command that creates an area
  (Add crossing areas, an automatic crossing, Take over merge, junction fills) now stores no line.
  Rows SL1–SL3, SL5, SL6 and SL8 pass (`stoplines`); SL1–SL2 were already met by D593, so they are
  guards here. SL3 failed first in the sense that a stored line moved in the file on a reshape.
- **An empty `waitingLineId`, not an absent key:** the field tables have no "required before N";
  an empty id was already refused, so older readers still refuse a derived side.
- **`frontStopDistance` lives in the project** (`rightOfWay`), not beside `gapTime`/`headway` in
  `data/priority-rules`: that catalog is shared by every project and hashed into archived sweep
  inputs, and the owner asked for one value per project.
- **No migration at load:** `deriveStopLines` is a command. Loading an old file and silently moving
  its lines would change its runs; the T-junction builder uses the new commands instead. Offering
  the command in the editor belongs to slice 3.
- **Setting Stop or Yield on a derived side** places a stop line before the first decided area on
  that path that waits at a derived line, and names it on every such area from there on (§2's
  coverage, written into the file as names). So a Stop halts a vehicle once, as D63's one line per
  lane did. In the T-junction the crossing turn's sign now also covers its merge. Coverage as a
  derived rule, the table column and SL4 are slice 3.
- **`storeDerivedLines`** is the inverse of `deriveStopLines`: it stores every derived line where it
  stands, for a reader older than 79 and for tests that pin another feature's schema floor.
- **Fallout, each updated on purpose:** schema pins of documents with conflict areas (`rightofway`
  a02, `stop_control_model`, `macronetwork`, `mesonetwork`, `nodeanalysis`, `nodedetail`,
  `studyarea`, `junctionfill`) now expect 79 or store the lines first; line counts
  (`rightofway_lifecycle`, `conflict_geometry`, `priority-ui`); rows that move, delete or drag a
  stored line now store the lines first (`rightofway`, `rightofway_resolution`, `rightofway_editor`,
  `priority-canvas`); D63's one-line-per-lane rows read the derived positions
  (`automatic_conflict`, `conflict_follow`); the T-junction's ids shift (fewer
  lines allocated), so `nodemembership` names `conflict-9/11/13` and the archived sweep metadata
  restores the old route names when every input still matches (`restoreArchivedRouteIds`).
- **Slice 3 (D595, owner "ทำส่วนที่ 3 ต่อได้เลย"):** the Stop line tool (`W`, palette after Conflict
  area): Ctrl+right-click on a lane places a line (`placeStopLine`), a click picks and drags one,
  Delete removes it (`removeStopLine`, its areas back on their default lines). Coverage is written
  as names when the line is placed (`stopLineCoverage`): decided areas on default lines at or past
  it on its path, or, from a Link lane, on a Connector leaving that lane at or past it. Refused with
  `EDIT_NO_CONFLICT_DOWNSTREAM` when that is none. The Conflict areas table has a Waits at column
  and a Use default lines button (`deriveStopLines`). Under the Conflict area tool the hovered or
  selected area's default lines are drawn dashed and faint (`derived-line`). Rows: `stoplines` SL4,
  `stop-line-ui` (SL4 in the table, SL7).
- **Still differs from §5:** dragging a default line does nothing (the draft offered to place a line
  there; place one with the tool instead); coverage stops one Connector past a Link lane rather than
  at the next Link of a Connector chain.
