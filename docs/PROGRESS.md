# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-11-04 — The dead end as a second obstacle (D456)

- **Owner:** asked for a lateral test project, then chose to fix the engine first, starting with
  the late dead-end braking. The lateral lab follows, as a lab-only exception to D178.
- **Cause** ([evidence](evidence/dead-end-second-obstacle.md)): in all 13 stub clamps of the lab on
  at ×1.0, a nearer moving leader whose route goes on hid the dead end. The vehicle then braked at
  −8 m/s² with about 14 m left.
- **D456 (M3_8_CONTRACT §2, "Dead end"):** behind a nearer leader, car-following runs once more
  against the dead end and keeps the lower acceleration, as the courtesy hold does. No new key.
- **Rows:** BA196 (`laneswap`) and BA197 (`weavinglab`), both failure-first.
- **Results:**
  - Weaving lab (seeds 42–51): no seed locks at any factor, off or on (D455: 0/1/0 at ×1.0, both).
    Clamps are 0 from ×0.3 to ×1.0, off and on (on at ×1.0: 23 → 0).
  - The D450 (a) overrun at ×0.4 clears. Delay on at ×1.0: 151/124/64 → 137/98/41 s.
  - Product runs: within 0.1 %, clamps unchanged, T-junction identical.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — Gap seeking, slice 1: the follower opens a gap anywhere in the span (D455)

- **Owner:** asked for gap seeking and chose the follower's slice.
- **D455 (M3_8_CONTRACT §2, M3.2.8c):** cooperative braking helps a refused moving stub anywhere in
  its span, not only inside its dead-end look-ahead.
  - **Deviation from the plan:** a stub that is the changer's swap partner is not asked. Without
    that, two weaves run alone locked.
- **Rows:** BA194 and BA195 are new and failure-first. A38 is revised: a hold is now issued 160 m
  out.
- **Results** ([evidence](evidence/gap-seeking-follower.md)):
  - Product runs: mean delay within 0.7 %, clamps unchanged.
  - Lab switched on: mean delay at ×0.8 112/62/28 → 64/24/15 s, and the ×0.8 lock clears.
  - Lab switched off at ×1.0: locks 0/2/1 → 0/1/0, clamps 35 → 21.
- **Costs:**
  - clamps on at ×1.0 7 → 23 (late braking for a dead end at speed);
  - one new lock on at ×1.0;
  - a D450 (a) overrun at ×0.4 (seed 42, 500 m).
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — The yielding partner brakes as cooperative braking does (D454)

- **Owner:** asked for a recommendation, then "do D454".
- **Cause** ([weaving-yield-cap](evidence/weaving-yield-cap.md)): a front stub reaching its dead end
  fast stops harder than comfortable. Its rear partner, capped at comfortable braking, stood inside
  the standstill distance, and the pair locked. This was P2b, and the off ×1.0 remainder.
- **D454 (M3_8_CONTRACT §2, D450 (a) revised):** once the front one closes in, the rear partner
  brakes up to `maxDecelerationCooperativeBraking` (M3.2.8c's key, 3 m/s²). No new parameter.
- **Rows:** BA192 and BA193, both failure-first.
- **Results:**
  - Product runs are identical except the lane-change lab on (delay −0.00005 s, clamps 0).
  - Lab on: one lock is left from ×0.5 to ×1.0 (seed 50, 500 m).
  - Lab off at ×1.0: 0/2/1 (from 0/3/2).
  - Cost: a 205 s wait on at ×1.0.
- **Track:** the weave-end lock track is closed by the recommendation. The leftovers are in NEXT.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — A change under way that may not land claims nothing (D453)

- **Owner:** "trace P2a first", then "do B+C+E as D453".
- **Traced** ([weaving-held-change](evidence/weaving-held-change.md)):
  - A two-change stub's crossing was held at the lane edge for 170 m beside lane-1 traffic.
  - It then landed beside a waiting partner, because D450 (b) was checked only at the start.
  - Checking (b) on every step was not enough. The held change still made its partner yield short
    of the dead end, where (b) lapsed, and it still reserved its target place (D206).
- **D453 (M3_8_CONTRACT §2):** a change under way that (b) refuses takes no outward step, is no
  partner for D451 (c), and holds no target place. Switched on only.
  - Every subset was measured; only B with E, and B with C and E, cut the locks, to 3 each.
- **Rows:** BA190 and BA191, both failure-first.
- **Results:**
  - Product runs are identical except the lane-change lab switched on (delay +0.003 s, clamps
    unchanged).
  - Lab switched on: locked seeds at ×0.6 0/1/0, at ×0.8 0/0/1, at ×1.0 0/1/0. The 237 s wait is
    gone.
  - Lab switched off: identical.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — No courtesy for a change that may not land (D452)

- **Owner:** "trace the remaining switched-on locks first".
- **Traced** ([weaving-lock](evidence/weaving-lock.md#d452-no-courtesy-for-a-change-that-may-not-land)):
  three patterns.
  - **(P1) the courtesy cycle:** a lane-1 follower held back for a two-change stub whose landing
    D450 (b) refuses, and stood short of a waiting lane-0 stub's place, so neither went.
  - **(P2a):** a held crossing resumes beside a partner.
  - **(P2b):** a late hard stop at the dead end beats the comfortable yield.
- **D452 (M3_8_CONTRACT §2):** no courtesy for a changer whose landing (b) refuses. It is one line in
  `courtesyHolds`, with no parameter and no state.
  - **Tried and dropped:** re-checking (b) as a held crossing resumes (P2a). It added locks at ×0.8.
- **Rows:** BA188 and BA189, both failure-first.
- **Results:**
  - Product runs are identical to D451, off and on.
  - Locked seeds on: ×0.5 0/0/0, ×0.8 1/0/0, ×1.0 0/2/0 (from 1/1/0, 4/5/0 and 1/4/3).
  - Locked seeds off at ×1.0: 0/3/2 (from 6/6/5).
  - Longest waits: off ×1.0 down from 510 s to 28 s.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — Switched-on weave-end lock: a change under way hid the pair (D451)

- **Owner:** "trace the switched-on case first".
- **Traced** ([weaving-lock](evidence/weaving-lock.md#d451-switched-on-a-change-under-way-hid-the-pair)):
  a lateral crossing (about 3.5 s) left the changer no one's partner until it crossed, so an
  approaching partner drove on and arrived too fast to yield. Two more traps were found on the way:
  a dead end at a join (D444), and a landing stub that drives on to its own dead end.
- **D451 (M3_8_CONTRACT §2):**
  - (b) revised: a partner is taken where it will wait, and the landing stub takes the lane up to
    its dead end;
  - (c) new: a change under way counts as on its target route for D207 (`yieldMutualUnderWay`).
  - **Deviation from the plan:** I had rejected (c) on its own, but the trace showed it is needed
    together with (b).
  - **Dropped:** a "could not stop comfortably" condition; it added a lab clamp.
- **Rows:** BA186 and BA187, both failure-first.
- **Results:**
  - Product runs are unchanged but for the lane-change lab's Diverge (clamps 0, delay ±0.01 s).
  - The weaving lab off is lock-free through ×0.8. On, its locks at ×1.0 fall from 10/10/10 to
    1/4/3 seeds.
  - Longer waits: up to 237 s on at ×0.6, and 510 s off at ×1.0.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — The weave-end lock, partly fixed (D450)

- **Owner:** "fix the weave-end lock first".
- **Traced** ([weaving-lock](evidence/weaving-lock.md)). Two ways a pair at a shared dead end locks
  for good:
  - D207 dropped the rear partner whenever it fell back, and it drove up into the standstill
    distance once the front one stood;
  - a two-change stub moved into the place a waiting partner needed.
- **D450 (M3_8_CONTRACT §2):** (a) the pair is kept while the front one brakes for or waits at its
  dead end; (b) `landsInWaitingPlace` refuses such a landing. No parameter, no state, and both
  switch modes.
- **Rows:** BA184 and BA185 were written failure-first, and BA183 turned from pinned-open to
  passing. Each rule is needed.
- **Results:**
  - All four product projects are identical apart from the build commit.
  - The lab switched off is lock-free through ×0.6 (×0.4 before). Locks remain at ×0.8+ and
    switched on.
  - Two experiments, not kept, are in the evidence. Harder yield braking is the owner's choice.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — Weaving lab, step 1: fixture and off/on sweep (D449)

- **Owner:** asked how other tools test weaving, then agreed to step 1: a synthetic fixture and an
  off/on measurement, contract first ([WEAVING_FIXTURE](reference/WEAVING_FIXTURE.md)).
- **Built:**
  - `data/projects/weaving-lab.traffic.json`, from `tools/weaving_network.hpp`, regenerated with
    `multra-weaving-fixture`: three Type A weaves of 150, 300 and 500 m;
  - `multra-weaving-sweep`;
  - rows BA179–BA183 in `weavinglab`.
  No engine or schema change.
- **Found** ([evidence](evidence/weaving-lab.md)):
  - A weave with two entry Links does not compile its crossing routes, because a change is
    compiled only on the entry Link. So the lab starts the weave at the network edge.
  - **The weave end locks for good** once the two lane heads stand short of each other's gap, since
    nothing is removed (M3 §4). The weave is lock-free through ×0.4. Locks begin at ×0.5. At ×1.0
    every seed of the 150 and 300 m weaves locks, off and on. Switched on locks earlier.
  - BA183 pins the lock as open.
- **Also:** the shipped-project tests count five files (SP1), and N1, MA1, NA8 and the Meso check
  include the lab.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — Lateral track closed; owner checks prepared

- **Owner:** chose to close the lateral track and turn to the pending owner checks. Gap seeking is
  deferred to M6 calibration or an explicit request (NEXT).
- **Prepared and verified** (Linux; no Windows desktop build in this container):
  - **UT10:** `tests/data/utdf/u2-u0-junction-geometric.csv` is byte-identical to today's
    `multra-cli --export-utdf` of `u2-u0-junction.traffic.json`, and `utdf` passes.
  - **A9:** the only Node-analysis code change since `086a4fd` is `32ab14a`, which moved the
    peak-hour-factor default into the shared `nodeAnalysisPeakHourFactor` for UTDF, with the same
    formula. `nodeanalysis` passes.
  - **Canvas:** the five `canvas-labels-after` images CANVAS_STYLE links are present.
- **Verdicts:** the owner's, on OWNER_SITTING I.5 and E32 and on the canvas review in NEXT.

## 2026-11-04 — Probe: the long m2.6 lane-change waits (no code change)

- **Owner:** "probe การรอนานใน m2.6 ก่อน", before deciding on gap seeking.
- **Method:** scratch probes, not committed.
  - `stubwait`: time on a route that still needs a change, a metric both runs share. Off and on,
    seeds 42–51; its arrival totals equal the CLI batches.
  - `refusal`: per-tick reasons for the three long waits.
- **Result** ([m26-long-waits](evidence/m26-long-waits.md)):
  - The long waits exist switched off (max 110.9 s; 61 over 30 s). They are signal-queue
    congestion on the two `path-4` approaches.
  - Switching on adds ≈ 2 s per change (the crossing time) and 13 more waits over 30 s.
  - The three long waits are mostly queued with the target place occupied (21–74 s). Being refused
    while moving, with the target lane's leader too close, is only 4–18 s; that is the part gap
    seeking would reach.
- **Next:** the owner's choice on gap seeking (NEXT). No decision ID and no engine change.
