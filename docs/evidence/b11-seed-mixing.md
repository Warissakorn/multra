# B11 — seed mixing (D159), 2026-10-14

Owner's answer (sitting B11, 2026-10-14): "B11 แก้" — fix it, as a recorded baseline change.

## The defect

`createSimulation` seeded xorshift32 with the seed itself. For small seeds the first output is
tiny: across seeds 42–81 the first draw is about 0.003. The first input in id order therefore
gets its first arrival `-ln(0.003) ≈ 5.8` mean headways late. That is about 800 s at 27 veh/h
and a few seconds at 600 veh/h. Every study run with the D88 seeds had this.

## The change

`randomState = high32(splitmix64Mix(seed + 0x9E3779B97F4A7C15))`. That is one splitmix64 step:
add the golden gamma, then apply Steele, Lea and Flood's finaliser. If the result is 0, the
existing constant `0x6d2b79f5` is used instead.

- **`state.seed` is still the user's seed.** W74 trait hashing (D129), reports and replay are
  keyed on it unchanged. W74's trait base is `mix(seed)` with no gamma added, so it does not
  equal the new random state.
- **The finaliser now lives in one place,** `detail::splitmix64Mix`, shared by W74 and seeding.
  W74 traits are bit-identical: the `w74traits` group passes unchanged.
- **`SeedStart::raw` keeps the pre-D159 mapping.** It is used only by `tests/reference_tests.cpp`.
  The four frozen TypeScript fixtures and `trajectory-digest.json` are **not regenerated** and
  pass unchanged under it (AGENTS rule).

Rows (group `seedmixing`). SM1, SM2 and SM4 failed before the change; SM3 passed both before and
after, since it pins the old mapping:

| Row | Check | Before | After |
|---|---|---|---|
| SM1 | First draw over seeds 42–1041: mean 0.5 ± 0.03, under 5 % below 0.05 | fail | pass |
| SM2 | Replay is exact, `state.seed` is the user's, and the state is not the seed | fail (state = seed) | pass |
| SM3 | Seeds 0, 1 and 2³²−1 give a non-zero state; raw keeps the old map | pass | pass |
| SM4 | One 27 veh/h input, seeds 42–81: mean first arrival within 3600/27 ± 1.96·SE | fail | pass |

OD3 (`odexpand`) now uses the D88 seeds 42–81 instead of the well-mixed seeds it needed before.
Its Poisson band holds.

## Expected numbers that changed

| Where | Before | After |
|---|---|---|
| `trafficsim-cli 42` (CTest `cli`, editor `scenario-run-ui`) | 31 completed, mean delay 29.249359418430977 s, 0 clamps | 45 completed, mean delay 31.75870154816726 s, 1 clamp, mean travel time 53.151111111111106 s |

More trips complete because the first vehicles now arrive on time instead of minutes late. The
one clamp is an ordinary trajectory event in the new draw. It is not new behaviour; the
standstill cap clamps in every model, sitting B5. `scenario-run-ui` is a Qt test and was not run
here: the desktop build needs Qt, which this machine does not have.

## Tests whose seed-42 pass was luck

Four tests asserted a property that held only for seed 42's old draws. Each was checked over
seeds 42–81 under both mappings before it was changed. None was re-pinned to a new number.

- **`movement.delay_responds_to_red_time`**
  - The test required green-run mean delay < 4.5 s, calling it "unimpeded".
  - Under the old seeding seed 42 gave 4.27 s. Under the new seeding, seeds 42–81 give
    3.4–12.2 s, because at 900 veh/h Poisson platoons add following delay.
  - The 3 s entry-acceleration floor (> 2.5) stays; the upper bound is removed with that
    reasoning. The red-versus-green check (> +10 s) is unchanged.
- **`m26study.runs_an_hour_with_every_movement_delivering`**
  - The test required `pending == 0`. Seed 42 now leaves one vehicle due at 3599.59 s, which
    had no tick to enter. Seeds 43–46 leave none.
  - It now counts only vehicles due more than 5 s before the end, which is what a jam would show.
  - OD3 has the same last-tick case; it now allows queue entries due in the last second.
- **`discretionary` A53 stream** (renamed `a_steady_two_lane_stream_never_clamps_and_a_hold_stops_quick_changes`)
  - The test required no change back within 3 s with no hold.
  - Under the old seeding, 24 of seeds 42–81 had such a repeat (33 in 952 changes); under D159,
    25 (44 in 1,023). Only seed 42's old draws had none.
  - This is the recorded A53 failure (D102, sitting B2), not a new one.
  - The test now asserts:
    - no clamp, with and without the hold;
    - repeats exist without the hold (the forcing);
    - none with D101's 3 s hold.
- **`w74run` BA23**
  - The test required that a vehicle at rest inside ABX never starts. Under D159, seed 42 has
    vehicle 4 at rest, gap 2.58 m, ABX 2.87 m, leader pulling away at 0.69 m/s, and it gets
    +0.0049 m/s².
  - The emergency formula `0.5·dv²/(AX−g) + emergencyLeaderWeight·aL + bMinAdd·(ABX−g)/BX` can
    be positive when the leader's own acceleration outweighs the dv² brake.
  - The engine follows the cited formula. W74.md and BA23 overstated it ("does not start until
    g > ABX"), and both now say this.
  - The test now asserts that such a start happens only while the leader accelerates.

## Not changed

- The historical evidence files and decision rows that quote numbers from seeds 42–81 under the
  old seeding: D88's ±1 s comparisons, M2.6 batch figures, the A53 shares in
  `m3.2.8c-discretionary.md`. They describe runs made at the time. A re-run of any of them
  under D159 gives different per-seed numbers.
- `docs/reference/MIGRATION.md`'s table, which is the TypeScript baseline under raw seeding.
