# Shipped example projects

Four projects, installed with the program and opened by the tests and the CLI. Each is built by a
C++ builder in `tools/` through the editor's own commands, and a test fails when the committed
file and its builder differ: regenerate with the named tool, never edit the JSON by hand.

**None of them is a study.** Volumes, timings and dimensions are plausible round numbers, not
counts, and no result from them is a claim about a real road (PRINCIPLES rule 4). Results stay
"not yet validated" until M6.

| File | What it is for | Traffic | Builder → regenerate with |
|---|---|---|---|
| `four-leg-signalised.traffic.json` | The four-leg signalised junction with right-turn pockets of the M1/M2 done-conditions; the main regression fixture. 900 s of demand plus a 300 s cool-down, so trips released in the window finish (D146, D201 B8) | Left-hand | `tools/four_leg_network.hpp` → `multra-four-leg-fixture` |
| `t-junction-priority.traffic.json` | An unsignalised Yield T-junction whose minor turn crosses one major stream and merges into the other (M3.2.7, A26); the M3.2.7d owner exercise | Right-hand | `tools/t_junction_network.hpp` → `multra-t-junction-fixture` |
| `m2.6-study-template.traffic.json` | The four-leg drawing set up as a gate study: one routeless input per approach over four 15-minute intervals, split by routing decisions, 1 h. **Placeholder volumes, a guessed timing-to-approach mapping and no aerial image**: replace them with a study's counts (OWNER_SITTING B4) | Left-hand | `tools/m26_study_network.hpp` → `multra-m26-study` |
| `lane-change-lab.traffic.json` | A development bed for lane changing, not an acceptance fixture: four separate scenes (overtaking, three lanes, lane drop, diverge) that run in seconds (D98) | Left-hand | `tools/lane_change_network.hpp` → `multra-lane-change-fixture` |

Regenerate from the repository root, for example
`./build/headless/bin/multra-four-leg-fixture data/projects/four-leg-signalised.traffic.json`.
