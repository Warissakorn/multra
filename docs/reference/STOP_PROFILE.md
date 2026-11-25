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
