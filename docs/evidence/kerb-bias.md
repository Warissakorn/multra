# M3.4 keep to the kerb lane, slice 1 — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1` on `cd93f86` plus this change (D215). Local host:
Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Contract:
[OVERTAKING](../reference/OVERTAKING.md) §2–§4 (D213).

## Rows (`tests/kerb_bias_tests.cpp`, ctest `kerbbias`, 6 tests; BA64 and BA69 below)

The fixture is D95's hand-built two-lane road, lengthened to 400 m. L1 is the kerb lane, so the span
L2 → L1 carries `towardKerb`. Every behaviour carries the D95 pair (threshold `T`, accepted
deceleration 1) and, when given, the bias `β`.

All six failed on `cd93f86`, each at its own assertion.

| Row | Case | Asserts |
|---|---|---|
| BA65 | A free car alone on L2, `T` 0.5, `β` 1 (free mode asserted) | It changes to L1; without the bias it stays (gain 0 < `T`) |
| BA67 | A47's scene: a 3 m/s vehicle (desired speed 3) on L1, a 12 m/s car 20 m behind; `β` 1, D101 hold 2 s | The gain with the bias is asserted ≥ `T + β`. It passes on L2, returns to L1 once ahead, arrives on L1; no clamp |
| BA68 | The car held on L2 by the slow vehicle, L1 free, `β` 1 | `T` set so the gain is 0.1 below `T + β`: no change, though the same `T` without the bias changes. 0.1 above: it undertakes |
| BA66 | A compiled two-lane Link on left- and right-hand traffic, with lateral keys for cross-sections | One span each way. The `towardKerb` span targets the lane on the kerb side: the larger centre on left-hand traffic, the smaller on right-hand |
| BA71 | Range and the pair | 1 and 5 accepted. 0, −1, 5.01, NaN and ∞ give `INVALID_BEHAVIOUR_PARAMETER`; the key without the D95 pair gives `INCOMPLETE_DISCRETIONARY_BEHAVIOUR` at `.kerbLaneBias` |
| BA70 | Codec | A document without the key stays below 35. With it, schema 35 round-trips. 34 carrying the key gives `EDIT_UNSUPPORTED_FIELD`; a string gives `INVALID_BEHAVIOUR_PARAMETER`; 36 gives `EDIT_VERSION` |

BA67 first left vehicle 1 at the default desired speed. It accelerated to 15 m/s and drove level
with the car, so a return was never safe. The row now keeps it slow, as the scene intends.

Mutation checks (each reverted):
- With `T + β` for every move (no return discount), BA65 and BA67 fail.
- With the discount for every kerbward move (undertaking at `T − β`), BA68 fails.

The behaviour editor shows `kerbLaneBias` as an optional row beside the D95 keys. The
`behaviour-library-ui` test edits it together with the D95 pair and checks the value. Its file
becomes schema 35, which updated that test's expectation from 34. The ten codec tests' "future
schema" checks now use 36. `parse.cpp` reached 505 lines, so the curve and kerb readers share one
helper (500).

`check` **166/166**.

## BA64: key absent

Byte-identical to `cd93f86`:
- `trafficsim-cli 42`;
- the `--seeds 42-51` JSON (without `buildCommit`) of all four shipped projects;
- the `discretionary` group, unchanged and passing.

## BA69: A53 with the bias (a measurement)

A53's stream: the 200 m two-lane road, 1100 and 700 veh/h, 300 s, `T` 0.5. Seeds 42–81 were summed,
counting changes made within 3 s of the same vehicle's previous change ("repeats"). The cap is 1 %.

| `β` | Changes | To the kerb | Repeats (no hold) | With a 3 s hold |
|---|---|---|---|---|
| absent | 1023 | 305 | 44 (4.30 %) | 0 |
| 0.25 | 857 | 233 | 33 (3.85 %) | 0 |
| 0.5 | 2634 | 1837 | 189 (7.18 %) | 0 |
| 1 | 2223 | 1632 | 139 (6.25 %) | 0 |
| 2 | 1611 | 1403 | 53 (3.29 %) | 0 |
| 3 | 1238 | 1238 | 0 | — |
| 5 | 1275 | 1275 | 0 | — |

- With `β ≥ T`, every free car returns, so returns multiply and so do repeats.
- At `β` 3 and 5 every change is a return, and nobody passes, because `T + β` exceeds any gain this
  stream offers. Zero repeats there means no overtaking, not a passing behaviour.
- No `β` in (0, 5] gives both passing and fewer than 1 % repeats without D101's hold. A53 therefore
  stays failed, and no D95 value ships (OVERTAKING §5). The hold is what stops quick changes, as
  before.
- Clamps are 0 in every row.

## Not covered here

- Lane preference by vehicle type (B-7).
- A hand-written scenario's spans default to `towardKerb` false (OVERTAKING §3).
- The bias in the Behaviour Lab waits on the circuit lane-change wrap rule.
