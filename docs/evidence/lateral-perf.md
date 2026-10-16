# Switched-on engine round: lateral cost back inside the budget — 2026-10-16

Branch `claude/m3.4-lane-change-duration-slice1` on `347fba9` plus this change (D209). Local host:
Linux, GCC 13.3.0, Ninja, 4 cores, shared container. Release `-O3 -g` headless builds. **Linux only.**
Budget: [LATERAL](../reference/LATERAL.md) §11 (a switched-on regression above 10 % needs a decision);
owed since D206 ([lane-change-duration](lane-change-duration.md)).

## Method

Callgrind instruction counts (Ir), one seed (42), `trafficsim-cli --project <p> --seeds 42
--threads 1`; switched on means a copy of `data/` with keys 0.5/1.0/1 on every behaviour (as in
D206–D208). `5847cb9` is the budget's reference: the lateral layer on, lane changes instantaneous.
Wall times are five interleaved pairs from **one build directory** (D206 found two directories 15 %
apart with identical source).

## Profile at `347fba9` (lane-change-lab, switched on, 1797 M Ir; off 1323 M)

| Cost (inclusive) | Ir | Share |
|---|---|---|
| `occupiedSpans` (all calls) | 426 M | 23.7 % |
| — of which `appendLateralSpans` | 194 M | 10.8 % |
| `moveLaterally` | 189 M | 10.5 % |
| — of which its full rebuild of refs, spans and buckets | 146 M | 8.1 % |
| `lateralCounts` (3.8 M calls) | 93 M | 5.2 % |
| `followerFilter` (0.7 M calls) | 61 M | 3.4 % |
| `coversSafely` (D208) | < 1 M | — |

## Changes (D209), all output-preserving

1. **`moveLaterally` builds only the bodies that can meet a mover's**: vehicles on a mover's
   segments, their neighbours, or the neighbours' neighbours (a body reaches a lane by a lateral copy
   from a neighbouring one). Overlap is an `any_of`, so a subset of spans that keeps every span on the
   read segments decides identically.
2. **`appendLateralSpans` skips the neighbour loop for a body inside its section's clear band**:
   `LateralIndex::clearLeft/clearRight`, precomputed per segment, with each neighbour's skip condition
   (`left ≤ nL + kLateralOverlap` or `right ≥ nR − kLateralOverlap`) tightened by 1e-9, so the fast
   path only ever skips neighbours the full test would skip.
3. **`lateralCounts` is inline** in `lateral.hpp`; it was 24 Ir a call, mostly the call.

## Result

| Run (seed 42) | `5847cb9` | `347fba9` | D209 | vs `5847cb9` |
|---|---|---|---|---|
| lane-change-lab, switched on | 1568 M | 1797 M | **1623 M** | **+3.5 %** |
| four-leg-signalised, switched on | 998 M | 1069 M | **1028 M** | **+3.0 %** |
| lane-change-lab, switched off | 1314 M | 1323 M | **1236 M** | −5.9 % |

Wall, Release `-g`, `--seeds 42-51 --threads 1`, `347fba9` → D209 medians: lane-change-lab
**2381 → 1857 ms** (−22 %), four-leg-signalised **1423 → 1381 ms** (noise; ranges overlap).
`trafficsim-engine-benchmark 40 1800` (switch off) run time medians **3662 → 3609 ms**, unchanged.

**Outputs are byte-identical** to `347fba9`: `trafficsim-cli 42`, and the `--seeds 42-51` JSON
(without `buildCommit`) of all four shipped projects switched off and of `lane-change-lab` and
`four-leg-signalised` switched on. `check` passes (Debug, Qt offscreen).

## Left

- `followerFilter` (61 M, a `frontPart` search per vehicle per tick) and the remaining
  `occupiedSpans` copy work are the next costs if lateral keys ever ship on large networks; the
  budget no longer requires them.
- Measured on two small projects; the switched-off engine benchmark is the only large network, and
  it carries no lateral keys.
