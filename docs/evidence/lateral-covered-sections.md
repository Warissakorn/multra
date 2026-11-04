# Covered-lane search from the wrong section — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `5207a42` (D444) plus this change (D445). Local host:
Linux, GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only; no desktop build.** Contract:
[LATERAL](../reference/LATERAL.md) §6 (D210). Follows Finding 2 of the
[lateral-run audit](lateral-run-audit.md). Nothing here is validated (rule 4).

## Diagnosis (m2.6 seed 42, vehicle 1133)

A scratch probe (not committed) printed, before each step, vehicle 1133's leader as the step finds
it: `followerFilter`, then `closestVehicle`, over that tick's spans, with each span of the leader.
Switched on as in the audit.

- 1133 is on lane-18 (one section, reference [0, 44]). It starts a change toward lane-17 at 249.6 m,
  with a target of 3.5. Lane-17 is cut into two sections: `lane-17` over reference [0, 37], and
  `lane-17/sec-2` over [37, 44].
- From `lateral` 0.9 on, its body covers lane-17, and `followerFilter` lists **two** covered lanes:
  `lane-17@11.85` and `lane-17/sec-2@0.00`. Both sections neighbour lane-18's one section, but only
  the first is level with the front. Mapped onto `sec-2`, the front is negative and was clamped to 0,
  so the search started at that section's start, 25 m ahead.
- At t = 2028.7 s, vehicle 1126 stands with its rear at the start of `sec-2`, about 28 m ahead. The
  search gives it as the leader at **gap 0**. The hard cap then holds 1133 at 0 m, and the clamp
  fires every tick while it waits there (13 clamps).
- The car-following and the hard cap read the same `closestVehicle` with the same filter, so they
  agreed with each other; both were wrong. Neither suspect from the review was the cause:
  `hiddenLength` and the clamp rule were not involved.

`appendLateralSpans` already skips such a section (`front < rear` after mapping), so a body's spans
were right. Only the leader search was wrong.

## Change

`followerFilter` (`lateral.cpp`) lists a covered lane only when the front, mapped onto that section,
falls within it (to 1e-9 m). The clamp to the section's length stays for rounding.

## Row (`straddlerleader`)

| Row | Case | Before | After |
|---|---|---|---|
| BA160 | A two-lane road whose left lane is cut into sections at 150 m. A straddler stands at 100 m, over the line. A car stands with its rear at the start of the second section, both bodies toward the line | Fails: gap ≠ 50 (the second section was searched from its start) | Leader is the car at gap 50; no clamp over 1 s; the straddler moves off |

The forcing is asserted first: the second section neighbours the straddler's section, and the body
casts a span on the first.

`ctest --preset headless`: **174/174** pass, and `check` passes.

## Product runs, seeds 42–51, `--threads 4`

| Project | Completed, on (D444 → now) | Clamps on (D444 → now) | Clamps off |
|---|---|---|---|
| m2.6-study-template | 1967, 2052, 1994, 1903, 1974, 1957, 1931, 1909, 1930, 1961 → the same except seed 51: **1965** (off: 1965) | **28 → 1** (seed 51) | 1 (seed 51) |
| lane-change-lab, four-leg-signalised, t-junction-priority | unchanged, equal to off | 0 | 0 |

Every switched-off batch is identical to D444's. `followerFilter` returns before this code when the
switch is off.

**Straddles** (the D444 probe), m2.6 on: in seed 42 the longest straddle falls from 5.8 s
(vehicle 1133) to 2.4 s, the same as the other seeds. No vehicle straddles at the end of a run.

## Still open

- **Finding 3** (the swap lock in the lab): not addressed.
- **A body over a line whose move is refused** (`blocked`) still keeps its place. No product run
  shows it now; it is the next guard in NEXT.
- **`followerFilter`'s lookup at the window's end** on the target route (D444's note): not changed.
