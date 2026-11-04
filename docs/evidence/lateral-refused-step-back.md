# A blocked body over the line steps back — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `6c847a1` (D446) plus this change (D447). Local host:
Linux, GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only; no desktop build.** Contracts:
[LATERAL](../reference/LATERAL.md) §7 and [LANE_CHANGE_DURATION](../reference/LANE_CHANGE_DURATION.md)
§4. Nothing here is validated (rule 4).

## The gap

`moveLaterally` refused a move in two ways:

- **Blocked:** the moved body would overlap another body in 2D. This returned at once, so the
  vehicle kept its place.
- **Not covered:** D208's `coversSafely` refused an outward move. Here a body already over the line
  stepped back toward its lane centre.

A body over the line whose move was blocked therefore waited straddling the line. That is the
review's path (b), and it contradicts D208 ("it never waits straddling the line").

Row BA53 (D206) pinned the old behaviour: the changer's body about 0.85 m over the line, a standing
car alongside one step from contact, and `lateral` kept. The owner chose D208's rule (2026-11-04).

## Change

One step-back path in `moveLaterally`, used for both refusals:

- a body entirely in its own lane waits;
- a body over the line moves toward its centre by at most one step, if that move is not blocked
  itself;
- the target is kept, and the next tick tries again.

## Row (`lanechangeduration`)

| Row | Case | Before | After |
|---|---|---|---|
| BA53 (revised, D447) | BA53's placement. Forcing, asserted first: the next step would overlap the blocker, there is no overlap now, and the body is over the line | Fails: `lateral` kept | It steps back; within 30 ticks its body no longer covers the target lane, with no 2D overlap; the target is kept and it stays on its own route; with the blocker removed it moves outward again |

`ctest --preset headless`: **174/174** pass, and `check` passes.

## Product runs, seeds 42–51

Every batch of all four projects, off and on, is byte-identical to D446's. The change-duration probe
gives identical results too. Since D444–D446 no product run reaches this case, so the evidence is the
row alone.
