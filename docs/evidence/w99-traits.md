# M3.3.3b W99 driver-trait evidence — 2026-11-24

Branch `claude/vibrant-maxwell-4uq0we` after `3893267` (D640). Local host: Linux, GCC 13.3.0,
Clang 18.1.3, desktop preset. Windows evidence belongs to the packaging CI. Contract:
[W99](../reference/W99.md) §5 (driver trait); decision D641; row BA229 of
[DRIVING_BEHAVIOUR](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows).

## Rows (`tests/w99_traits_tests.cpp`, ctest `w99traits`)

| Row | Case | Asserts |
|---|---|---|
| BA229 | `w99Trait` at (42, 1), (0, 7), (2³²−1, 2⁴⁰) | Bit-equal (`==`) to an independent Python implementation (below); seed 0 is not remapped |
| BA229 | 2,000 ids at seed 42 | Each in [0, 1), repeatable, distinct per id and seed, and different from W74's draws 0–14 of the same vehicle; `w74Traits` unchanged by the factored `w74Uniform` |
| BA229 | `test::straight` vs the same plus an unused `w99` behaviour, seed 42, every tick to the end | Equal `randomState`, `nextVehicleId`, events, vehicles and queues once `w99Trait` is removed; prototype vehicles carry none; every mixed vehicle carries `w99Trait(42, traitIdentity(…))`; `upcomingArrivals` predicts it |
| BA229 | Prototype + `w74` + `w99` vs prototype + `w74`, 600 ticks | Same draws and events; `w74Traits` identical; both traits present and different |
| BA229 | State copied at tick 300 and both stepped 300 more; two full runs at seed 7 | Equal vehicles, trait included |
| BA229 | `perInput` streams, 600 ticks | The hash takes the stream identity (`streamKey`, `ordinal`), not the bare id |
| W74 §9's rule | Checkpoints | `w99Trait` only in the w99 run, with the vehicle's value |

`w99codec`'s unused-behaviour run now compares vehicles without `w99Trait` and asserts it is present.

Golden values (hex doubles) from `python3` with 64-bit masking, W74 §5's `u(15)`:

```
(42, 1)                 0x1.19f600c4886eep-1
(0, 7)                  0x1.4aa7714cb5597p-1
(4294967295, 2^40)      0x1.9047ca15f75a6p-1
```

The same script gives `u(0)` = W74's golden `zEx` for all three, so it is W74 §5's hash.

## Two compilers

A one-off driver (not committed) printed 1,000 values of `zW` (seeds 0, 42, 81, 2³²−1; ids 7919·1..250)
as `%a` after compiling `src/core/w74.cpp` and `src/core/w99.cpp` with g++ 13.3.0 and clang++ 18.1.3
(`-std=c++20 -O2 -fno-fast-math -ffp-contract=off`): `cmp` identical, SHA-256 prefix `d206e1ac8a8750ac`,
and every value equal to the Python script's. MSVC is CI's.

## Mutations

Each fails a case, then was restored: index 14 instead of 15; the trait on every scenario; the trait
drawn from the run's stream; the checkpoint key always written; `>> 12` instead of `>> 11`.

## Byte guards and limits

`multra-cli 42` summary and `--events` output byte-identical before and after (prototype only).
Nothing reads `zW` until composition (BA230); Run still refuses a `w99` behaviour in use, so these rows
use an unused one.
