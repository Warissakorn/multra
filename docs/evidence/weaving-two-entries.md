# Two-entry weaves: chains that join on a downstream Link (D459)

2026-11-04. Not validated; development evidence (rule 4). Linux, headless build.
The contract is [M3_8_CONTRACT](../reference/M3_8_CONTRACT.md) §2 ("A joined chain") and
[WEAVING_FIXTURE](../reference/WEAVING_FIXTURE.md) §2 (scene 4).

## Cause

`routeLaneFamily` built one chain per lane of the route's first Link. It kept a stub only next to a
full chain on that first Link. In a weave drawn with two entries:
- **ramp → mainline:** the ramp's one chain stops on weave lane 0;
- **mainline → exit:** both chains stop on weave lanes 1–2.

Neither family has a full chain, so both routes were refused with `UNSUPPORTED_ROUTE_TOPOLOGY`
(BA179, pinned since D449). The editor refused to draw them for the same reason.

## Change

- **Joined chains:** an authored route's family may start a chain on a later Link's lane that no
  chain enters. It is kept only when a stub that the first-Link rule does not keep stops beside it.
  It takes no input share.
- **Lead-in:** its core route carries a lead-in (`Route::leadIn`). The plain free-flow time is
  `(length + leadIn) / desired speed`.
- **Rear check:** a change onto a joined route also looks at the segments that feed its first
  segment (`ScenarioIndex::feedersOfRoute`).
- **Not built for D93:** downstream routing decisions never build a joined chain.

`weavingTwoEntries()` compiles to two joined chains:
- `main-exit/lane-3` on weave lane 0, lead-in 320 m;
- `ramp-main/lane-2` on weave lane 1, lead-in 320.3 m.

Their spans lie inside the weave, from 320 to 620 m.

## Before the rear check

With joined chains alone, scene 4 run alone at ×0.6 switched off clamped one ramp car (seed 42,
t = 387.1 s, at the weave's start, 13.5 m/s).
- **Cause:** a mainline car changed onto the lane-0 joined chain at the weave's start.
- **Why the check missed it:** the change's rear check searched only that route, and the ramp car
  was still on its Connector.

With the feeding segments seen, scene 4 alone at ×0.6, ×0.8 and ×1.0, off and on, has no clamp.

## Identity

- **Products:** the four product projects (off and on, seeds 42–51) and the lateral lab are
  byte-identical to their batches before D459. The check ran after joined chains and before the
  rear check, which runs only for routes with a lead-in.
- **Tests:** every test that existed before passes. BA179 is revised from "refused" to "compiles".

## Rows

- **BA179:** the two-entry drawing compiles, and each crossing has its joined chain.
- **BA206:** no input enters a joined chain. The four-leg routes and the edge weaves' routes have
  no lead-in.
- **BA207:** ramp → mainline trips count a free-flow length within 5 m of ramp → exit's, about
  940 m. Without the lead-in it would be 620 m.
- **BA208:** scene 4 serves all four movements at ×0.6, off and on, with no clamp.

## Weaving sweep (seeds 42–51, full lab with scene 4)

`multra-weaving-sweep . --seeds 42-51 --factors 0.3,0.4,0.5,0.6,0.8,1.0,1.2`

**Scene 4, the two-entry 300 m weave:**

| Factor | Off: served %, delay (s) | On: served %, delay (s) | Locked seeds |
|---|---|---|---|
| ×0.3 | 100, 6.5 | 100, 6.7 | 0 |
| ×0.6 | 100, 9.8 | 100, 10.5 | 0 |
| ×0.8 | 100, 13.1 | 100, 15.2 | 0 |
| ×1.0 | 100, 20.0 | 100, 26.8 | 0 |
| ×1.2 | 100, 47.7 | 100, 80.6 | 0 |

- **Waiting:** no vehicle waits at a dead end in scene 4 at any factor. With a 300 m approach, a
  mainline car bound for the exit moves toward lane 1 before the weave.
- **Compared with the edge 300 m weave**, at the same volumes, delay is close:
  - switched off at ×1.0: 20.0 s against 19.6 s;
  - switched on at ×1.0: 26.8 s against 33.1 s.

**The edge scenes in the full run are not byte-identical to D458's sweep.** The full lab draws
every scene's arrivals from one random stream, so scene 4's inputs shift the draws. Even at ×0.3,
where nothing interacts, served counts move by up to 7 %. That is a different arrival draw, not
different behaviour. Each edge scene run alone (the rows' runs) is unchanged, because its inputs are
filtered first.

**One clamp:** at ×1.0 switched on, seed 48, the 150 m edge weave (`route-13/lane-1`, t = 261.5 s)
has a car standing mid-crossing at lateral −1.6 m. It is in a scene D459 does not touch. The new
draw exposed it, and it is left open (NEXT).

{table}
