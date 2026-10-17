# M3.6 slice 1: curve speed — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1` on `f7c7928` (D211, the contract) plus this change
(D212). Local host: Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug)
and `cmake --build build/desktop --target check`. **Linux only.** Contract:
[SPEED_CONTROL](../reference/SPEED_CONTROL.md) §2–§5.

## Rows (`tests/curve_speed_tests.cpp`, ctest `curvespeed`, BA56–BA63, 8 tests)

The fixture is built with the project commands:
- an 80 m Link heading east;
- a default one-lane Connector turning left onto a Link heading north from (15, 15);
- a route over all three, and owned catalogs from `data/` with every behaviour carrying the key.

The compiled curvature of the Connector path is asserted first: a radius between 10 and 20 m.

| Row | Case | Asserts | Before the bound |
|---|---|---|---|
| BA56 | Key absent | Every segment's `curvature` is 0 | passes |
| BA57 | Key 3, desired 13.9 m/s (`vC` ≈ 6.70 m/s) | Speed on the Connector ≤ `vC` + 1e-6 at every tick (> 10 ticks there) | fails |
| BA58 | Same | ≤ `vC` on entry; before it, never above `sqrt(vC² + 2·b·d)`; slowing 2 s out; no acceleration below −2 m/s²; no clamp | fails |
| BA59 | A straight Connector | Curvature 0; trajectory identical to key-absent | passes |
| BA60 | Key 0.5, desired 2 m/s (`vC` > 2) | Trajectory identical to key-absent | passes |
| BA61 | A follower 20 m behind a turner; a 3 m/s leader | No clamp, never closer than a car length; behind the slow leader, the follower's speed on the turn ≤ 3.5 m/s | passes |
| BA62 | BA57 under W74 (traits as generation gives) | The same bound; no clamp | fails |
| BA63 | Codec | Schema 34 round-trips; 33 carrying the key → `EDIT_UNSUPPORTED_FIELD`; a string or a value outside (0, 10] → `INVALID_BEHAVIOUR_PARAMETER` at its path; 10 accepted; a document without the key keeps its schema; 35 → `EDIT_VERSION` | — (no key yet) |

BA58 first asserted the speed on the last tick *before* the turn ≤ `vC`. The run showed the vehicle
0.08 m short of the turn at 6.718 m/s, which is on §4's braking profile, and arriving at exactly
6.698 m/s on the first tick on the turn. The row now asserts the contract's wording: at or below
`vC` on entry, and never above the profile before it.

Mutation checks (each reverted):
- With the bound applied only on the curve itself (no anticipation), BA57, BA58 and BA62 fail: the
  vehicle enters too fast.
- Measuring `d` from the front's current place instead of where it will be after the tick, the same
  three fail: a capped vehicle lags the profile by a tick.

Schema 34 is now valid, so the "future schema is refused" checks in nine codec tests now use 35.

`check` **165/165**.

## BA56: switched off

Byte-identical to `2d8bc51`:
- `trafficsim-cli 42`;
- the `--seeds 42-51` JSON (without `buildCommit`) of all four shipped projects;
- `lane-change-lab` and `four-leg-signalised` with the lateral keys on.

## Switched on (a scratchpad copy of `data/` with the key 3 on its one behaviour, seeds 42–51)

| Project | Mean delay | Completed / active | Clamps |
|---|---|---|---|
| `four-leg-signalised` | +3.4 to +13.2 s per seed (e.g. seed 42: 46.6 → 53.2 s) | 2–12 fewer completed per seed, the same more active | 0 → 0 |
| `t-junction-priority` | +0.7 to +1.6 s per seed | unchanged | 2 → 3 |

Turners slowing for their curves is the intended effect (review §3 E), and delay is measured against
the desired speed (owner, §5). The clamps are the existing priority-rule clamps of `t-junction-priority`:
- switched off, seed 46 has two;
- switched on, seeds 42, 47 and 50 have one each.

The seed-42 clamp is vehicle 110, waiting on a Connector at 0.7 m/s and braking at 3.45 m/s². That
is beyond the 2 m/s² the curve bound ever asks for, at a give-way line. Arrival times shift, so the
priority clamps move between seeds. No shipped value is set (D178).

## Performance (callgrind Ir, Release `-g`, seed 42, `four-leg-signalised`)

| Run | Ir |
|---|---|
| D209 (`53ba453`), switch off | 878.8 M |
| D210 (`2d8bc51`), switch off | 893.6 M (+1.7 %) |
| D212, switch off | **886.3 M** (+0.85 % on D209) |
| D212, key on | 994.9 M; `speedTarget` 45.4 M = **4.57 %**, inside §7's 5 % |

The rest of the switched-on increase is more traffic: slower turns leave more vehicles active.

**D210's switched-off regression.** D210 moved `closestVehicle` out of `simulation.cpp`, where it had
been inlined. That cost the switched-off run 1.7 %, which LATERAL §11 says must not happen and which
D210 did not measure.
- Making the whole function inline, covered scan included, made it worse (906 M).
- Now `closestVehicle` is inline in `leaders.hpp`, and the covered-lane scan is the out-of-line
  `coveredLeader`, called only when a lane is covered.
- `speedTarget` is called only when the behaviour carries the key.

The residual +0.85 % has no single source in the profile; it is spread through `stepSimulation`.
Switched-on lateral cost improves too: `lane-change-lab` 1662 → 1648 M (+5.1 % against `5847cb9`).

Starting `speedTarget`'s scan at the front part with `frontPart`'s binary search cost more than it
saved on these short routes (52 M instead of 45 M), so it was reverted.

## Not covered here

- No Link curvature, reduced-speed areas, desired-speed decisions, speed-dependent acceleration or
  grades (SPEED_CONTROL §1, later slices).
- The radius depends on the vertex count of a non-circular Connector (§3); turning speeds are
  measured in M6.
- The display draws the engine's speed; no display change was needed.
