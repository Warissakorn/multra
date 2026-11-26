# A softer stop profile at lines and behind standing vehicles (B)

Written 2026-11-25 (D663). In F9 the owner found the first car's stop unrealistic: a constant −2 m/s², then a jump
to 0. They chose **A then B** (LINE_STOP §9): one stop law for lines and standing vehicles (A, done:
D652–D662), then a softer, cited stop profile (B). They asked for the sources first, then for the contract from the
sources read ("เขียนสัญญา B จากแหล่งที่อ่านแล้ว").

This contract follows LINE_QUEUE's pattern:
- options are measured before code ([evidence](../evidence/stop-profile-options.md));
- **the owner chooses** (§6);
- no engine change is committed with it.

Rule 4 applies: nothing here is validated, and the unvalidated marker stays.

## 1. The problem

Under `lineRest`, LINE_STOP's law (`lineStopAcceleration`, D652) stops every model toward:
- the nearest holding line;
- a vehicle at rest less the standstill gap (LINE_QUEUE, D655–D656, D662).

It is the lower of the model's acceleration and the law. The law accelerates up to the curve `v = √(2b·gap)`, brakes
on it at the type's `comfortableDeceleration` b, then rests.

**What that does** (one car from 15 m/s, b = 2):
- it holds its speed to 56–64 m out;
- the deceleration jumps from 0 to 2 m/s² in one tick;
- it stays at 2 m/s² for 7.5 s, then jumps to 0 at rest.

SUMO's default Krauss model does exactly this (measured, SUMO 1.18).

**Measured today** (B0, seeds 42–51, per run):

| | Four-leg | M2.6 |
|---|---|---|
| Step jerk (acceleration changed by > 4 m/s² in a tick), prototype / W74 / W99 | 296 / 625 / 266 | 1,170 / 2,842 / 1,102 |
| Hard braking (moving, below −3 m/s²), prototype | 1,027 | 3,814 |

PROTOTYPE_QUEUE §3 traced the prototype's extra hard braking to followers arriving behind this late constant stop.

## 2. What the sources read support

From [stop-profile-sources](../evidence/stop-profile-sources.md):
- **Kesting & Treiber (2008), Eq. (2)–(3), p. 3.** IDM's braking strategy "limits braking decelerations to the
  comfortable deceleration b" in nearly all situations, and brakes harder only when the gap is too small.
- **Zhou et al. (2025), p. 5, restating Treiber & Kesting.** "Under normal conditions, braking is gradual, smoothly
  reducing deceleration to zero as the vehicle approaches a steady state or stops", with finite jerk.
- **SUMO 1.18, measured.**
  - Krauss is B0's profile.
  - IDM builds its deceleration up, peaks at about 1.9 m/s² about 9 m out, and eases off.
  - IDM **rests 0.11 m short** of its stop, with 3 s below 1 m/s.

**Not read:**
- **Akçelik & Biggs (1987):** a polynomial profile, zero at both ends. It is paywalled.
- **Deligianni et al. (2017):** in the field, "hard braking at the beginning ... then becomes smoother", which differs
  from IDM. The page needs a browser.

Neither is used.

**So B can cite IDM's form, with the type's own a and b.** It cannot cite a profile that falls to zero deceleration
exactly at rest. IDM reaches that only by creeping toward the target (§3, B1).

## 3. Options measured

The IDM term toward the target (s0 = 0, the target at rest):

`a_IDM = a·[1 − (v/v0)⁴ − (s*/gap)²]`, with `s* = v·T + v²/(2√(a·b))`.

- a is the type's acceleration at v (D608);
- b is the type's `comfortableDeceleration`;
- v0 is the vehicle's desired speed.

Every option keeps today's law as a bound, so it is never later than today and still ends exactly at the target (D596).

| Option | The law | New value |
|---|---|---|
| B0 | Today | — |
| B1 | `min(B0, a_IDM)` | T |
| B2 | `min(B0, max(a_IDM, −v²/(2·gap)))`: IDM, but never braking harder than an exact stop needs | T |
| **B3** | B2 with T = 0 | **none** |
| B4 | B2 without the free-road term `(v/v0)⁴` | T |

T is not a MULTRA value. Kesting & Treiber's Table 1 gives 1.03–1.51 s from one car in city traffic: a range, not a
default (D178). Both ends were measured.

**Results** (four-leg, per run; [all six projects, dt 0.05–0.2 and `reactionTime` 0.5 s](../evidence/stop-profile-options.md#results-per-run-seeds-4251-dt-01)):

| Option | Step jerk, prototype / W74 / W99 | Prototype hard braking | Prototype delay (s) | Clamps |
|---|---|---|---|---|
| B0 | 296 / 625 / 266 | 1,027 | 47.7 | as D662 |
| B1 | 3,120 / 12,558 / 5,779 | 1,065 | 28.9 | W99 83; M2.6 345 |
| B2, T 1.03 | 24 / 175 / 56 | 285 | 56.7 | about B0; W99 `weaving-lab` 31 → 70 |
| B2, T 1.51 | 23 / 177 / 68 | 220 | 56.6 | W99 four-leg 15, `weaving-lab` 86 |
| **B3** | 25 / 389 / 154 | 410 | 57.2 | about B0; W99 `weaving-lab` 43 |
| B4 | 25 / 178 / 56 | 677 | 59.5 | about B0; W99 `weaving-lab` 73 |

- **The onset step goes.** Under B2–B4 the deceleration builds up from 0 over seconds, as in IDM and in SUMO's IDM.
  The reductions hold at dt 0.05 and 0.2.
- **The prototype's delay rises by 9–12 s** on the four-leg and on M2.6 (about 20 %), with the same trips. IDM brakes
  80–110 m earlier. Where the delay comes from was not traced: an approach to a red that turns green before
  arrival, or an approach to a queue.
- **W74's trips on M2.6 swing by ±11 %** between options (oversaturated). They are not read as better or worse
  without a trace.
- **The last step to rest stays under every option.** An exact stop in finite time ends at a non-zero deceleration
  (§2). B1, which has none, creeps and clamps instead.
- **Under a reaction delay** (R9), step jerk is not reduced for the prototype: R9's start rule and reapplied law
  still switch in one tick.

## 4. The rule proposed (B3)

Under `lineRest`, the line law toward the target (a holding line, or a vehicle at rest less its standstill gap)
becomes:

`a_line = min(B0, max(a_IDM, −v²/(2·gap)))`, with T = 0: `s* = v²/(2√(a·b))`.

1. **IDM's form with the type's own a and b**, and the vehicle's desired speed as v0 (Kesting & Treiber Eq. (2)–(3)).
   No value is added.
2. **Never later than today.** B0 is a bound, so the stop still ends exactly at the target at b at most. D596's finish,
   the hard cap and D258's floor are unchanged.
3. **Never harder than the stop needs.** The bound `−v²/(2·gap)` keeps IDM from stopping short and creeping.
4. **No switch of its own.** It is part of `lineRest`, so runs with the switch off and bare M0 are unchanged, and
   `multra-cli 42` stays byte-identical.

## 5. The session's recommendation

**B3, after one trace.**
- **For it:**
  - It is the one option that needs no new value.
  - It removes 92 % of the prototype's step jerk, 38 % of W74's and 42 % of W99's, and 60 % of the prototype's hard
    braking.
  - Its clamps stay about B0's.
- **Against it:**
  - The prototype's delay rises by about 9.5 s, and that is not explained yet.
  - W99's clamps in `weaving-lab` rise from 31 to 43 per run.
  - W74's and W99's step jerk falls less than under B2 or B4. Their D258 floor then sets the profile.
- **B2 or B4 with a T** cut W74's and W99's step jerk further. They need T, for which only a range from one car is in
  hand. B2 (T 1.03) cuts W99's weaving clamps least well (70).
- **The trace before code.** Split the prototype's added delay by situation:
  - an approach to a red that turns green before the vehicle arrives;
  - an approach to a standing queue;
  - other.

  If most of it is the first, it is the early braking that IDM and SUMO's IDM both show. If it is the second, B3 is
  re-measured toward standing vehicles only with today's law.

## 6. The owner's question

1. **B3 (recommended), B2 or B4 with a T from Kesting & Treiber's range, or none?**
2. **The trace first, or straight to code?**
3. **The last step to rest** stays under every cited option. Should it wait for Akçelik & Biggs (1987) or Deligianni
   (2017), if the owner has a copy?

## 7. Rows (for the code session, if B3)

| Row | Case | Asserts |
|---|---|---|
| BA268 | One car of each model at 15 m/s toward a red line 200 m ahead | The line law's own value never changes by more than 1 m/s² in a tick before the last tick; it rests within 10⁻⁶ m of the line; no clamp. Forcing: B0 changes by 2 m/s² at the onset |
| BA269 | The same toward a car at rest | It rests at the standstill gap, with the same bounds |
| BA270 | A hold that appears inside the comfortable stopping distance | It is capped at `maxDeceleration` as today (BA242). B0's bound binds |
| BA271 | Switch off; bare M0 | Today's ticks; `multra-cli 42` byte-identical |
| BA272 | `reactionTime` 0.5 s (R9) | No pass on red and no clamp, as BA262–BA263 |
| BA273 | The measurement of §3, re-run on the committed code | Recorded, not a row |

## 8. What stays out

- **The last step to rest**, until an unread source is read (§6.3).
- **Calibration of a or b** from Kesting & Treiber's Table 1 (D544).
- **R9's start rule's step** under a reaction delay.
- **W99's weaving clamps** under every option (W99 §6's question).

## 9. The trace (D664)

The owner took the recommendation on 2026-11-25 ("ตามแนะนำ"): B3, after §5's trace
([evidence](../evidence/stop-profile-options.md#the-trace-of-the-prototypes-delay-d664)).

- **All of the prototype's added delay is the approach to a vehicle at rest.** Toward lines only (B3L) its delay is
  unchanged (four-leg 47.74 → 47.76 s; M2.6 45.35 → 45.38 s).
- **So is most of B3's gain.** On the four-leg, B3 cuts step jerk 296 → 25 and hard braking 1,027 → 410. B3L cuts
  step jerk 296 → 299, which is no change, and hard braking 1,027 → 903. A prototype stops behind a car far more often
  than at a line.
- **W74 and W99 gain nothing from B3L.** Their jerk > 10 m/s³ rises by up to 56 %, probably through D258's floor.

§5's rule ("if it is the second, B3 is re-measured toward lines only") was followed, and the result changes the
trade-off. No code is written, and the owner chooses again:

| Choice | What it gives (four-leg, prototype) | Cost |
|---|---|---|
| **B3P: B3L for the prototype only** (recommended) | The head car's stop at a line softens (hard braking −12 %); the same delay, clamps and stops | Small aggregate gain. W74 and W99 unchanged by construction, so the owner's F9 look on a W74 or W99 project sees no change |
| B3 everywhere (§4) | Step jerk −92 %, hard braking −60 %, 30 % fewer stops | Delay +9.5 s (+20 %), not traced as real or not |
| B3 toward vehicles at rest for the prototype, lines B3L | As B3 for the prototype | The same delay |
| None | — | F9's constant 2 m/s² stays |

**The sources** for the approach to a standing queue were searched on 2026-11-26
([queue-approach-sources](../evidence/queue-approach-sources.md)); none read settles it.

**The session's recommendation is B3P.** It is the one choice with no measured cost, and it answers F9's head car for
the prototype. The approach to a standing queue (B3's delay) is a question of anticipation: whether drivers slow
early for a queue that will move off. No source read measures it, so it stays out until one does.

Rows for B3P are §7's BA268–BA273 for the prototype at lines. BA269, toward a car at rest, asserts today's law.

## 10. The owner's choice and the implementation (D665)

The owner chose **B3P** on 2026-11-25 ("ตามแนะนำ").

- **Engine** (`src/core/line_stop.cpp`, `idmLineAcceleration`). Under `lineRest`, for a behaviour that is neither W74
  nor W99, toward a holding line, `applyLineStop`'s law becomes
  `max(−maxDeceleration, min(today's law, max(a_IDM, −v²/(2·gap))))`. a_IDM uses T = 0, the type's acceleration at v
  (D608) and its `comfortableDeceleration`, and the vehicle's desired speed.
  - `LineTarget` tells the law whether its target is a vehicle at rest. The target is a vehicle when the standing
    target is the nearest one. Toward a vehicle at rest, the law is today's.
  - A desired speed of 0 keeps today's law.
  - The delayed path (R9) passes the same target.
- **Against the measurement.** The committed code reproduces B3L's prototype rows and B0's W99 rows to the digit
  (four-leg, M2.6, `t-junction-priority`, `priority-rule-lab`). `multra-cli 42` is byte-identical.
- **Rows** (`tests/stop_profile_tests.cpp`, `stopprofile`):

  | Row | Case |
  |---|---|
  | BA268 | A prototype at its desired 15 m/s, 200 m from a red line: no tick changes by more than 1 m/s² before the last two, never harder than b; rests within 10⁻⁶ m of the line; no clamp. Forcing: today's law alone steps by more than 1 m/s² at its onset |
  | BA269 | 120 m out at 15 m/s: toward a vehicle at rest the law is today's; toward a line it is IDM's lower value; a desired speed of 0 keeps today's |
  | BA270 | 15 m/s, 5 m short: capped at `maxDeceleration` |
  | BA271 | W74 and W99 keep today's law toward a line |
  | BA272 | IDM is never harder than the exact stop `−v²/(2·gap)`, and equals the raw IDM term above it |

  Each of these mutations fails a row:
  - no IDM;
  - IDM toward vehicles too;
  - IDM for W74 and W99;
  - no kinematic bound;
  - no cap at the maximum.

  §7's BA272 (the reaction delay) is BA262–BA263, which pass unchanged. §7's BA273 is the measurement above.
- **BA259 amended.** In its queue at dt 0.5, the head car now eases into its stop at the line. The third car arrives
  slower behind it and spends five ticks below 1 m/s (2.5 s, against at most 2 s before). The bound at dt 0.5 is 2.5 s,
  and at dt 0.25 it is tightened to 1 s (measured 0.5 s). The queue still rests at its targets, without a clamp.
- **Left:**
  - the approach to a standing queue (B3's delay, a question of anticipation without a source);
  - the last step to rest, until Akçelik & Biggs or Deligianni is read;
  - W74's and W99's stop profile, which is set by D258's floor.

## 11. The approach to a vehicle at rest, measured (D666)

The owner asked for sources, then chose to measure ("ตามแนะนำ").
- **Sources:** [queue-approach-sources](../evidence/queue-approach-sources.md). None read measures the onset toward a
  standing queue.
- **Measurement:** NGSIM Lankershim ([evidence](../evidence/queue-approach-ngsim.md)), 1.6 million rows, cars, mid-block.
  The measure is the required deceleration `v²/(2·d)` at braking onset, where d is the distance to where the car
  actually stopped.

| | Median req at onset (m/s²) | Share of drivers starting at least this late |
|---|---|---|
| First deceleration episode toward a car at rest (354 approaches) | 1.42 (IQR 1.00–1.93) | — |
| Last braking phase, leader at rest (195) | 1.88 (1.79–2.05 across onset thresholds, 1.45–2.56 across smoothing) | — |
| Last braking phase, toward a line (100) | 2.07 | — |
| Today's law (B0; B3P toward vehicles) | 2.0, by construction | 22 % (first episode), 43 % (last phase) |
| B3 at the desired speed | about 0.77 | 84 % (first episode) |

- **The field lies between the two laws, and nearer today's.** B3's early onset, the source of its +9.5 s, is earlier
  than 84 % of the first onsets measured.
- Approaches are often staged (a gentle first episode, a pause, then the final braking). The final braking peaks at a
  median of 3.2–3.7 m/s², an upper reading because of NGSIM's noise, against MULTRA's b = 2.
- Approaches to a line and to a queue look alike (medians 2.07 and 1.88).
- A law whose onset sits at the measured median would need a value fitted to one arterial: calibration (D544).

**The session's recommendation: keep today's law toward vehicles at rest (B3P as committed).** B3 toward queues is
not supported by the measurement. The staged approach and the higher final braking stay as known deviations, recorded
here, with no change to the engine. The owner chooses.

**The owner's choice (D667, 2026-11-26, "ตามแนะนำ"):** today's law stays toward vehicles at rest. B3P, as committed,
is B's final state. These are known deviations, recorded and not modelled:
- the staged approach (a gentle first episode, then the final braking);
- the final braking's higher peak;
- the last step to rest (§8).

They are reopened only with a second site's measurement or an unread source (Akçelik & Biggs; Deligianni).
