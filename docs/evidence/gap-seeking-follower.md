# Gap seeking, slice 1: the follower opens a gap anywhere in the span (D455) — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `c3a0fcb` (D454). Local host: Linux, GCC 13.3.0, Ninja,
`--preset headless` (Debug). **Linux only; no desktop build.** Nothing here is validated (rule 4).

- **Contract:** [M3_8_CONTRACT](../reference/M3_8_CONTRACT.md) §2, M3.2.8c, "Gap seeking, slice 1".
- **Follows:** the weave-lock series ([weaving-lock](weaving-lock.md),
  [held-change](weaving-held-change.md), [yield-cap](weaving-yield-cap.md)).

## Why and how other tools do it

The weave traces end on one cause: nobody makes room for a refused changer until its dead-end
look-ahead. Lab ×0.6, seed 48: a two-change stub rode for 170 m between two lane-1 cars 8 m apart,
all at about 12 m/s. The owner asked for gap seeking and chose the follower's slice (2026-11-04).

These sources were read through search summaries only. eclipse.dev and cgi.ptvgroup.com are outside
this session's network, so the primary manuals were not read.

- **SUMO LC2013:**
  - Strategic (route-required) changes rank above cooperative ones.
  - A blocked strategic change gets a speed adjustment to clear its blocker.
  - Followers adjust their speed to let it in, scaled by `lcCooperativeSpeed`
    ([sumo-user](https://www.eclipse.org/lists/sumo-user/msg08653.html); Erdmann 2014, "SUMO's
    Lane-Changing Model").
- **Vissim:**
  - "Maximum deceleration for cooperative braking" is a calibration parameter.
  - "Advanced merging" gives the merging vehicle and its cooperating trailing vehicles an earlier
    tactical look-ahead, so more vehicles change earlier.
  - Sources: the VISSIM 5.40 release notes, and the
    [Maryland SHA guidance](https://roads.maryland.gov/OPPEN/VISSIM%20Modeling%20Guidance%209-12-2017.pdf).

## Change (D455)

`courtesyHolds` (`src/core/lanes.cpp`):

- **Anywhere in the span:** a moving stub that is still a stub after this tick's changes no longer
  has to be inside its look-ahead (`deadEndGoverns`) to get a cooperative helper.
- **Not the swap partner:** a stub that itself needs the changer's lane is not asked; D207 holds it.
- **Unchanged:** who helps, how, the cap `maxDecelerationCooperativeBraking`, the closing-speed test,
  and D452/D453's exclusion of a landing (b) refuses.

**Deviation from the approved plan:** the partner exclusion was not in it.
- **Why it was needed:** without it, two weaves run alone locked that D454 served (150 m ×0.8,
  seed 51; 500 m ×1.0, seed 47).
- **How they locked:** the lane-0 stub was asked to help its own partner as an ordinary follower,
  stood 5.8 m short of its dead end where (b) does not count it, and the pair stood alongside.
- **With it:** in the same search (three weaves × ×0.8 and ×1.0 × seeds 42–51), no run locks.

## Rows

- **BA194** (`lanechange`): a changer at 40 m, 160 m short of its dead end, at 10 m/s on a stub. The
  stream on its target lane moves at 10 m/s, 14 m ahead of it and 5 m behind.
  - **Forcing:** it is refused, and it is outside its look-ahead.
  - **Before:** it got in at 133 m.
  - **After:** it gets in by 60 m (at 45 m), with no clamp. No stream vehicle brakes beyond 3 m/s².
  - **Setup search:** 24 setups were tried (gap ahead 6–14 m, gap behind 3–8 m, 10 or 12 m/s).
    - Five that waited for their look-ahead before now get in by 41–46 m.
    - One that already got in early gets in 4 m sooner.
    - One gets in 2.2 m later (gap ahead 10 m, behind 5 m, 10 m/s: 140.3 → 142.5 m).
    - Seventeen are unchanged: where the gap ahead is short too, opening the gap behind is not
      enough, which is slice 2's case.
    - In A36's dense stream (7.5 m gaps), the changer still gets in at the same place.
- **A38, revised:** 160 m short of its dead end, refused, a cooperative hold is now issued. Before,
  none was.
- **BA195** (`weavinglab`): the 300 m weave alone, switched on, ×0.8, seed 42, 450 s.
  - **Before:** mean delay 30.1 s.
  - **After:** below 20 s (13.1 s).

## Product runs, seeds 42–51 (mean delay, s)

| Project | Off | On |
|---|---|---|
| Four-leg | 47.43 → 47.49 | 47.73 → 47.64 |
| Lane-change lab | 8.98 → 9.03 | 9.06 → 9.07 |
| M2.6 | 46.42 → 46.40 | 46.92 → 46.60 (completed 1958.2 → 1958.5) |
| T-junction | identical | identical |

Clamps are unchanged in every project.

## Weaving lab, seeds 42–51

D454 → D455 (150 / 300 / 500 m). The ×1.2 column was not run for D454.

| Factor | Lateral | Locked | Mean delay (s) | Served (%) | Clamps |
|---|---|---|---|---|---|
| ×0.4 | on | 0/0/0 → 0/0/0 | 6.1/6.4/7.3 → 5.9/6.3/7.2 | 100/100/100 → 100/100/**98.3** | 3 → 2 |
| ×0.6 | on | 0/0/0 → 0/0/0 | 21.4/9.6/10.9 → 13.6/8.6/10.1 | 100 → 100 | 16 → 15 |
| ×0.8 | off | 0/0/0 → 0/0/0 | 11.5/10.9/12.3 → 10.9/10.8/12.1 | 98.8/100/100 → 100/100/100 | 12 → 12 |
| ×0.8 | on | 0/0/1 → **0/0/0** | 112/62/28 → **64/24/15** | 97.3/99.6/93.8 → 99.9/100/100 | 11 → 12 |
| ×1.0 | off | 0/2/1 → **0/1/0** | 47/35/25 → 35/23/23 | 100/84/94 → 100/90/100 | 35 → **21** |
| ×1.0 | on | 0/0/0 → 0/**1**/0 | 183/164/129 → 151/124/64 | 79/84/89 → 91/86/100 | 7 → **23** |

2D overlaps and removals are 0 throughout. The full table:

| scene | factor | lateral | served | unfinished | served % | mean delay (s) | changes | waiting at dead end | longest wait (s) | last arrival (s), earliest seed | locked seeds (of the range) | clamps | 2D overlap | removed |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Weave 150 m | 0.3 | off | 189.7 | 0.0 | 100.0 | 4.5 | 81.7 | 0.1 | 1.9 | 634.5 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.3 | off | 189.2 | 0.0 | 100.0 | 5.0 | 80.2 | 0.0 | 0.0 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | off | 201.0 | 0.0 | 100.0 | 6.2 | 83.0 | 0.0 | 0.0 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.3 | on | 189.7 | 0.0 | 100.0 | 4.9 | 81.7 | 1.2 | 5.2 | 634.4 | 0 | 2 | 0 | 0 |
| Weave 300 m | 0.3 | on | 189.2 | 0.0 | 100.0 | 5.2 | 80.2 | 0.7 | 5.5 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | on | 201.0 | 0.0 | 100.0 | 6.2 | 83.0 | 0.7 | 6.2 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.4 | off | 258.8 | 0.0 | 100.0 | 5.2 | 110.9 | 0.4 | 4.9 | 632.3 | 0 | 4 | 0 | 0 |
| Weave 300 m | 0.4 | off | 256.1 | 0.0 | 100.0 | 6.0 | 108.8 | 0.3 | 2.5 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | off | 257.8 | 0.0 | 100.0 | 7.1 | 111.1 | 0.6 | 3.5 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.4 | on | 258.8 | 0.0 | 100.0 | 5.9 | 110.9 | 1.6 | 5.0 | 632.3 | 0 | 2 | 0 | 0 |
| Weave 300 m | 0.4 | on | 256.1 | 0.0 | 100.0 | 6.3 | 108.8 | 1.3 | 9.5 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | on | 253.3 | 4.5 | 98.3 | 7.2 | 108.2 | 0.6 | 4.3 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.5 | off | 317.3 | 0.0 | 100.0 | 6.0 | 138.7 | 0.6 | 3.5 | 630.9 | 0 | 5 | 0 | 0 |
| Weave 300 m | 0.5 | off | 326.2 | 0.0 | 100.0 | 6.8 | 138.6 | 1.1 | 5.8 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | off | 321.0 | 0.0 | 100.0 | 7.9 | 136.5 | 0.7 | 4.8 | 661.9 | 0 |  |  |  |
| Weave 150 m | 0.5 | on | 317.3 | 0.0 | 100.0 | 7.8 | 138.7 | 2.7 | 8.0 | 631.1 | 0 | 4 | 0 | 0 |
| Weave 300 m | 0.5 | on | 326.2 | 0.0 | 100.0 | 7.5 | 138.6 | 2.7 | 9.0 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | on | 321.0 | 0.0 | 100.0 | 8.2 | 136.5 | 1.1 | 6.4 | 662.2 | 0 |  |  |  |
| Weave 150 m | 0.6 | off | 376.8 | 0.0 | 100.0 | 6.8 | 159.6 | 2.0 | 4.6 | 632.4 | 0 | 9 | 0 | 0 |
| Weave 300 m | 0.6 | off | 373.5 | 0.0 | 100.0 | 7.4 | 156.8 | 1.4 | 7.4 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | off | 399.0 | 0.0 | 100.0 | 9.3 | 168.1 | 2.1 | 7.2 | 665.7 | 0 |  |  |  |
| Weave 150 m | 0.6 | on | 376.8 | 0.0 | 100.0 | 13.6 | 159.6 | 5.7 | 15.0 | 639.2 | 0 | 15 | 0 | 0 |
| Weave 300 m | 0.6 | on | 373.5 | 0.0 | 100.0 | 8.6 | 156.8 | 3.7 | 11.2 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | on | 399.0 | 0.0 | 100.0 | 10.1 | 168.1 | 3.4 | 9.9 | 665.5 | 0 |  |  |  |
| Weave 150 m | 0.8 | off | 516.3 | 0.0 | 100.0 | 10.9 | 216.2 | 5.1 | 11.6 | 633.9 | 0 | 12 | 0 | 0 |
| Weave 300 m | 0.8 | off | 509.3 | 0.0 | 100.0 | 10.8 | 207.0 | 5.3 | 11.5 | 657.3 | 0 |  |  |  |
| Weave 500 m | 0.8 | off | 502.6 | 0.0 | 100.0 | 12.1 | 218.5 | 3.2 | 4.7 | 668.4 | 0 |  |  |  |
| Weave 150 m | 0.8 | on | 515.6 | 0.7 | 99.9 | 64.1 | 216.0 | 16.7 | 23.8 | 682.4 | 0 | 12 | 0 | 0 |
| Weave 300 m | 0.8 | on | 509.3 | 0.0 | 100.0 | 24.2 | 207.0 | 10.7 | 24.3 | 658.4 | 0 |  |  |  |
| Weave 500 m | 0.8 | on | 502.6 | 0.0 | 100.0 | 15.1 | 218.5 | 7.6 | 9.8 | 668.6 | 0 |  |  |  |
| Weave 150 m | 1.0 | off | 639.0 | 0.0 | 100.0 | 34.6 | 269.4 | 15.7 | 15.8 | 643.6 | 0 | 21 | 0 | 0 |
| Weave 300 m | 1.0 | off | 566.8 | 61.5 | 90.2 | 23.4 | 232.6 | 12.6 | 13.1 | 82.5 | 1 |  |  |  |
| Weave 500 m | 1.0 | off | 635.9 | 0.0 | 100.0 | 23.0 | 272.1 | 8.3 | 15.6 | 670.0 | 0 |  |  |  |
| Weave 150 m | 1.0 | on | 584.0 | 55.0 | 91.4 | 150.6 | 251.5 | 23.2 | 23.7 | 889.2 | 0 | 23 | 0 | 0 |
| Weave 300 m | 1.0 | on | 540.1 | 88.2 | 86.0 | 123.8 | 224.8 | 20.6 | 205.3 | 288.5 | 1 |  |  |  |
| Weave 500 m | 1.0 | on | 633.4 | 2.5 | 99.6 | 64.1 | 272.1 | 16.7 | 21.7 | 705.9 | 0 |  |  |  |
| Weave 150 m | 1.2 | off | 771.6 | 0.7 | 99.9 | 93.0 | 319.1 | 22.6 | 20.0 | 748.7 | 0 | 37 | 0 | 0 |
| Weave 300 m | 1.2 | off | 757.5 | 0.0 | 100.0 | 62.1 | 310.1 | 16.5 | 21.0 | 713.4 | 0 |  |  |  |
| Weave 500 m | 1.2 | off | 758.9 | 0.0 | 100.0 | 53.4 | 308.7 | 13.1 | 11.1 | 733.1 | 0 |  |  |  |
| Weave 150 m | 1.2 | on | 586.7 | 185.6 | 76.0 | 193.9 | 248.8 | 23.0 | 20.9 | 853.9 | 0 | 14 | 0 | 0 |
| Weave 300 m | 1.2 | on | 640.3 | 117.2 | 84.5 | 161.6 | 270.0 | 23.0 | 30.7 | 895.4 | 0 |  |  |  |
| Weave 500 m | 1.2 | on | 685.5 | 73.4 | 90.3 | 124.4 | 287.7 | 22.6 | 18.7 | 886.9 | 0 |  |  |  |

## Reading

- **Mostly better.** Delay falls sharply switched on at ×0.6–1.0, served shares rise, and the last
  switched-on lock below ×1.0 clears (×0.8, seed 50). Switched off at ×1.0, locks and clamps fall.
- **Costs:**
  - **Clamps on at ×1.0 rise from 7 to 23.** Traced: a crossing still held at the lane edge reaches
    its dead end at about 12.5 m/s, and the car-following brakes for the dead end only about 14 m
    out (lab ×1.0, seed 47, vehicle 125). That is P2b's late dead-end braking, now met more often
    because traffic flows faster.
  - **One new lock** on at ×1.0 (300 m).
  - **×0.4 on, 500 m, seed 42:** 45 vehicles were left at the end. A swapping pair stood with the
    rear one 1.5 m behind its partner's place, against a 2 m standstill distance. That is D450 (a)'s
    overrun in a new seed.
- **Still deferred:**
  - the changer's own speed adjustment (slice 2), for the dense-stream case BA194's search shows;
  - the lane-change state machine, a no-progress timer, and abandoning a change.
