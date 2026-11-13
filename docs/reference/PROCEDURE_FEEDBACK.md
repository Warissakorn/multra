# Procedure feedback — congested costs back into demand (PROCEDURE_SEQUENCE §5, D547)

Written 2026-11-13 as a contract, before code, for the slice the owner booked in D483 (4a): "an
assignment's congested times as the next distribution's costs, iterated to a stopping rule". The
owner answered §7 in the same session, each with the recommended option. Rule 4 holds:
- every output keeps the not-validated marker;
- feedback adds no behavioural number, it only re-runs existing units on other costs and averages
  their results.

**Source:** Ortúzar and Willumsen (2011), *Modelling Transport* 4th ed., §11.3 (book pp. 404–411;
repository `Modelling-Transport-4-edition-Markdown`, commit `1b3d41d`). The book's points this
contract rests on (paraphrased, not quoted):
- **Naive feedback** — run every model, feed the new times back, repeat — may never converge.
  Example 11.3 (p. 405) shows it oscillating for larger logit parameters. The book says it should
  always be avoided (p. 410).
- **Evans' method** (p. 408, after Boyce 2007) averages flows, OD-mode trips and link volumes, not
  costs.
- **Pre-set step weights** may replace Evans' line search (p. 409):
  - MSA uses 1, 1/2, …, 1/n;
  - constant weights (CW) are reported to converge faster on real networks. Bar-Gera and Boyce
    (2006) recommend w in 0.2–0.5, and w = 0.25 worked for three very different networks (p. 410).
- **The general algorithm (pp. 409–410):** solve the demand model on the current costs (auxiliary
  matrices e^n), average it with the previous trip matrix, assign the average, and test e^n against
  T^(n−1).
- **Convergence measures (p. 410):**
  - total misplaced flow, TMF = Σ|T^(n−1) − e^n|, Eq. (11.28);
  - root squared error, RSE, Eq. (11.29).

  §11.3.4 (p. 411) prefers TMF in trips per hour, because it reads intuitively.

## 1. Scope

- **In:**
  - an optional `feedback` object on a stored procedure (schema 75);
  - a run that iterates the procedure's demand steps on congested costs, averages, and
    re-assigns, under §3;
  - per-iteration convergence in the procedure JSON, the CLI and the *Procedure* page.
- **Out:**
  - Evans' line search and the combined relative gap of §11.3.4;
  - logsum feedback (G/MS/D/A, Ortúzar §6.5.2);
  - feedback into trip generation;
  - several assignment periods in one loop;
  - writing any result into the document. Matrices stay in memory, as D483 (2a).

## 2. Stored form (schema 75)

```json
{ "id": "base-am", "name": "Base, AM", "steps": [ … ],
  "feedback": { "method": "constantWeight", "weight": 0.25, "maxIterations": 30, "tolerance": 50 } }
```

| Key | Rule | Fault |
|---|---|---|
| `method` | `constantWeight` or `msa` | `INVALID_PROCEDURE` at `procedures[i].feedback.method` |
| `weight` | required with `constantWeight`, in [0, 1); absent with `msa` | `INVALID_PROCEDURE` at its path |
| `maxIterations` | an integer in [2, 1000] | `INVALID_PROCEDURE` |
| `tolerance` | TMF in persons over the assigned period (per hour for a one-hour period), finite and ≥ 0 | `INVALID_PROCEDURE` |

- **`weight` is the book's `w`** (p. 410, checked on the page image), the share of the previous
  matrix: T^n = w·T^(n−1) + (1 − w)·e^n.
  - w = 0 is naive feedback. It is allowed so that FB7 can show what averaging prevents.
  - MSA is T^n = ((n − 1)/n)·T^(n−1) + (1/n)·e^n.
- **A procedure without `feedback`** keeps its bytes and its schema. Below 75, `feedback` is
  `EDIT_UNSUPPORTED_FIELD`.
- **Run-time shape check, before any step runs:** a feedback procedure needs, among its active
  steps, exactly one `tripDistribution`, one `modeChoice` after it, and one `assignment` after that.
  The assignment must be the last active step. Otherwise it is `FEEDBACK_STEPS_INVALID` at
  `procedures[i].feedback`.

## 3. Run

Iteration 1 is today's procedure run, unchanged: free-flow costs, and the assignment of the
mode-choice matrices. Then, for n = 2, 3, …, `maxIterations`:

1. **Congested skim.** Each link's time is taken at the previous assignment's final loads
   (`macroTime`, the times that assignment reports). The zone skim (`zoneSkim`) is recomputed for
   each mode the distribution and mode-choice steps read, with the same connector shares and
   unreachable rules. Owned cost-matrix overrides still replace their cells, so a non-network
   alternative (walk, rail) keeps its fixed costs.
2. **Auxiliary demand e^n.** The distribution and mode-choice steps run again with those costs. The
   generation step is not re-run, because it reads no cost. e^n is the mode-choice step's per-period
   person matrices, for every alternative.
3. **Average.** T^n = w_n·T^(n−1) + (1 − w_n)·e^n, per period, alternative and cell, where w_n is
   `weight`, or (n − 1)/n under MSA. Vehicles are T^n divided by each alternative's occupancy, as E2 does.
4. **Assign** T^n's vehicle matrices with the assignment step's settings, to its own relative gap.
5. **Measure:**
   - TMF_n = Σ|T^(n−1) − e^n| over the cells of every period and alternative, in persons over each
     period. That is persons per hour, as §11.3.4 prefers, for a one-hour period. Amended before
     code: mode choice's period matrices are counts in the period;
   - RSE_n, Eq. (11.29);
   - the assignment's iterations and relative gap.
6. **Stop** when TMF_n ≤ `tolerance` (*converged*) or n = `maxIterations` (*not converged*, which
   is no refusal). A refusal at any unit stops the run as in PROCEDURE_SEQUENCE §3.3, keeping the
   earlier iterations. Cancel stops between iterations and inside an assignment.

The step results are those of the last iteration:
- the distribution step holds the last e^n's distribution;
- the mode-choice step holds T^n;
- the assignment step holds the last assignment.

## 4. Outputs

- **JSON:** `procedureJson` gains `feedback {method, weight, maxIterations, tolerance, converged,
  iterations[{n, weight, tmf, rse, assignmentIterations, relativeGap}]}`. Iteration 1 has
  `weight`, `tmf` and `rse` null. The block is absent without feedback, so existing outputs stay
  byte-identical.
- **CLI:** `--procedure ID` is unchanged. A feedback procedure prints one line per iteration on
  stderr.
- **Editor:**
  - *Procedures…* gains a *Feedback* group (method, weight, maximum iterations, tolerance), off by
    default.
  - The *Procedure* page adds an iterations table (n, TMF, RSE, assignment gap) and the outcome
    (*converged after n* or *stopped at the limit*).

## 5. Fixture and rows

**The fixture** is Ortúzar Example 11.3 (pp. 405, 368).
- **Network:** the town-centre/bypass network of Example 10.4, with
  - t_b = 15 + 0.005·V_b;
  - t_t = 10 + 0.02·V_t.

  Both are exact as BPR with β = 1 and α = 1: t0 15 min at capacity 3,000, and t0 10 min at
  capacity 500.
- **Rail:** 18 generalised minutes, as an owned cost matrix.
- **Demand:** 2,000 persons per hour with a car occupancy of 1, split by a binary logit in λ.

The fixed point is solved by hand: the car share at the road's Wardrop cost equals the logit share.
The book prints no equilibrium numbers, only Figure 11.3's convergence curves.

**The fixture's geometry** (`tests/data/procedure-feedback/`) adds small fixed connector times.
- **The links:**
  - entry and exit links: 1 m each at 100 m/s, capacity 10⁶ (load term negligible);
  - the bypass: 9,000 m at 10 m/s, one lane of 3,000;
  - the town centre: 9,000 m at 15 m/s, one lane of 500.
- **The connectors** are each √26 m. Each is timed at its `from` Link's speed (MACRO_NETWORK §3).
- **The route constants** are therefore c_b = 15 + 0.009682 + 0.005·V_b and
  c_t = 10 + 0.006849 + 0.02·V_t minutes.
- **The demand:** 2,000 persons per hour, A to B, car occupancy 1. The logit is U = −λ·cost with no
  constants, against rail at 18 minutes.

**Hand values** (bisection on V_C; written before any code):

| λ | V_C (veh/h) | V_b | V_t | car cost (min) | rail (persons/h) |
|---|---|---|---|---|---|
| 0.05 | 999.793 | 599.721 | 400.072 | 18.0083 | 1,000.207 |
| 0.34 | 999.078 | 599.149 | 399.929 | 18.0054 | 1,000.922 |

- **Without the connector constants** the fixed point is exactly V_C = 1,000 at a cost of 18 for
  any λ. This is the book's construction: 14 + 0.004·1,000 = 18.
- **Tolerance.** FB6 accepts V_C within 1 veh/h of the table, because the assignment stops at its
  own relative gap.

| Row | Asserts |
|---|---|
| FB1 | Codec and schema 75: round trip; each §2 fault at its path; no feedback keeps the bytes and schema; < 75 refuses |
| FB2 | `FEEDBACK_STEPS_INVALID` for a missing, repeated or misordered distribution, mode-choice or assignment step |
| FB3 | Iteration 1 equals today's procedure run, step by step (PS2–PS4 unchanged) |
| FB4 | The congested skim equals `zoneSkim` over the assignment's link times; a cost-matrix override still wins |
| FB5 | Averaging: CW and MSA by hand on a two-iteration case; vehicles equal persons / occupancy |
| FB6 | Example 11.3 at λ = 0.05 converges under CW 0.25 and under MSA to the hand-solved fixed point within the tolerance |
| FB7 | Example 11.3 at λ = 0.34: CW and MSA converge to the hand value. At λ = 0.6, CW converges and naive feedback (w = 0) does not within 100 iterations (the forcing that averaging matters) |
| FB8 | The limit: `maxIterations` reached is *not converged*, with no refusal; Cancel inside an assignment keeps the earlier iterations |
| FB9 | CLI and JSON: the `feedback` block; absent without feedback, so shipped projects and every existing output are byte-identical |
| FB10 | Editor: the Feedback group is one Undo step and saves 75; the page shows the iterations and goes stale after an edit |

Each row first asserts that its forcing worked.

**Rows amended after the first test run** (2026-11-13, before F1 was committed):
- **MSA's stop.** It stopped at a TMF of 2.6 persons after 100 iterations, against a tolerance of
  0.1. This is MSA's known slow rate (p. 409). The fixture's MSA procedures now stop at 2 persons
  (0.1 % of the demand) within 200 iterations. FB6's 1 veh/h acceptance is unchanged.
- **Naive feedback at λ = 0.34 converged here.** The book's Figure 11.3 shows it oscillating.
  Linearised at the fixed point, naive feedback multiplies a car-flow error by about
  λ·P(1 − P)·2,000·0.004 = 2λ: 0.68 at λ = 0.34, so it converges, and 1.2 at λ = 0.6, so it cannot.
  - This network's Wardrop cost rises 0.004 min per car (§5). The book does not say why its
    threshold differs.
  - FB7's forcing therefore moved to λ = 0.6, a value derived from that factor, not tuned to a
    figure. CW 0.25 has a factor of 0.25 − 0.75·1.2 = −0.65 there, so it converges.
- **No definition, hand value or acceptance tolerance changed.**

## 6. Slices

| Slice | Content | Rows |
|---|---|---|
| F0 | This contract and §7's answers | — |
| F1 | Codec at schema 75, the congested skim, the loop, JSON and CLI | FB1–FB9 |
| F2 | The dialog group and the page's iterations table | FB10 |

## 7. Owner choices

All four answers are the recommended options (owner, 2026-11-13, D547):
- (1a) a `feedback` object;
- (2a) CW or MSA, with CW `w` = 0.25 as the book defines it (the weight on the previous matrix);
- (3a) average the OD-mode matrices;
- (4a) both settings required.

The options as they were put:

1. **Where feedback lives.**
   - **(a) A `feedback` object on a procedure, looping its demand steps (recommended, chosen).** One
     procedure says what is fed back and how.
   - (b) A new step kind, `feedback`, that names the step to loop back to. This is more general,
     but the step list could then express loops the book does not describe.
2. **Averaging.**
   - **(a) Constant weight or MSA, chosen per procedure. The dialog default is CW with the book's
     w = 0.25 (recommended, chosen).**
   - (b) MSA only: slow, but proven to converge.
3. **What is averaged.**
   - **(a) The OD-mode person matrices, then assign the average (Evans and Boyce, the book's
     method) (recommended, chosen).**
   - (b) The costs: average the congested skims and re-run the demand. This is easier to explain,
     but it is the soft-speed change the book warns about.
4. **Convergence settings.**
   - **(a) `maxIterations` and `tolerance` are both required in the file, and the dialog proposes
     30 and 1 % of the total trips (recommended, chosen).**
   - (b) Fixed defaults in code, overridable.

## 8. As implemented (F1, 2026-11-13)

- **Files:**
  - `src/project/procedure_codec.*`: `ProcedureFeedback` and `FeedbackMethod`, parse, write and
    issues. `document.cpp` selects schema 75 and refuses `feedback` below it; `kLatestSchema` is 75.
  - `src/macro/assign.*`: `skimAt`, the skim at given link times, which `freeFlowSkim` now calls
    with free-flow times.
  - `zoneSkim`, `runTripDistribution` and `runModeChoice` take an optional `linkTimes`. It is null
    by default, so every existing call is unchanged.
  - `src/project/procedure_feedback.cpp`: `checkFeedbackSteps` and `iterateFeedback`, plus the
    assignment helper `assignProcedureStep`, factored out of `runProcedure`.
- **One re-run per iteration.** Mode choice runs its own distribution, so each iteration re-runs
  the mode-choice unit once, and its distribution becomes the distribution step's result.
- **Cancel.**
  - Between iterations, Cancel calls the step observer with the mode-choice step's index.
  - Inside an assignment, Cancel keeps the previous iteration's results and marks the run
    cancelled.
- **Fixture:** `tests/data/procedure-feedback/`, with the project, local VDF `linear`, and the rate,
  deterrence and mode-choice sets. Its procedures:
  - `plain-005`;
  - `cw-005`, `msa-005`;
  - `cw-034`, `msa-034`, `naive-034`;
  - `cw-060`, `naive-060`.
- **Rows:**
  - `tests/procedure_feedback_tests.cpp` (group `procfeedback`) covers FB1–FB8;
  - `cli-procedure-feedback` covers FB9;
  - the existing suites pass unchanged.
- **Measured (seed-free; not a validation):**

  | Procedure | Iterations | V_C (veh/h) | Hand value |
  |---|---|---|---|
  | `cw-005` | 7 | 999.799 | 999.793 |
  | `msa-005` | 127 | 1,000.69 | 999.793 |
  | `cw-034` | 10 | 999.074 | 999.078 |
  | `msa-034` | 49 | 999.64 | 999.078 |
  | `naive-060` | 100, not converged | — | — |

