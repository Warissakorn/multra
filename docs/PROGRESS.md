# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-05 — Canvas review fixes (D460)

- **Owner review** of the canvas: four items, then three more rounds (single-colour square heads
  on a pole, thin dark road edges, bare Node outlines and tags, greyer roads, no resting lane guides).
- **Node:** a thin outline with no fill, drawn above the roads. The polygon is no longer filled as
  junction asphalt. Highlighted Nodes stay 2 px accent and study-area Nodes green.
- **Road colour:** neutral greys instead of blue-greys (`data/display-types/`).
- **Node name:** shown only while the Node is selected, as a tight framed tag centred on it, no halo.
- **Connector lane guides:** the faint resting lane-centre lines (D423 Q7) are gone; they tangled
  over every junction. A Connector's lanes still show on hover, selection or the Connect tool.
- **Pole:** the mast arm is 3 px and the pole 0.30 m.
- **Signal head:** a 0.90 m square box of one colour with sharp corners, 0.35 m upstream of the
  stop line: dark while editing, the state's colour in a run, with no lamps. The run stop line
  keeps D199's shape cue. A mast arm meets the box's kerb-side edge from a pole just outside the
  road on the driving side. It zooms with the road.
- **Road edge:** a thin dark line replaces the thick light kerb, on Links and Connectors. A Link's
  plain solid outer-edge marking is no longer painted white over it; dashed and double edges still
  are.
- **Clear button:** measured 24 px tall at y 2 inside the combo's 22 px edit, so the X sat 3 px low.
  The cause was the app-wide `QToolButton` field-height rule, which also styles a line edit's clear
  button. That button is now exempt.
- **Input rates:**
  - A rate lies in the middle of one of its served lanes, along the road and upright.
  - The rates on one Link fill its lanes side by side, then line up along each lane.
  - While the text is taller than the lane it hides, so there is nothing at Fit on the shipped
    projects. A rate whose origin road is selected or hovered falls back to its box.
  - Boxes are now as tall as the text's ink.
  - This replaces D432's "all visible after Fit" and D426's columns for unpinned rates.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not checked. Images: `docs/images/canvas-review-d460/`.


## 2026-11-04 — Two-entry weaves run (D459)

- **Owner:** "ทำให้วาด weave แบบสองทางเข้าได้", with the test as a fourth weaving-lab scene.
- **Cause:** a lane family kept a stub only beside a full chain on its first Link, so a weave drawn
  with two entries refused both crossing movements (BA179).
- **D459 (M3_8_CONTRACT §2):** a family may start a **joined chain** on a later Link's lane that no
  chain enters, kept only beside a stub that needs it.
  - It takes no input share.
  - Its route carries `leadIn`, which the free-flow time counts.
  - A change onto it also sees the segments feeding its first segment. Without this, a ramp car at
    the gore was clamped.
  - Downstream routing decisions (D93) never build one.
- **Weaving lab, scene 4:** the 300 m weave with a 2-lane mainline and a 1-lane ramp, each with a
  300 m approach, at the same volumes. Scenes 1–3 are unchanged.
- **Rows:** BA179 revised; BA206–BA208.
- **Results** ([evidence](evidence/weaving-two-entries.md)):
  - products and the lateral lab are byte-identical;
  - scene 4 serves 100 % at every factor, off and on, with no lock and no wait at a dead end;
    delay at ×1.0 is 20.0 s off and 26.8 s on;
  - the edge scenes in the full run move only by the shifted random draw;
  - one clamp at ×1.0 on, seed 48, in the 150 m weave (open).
- **Verified:** `ctest --preset headless` passes 176/176.

## 2026-11-04 — Gap seeking, slice 2: the changer falls back behind its blocker (D458)

- **Owner:** "ทำต่อ". Shown both variants measured, the owner chose "switched on only".
- **D458 ([GAP_SEEKING](reference/GAP_SEEKING.md) §2):** with the lateral switch on, a moving
  refused stub whose forward test fails against the target-lane vehicle ahead of it falls back behind
  that vehicle, braking at most comfortably. Its swap partner is excepted.
  - Slice 1 moved from M3_8_CONTRACT to GAP_SEEKING.
  - `courtesyHolds` moved to `src/core/courtesy.cpp`, because `lanes.cpp` would have been over 500
    lines.
- **Variants rejected** ([evidence](evidence/gap-seeking-changer.md)):
  - **Capped at the cooperative key:** BA184 clamped.
  - **Both switch states:** switched off grew slower when dense (lab ×1.0 25/19/21 → 29/25/30 s;
    lane-change lab +1.7 %).
  - **Look-ahead only:** no gain.
- **Rows:** BA204 (218 → 200 m) and BA205 (150 m weave, ×1.0: 295 → 389 served, 83 → 37 s).
  BA197's served floor is revised to 365.
- **Results:**
  - Lab on at ×1.0: delay 137/98/41 → 87/44/35 s, and served 100 %.
  - Lateral lab: delay 30.3 → 13.1 s.
  - Switched off: byte-identical.
  - **Cost:** the 500 m weave alone at ×1.0 on, 24 → 28.5 s.
- **Verified:** `ctest --preset headless` passes 176/176; `check` passes.

## 2026-11-04 — The lateral lab (D457)

- **Owner:** asked for a project to test lateral behaviour, with all four scenes, and the lab's
  values in the file. "ทำโปรเจกต์ lateral-lab".
- **D457 ([LATERAL_LAB](reference/LATERAL_LAB.md)):** `data/projects/lateral-lab.traffic.json`.
  - **Scenes:** a timed lane change, a lane drop, a 150 m weave at ×0.8, and keep-left with cars,
    heavy vehicles and motorcycles.
  - **Keys:** every behaviour carries the lateral keys as lab placeholders (0.5 m, 1.0 m, 1 m/s).
    The keep-left Link has its own behaviour, which adds D95's pair and `kerbLaneBias`.
  - **The exception:** it is a lab-only exception to D178. No other project and no `data/`
    behaviour carries the keys.
  - **The builder:** `tools/lateral_lab_network.hpp`, through the editor's commands, regenerated
    with `multra-lateral-fixture data <out>`. The keys have one source, `tools/lateral_lab_keys.hpp`,
    shared with the weaving sweep.
- **Rows:** BA198–BA203 (`laterallab`). The four shipped-project lists in the tests now include the
  lab, and SP1 counts six files.
- **Results** ([evidence](evidence/lateral-lab.md), seeds 42–51):
  - mean delay 30.3 s; no clamp and nothing unfinished;
  - the short weave's delay is the highest, 47–62 s;
  - with the bias, the kerb lane's share of vehicle time is 68–80 % (45–49 % without it).
- **Verified:** `ctest --preset headless` passes 176/176.
- **Not done here:** the owner's check in the editor.

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
