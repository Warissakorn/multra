# Stop lines: derived waiting positions and a Stop line tool (M3.2.12)

Written 2026-11-19 (D591) as a draft for the owner's review. **Not implemented.** No gate closes
with it, and rule 4 applies: nothing here is validated. It joins the planned M3.2.4e (merge
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

1. M3.2.4e physical merge admission with its own failure-first rows, then the derived line and
   `frontStopDistance` (SL1–SL3, SL6, SL8).
2. Schema step and migration, shipped projects regenerated (SL5).
3. Stop line tool and coverage (SL4, SL7).

**Owner:** review §2–§6 before slice 1 starts.
