# M3.7 slice 2: lateral spans and predicates evidence — 2026-10-15

Branch `claude/amazing-allen-pxg1co` on `3a487d8` plus this slice (D188). Local host: Linux,
GCC 13.3.0, Ninja, nlohmann/json 3.11.3. `--preset headless` and
`cmake --build build/headless --target check` (architecture, file sizes and docs included).
**Linux headless only**; desktop and Windows evidence belongs to the PR's native CI. Contract:
[LATERAL](../reference/LATERAL.md) §3–§6, §8, §10; decisions D180, D187, D188.

## Rows (`tests/lateral_span_tests.cpp`, ctest `lateralspan`, 7 tests)

| Row | Case | Asserts |
|---|---|---|
| BA42 | `lateralDistance` at 0, 25, 50 and 70 km/h; no keys | 0.5, the exact interpolation, 1.0, 1.0 (keys 0.5/1.0); 0 without keys |
| BA40 | A car pushed into the next 3.5 m lane by 0.0101, 0.01 (in decimal) and 0.005 m (each covered width asserted first); a 0.02 m body centred on the line, covering exactly the double `kLateralOverlap` (asserted) | A span on the other lane only past the tolerance; none at exactly it |
| BA40 | That 0.0101 m straddler, standing, with a centred car at 10 m/s behind on the other lane, `dLat` 0 and 0.85 m | It brakes only when `dLat` closes the 0.8399 m between the bodies; a centred car next door never makes it brake |
| BA41 | Two 0.5 m bodies at ∓0.75 m in one 3.5 m lane, 1.0 m apart across, the rear one 1.5 m behind at 8 m/s; `dLat` 0.9, 1.0 (touching) and 1.1 m; 40 ticks | Passes alongside only at 0.9; follows at 1.0 and 1.1; no two bodies overlap in 2D at any published tick |
| D188 | A two-lane Link with a bend | The lanes differ in length (asserted); both reference ends agree; `mapStation` takes end to end and middle to middle |
| D188 | The switch on with the cross-sections removed | `validateScenario` refuses (`INVALID_CROSS_SECTION`) |
| BA39 | Four-leg project with owned catalogs and keys 0.5/1.0 on every behaviour (cross-sections and the index asserted on) | Vehicles and events equal the lane-based run on every tick, seed 42: centred cars narrower than their lanes reach no other lane |

`lateral_codec_tests.cpp`'s BA39 test now checks only the key-absent scenario and index; its
"keys present, no runtime reader" half moved here, with the switch really on.

Mutation checks (each reverted): `<=` → `<` at the tolerance fails the exact-tolerance case;
removing the lateral filter fails both BA40 and BA41; an identity station mapping fails the bend test.

`check` **102/102**. Also kept green: every other group, the CLI pin (`trafficsim-cli 42` = 45 completed /
31.75870154816726 s / 1 clamp) and the frozen baselines. With the switch on, the CLI's
four-leg `--seeds 42-51` output is byte-identical to the switch-off output; a planted out-of-range
key in the same data directory is refused, so the keys were really read.

## Performance (LATERAL §11)

- **Switch off:** `trafficsim-engine-benchmark 40 1800`, Release, five interleaved pairs against
  `3a487d8`: medians **3388 ms before, 3312 ms after**, inside noise.
- **Switch on:** Release `trafficsim-cli --project four-leg --seeds 42-51 --threads 1`, five
  interleaved pairs of whole-process wall time: medians **958 ms off, 1035 ms on (+8 %)**. That is
  under the 10 % budget but close to it. The neighbour scan and the band per span run for every
  vehicle every tick even when nothing straddles; slice 3 should re-measure with lateral motion.

## Not covered here

- **Zones (owner's decision, D188):** a straddler occupies only its own route's zone; slice 3.
- **`around`'s alongside filter** is exercised only through the code path it shares with the leader
  filter (`lateralCounts`). No test drives a lane change next to a straddler. BA44 in slice 3 adds
  lane-change crossings.
- **Priority rules** see a straddler's other-lane span through the existing give-way scan. No
  dedicated row.
- **Hand mirrors stay one-dimensional:** `placeVehicle`, Lab telemetry and `dead_end_waits.cpp`.
