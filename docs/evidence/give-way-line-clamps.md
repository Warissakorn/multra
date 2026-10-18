# Give-way line clamps traced: a slow move-off meets a closing gap — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1`, on `1c10bf9` (D231). Trace only, no code change
(D232).

- **Host:** Linux, GCC 13.3.0, Ninja, Debug `--preset desktop`.
- **Linux only.**
- **Question:** NEXT asked where the clamps at the Stop lines of D231's
  [Stop T-junction copy](stop-dwell.md#shipped-runs) come from.

## Method

Temporary logging (`TS_PROBE`, never committed) was added at both clamp sites in
`src/core/simulation.cpp`:
- **Phase 1:** the move exceeds `allowedDistance`.
- **Phase 2:** a `resolveRequests` cap.

For each clamp it logged:
- the gap to each minor zone's waiting line, the speed, and the wanted and allowed move;
- the Stop service (`since`, `ticks`);
- `committed(...)`, `majorInside` and `majorBlocks` for each zone.

It was run at seeds 42–81 on these projects:
- the Stop copy (`tj-stop-0.1` and `tj-stop-0.5` in the scratchpad), with and without a 2 s
  `stopDwellTime`;
- the shipped `t-junction-priority` (Yield; its dt is 0.1);
- that project at dt 0.5.

The source was restored with `git checkout`. Each pair of log lines (one per chained minor zone)
counts as one clamp.

## What fires

| Project | dt | Clamps | Moving | Standing | Speed of the moving (m/s) | Gap to the line (m) | Deceleration to stop in the gap, max (m/s²) |
|---|---|---|---|---|---|---|---|
| `t-junction-priority` (Yield, shipped) | 0.1 | 6 | 6 | 0 | 0.19–1.02 | ≤ 0.08 | 7.3 |
| Same, Yield | 0.5 | 10 | 7 | 3 | ≤ 4.30 | ≤ 1.42 | 7.1 |
| Stop copy, no key | 0.1 | 8 | 8 | 0 | 0.30–0.82 | ≤ 0.06 | 7.5 |
| Stop copy, 2 s dwell | 0.1 | 15 | 15 | 0 | 0.24–1.27 | ≤ 0.10 | 7.9 |
| Stop copy, no key | 0.5 | 12 | 10 | 2 | ≤ 2.50 | ≤ 0.73 | 7.7 |
| Stop copy, 2 s dwell | 0.5 | 10 | 9 | 1 | ≤ 2.50 | ≤ 0.65 | 6.9 |

Every clamp is a **phase-1** cap: the move exceeds the line hold. Phase 2 never fires.

**Moving clamps, all of them.** Each has the same shape:
- The minor vehicle had stood at its line and moved off into an open gap. It is now creeping at
  walking pace, centimetres short of the line.
- A major vehicle comes within the zone's headway or gap time, so `majorBlocks` turns true. No major
  vehicle is inside the area (`majorInside` false in every case).
- The minor could still stop at its maximum deceleration, so it is not committed (D69). The
  anticipation part of the gap test therefore holds it again at the line.
- The prototype's braking softens near a standing target (as in D227), so its wanted move overshoots
  the line by a few millimetres to centimetres, and phase 1 clamps it.

The deceleration needed to stop within the gap is at most 7.9 m/s², below the car's 8 m/s²
maximum. That follows from D69's test: a vehicle that needed more would have been committed and
would have gone.

**Standing clamps (dt 0.5 only).** The vehicle is at zero speed, about 0.3–0.7 m short of the
line. Its vehicle leader is 2.07–2.31 m ahead, just past the 2 m `standstillDistance`, and moving
at 11–16 m/s: a major vehicle merging ahead. The allowance left by the standstill buffer is smaller
than the first step from rest (0.3125 m at dt 0.5). This is D105/D108's small-clearance case at a
line: the standstill cap that treats a moving leader as standing (D201 B5).

## What it means

- **It is not the Stop rule's.** The shipped Yield project clamps the same way: 6 clamps at seeds
  42–81. (This first read "none at 42–51"; seed 46 has 2, and D225's baseline shows them, D240.) A Stop only changes which
  vehicles stand at the line, and how often.
- **A dwell can add clamps.** It raises them at dt 0.1 (8 → 15) because it leaves more vehicles
  moving off slowly from the line. It lowers them at dt 0.5 (12 → 10).
- **These are D105's "moving minor clamps",** the M3.2.8a.1 cases D108 left (21 then). D105's
  case needed 9.06 m/s² behind a merge leader's standstill buffer. Today's cases at a zone line
  need less than the maximum: the gap test changed its mind on a vehicle that could still stop,
  and the braking law did not stop it in time.

## Not done here

M3.2.8a.1's gate asks for a contract and failure-first rows before any motion change. It also
forbids suppressing the events. The changes considered need the owner, and none was measured:
- keep a gap that was accepted from rest until the line is passed (state, against D69's stateless
  rule);
- let a vehicle that is not committed brake up to its maximum to stop at a line that has just closed
  (a braking-law change, like D227's floor);
- ignore the anticipation part once a vehicle has started from its line.

Each would move the shipped `t-junction-priority` output, so this is recorded rather than changed.
The owner later chose the second, limited to ticks that clamp today (D239,
[GIVE_WAY_STOP](../reference/GIVE_WAY_STOP.md)).
