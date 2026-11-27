# M3.3.3b W99 CC1 draw evidence — 2026-11-24

Branch `claude/vibrant-maxwell-4uq0we` after `7c2f673` (D646). Local host: Linux, GCC 13.3.0, desktop
preset. Contract: [W99](../reference/W99.md) §5a; decision D647; rows BA234–BA236 of
[DRIVING_BEHAVIOUR](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows).

## Rows (`tests/w99_cc1_run_tests.cpp`, ctest `w99cc1run`)

| Row | Case | Asserts |
|---|---|---|
| BA234 | `w99Cc1Share` at (42, 1), (0, 7), (2³²−1, 2⁴⁰); 2,000 ids at seed 42 | Bit-equal to an independent Python implementation of W74 §5's `u(16)` (below); each in [0, 1) and different from the same vehicle's draws 0–15 |
| BA234 | `test::straight` vs the same plus an unused curved behaviour, and plus a fixed `w99` one, seed 42, to the end | Equal `randomState`, events, vehicles and queues once the share and `zW` are removed; every curved-scenario vehicle carries `u(16)` of its stream identity, predicted by `upcomingArrivals`; the fixed scenario draws no share; checkpoints carry `w99Cc1Share` only when present; a copied state replays exactly |
| BA235 | `w99Cc1At` on `(0.6, 0), (0.9, 0.5), (1.5, 1)`; a flat stretch; a two-point curve and a speed curve over 1,001 shares | 0.6, 0.75, 0.9, 1.2, 1.5 at shares 0, ¼, ½, ¾, 1; nobody on the flat stretch; `a + u·(b − a)` and `speedAt` equal to the bit |
| BA235, BA236 | Two cars behind standing trucks, one on a curved behaviour and one on another curve (`segmentBehaviours`) | Each published acceleration equals `w99Acceleration` with the driver's own CC1 at its share; the share is unchanged |
| BA236 | `desiredGap`/`stopLineReach` at shares 0.1, 0.5, 0.9; two drivers with shares 0.1 and 0.9 behind the same leader | The accessors read the driver's CC1; a curved behaviour without a share is a logic error; the two decisions differ and each equals `w99Acceleration` with its CC1 |
| BA236 | A curved behaviour used by the car type, 900 veh/h for 120 s; the owned-project case in `w99cc1codec` | It validates and runs (trips complete), with no `UNSUPPORTED_*` row |

The fixed-CC1 group `w99run` (D642) passes unchanged.

Golden values (hex doubles), `python3` with 64-bit masking, W74 §5's `u(16)`:

```
(42, 1)              0x1.510077babb480p-7
(0, 7)               0x1.3f190912b46b8p-2
(4294967295, 2^40)   0x1.fd6398c1ed0e0p-4
```

## Mutations

Each fails a case, then was restored: index 15 instead of 16; the share drawn on every scenario; CC1
not replaced by the driver's value; `desiredGap` ignoring the share; D646's refusal kept.

## Byte guards and limits

`multra-cli 42` summary and `--events` output byte-identical. No curve ships; no calibration claim. The
behaviour dialog still declines a curved behaviour until BA237.
