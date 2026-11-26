# How far upstream a priority rule looks: options measured — 2026-11-26

Evidence for [RULE_LOOKBACK](../reference/RULE_LOOKBACK.md) (D674). The patch is
[rule-lookback-options.patch](rule-lookback-options.patch) (reverted): `ZZ_E2=1` is G1 and `ZZ_E2=2` is G1R. It
was built on `8e2906d` (with E1), Release, and `multra-cli 42` is byte-identical with it and after the revert. Per
run, seeds 42–51, `lineRest` on. Arms are the project's own behaviours ("prototype"), the BA27 W74 fixture and the
BA231 W99 fixture on every type.

| Project, arm | Clamps G0 / G1 / G1R | Delay G0 / G1 / G1R (s) | Hard braking G0 / G1 / G1R |
|---|---|---|---|
| node analysis, prototype | 0 / 0 / 0 | 34.54 / 42.06 / 34.54 | 1,040 / 1,156 / 1,040 |
| node analysis, W74 | 15.7 / 17.7 / 15.7 | 85.79 / 97.08 / 85.79 | 284 / 313 / 284 |
| node analysis, W99 | 12.1 / 14.4 / 12.1 | 37.19 / 50.86 / 37.19 | 211 / 248 / 211 |
| M2.6, prototype | 0.2 / 0 / 0 | 45.38 / 45.44 / 45.44 | 3,360 / 3,357 / 3,358 |
| M2.6, W74 | 29.5 / 28.5 / 28.5 | 122.55 / 122.69 / 122.69 | 728 / 702 / 702 |
| M2.6, W99 | 32.9 / 32.8 / 32.8 | 47.84 / 47.87 / 47.87 | 646 / 603 / 603 |

- **Unchanged under G1 and G1R in every arm:** four-leg, the T-junction, the priority-rule lab and the
  waiting-acceptance lab.
- **G1 on node analysis:** turners are held behind straight cars approaching or standing at their red, which cannot
  reach the point before the red ends.
- **Cost:** wall times from six parallel runs varied by more than the differences, so none is reported.
