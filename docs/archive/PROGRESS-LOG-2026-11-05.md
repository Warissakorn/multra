# PROGRESS archive — 2026-11-05

Whole entries moved from the live PROGRESS log.

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
