# M3.7 slice 3a: lateral motion, admission and crossing evidence — 2026-10-15

Branch `claude/amazing-allen-pxg1co` on `4c7b204` plus this slice (D189). Local host: Linux,
GCC 13.3.0, Ninja, nlohmann/json 3.11.3. `--preset headless` and
`cmake --build build/headless --target check` (architecture, file sizes and docs included).
**Linux headless only**; desktop and Windows evidence belongs to the PR's native CI. Contract:
[LATERAL](../reference/LATERAL.md) §4, §7, §10; decisions D180, D188, D189.

## Rows (`tests/lateral_motion_tests.cpp`, ctest `lateralmotion`, 8 tests)

Keys 0/0 (or 0.2/0.5) and `maxLateralSpeed` 1 m/s at dt 0.1 s: a move is at most 0.1 m a tick.
Targets are set by the test; nothing in the product writes one (D189).

| Row | Case | Asserts |
|---|---|---|
| BA43 | Two 0.5 m bodies side by side, 0.15 m apart, each targeting the other's side (the 0.05 m overlap of both moves asserted) | Id 1 moves 0.1 m with `lateralSpeed` 1; id 2 keeps its `lateral` with `lateralSpeed` 0; no 2D overlap |
| BA43 | Both bodies reach y = 0 exactly (binary-exact values, 0.3 m step) | Both admitted: touching is not overlapping |
| BA43 | Id 1 moving into id 2's published body, 0.08 m away | Refused, `lateral` kept, `lateralSpeed` 0, although id 2 is later; at 0.7 m the same target is admitted |
| BA43 | Four cars with targets across two lanes; a `SimState` copied after 12 ticks | Both copies give equal vehicles and events for 60 ticks |
| BA44 | A car in the left lane targeting the right lane's centre; the compiled spans asserted present | One `LaneChangeEvent`, from a centre within one step of the line; route changed; the tick's `MovedEvent` names the right lane; the absolute target and body never jump; the move per tick never exceeds 0.1 m; ends centred |
| BA44 | The same with the spans removed | The route never changes; the centre ends exactly on the lane line |
| BA44 | Each lane's car targeting 5 m outward | The body's outer edge stops exactly at ±3.5 m and never passes it |
| §6–§7 | Eight cars on two lanes swapping lanes every 5 s for 30 s | No 2D overlap at any tick; more than four crossings (the forcing) |

The span tests' helpers moved to `tests/lateral_test_support.hpp`, shared by both files.

Mutation checks (each reverted): without the admission test, both refusal rows and the overlap
invariant fail; an inclusive overlap fails the touching row; skipping the re-expression fails the
crossing row; ignoring every span fails the crossing row and the invariant's forcing.

## Not covered here

- **Zones** still read route positions (slice 3b, D188/D189).
- **No product writer** of `lateralTarget`: M3.4's lane-change duration and M3.7.1's desired
  position come later. Crossings on a Connector cannot happen (no spans there).
- **Hand mirrors** stay one-dimensional: `placeVehicle`, Lab telemetry and `dead_end_waits.cpp`.
