# M3.5 slice 5: reaction-time discharge record (BA38) — 2026-10-15

D185; engine base `54f76be` (D184), plus the new evidence tool whose source hashes and exact
compiler command are in the [captured manifest](perception-discharge-manifest.json).
Contract: [PERCEPTION](../reference/PERCEPTION.md) §5/§9 and
[DISCHARGE](../reference/DISCHARGE.md). **Not yet validated; no calibration, Vissim-parity,
capacity, PCU or HCM control-delay claim.** No owner or milestone gate is closed.

## Experiment and verification

The four-leg project at dt 0.1 s, duration 900 s, seeds 42–81: prototype and the cited
[SUMO-defaults W74 preset](../../data/driver-behaviour/presets/w74-sumo-defaults.json), each at
`reactionTime` 0 / 0.5 / 1.0 s: **240 experimental runs**. Model-specific settings are constant
within a profile. Prototype retains `amberDeceleration=3`; the W74 preset has no amber key.
Their cross-model differences therefore do not isolate a following-model effect.

The same existing estimator observes all snapshots: full [0,900] window, warmup 0,
steady ranks 3–5, startup ranks 1–2; end-of-tick front crossings and half-open green intervals.
Initial/last partial greens, insufficient queues and remaps keep their explicit unavailable
reasons. No threshold or model parameter was tuned to get an estimate.

- **80 absent-key controls:** every zero-delay trajectory matches the corresponding absent-key
  trajectory on all 9,000 ticks: 720,000 comparisons, including events, queues, RNG and FIFOs.
- **240 copied-state checks:** each copies the state at tick 4,500 and matches the following
  32 ticks exactly (7,680 ticks total). Every positive-delay copy contained live FIFO history.
- All 240 runs pass vehicle accounting. Generated counts agree per seed across all six variants
  (mean 507.550 vehicles/run); the seed set is exactly 42–81 for each variant.
- All eight snapshot hashes, three raw-output hashes and three tool-source hashes were checked
  against the captured manifest. All 21,600 CSV cycle rows agree with the raw JSON, including
  null estimates, reasons and exact numeric cycle boundaries/estimates.
- Linux headless `check` **100/100 passed**, including five focused evidence-tool tests,
  CLI help/output protection and all existing perception/safety/reference regressions.

The C++ tool writes only a fresh output directory and writes `metadata.json` with `complete=true`
last. Output directories lacking that marker are incomplete. Roads with behaviour assignments
and parking lots are refused by this fixture-specific tool, rather than changing a different
set of drivers in one arm.

## Whole-run outcomes

Means over **40 seeds per row**. Clamps are numerical intervention counts, not collision counts.
Active and pending vehicles are unfinished at 900 s; source waiting is not hidden.

| Model | τ (s) | Completed | Active | Pending | Clamps | Valid headway cycles / total |
|---|---:|---:|---:|---:|---:|---:|
| prototype | 0 | 453.500 | 53.625 | 0.425 | 0.000 | 1130 / 3600 |
| prototype | 0.5 | 453.075 | 53.950 | 0.525 | 15.725 | 1128 / 3600 |
| prototype | 1 | 452.225 | 54.650 | 0.675 | 276.450 | 1072 / 3600 |
| w74 | 0 | 402.825 | 99.675 | 5.050 | 3.200 | 1221 / 3600 |
| w74 | 0.5 | 378.225 | 121.025 | 8.300 | 26.575 | 1002 / 3600 |
| w74 | 1 | 354.725 | 137.475 | 15.350 | 37.225 | 1184 / 3600 |

Prototype completed flow changes little while clamps rise from 0 to 276.450/run at τ=1 s:
unchanged throughput is not evidence of plausible delayed driving. W74 completed trips fall
from 402.825 to 354.725/run (about 11.9 %), with more active/pending vehicles. These are this
fixture's observed responses, not a universal reaction-time law or a failure threshold.
No clamp attribution study is claimed here; causes can be traced separately.

## Lane headways and startup estimates

`h / L (n)` is seconds: headway and signed startup lost time, each averaged first within a seed,
then equally across available seed means. `n` is the number of seeds with an estimate, **not**
40 by assumption. In this run the h/L seed counts coincide; the full summary tracks them
independently. Negative L is retained by the declared estimator. All-lane statistics, SDs and
unavailable-reason counts are in the [compact summary](perception-discharge-summary.json).

| Model | Signal lane (head ID) | τ=0: h / L (n) | τ=0.5: h / L (n) | τ=1: h / L (n) |
|---|---|---:|---:|---:|
| prototype | West signal, lane 1 (head-62) | 2.056 / -0.126 (40) | 2.194 / 0.181 (40) | 2.379 / 0.461 (40) |
| prototype | West signal, lane 2 (head-63) | 2.050 / -0.096 (40) | 2.179 / 0.216 (40) | 2.375 / 0.430 (40) |
| prototype | West signal, lane 3 (head-64) | 2.044 / -0.078 (17) | 2.169 / 0.244 (18) | 2.375 / 0.458 (15) |
| prototype | East signal, lane 1 (head-65) | 2.054 / -0.119 (40) | 2.178 / 0.221 (40) | 2.399 / 0.421 (40) |
| prototype | East signal, lane 2 (head-66) | 2.050 / -0.103 (40) | 2.175 / 0.224 (40) | 2.368 / 0.530 (40) |
| prototype | East signal, lane 3 (head-67) | 2.052 / -0.123 (18) | 2.170 / 0.231 (18) | 2.389 / 0.310 (16) |
| prototype | South signal, lane 1 (head-68) | 2.047 / -0.109 (40) | 2.175 / 0.217 (40) | 2.384 / 0.435 (40) |
| prototype | South signal, lane 2 (head-69) | 2.045 / -0.091 (29) | 2.170 / 0.235 (29) | 2.369 / 0.445 (25) |
| prototype | South signal, lane 3 (head-70) | 1.967 / 0.067 (1) | 2.133 / 0.233 (1) | 2.400 / 0.200 (1) |
| prototype | North signal, lane 1 (head-71) | 2.051 / -0.115 (40) | 2.174 / 0.227 (40) | 2.378 / 0.370 (40) |
| prototype | North signal, lane 2 (head-72) | 2.053 / -0.105 (29) | 2.176 / 0.233 (27) | 2.378 / 0.388 (27) |
| prototype | North signal, lane 3 (head-73) | 2.033 / -0.167 (1) | 2.167 / 0.167 (1) | 2.333 / 0.633 (1) |
| w74 | West signal, lane 1 (head-62) | 3.432 / -1.423 (40) | 3.726 / 0.109 (40) | 4.017 / 0.943 (40) |
| w74 | West signal, lane 2 (head-63) | 3.442 / -1.208 (40) | 3.741 / -0.348 (40) | 4.021 / 1.481 (40) |
| w74 | West signal, lane 3 (head-64) | 3.431 / -1.271 (18) | 3.744 / -0.954 (19) | 4.052 / -0.565 (18) |
| w74 | East signal, lane 1 (head-65) | 3.429 / -1.377 (40) | 3.714 / 0.447 (40) | 4.016 / 0.941 (39) |
| w74 | East signal, lane 2 (head-66) | 3.442 / -1.250 (39) | 3.746 / -0.596 (40) | 4.023 / 1.293 (38) |
| w74 | East signal, lane 3 (head-67) | 3.429 / -1.243 (16) | 3.735 / -0.940 (18) | 4.067 / -0.847 (15) |
| w74 | South signal, lane 1 (head-68) | 3.439 / -1.537 (40) | 3.727 / -1.081 (38) | 4.056 / -1.303 (40) |
| w74 | South signal, lane 2 (head-69) | 3.449 / -1.291 (29) | 3.748 / -1.178 (26) | 4.045 / -1.087 (26) |
| w74 | South signal, lane 3 (head-70) | 3.417 / 0.317 (2) | 3.783 / -1.317 (2) | 4.067 / -1.033 (1) |
| w74 | North signal, lane 1 (head-71) | 3.439 / -1.478 (39) | 3.734 / -1.228 (38) | 4.055 / -1.246 (39) |
| w74 | North signal, lane 2 (head-72) | 3.449 / -1.273 (30) | 3.738 / -1.099 (27) | 4.059 / -1.105 (26) |
| w74 | North signal, lane 3 (head-73) | 3.467 / -1.333 (1) | unavailable (0) | unavailable (0) |

Lane availability changes with τ; comparing two unpaired means may compare different samples.
The [paired differences](perception-discharge-paired.csv) use only seeds with a numeric estimate
in both the variant and its own model's τ=0 baseline, separately for h and L. West lane 1,
where all 40 seeds remain available, illustrates those paired changes:

| Model | τ (s) | Matched seeds | Δh (s) | ΔL (s) |
|---|---:|---:|---:|---:|
| prototype | 1.0 | 40 | +0.323 | +0.587 |
| prototype | 0.5 | 40 | +0.138 | +0.306 |
| w74 | 1.0 | 40 | +0.585 | +2.367 |
| w74 | 0.5 | 40 | +0.294 | +1.533 |

Sparse lanes such as head-73 (one baseline seed, then no W74 estimate) support no W74 paired
comparison at τ=0.5 or 1. They remain explicitly unavailable. Every row is retained in the
[240-run scalar table](perception-discharge-runs.csv); raw crossings and cycle reasons are in
the raw bundle, not reconstructed from these averages.

## Inputs and raw artifacts

Local host: **Linux headless only**, GCC 14.2.0, CMake 4.4.4, Ninja 1.13.2,
nlohmann/json 3.11.3. Release `-O3 -DNDEBUG -std=c++20 -fno-fast-math -ffp-contract=off`;
unit tests use `-O1`, as the project specifies. Qt desktop and Windows were not verified.
The exact compiler command and input byte lengths/read counts live in the manifest.

| Captured input | SHA-256 |
|---|---|
| `build/compile_commands.json` | `e50825e74bf50e9594ae3cf20fce4f132f2fa47bf94c407499334aa64501d72f` |
| `catalog/compositions/car-only.json` | `01e212b12301f5fe6f208c60a078f2ea81ef467d1cfd30bf9e289ecc19b702dd` |
| `catalog/compositions/urban-mixed.json` | `13618f4f33b0d86a8d2c53c1afa9995af9f809ef75f814009d1761fbe3669531` |
| `catalog/driver-behaviour/default.json` | `5c196dbad0fcead42ef31009d201430cf3c3314b006fecffb59bc323eff3f65f` |
| `catalog/priority-rules/default.json` | `133a43fa486ddbc45be23d0f62a100ddd351fabec8996ba01dc2b69ad676da51` |
| `catalog/vehicle-types/car.json` | `c8d99f76ed01dec7d91866d239b40ed1e682a1f4358a9c8ca084fc4dec8d87e9` |
| `catalog/vehicle-types/heavy-vehicle.json` | `158e88a84ef482b054c727900a5c91d41732dae639c5804667f8fe6d73ddaca8` |
| `evaluation/queue-counter.json` | `04bff220abb06e80db644e1b94afb53f3e0af790ffe2371e06eff5f2b82a1de9` |
| `project` | `3b0eb4450904ed66ef6f698b24280f7318a6031d6423dc60e16a2ba11542c016` |
| `w74-preset` | `c67bf20d93b1afe6897131a9f1241ca525a18a7d68c0fbb64dc56728f1dab859` |

Raw directory: `out/ba38-reaction-discharge/` (not committed; regenerate with the command below).
It retains eight resolved project snapshots, `runs.jsonl` with all cycles/crossings,
`cycles.csv`, `summary.json` with 2,880 per-seed/lane means, and completion metadata. The
manifest's output filenames refer to this raw directory. The committed summary omits only
`seedMeans`; the scalar and paired CSVs are derived from the raw JSON and those seed means.
The complete local bundle is retained as `out/ba38-reaction-discharge.tar.gz`
(SHA-256 `2d332c15ab35dbe6327e834fe24d2a0c8e675f4abbd243096105f414c2eb65e1`).

| Raw output | Bytes | SHA-256 |
|---|---:|---|
| `runs.jsonl` | 14551679 | `dc71feeb6edaf74ea8c1df72bf8a1ef81f59db0c45bc254a3d4e21b58c3e5f8c` |
| `cycles.csv` | 2207949 | `561c832b8e6f3d1134d2c6461437489b6359e92e365f792f3c35b82fe8e3111e` |
| `summary.json` | 1026550 | `f07eb2b428e96ba4df3c30543a4a5895b48be10d26c68834aec4cba671feb5a4` |

## Reproduce

```bash
cmake --preset headless -DCMAKE_BUILD_TYPE=Release
cmake --build --preset headless --target trafficsim-reaction-discharge-sweep trafficsim-tests
build/headless/bin/trafficsim-tests reactiondischarge
build/headless/bin/trafficsim-reaction-discharge-sweep \
  data/projects/four-leg-signalised.traffic.json \
  data/driver-behaviour/presets/w74-sumo-defaults.json data \
  build/headless/compile_commands.json out/ba38-new
cmake --build build/headless --target check
```

Use a new output directory. For a header-only JSON install, pass
`-DTRAFFICSIM_JSON_INCLUDE_DIR=<include-directory>` when configuring. The fixed matrix and
existing estimator make raw numerical results reproducible with the same inputs/engine/toolchain;
build paths/commit/compiler-command metadata naturally change across environments.
