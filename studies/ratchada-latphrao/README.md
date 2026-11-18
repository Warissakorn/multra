# Ratchadaphisek × Lat Phrao (study, 15:00–16:00)

A counted signalised study built on the four-leg drawing of the M2.6 template. It is not a shipped
example (`data/projects/`), it is not installed, and no result from it is validated (rule 4).

| File | What it holds |
|---|---|
| `counts.csv` | The owner's count, 4 approaches × 4 intervals of 15 min × left/through/right/U-turn, in motorcycle, car (with pickups and vans), bus and truck. The count sheet's count tab exported as CSV. |
| `site.csv` | Which leg is which four-leg approach (NW→West, SE→East, S→South, N→North). It also holds the lanes measured on the owner's junction drawing (stop line / exit / right-turn: N 5/3/2, S 4/3/2, NW 5/3/1, SE 3/3/1; not surveyed), the 20 m stop-line distance, `entrylanes,serving` (D580: each movement enters on the lanes that serve it) and the fixed-time plan: 155 s cycle, split phasing, 3 s amber + 1 s all-red. The plan is a Webster design from the counts with those lanes, not a surveyed timing. |
| `ratchada-latphrao.traffic.json` | The project. Regenerate it, never edit it by hand. Test `countstudy` fails when it differs. |

Regenerate from the repository root:

```bash
./build/headless/bin/multra-count-study data studies/ratchada-latphrao/counts.csv studies/ratchada-latphrao/site.csv ratchada-latphrao studies/ratchada-latphrao/ratchada-latphrao.traffic.json
```

## What the drawing does not have

- **The Ratchadaphisek flyover.** It is not drawn, and its traffic was not counted.
- **U-turns.** The drawing has no U-turn exit, so a U-turn joins the right turn of its approach. It uses the same lane and phase, so approach totals stay as counted, but the destination does not.
- **The real lane use, in part.** Lane counts and right-turn lanes follow the drawing. N's two left-turn lanes, S's shared through/right lane and NW's channelised left slip are not drawn: the left turn leaves the kerb lane before the stop line. NW's four through lanes merge into SE's three exit lanes.
- **A driver's late lane choice.** Since D580 (LANE_LOCKS option A) each movement enters the
  approach Link on the lanes that serve it, so no vehicle needs a forced lane change there. Before
  it, every movement spread over every lane and changed into queues that stood still. A vehicle
  that could not change waited at the end of its lane for the rest of the run and blocked the
  lane. On site, drivers pick their lane further upstream, and some change late.
- **Motorcycle filtering.** Motorcycles drive in lanes like cars. About half the counted vehicles are motorcycles, so queues and unserved demand will be larger than on site.
