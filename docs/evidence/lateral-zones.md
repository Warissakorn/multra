# M3.7 slice 3b: zone occupancy by span evidence — 2026-10-16

Branch `claude/m3.7-slice-3b-zones` on `5b97038` plus this slice (D202). Local host: Linux,
GCC 13.3.0, Ninja, Qt 6.4.2 installed. `--preset headless`, `ctest --preset headless` and
`cmake --build build/headless --target check`. **Linux headless only**; desktop and Windows evidence
belongs to the PR's native CI. Contract: [LATERAL](../reference/LATERAL.md) §5, §6, §10; decisions
D188, D202.

## Rows (`tests/lateral_zone_tests.cpp`, ctest `lateralzone`, 6 tests)

A two-lane, 3.5 m Link and a crossing segment `north`; the zone's area is 148..152 on the right
lane and 98..102 on `north`. The straddler is on the left lane with `lateral` pushing its body
0.3 m into the right lane; keys 0/0 and `maxLateralSpeed` 1. Every row first asserts the
straddler's `own = false` span on the right lane (the forcing).

| Row | Case | Asserts |
|---|---|---|
| BA47 | Right lane major; the straddler standing over the area; a minor car on `north` at 80 m, 8 m/s | `majorInside`; the minor car never passes its line at 90 in 80 ticks. Centred, the same car leaves the area free and the minor car passes 102 |
| BA47 | The body covering half of `kLateralOverlap` | No span on the right lane; nothing blocks |
| BA47 | Right lane minor (waiting at 140), `north` major; the straddler standing over the area | It is the only holder; the major car waits at entry 98. Centred, it passes 102 |
| BA47 | The straddler at 10 m/s, front 140 / 125 / 100 (headway 10 m, gapTime 3 s) | Blocks / blocks / free, never `majorInside` |
| BA47 | Headway 0, gapTime 0.01 s; the straddler's front 147.5 moving 1 m, a minor car crossing 90 this tick | The snapshot does not block; `resolveRequests` caps the minor car at 0.5 m. Centred: no cap |
| BA39 | Keys absent, `lateral` 1.2 set on the vehicle | Nothing compiled; route positions alone decide; `zoneSides` empty |

Mutation checks (each reverted): without the span scan, four BA47 rows fail; without approach,
the anticipation row; without the swept span check, the swept row; without span holders, the
minor-area row.

`ctest` **105/105**, `check` green (architecture, file sizes, docs). CLI pin: `trafficsim-cli 42`
= 45 completed / 31.75870154816726 s / 1 clamp; the Release CLI output is byte-identical to `5b97038`.

## Performance (LATERAL §11)

Release, five interleaved pairs against `5b97038` (main) on a shared 4-core container, so the
spread is wide:
- **Switch off:** `trafficsim-engine-benchmark 40 1800` run time, medians **5949 ms main,
  5521 ms branch**, inside noise (range 5321–6270 vs 5280–6113 ms).
- **Switch on, no targets:** keys 0.5/1.0/1 on every catalog behaviour (a copied data directory;
  an out-of-range key there was refused, so it is read), `trafficsim-cli --project <p> --seeds 42-51
  --threads 1`, whole-process wall: `t-junction-priority` medians **323 → 300 ms**,
  `four-leg-signalised` **1458 → 1558 ms** (+7 %, ranges 1417–1762 vs 1445–1683 ms). Outputs are
  identical to main's switched-on runs: no product run straddles. Below D164's 10 % threshold, but
  the four-leg figure is not a no-change claim; M3.4 re-measures with real targets.

## Not covered here

- **The straddler itself is not held** at the other lane's zone (D202); M3.4 decides with real targets.
- Approach on segments upstream of the chain is not seen; the swept move is mapped one to one.
- The hand mirrors stay one-dimensional: `placeVehicle`, Lab telemetry, `dead_end_waits.cpp`.
- No desktop, Windows or high-DPI run.
