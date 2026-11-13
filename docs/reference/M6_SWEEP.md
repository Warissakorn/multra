# M6_SWEEP — the pre-registered sensitivity sweep, phase 1 (D545)

The owner's choice ([m6-calibration-options](../evidence/m6-calibration-options.md), D544) is no
calibration on M6, plus a sweep that measures what each lever does. **A sweep figure decides
nothing, has no band of its own, and is never adopted.** It is shown beside the first runs. The
candidate values are fixed here, before any sweep run, and are never changed after a figure.
Written before code; rows SW1–SW4 fail first.

## 1. Candidates (phase 1: data-only levers)

| Name | Change to every compiled behaviour | Everything else |
|---|---|---|
| `base` | none | `default.json` |
| `B` | `standstillDistance` = 0.95 m (the jam spacing [AkcBes02] Table 2 implies: 6.9 − 4.5 − 1.45 m of `√v` gap at the creep) | as `base` |
| `C` | `followingTime` = 1.0 s | as `base` |

Phase 2 covers D and E, the engine levers. It needs runtime switches, off by default, each with
its own contract. It is not here.

## 2. The runs

`tools/m6_candidates.hpp` holds `candidate(name)` and `apply(Scenario&, const Candidate&)`. `apply`
patches every `Scenario::behaviours` entry after `compileDocument`. It refuses a behaviour with
`w74` set, because the levers belong to the prototype.

Three tools take an optional last argument, `--candidate base|B|C`:

| Tool | Modes | Figures (as the first runs) |
|---|---|---|
| `multra-m6-s1-run` | — | S1-a, S1-f and S1-d (D535) |
| `multra-m6-s2-run` | `ab` only | S2-b and S2-a (D539) |
| `multra-m6-u-run` | `run` only | U-a and U-c (D542) |

- **The candidate is recorded.** It appears as `candidate` in `summary.json` and `metadata.json`.
- **Without the flag**, every output is byte-identical to today's. The first runs therefore stand
  as `base`.
- **Seeds:** 42–81.
- **S2-c is not swept.** The S2-b and S2-a pair shows the capacity side of the levers.
- **Verdicts** are computed with the first runs' bands and shown as "would pass" or "would miss".
  They are not M6 verdicts.

## 3. Acceptance rows

| Row | Statement | Group |
|---|---|---|
| SW1 | `apply` with `B` sets `standstillDistance` 0.95 and with `C` sets `followingTime` 1.0 on every behaviour, leaving the other fields; `base` changes nothing | `m6sweep` |
| SW2 | `apply` refuses a scenario with a w74 behaviour; `candidate("X")` is refused | `m6sweep` |
| SW3 | `junction(200)` cut to 600 s, seed 42: with `B` the follow-up split's mean `standDistance` is smaller than with `base` (the forcing: the lever reaches the run) | `m6sweep` |
| SW4 | The S1 and U tools refuse an unknown candidate, and the S2 tool refuses a candidate with mode `c` | `cli-m6-sweep-refused` |

Return to [the reference index](README.md).
