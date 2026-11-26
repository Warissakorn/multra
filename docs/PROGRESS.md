# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-26 — The owner's Windows checks F7, F9, F10 and F11 pass on package #152

- **Owner, on package #152** (`a18a229`, D643–D665):
  - **F7:** "ข้อความ error ลงท้ายด้วย "· cc0" และเคอร์เซอร์ยังอยู่ที่ช่อง cc0" (D643's fix holds).
  - **F9:** "ผ่าน".
  - **F10:** "ผ่านทั้งหมด" (W99 queues at rest, with and without `reactionTime` 0.5 s).
  - **F11:** "ผ่านทั้งหมด" (the prototype's first car eases into its stop at a red line).
- **Left in F:** F8, the CC1 table, on the same package. Removed from the
  [Windows sheet](plans/OWNER_SITTING_WINDOWS.md); its order line is now `… I.5 → F8 → G1 …`.
- **Also asked:** whether `lineRest` should stop a set distance short of the line (answered in chat; nothing changed).
- **Docs** cut NEXT to open items (27,890 → 24,163 characters; history in
  [archive/NEXT-state-2026-11-26](archive/NEXT-state-2026-11-26.md)).

## 2026-11-26 — The approach to a car at rest, measured on NGSIM Lankershim (D666)

- **Owner:** "ทำเรื่องการชะลอเข้าหาแถวรถต่อ หาแหล่งอ้างอิงก่อน", then "ตามแนะนำ" (measure).
- **Sources** ([queue-approach-sources](evidence/queue-approach-sources.md)): CAMP's last-second braking (DOT HS 809 574),
  Rittger's simulator, and Treiber et al. None measures the onset toward a standing queue.
- **Measured** ([evidence](evidence/queue-approach-ngsim.md)): NGSIM Lankershim, 1.6 million rows. Required deceleration at
  onset:
  - the first episode toward a car at rest has a median of 1.42 m/s²;
  - the last braking phase has a median of 1.88;
  - toward a line it is 2.07.

  Today's law starts at 2.0 and B3 at about 0.77, which is earlier than 84 % of the first onsets.
- **No code.** The owner kept today's law toward vehicles at rest ("ตามแนะนำ", D667). B is closed with B3P. The staged
  approach, the higher final braking and the last step to rest are known deviations.
- **Verified (Linux only):** `check-docs` passes. The scripts are committed beside the evidence; the data is not.

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
