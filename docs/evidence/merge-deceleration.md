# B-3: accepted deceleration for mandatory lane changes — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1` on `75e90fd` plus this change (D220, D221). Local
host: Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Contract:
[MERGE_DECELERATION](../reference/MERGE_DECELERATION.md).

## Rows (`tests/merge_deceleration_tests.cpp`, ctest `mergedecel`, BA80–BA85)

The fixture is `lane_fixture`'s three-lane road: a vehicle on `stubA` must change to `full`. Each scene
is found by the car-following model itself, so the scene cannot drift from the model. The test scans
gaps until the braking asked for lies between −2.2 and −3.8 m/s², which is beyond the comfortable 2
and inside the key 4. It also checks that this tick's move fits the room, since no key relaxes that.
The decision is read straight from `decideLaneChanges`.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA79 | Keys absent | `lanechange` and `discretionary` groups unchanged; shipped outputs byte-identical (below) | passes |
| BA80 | Changer at 10 m/s behind a 4 m/s leader on `full` | Forcing: asks below −2. Refused without the key; accepted with `mandatoryDecelerationOwn` 4, no clamp in 5 s. The trailing key alone does not do it | fails |
| BA81 | A 15 m/s follower on `full`; changer and follower have different behaviours | Forcing: the follower asks below its comfortable value. Refused without the key; accepted with the trailing key on the changer's behaviour; still refused with it on the follower's only; no clamp | fails |
| BA82 | Each key at 15 | Every tick equals the key at the car's `maxDeceleration` (8) | passes |
| BA83 | Two full lanes, D95 pair with accepted deceleration 5, both keys at 4 | Forcing: refused by the comfortable rear test. Still refused with the keys; every tick equals the scene without them | passes |
| BA84 | BA80 with a W74 behaviour | Refused without, accepted with | fails |
| BA85 | Codec | Schema 37 round-trips for each key alone and both; 36 carrying a key → `EDIT_UNSUPPORTED_FIELD`; `"4"`, 0, 16 → `INVALID_BEHAVIOUR_PARAMETER`; 15 accepted; 38 → `EDIT_VERSION` | fails |

BA85 first checked the 36 refusal on a document carrying both keys. The refusal names the first key it
meets, so the check for the second key could not pass. Each key is now checked on a document that
carries only that key.

Mutation checks (each reverted):
- **The trailing key read from the follower's behaviour:** BA81 fails.
- **The keys also applied in the discretionary pass:** BA83 fails.
- **Without the `min(key, maxDeceleration)` the contract first wrote:** nothing fails. The mutation
  survived because the car-following model already clamps its acceleration at `−maxDeceleration`
  (`following.cpp`). A key above the maximum therefore can never change an outcome. The `min` was
  dead code, so it was removed and the contract says why (§2). BA82 stays as the statement of that
  equivalence.

**The behaviour editor** has two rows in D217's either-model section. The `behaviour-library-ui` row
failed first (`FAIL: mandatoryDecelerationOwnSet`). The edited copy carries both keys, so that test's
file is schema 37. There is no Lab row, because the Lab clears lane changes.

**Other changes:**
- The codec reads every optional number (curve, kerb, commitment and both mandatory keys) in one
  helper, which keeps `parse.cpp` at its 500-line limit.
- Schema 37 is now valid, so the twelve "future schema is refused" checks moved to 38.

`check` **168/168**.

## Shipped projects (`--seeds 42–51`)

**Keys absent:** byte-identical to `75e90fd`. This was checked for `trafficsim-cli 42` and the JSON
(without `buildCommit`) of all four shipped projects, against the D210 baselines.

**With the keys** on every behaviour (a scratchpad copy of `data/`):

| Project | (own, trailing) | Mean delay (s, ±95 %) | Completed | Clamps per seed | Seed 42: changes / dead-end wait |
|---|---|---|---|---|---|
| `four-leg-signalised` | absent | 46.28 ± 0.82 | 452.0 | 0.0 | 80 / 0 s |
| | (3, 3) | 46.32 ± 0.92 | 452.0 | 0.0 | 80 / 0 s |
| | (4, 4) | 46.35 ± 0.90 | 452.0 | 0.0 | 80 / 0 s |
| | (2, 4) | 46.33 ± 0.92 | 452.0 | 0.0 | 80 / 0 s |
| `m2.6-study-template` | absent | 46.40 ± 0.88 | 1959.5 | 1.1 | 307 / 0 s |
| | (3, 3) | 46.26 ± 0.91 | 1959.3 | 1.1 | 307 / 0 s |
| | (4, 4) | 46.24 ± 0.91 | 1959.0 | 1.2 | 307 / 0 s |
| | (2, 4) | 46.25 ± 0.91 | 1959.4 | 1.1 | 307 / 0 s |
| `lane-change-lab` | absent | 8.98 ± 0.20 | 873.3 | 0.0 | 195 / 2.5 s |
| | (3, 3) | 9.01 ± 0.21 | 873.3 | 0.0 | 195 / 0 s |
| | (4, 4) | 9.04 ± 0.20 | 873.3 | 0.0 | 195 / 0 s |
| | (2, 4) | 9.00 ± 0.20 | 873.3 | 0.0 | 195 / 0 s |

- **The keys barely move the shipped projects.** Their demand seldom meets a mandatory change that
  the comfortable limits refuse, so every movement changes lanes anyway. Only one vehicle in the
  seed-42 lab ever waited at a dead end, for 2.5 s, and with the keys it no longer waits.
- **No clamps appear,** as §3 expects, because an accepted change still has to fit the room.
- **The values do matter in dense merging.** A study of tight Thai merging needs a project dense
  enough to show them. No value ships (D178).

## Performance (callgrind Ir, Release `-g`, `four-leg-signalised` seed 42, keys absent)

| Build | Ir |
|---|---|
| This change | 883.93 M |
| The same build with the three tests reading the comfortable deceleration directly | 883.92 M |

The difference is +0.002 %.

## Not covered here

- Vissim's reduction of the accepted deceleration with distance to the emergency stop.
- A dense merging scene that shows capacity changing with the keys.
