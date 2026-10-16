# Swapping pair at shared dead ends: the rear one yields — 2026-10-16

Branch `claude/m3.4-lane-change-duration-slice1` on `621c7b6` plus this change (D207). Local host:
Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Contract:
[M3_8_CONTRACT](../reference/M3_8_CONTRACT.md) §2 "Cooperation"; decision D207 (owner's choice).

## The failure

Found by D206's switched-on runs ([lane-change-duration](lane-change-duration.md)), and present with
the switch off. In `lane-change-lab`, seed 44, vehicle 234 (route-30's stub on lane 1, needing lane 2)
and vehicle 237 (route-29's stub on lane 2, needing lane 1) drove 3.7 m apart, closer than a car
length, from 520 m to their shared dead end at 600 m. Each lay alongside the other's target place,
so rule 5 refused both every tick. D90's cooperative braking slowed 237, but both slowed together
and stayed level. Both crept to 598 m and stood there; 183 vehicles behind them never finished.

## Rows (`tests/lane_swap_tests.cpp`, ctest `laneswap`, 4 tests)

A two-lane Link with two families: A full on the left lane and a stub on the right, B the reverse;
both stubs end at 250 m. Vehicles at 12 m/s. Rows 1–3 failed on `621c7b6` (neither changed).

| Row | Case | Asserts |
|---|---|---|
| 1 | B's stub at 100 m, A's stub at 97 m (both stubs, 3 m apart, asserted) | First tick: the rear one brakes at exactly the comfortable 2 m/s², the front one not at all; both change within 40 s, the front no later than the rear; no clamp; later braking within D90's 3 m/s² |
| 2 | Level at 100 m, then the same with the ids on the other stubs | Vehicle 2 yields in both; both change, vehicle 1 no later; no clamp |
| 3 | Row 1 with the lateral switch on (keys 0/0/1) | Both change, front no later; no clamp; no 2D overlap |
| 4 | A full-route vehicle alongside a stub | Not a partner: it does not brake, and the stub stays on its route |

Mutation checks (each reverted): without the hold rows 1–3 fail; with the tie-break reversed row 2
fails. A first version of rows 1–2 compared event order only and survived the tie-break mutation,
since both changes can fall in one tick and events come in id order; they now assert who brakes.

`check` **163/163**. `trafficsim-cli 42` output is byte-identical to `621c7b6`.

## Shipped projects (`--seeds 42-51`, against `621c7b6`, `buildCommit` ignored)

| Project | Change |
|---|---|
| `t-junction-priority` | identical |
| `lane-change-lab`, switch off | **seed 44: 680 → 863 completed, 183 → 0 active**; mean delay of other seeds +0.00–0.07 s |
| `four-leg-signalised`, switch off | 5 seeds differ; mean delay by ≤ 0.01 s; completed counts unchanged |
| `m2.6-study-template`, switch off | every seed's output differs; mean delay by ≤ 0.11 s; seed 46 has one clamp fewer (1 → 0); completed unchanged |
| `lane-change-lab`, switch on (keys 0.5/1.0/1) | **seed 44: 693 → 863; seed 49: 626 → 852**, no seed stalls; seed 49 keeps its 2 mid-change clamps (D206) |
| `four-leg-signalised`, switch on | 6 seeds differ; mean delay by ≤ 0.20 s; counts unchanged |

Mutual pairs form in the shipped four-leg and M2.6 networks too, so their multi-seed numbers move
slightly; reports published before D207 are not re-run here.

## Not covered here

- A pair that forms level **at standstill** (both arriving at their dead ends in the same tick)
  cannot separate: the rear one cannot reverse. Not seen in any run above.
- Chains across three or more lanes are not exercised.
