# B-4: driver-factor spread — 2026-10-18

Branch `claude/m3.4-lane-change-duration-slice1`, on `92f7f8a` (D235's contract) plus this change (D236).

- **Host:** Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores.
- **Build and check:** `--preset desktop` (Debug), then `cmake --build build/desktop --target check`.
- **Linux only.**
- **Contract:** [DRIVER_FACTOR](../reference/DRIVER_FACTOR.md).

## Rows (`tests/driver_factor_tests.cpp`, ctest `driverfactor`)

The engine rows run `test::straight()`'s road with one car input of 1800 veh/h for 300 s, about 150
drivers, at seed 42. "Before the change" means the same rows run with the field present but
`samplePending` still using 0.15.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA131 | No key | Sample deviation 0.15 ± 0.03, every factor in [0, 1] | passes (it pins today) |
| BA132 | Spread 0 | Every factor is 0.5; arrival times and desired speeds equal to no key, vehicle by vehicle | fails |
| BA133 | Spread 0.3 | Deviation more than 0.08 above no key's, 0.3 ± 0.05; mean 0.5 ± 0.05; inside [0, 1]; same stream | fails |
| BA134 | Spread 0.15 | Same stream and the same factors as no key, bit for bit | passes (it pins the equal doubles) |
| BA135 | Heavy vehicles on a copy of the behaviour with spread 0 | Every heavy factor 0.5; cars keep 0.15 | fails |
| BA136 | Type behaviour spread 0; both roads assigned a behaviour with 0.3 | Every factor is 0.5 on every tick, including on the second road | fails |
| BA137 | Core validation | −0.1, 0.6 and ∞ refused on both models; 0 and 0.5 accepted | written after the code; proved by mutation |
| BA137 | Codec | Writes schema 43 and round-trips; 42 is refused at `behaviours[0].driverFactorSpread`; −0.1, 0.6 and a string are refused (`INVALID_BEHAVIOUR_PARAMETER`); removing the key returns the file to its old schema; the compiled behaviour carries it | written after the code; proved by mutation |

**Mutations,** each removed in turn:

| Mutation | Caught by |
|---|---|
| No parse | BA137 codec |
| No writer | BA137 codec |
| No schema-43 term | BA137 codec |
| No refusal below 43 | BA137 codec |
| No library key | BA137 codec |
| No validation, prototype branch | BA137 core and codec |
| No validation, W74 branch | BA137 core |
| No demand hook | BA132, BA133, BA135, BA136 |
| No dialog row | `behaviour-library-ui` (`driverFactorSpreadSet`) |

**Dialog (`behaviour-library-ui`).** The either-model block now also checks that the
`driverFactorSpread` row is off by default, ticks it, sets 0.3, and expects 0.3 on the edited copy
only. The edited W74 file now writes schema 43 instead of 41: this is the newest key it carries.

`check` **176/176**: 175 tests plus the new `driverfactor`.

## Shipped runs

**Byte-identical** (no shipped behaviour carries the key):
- `trafficsim-cli 42` against D210's baseline;
- the `--seeds 42–51` output (without `buildCommit`) of all four shipped projects against D225's
  baseline.

**With the key:** scratchpad copies of the data directory with `driverFactorSpread` on the shared
`wiedemann-inspired-prototype`. Seeds 42–81, mean delay in seconds (95 % half-width) and safety
clamps summed over the 40 runs:

| Project | No key (0.15) | 0 | 0.3 | 0.5 |
|---|---|---|---|---|
| `four-leg-signalised` | 46.00 (±0.55), 0 | 45.95, 0 | 45.94, 2 | 46.01, 4 |
| `t-junction-priority` | 7.07 (±0.48), 6 | 7.04, 3 | 7.05, 7 | 7.07, 4 |
| `lane-change-lab` | 8.92 (±0.09), 1 | 8.92, 0 | 8.92, 4 | 8.91, 4 |
| `m2.6-study-template` | 46.01 (±0.38), 41 | 46.07, 37 | 46.03, 45 | 46.04, 52 |

Completed trips do not change by more than 0.1 vehicles per run.

- **Mean delay does not move.** Every change is well inside the half-width. The clamp at 0 and 1 is
  symmetric about 0.5, so the mean factor, and with it the mean safety distance, stays the same.
  Only how the gaps are spread changes.
- **A wide spread adds clamps.** On the four-leg and M2.6 projects, clamps rise steadily with the
  spread. At 0.5 about 16 % of drivers sit at a factor of 0, with the shortest safety distance.
  Spread 0 gives the fewest clamps on every project (tied with no key on the four-leg). The counts are small, so this is a
  tendency, not a rate.

## Performance (callgrind Ir, Release `-g`, seed 42, no key set)

Both builds were made in the same build directory.

| Project | `92f7f8a` | This change |
|---|---|---|
| `four-leg-signalised` | 890.32 M | 890.44 M (+0.013 %) |
| `lane-change-lab` | 1250.36 M | 1250.18 M (−0.015 %) |

The hook reads one optional value when each vehicle is made.

## Not covered here

- A mean, another distribution or a spread by road (DRIVER_FACTOR §5).
- W74's `zOp` spread.
- Calibrating a spread against field data.
