# Ratchadaphisek × Lat Phrao (study, 15:00–16:00)

A counted signalised study built on the four-leg drawing of the M2.6 template. It is not a shipped
example (`data/projects/`), it is not installed, and no result from it is validated (rule 4).

| File | What it holds |
|---|---|
| `counts.csv` | The owner's count, 4 approaches × 4 intervals of 15 min × left/through/right/U-turn, in motorcycle, car (with pickups and vans), bus and truck. The count sheet's count tab exported as CSV. |
| `site.csv` | Which leg is which four-leg approach (NW→West, SE→East, S→South, N→North) and the fixed-time plan: 180 s cycle, split phasing, 3 s amber + 1 s all-red. The plan is a Webster design from the counts, not a surveyed timing. |
| `ratchada-latphrao.traffic.json` | The project. Regenerate it, never edit it by hand. Test `countstudy` fails when it differs. |

Regenerate from the repository root:

```bash
./build/headless/bin/multra-count-study data studies/ratchada-latphrao/counts.csv studies/ratchada-latphrao/site.csv ratchada-latphrao studies/ratchada-latphrao/ratchada-latphrao.traffic.json
```

## What the drawing does not have

- **The Ratchadaphisek flyover.** It is not drawn, and its traffic was not counted.
- **U-turns.** The drawing has no U-turn exit, so a U-turn joins the right turn of its approach. It uses the same lane and phase, so approach totals stay as counted, but the destination does not.
- **The real lane layout.** Every approach has the four-leg's two lanes widened to three at the stop line, with a right-turn pocket. The site's widths and lane counts differ.
- **Motorcycle filtering.** Motorcycles drive in lanes like cars. About half the counted vehicles are motorcycles, so queues and unserved demand will be larger than on site.
