# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-25 — The prototype eases into its stop at a line: B3P (D665)

- **Owner:** "ตามแนะนำ" (B3P).
- **Engine** ([STOP_PROFILE §10](reference/STOP_PROFILE.md)): under `lineRest`, a prototype's law toward a holding line
  is the lower of today's and IDM's (T = 0, the type's a and b), no harder than the exact stop. `LineTarget` tells
  the law whether the target is a vehicle at rest; toward one, and for W74 and W99, nothing changes.
- **Measured:** B3L's prototype rows to the digit. Four-leg hard braking 1,027 → 903, delay 47.74 → 47.76 s; M2.6
  hard braking 3,814 → 3,360. W99's rows are B0's.
- **Rows** BA268–BA272 (`stopprofile`, a new file). Each of five mutations fails a row.
- **BA259 amended:** at dt 0.5 a third car behind the eased head car spends 2.5 s below 1 m/s (bound 2 → 2.5 s). At
  dt 0.25 the bound is tightened to 1 s.
- **Verified (Linux only):** `check` passes; `multra-cli 42` byte-identical.
- **Package #152** (`a18a229`) for the owner's [F11](plans/OWNER_SITTING_WINDOWS.md) (and F10).

## 2026-11-25 — B3's delay traced: all of it is the approach to standing cars (D664)

- **Owner:** "ตามแนะนำ" (B3, after STOP_PROFILE §5's trace).
- **Trace** ([evidence](evidence/stop-profile-options.md#the-trace-of-the-prototypes-delay-d664)): B3 toward lines only
  keeps the prototype's delay (four-leg 47.74 → 47.76 s), so all of the +9.5 s is the approach to a vehicle at rest.
  - Most of B3's gain is there too: toward lines only, step jerk 296 → 299 and hard braking 1,027 → 903.
  - W74 and W99 gain nothing toward lines only. Their jerk > 10 m/s³ rises by up to 56 %.
- **No code.** The trade-off changed, so the owner chooses again (STOP_PROFILE §9). The recommendation is B3P,
  B3 toward lines for the prototype only.
- **Verified (Linux only):** `multra-cli 42` byte-identical after the patch's revert; `check-docs` passes.

## 2026-11-25 — B, a softer stop profile: the contract and its measured options (D663)

- **Owner:** "อนุญาตให้ดาวน์โหลด PDF ได้", then "เขียนสัญญา B จากแหล่งที่อ่านแล้ว".
- **Sources read** ([stop-profile-sources](evidence/stop-profile-sources.md)):
  - Kesting & Treiber (2008): IDM's equations and its braking strategy;
  - Zhou et al. (2025), restating Treiber & Kesting: braking eases to zero at rest.
  - Akçelik & Biggs (1987) and Deligianni (2017) are still unread.
- **Contract** ([STOP_PROFILE](reference/STOP_PROFILE.md)): IDM-shaped line laws, bounded by today's so the stop
  still ends exactly at the target.
  - B2–B4 remove the onset step (four-leg step jerk: prototype 296 → 23–25, W74 625 → 175–389, W99 266 → 56–154).
  - The prototype's delay rises by 9–12 s, which is not traced.
  - No option removes the last step to rest.
  - SUMO 1.18 measured: Krauss is today's profile; IDM rests 0.11 m short.
  - **Recommendation:** B3 (T = 0, no new value) after a trace of the delay. The owner chooses (§6).
- **Verified (Linux only):** no engine change; `multra-cli 42` byte-identical after the patch's revert;
  `check-docs` passes.

## 2026-11-25 — Prototype queues rest behind a standing car: P17 (D662)

- **Owner:** "ทำ P17 ต่อเลย". Package #150 (`a5abe16`, D652–D661) was queued with owner item F10.
- **Engine** ([PROTOTYPE_QUEUE §7](reference/PROTOTYPE_QUEUE.md)): under `lineRest` the queue law covers the
  prototype. A prototype at rest starts once its gap admits `speedThreshold`.
  - On D661 the attempt's `lab` d217 clamps are gone.
  - Without a delay: four-leg crawl 17.0 → 1.3 s, 0 clamps, jerk 23 → 296, hard braking 417 → 1,027.
  - With R9 at 0.5 s: 0.5 / 3.1 clamps per run, against 12.1 / 51.2 with the switch off.
  - At 1 s: delay +28 s.
- **Forcings settled** (each still asserts its purpose):
  - the archived M3.2.7c congested variant runs with `lineRest` off;
  - BA248 gains a fourth major car;
  - BA107 averages seeds 42–46.

  BA197's served floor is 350 (357, from the start rule).
- **Rows** BA257–BA260, and BA241 amended. Each of four mutations fails a row.
- **Verified (Linux only):** `check` passes, `ctest` 376/376; `multra-cli 42` byte-identical.

## 2026-11-25 — Delayed vehicles rest at lines and in queues: R9 implemented (D661)

- **Owner:** asked whether creep, stop and creep again is how people drive (no cited source either way; Vissim
  and SUMO rest a queue at its standstill distance), then "เลือก R9 ทำเลย".
- **Engine** ([REACTION_STOP §7](reference/REACTION_STOP.md)): for a behaviour with `lineRest` and a reaction delay,
  `holdDelayedStart` runs before the FIFO and `applyDelayedLineStop` after it, in `src/core/line_stop.cpp`.
- **Against the measurement:** R9 reproduced to the digit.
  - At 0.5 s, W74 clamps 9 / 44 per run (four-leg / M2.6), against 755 / 3,277 before.
  - W99 clamps 8 / 29, against 391 / 1,506.
  - The prototype passes red 0 times, against 128–453.
  - Runs without a delay are unchanged.
- **Rows** BA262–BA265 and BA267 (`linestop`). Each of six mutations fails a row. BA263 found that a delayed stop can
  end a few centimetres short, and then holds there, since that short a start does not fit the delay.
- **Verified (Linux only):** `check` passes, `ctest` 376/376; `multra-cli 42` byte-identical.

## 2026-11-25 — A delayed vehicle rests: R9 measured (D660)

- **Owner:** asked whether R7's crawl is creep-then-stop or endless, then "หากฎให้รถที่มี delay หยุดนิ่งได้ก่อน".
- **Answer** ([evidence](evidence/reaction-stop-options.md)): it is creep, stop, creep again (median 6 stops per
  vehicle), W74's own, and the same with no delay and the switch off.
- **Traced:** R6's clamps were a residue at rest at `s0` behind a leader creeping by micrometres, and at 1 s a start
  from rest carried on by the delay.
- **R9** (REACTION_STOP §6): the laws reapplied after the delay, a hold within 10⁻⁶ m of `s0`, and a start from rest
  only when the gap fits `a·τ²/2 + (a·τ)²/(2·maxDeceleration)`. No value is added.
  - At 0.5 s: W74 9 / 44 clamps per run (off 23 / 91), and W99 8 / 29 (off 555 / 2,410).
  - Queues rest (median 1 stop per vehicle), with 0 red passes.
  - Costs: the SUMO preset at about the switch off, jerk above R7's, and the prototype's delay +12 s at 1 s.
  - **The owner chooses.**
- **Verified:** `multra-cli 42` byte-identical after the revert; `check` passes; docs only committed.

## 2026-11-25 — The stop laws under a reaction delay: contract and options (D659)

- **Owner:** "ตามแนะนำ": contract the reaction delay for every model first (PROTOTYPE_QUEUE §6).
- **Found** ([evidence](evidence/reaction-stop-options.md); a temporary [patch](evidence/reaction-stop-options.patch),
  reverted). The line and queue laws are delayed with the decision. At `reactionTime` 0.5 s:
  - **W74 clamps** 755 / 3,277 per run (four-leg / M2.6), against 23 / 91 with `lineRest` off, mostly behind a
    vehicle at rest;
  - **the prototype passes red lines** 128–453 times per run through M4.2's `committed` rule, not counted as red
    running, and its delay falls by a third;
  - **W99 clamps** under a delay with or without the line rules.
- **Options R0–R7.** R7 is recommended: for a delayed vehicle, the line law is applied again after the delay, and
  behind a standing car its model follows, delayed.
  - 0 red passes;
  - W74 5–8 / 33–48 clamps per run, below the switch off;
  - the cost is the crawl again for delayed vehicles behind standing cars.
- **Contract** [REACTION_STOP](reference/REACTION_STOP.md): rows BA262–BA266. **The owner chooses.**
- **Verified:** `multra-cli 42` byte-identical after the revert; `check` passes; docs only committed.

## 2026-11-25 — P17 attempted, not committed; the reaction delay is a gap (D658)

- **Owner:** "P17".
- **Correction:** D657's "P17" was measured with P6's late law (the probe's branch ran for `ZZ_P ≥ 6`). It is now
  P17L, and P7, P14, P16 and P18 are relabelled the same way.
  - P17 as written: crawl 1.3 s at the median, clamps ≤ 0.8 per run at dt 0.1–0.5, jerk 8–13×, and moving hard
    braking 2.1–2.5×.
- **Jerk traced:** it is the stop law's bang-bang profile behind a car at rest, not the hand-back. That is B.
- **Attempt** ([patch](evidence/prototype-queue-attempt.patch)):
  - Rows BA257–BA260 passed, and each of four mutations failed one. `multra-cli 42` was byte-identical.
  - **The full `check` failed five groups:**
    - `lab` d217 clamps with `reactionTime` 0.5 s;
    - three scene forcings were lost to the law itself;
    - BA197 served 372 → 357, from the start rule.
  - Reverted.
- **Found:** the reaction delay reaches D656 too. On the four-leg at `reactionTime` 0.5 s, W74 clamps 755 per run
  with `lineRest` on, against 23 off. No shipped project sets it.
- **Contract** [PROTOTYPE_QUEUE §6](reference/PROTOTYPE_QUEUE.md): choices for the owner. The recommendation is to
  contract the reaction delay for every model first.
- **Verified:** `multra-cli 42` byte-identical after the revert; `check` passes; docs only committed.

## 2026-11-25 — The prototype's crawl: the contract and its measured options (D657)

- **Owner:** "เรื่องการคืบของ prototype (เขียนสัญญาก่อน)".
- **Cause found:**
  - The prototype's desired gap `s0 + k·√v` closes as `k²/t`, so it never rests.
  - LINE_QUEUE's law alone clamps, because every clamp is a restart from an exact rest at `s0` (√v's slope is
    unbounded at 0).
- **Measured** ([evidence](evidence/prototype-queue-options.md); a temporary [patch](evidence/prototype-queue-options.patch),
  reverted). Six projects, options P0–P18, seeds 42–51, and dt 0.25 / 0.5 on the four-leg and M2.6:
  - **P17** (the stop law behind a car at rest, plus a start rule at the desired gap for `speedThreshold`) is the
    only option clean at every dt. Crawl medians are 2.5–4.7 s against 11–26 s, with 0–0.5 clamps per run.
  - Its costs are 13–22× the jerk ticks, hard braking +29–33 %, T-junction delay +0.45 s, headway +0.2 s at dt 0.5,
    and a second meaning for `speedThreshold`.
  - The start rule at `a·dt` (P2) is clean at dt 0.1 only: at dt 0.5, headway is +0.7 s and delay +11 s.
- **Contract** [PROTOTYPE_QUEUE](reference/PROTOTYPE_QUEUE.md):
  - P17 if the prototype's queues are to rest, else P0, with W74/W99 recommended for realism;
  - trace the jerk before code;
  - rows BA257–BA261.
  - **The owner chooses.**
- **Verified:** `multra-cli 42` byte-identical after the revert; `check` passes; docs only committed.

## 2026-11-25 — W74 and W99 queues come to rest behind a standing car (D656)

- **Owner:** "เลือก Q2 ตามแนะนำ ทำเลย".
- **Trace first:** M2.6's extra W74 "hard braking" under Q2 was vehicles at rest publishing W74's −8 m/s²
  request. Moving vehicles brake hard less often (801 → 728, 746 → 592 per run).
- **Engine** ([LINE_QUEUE §6](reference/LINE_QUEUE.md)): under `lineRest`, a W74 or W99 vehicle behind a vehicle
  at exactly 0 m/s stops with LINE_STOP's law at its rear less the standstill gap. The model sees no leader, and
  at its target the vehicle waits until the leader moves. The prototype is unchanged, and so is M0.
- **Rows:**
  - BA252–BA255 in `linestop`: a queue at rest without a crawl; a moving leader unchanged; the hold; the
    prototype's scope.
  - BA241's W99 row amended.
  - Four mutations each fail a row.
- **Measured:**
  - The code reproduces D655's Q2 on six projects to the digit.
  - [BA231](evidence/w99-discharge.md#after-d656): W99 clamps 42 → 8 per run at dt 0.1, headway unchanged.
  - [BA27](evidence/w74-discharge.md#after-d656): W74 headway 3.229 → 3.256 s.
- **Verified (Linux only):**
  - `multra-cli 42` is byte-identical, and so are the prototype sweep rows.
  - Each sweep, run twice, gives identical output.
  - `check` passes.
