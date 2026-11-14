# M6 kind U — where the Stop follow-up's 13.7 s goes (D543), 2026-11-13

[The first U run](m6-u-first-run.md) measured a follow-up headway of 13.7 s at the Stop line,
against HCM's 3.3 s. This run splits each follow-up into the engine's own events. Contract:
[M6_U_RUN](../reference/M6_U_RUN.md) §6, with rows SU1–SU5 written before code. **No band, it
decides nothing, and no parameter was changed.**

## 1. The run

```bash
./build/desktop/bin/multra-m6-u-run split data build/desktop/compile_commands.json <new-output-dir>
```

- **Build:** `755d70d`, the five D542 levels, seeds 42–81.
- **Copied here:** [summary](m6-u-split-summary.json) and [metadata](m6-u-split-metadata.json).
- **Not vendored:** `split.csv`, one row per pair (1.26 MB).
- **Pairs:** the observer's pair count equals the priority-line counter's `followUps` at every
  level (4,246, 1,752, 648, 247 and 98), so the split covers the same follow-ups.

## 2. The split (s, mean of seeds, 95 % CI)

| `v_c` | Headway | `moveUp` | of which `settle` | `dwell` | `release` | of which `blocked` |
|---|---|---|---|---|---|---|
| 200 | 13.88 ± 0.03 | **12.18** ± 0.03 | 3.77 | 0.10 | 1.60 | 0.005 |
| 400 | 13.53 ± 0.05 | **11.83** ± 0.05 | 3.76 | 0.10 | 1.60 | 0.013 |
| 600 | 13.22 ± 0.09 | **11.51** ± 0.09 | 3.75 | 0.10 | 1.60 | 0.019 |
| 800 | 12.68 ± 0.18 | **10.97** ± 0.18 | 3.75 | 0.10 | 1.60 | 0.025 |
| 1,000 | 12.46 ± 0.26 | **10.75** ± 0.26 | 3.75 | 0.10 | 1.61 | 0.043 |

| `v_c` | `moveDistance` (m) | `peakSpeed` (m/s) | `standDistance` (m) |
|---|---|---|---|
| 200 | 6.93 | 3.11 | 3.20 |
| 600 | 6.40 | 2.92 | 3.17 |
| 1,000 | 5.67 | 2.66 | 3.18 |

## 3. What the figures say

- **`moveUp` is 88 % of the headway.** This is the time from the leader's crossing to the
  follower's Stop service start: 10.7–12.2 s to move up about 6–7 m, at a peak of about 3 m/s.
- **The last 0.5 m takes 3.75 s** (`settle`) at every level. The follower creeps up to its stand
  position before its speed falls below 0.1 m/s, the condition `atStopLine` requires. This is the
  same creep S1 found in queues at a signal ([m6-s1-diagnostic](m6-s1-diagnostic.md)).
- **The other 7–8.4 s** cover the first roughly 6 m. This run does not split them further. They
  include the follower's start after its leader leaves and its approach to a standing target.
- **The Stop service itself is one tick** (0.1 s).
- **The release is 1.6 s and is almost all free.**
  - The follower stands 3.2 m short of the line (`stopLineReach`) and covers that from rest after
    service.
  - The gap rule blocks it for only 0.005–0.043 s on average.
- **The gap test is not what limits the movement.** The approach to the Stop line is: about
  12 s of a 13.7 s follow-up is spent getting the next vehicle to stand at the line.

## 4. What this decides

Nothing by itself (D543). It gives the held calibration decision (D540) one more fact. The
engine's approach to a standing target is slow:
- at a signal it shows as the creep and the long jam spacing;
- at a Stop it puts about 12 s into every follow-up.

How to change that, if at all, is the owner's decision. Any change would move S1, S2 and U
together.
