# PROGRESS archive — 2026-11-05

Whole entries moved from the live PROGRESS log.

## 2026-11-04 — Gap seeking, slice 2: the changer falls back behind its blocker (D458)

- **Owner:** "ทำต่อ". Shown both variants measured, the owner chose "switched on only".
- **D458 ([GAP_SEEKING](../reference/GAP_SEEKING.md) §2):** with the lateral switch on, a moving
  refused stub whose forward test fails against the target-lane vehicle ahead of it falls back behind
  that vehicle, braking at most comfortably. Its swap partner is excepted.
  - Slice 1 moved from M3_8_CONTRACT to GAP_SEEKING.
  - `courtesyHolds` moved to `src/core/courtesy.cpp`, because `lanes.cpp` would have been over 500
    lines.
- **Variants rejected** ([evidence](../evidence/gap-seeking-changer.md)):
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
- **D457 ([LATERAL_LAB](../reference/LATERAL_LAB.md)):** `data/projects/lateral-lab.traffic.json`.
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
- **Results** ([evidence](../evidence/lateral-lab.md), seeds 42–51):
  - mean delay 30.3 s; no clamp and nothing unfinished;
  - the short weave's delay is the highest, 47–62 s;
  - with the bias, the kerb lane's share of vehicle time is 68–80 % (45–49 % without it).
- **Verified:** `ctest --preset headless` passes 176/176.
- **Not done here:** the owner's check in the editor.

## 2026-11-04 — The dead end as a second obstacle (D456)

- **Owner:** asked for a lateral test project, then chose to fix the engine first, starting with
  the late dead-end braking. The lateral lab follows, as a lab-only exception to D178.
- **Cause** ([evidence](../evidence/dead-end-second-obstacle.md)): in all 13 stub clamps of the lab on
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
