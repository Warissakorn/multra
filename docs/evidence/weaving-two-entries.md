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

## Weaving sweep

The full sweep with scene 4 was still running when this was first committed. Its rows follow in
the next commit.
