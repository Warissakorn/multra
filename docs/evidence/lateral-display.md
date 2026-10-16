# M3.7 slice 4: lateral display evidence — 2026-10-16

Branch `claude/m3.7-slice-4-display`, stacked on `claude/m3.7-slice-3b-zones` (`dea2bea`), plus this
slice (D203). Local host: Linux, GCC 13.3.0, Ninja, Qt 6.4.2, `QT_QPA_PLATFORM=offscreen`.
`--preset desktop`, `ctest --preset desktop` and `cmake --build build/desktop --target check`.
**Linux offscreen only**; Windows and high-DPI belong to the PR's native CI. Contract:
[LATERAL](../reference/LATERAL.md) §10; decisions D180, D189, D203.

## Rows

`tests/lateral_display_ui_tests.cpp` (ctest `lateral-display-ui`): a hand-built straight two-lane Link
(lanes `a1`, `a2`, 3.5 m) and Scenario; the drawn front is read from the run item, as
`lane_change_display_ui_tests.cpp` does.

| Row | Case | Asserts |
|---|---|---|
| BA46 | `lateral` 0.8 on left- and right-hand networks, the Link drawn along +x and −x | The front moves exactly 0.8 to the left of travel (+y, −y), not along the lane; rotation unchanged (1e-9) |
| BA46 | A crossing: on `a1` with `lateral` −1.75 (on the line), then on `a2` with +1.75 and one trace entry | Unflagged, the D97 slide draws it more than 1 m off the line (the forcing); flagged `lateral`, the front is where it was (1e-7) |
| BA46 | A frame drawn every tick for 20 ticks, against the same state stepped undrawn | Equal vehicles every tick |

Also: `lateralmotion`'s BA44 crossing asserts its single trace entry is flagged `lateral`, and
`lane-change-display-ui` asserts the lab's instantaneous change is not.

Failure first: with the core flag in but the canvas unchanged, the offset row failed ("The body is
not drawn at lane centre + lateral"). Mutation: with the canvas ignoring the flag, the crossing row
fails ("A lateral crossing jumps on screen"). Both reverted.

`ctest --preset desktop` **161/161** with this file in place, `check` green. BA39: `trafficsim-cli 42` =
45 completed / 31.75870154816726 s / 1 clamp, and the Release CLI output is byte-identical to main
(`5b97038`). `lane-change-display-ui`, `run-presentation-ui` and `canvas-lod-ui` pass unchanged, so
product pictures, where `lateral` is 0, do not move.

## Not covered here

- The offset is along the body's heading, exact on straight lanes; on a turned body it is off by
  `lateral·(1−cos θ)`. Lateral speed does not yaw the body.
- A lateral crossing while an instantaneous change is still sliding drops that slide.
- No product run sets a lateral target, so no product picture shows an offset yet (M3.4).
- No Windows, high-DPI or Release paint timing; the change adds two multiplications per vehicle.
