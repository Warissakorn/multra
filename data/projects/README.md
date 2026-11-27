# Shipped example projects

Twelve projects, installed with the program and opened by the tests and the CLI. Each is built by a
C++ builder in `tools/` through the editor's own commands, and a test fails when the committed
file and its builder differ: regenerate with the named tool, never edit the JSON by hand.

**None of them is a study.** Volumes, timings and dimensions are plausible round numbers, not
counts, and no result from them is a claim about a real road (PRINCIPLES rule 4). Results stay
"not yet validated" until M6.

| File | What it is for | Traffic | Builder → regenerate with |
|---|---|---|---|
| `four-leg-signalised.traffic.json` | The four-leg signalised junction with right-turn pockets of the M1/M2 done-conditions; the main regression fixture. 900 s of demand plus a 300 s cool-down, so trips released in the window finish (D146, D201 B8) | Left-hand | `tools/four_leg_network.hpp` → `multra-four-leg-fixture` |
| `m6-s1-approach.traffic.json` | M6 kind S1: one straight one-lane 300 m approach to a fixed-time signal (cycle 90 s, green 40 s, amber 3 s), 1,200 veh/h of cars, 900 s warm-up and 3,600 s window, `perInput` streams; the S1-a/S1-d/S1-f comparison runs (D535, [M6_S1_RUN](../../docs/reference/M6_S1_RUN.md)) | Left-hand | `tools/m6_s1_network.hpp` → `multra-m6-s1-fixture` |
| `t-junction-priority.traffic.json` | An unsignalised Yield T-junction whose minor turn crosses one major stream and merges into the other (M3.2.7, A26); the M3.2.7d owner exercise | Right-hand | `tools/t_junction_network.hpp` → `multra-t-junction-fixture` |
| `yield-impatience-lab.traffic.json` | The T-junction with 700 veh/h each way on the major road, every area's gap reduced as Aimsun does (5 s to 2 s over 1–2 × the yield time) and SUMO's impatience 0.2 on every behaviour (YIELD_IMPATIENCE, D618); lab placeholders, not validated; the owner's Windows look (OWNER_SITTING_WINDOWS item 52) | Right-hand | `tools/yield_impatience_network.hpp` → `multra-yield-impatience-fixture` |
| `priority-rule-lab.traffic.json` | The T-junction's roads and demand with two Vissim priority rules instead of conflict areas (PRIORITY_RULES, D614–D616): the right turn yields to both streams at its stop line, the left turn has a Stop with no area; the owner's Windows look (OWNER_SITTING_WINDOWS item 51) | Left-hand | `tools/priority_rule_network.hpp` → `multra-priority-rule-fixture` |
| `line-stop-lab.traffic.json` | The four-leg with its catalogs owned and three test behaviours for the owner's line-stop checks (LINE_SETBACK §8, D671): `w74-lab` (BA27's W74 fixture), `w74-sumo` (the SUMO-defaults preset) and `w99-lab` (BA231's W99 fixture); every vehicle type starts on `w99-lab`. Test values, not presets, not validated | Left-hand | `tools/line_stop_lab_network.hpp` → `multra-line-stop-lab-fixture` (takes the data directory first) |
| `m2.6-study-template.traffic.json` | The four-leg drawing set up as a gate study: one routeless input per approach over four 15-minute intervals, split by routing decisions, 1 h. **Placeholder volumes, a guessed timing-to-approach mapping and no aerial image**: replace them with a study's counts (OWNER_SITTING B4) | Left-hand | `tools/m26_study_network.hpp` → `multra-m26-study` |
| `lane-change-lab.traffic.json` | A development bed for lane changing, not an acceptance fixture: four separate scenes (overtaking, three lanes, lane drop, diverge) that run in seconds (D98) | Left-hand | `tools/lane_change_network.hpp` → `multra-lane-change-fixture` |
| `weaving-lab.traffic.json` | A synthetic weaving fixture, not a study: three Type A weaves of 150, 300 and 500 m, each starting at the network edge, origins by entry lane, and a 300 m weave with two entry Links (D449, D459, [WEAVING_FIXTURE](../../docs/reference/WEAVING_FIXTURE.md)) | Left-hand | `tools/weaving_network.hpp` → `multra-weaving-fixture` |
| `lateral-lab.traffic.json` | A development bed for lateral behaviour, not a study: four scenes (timed lane change, lane drop, 150 m weave, keep-left with a mixed fleet) and the only shipped project whose behaviours carry the lateral keys, as lab placeholders (D457, [LATERAL_LAB](../../docs/reference/LATERAL_LAB.md)) | Left-hand | `tools/lateral_lab_network.hpp` → `multra-lateral-fixture` (takes the data directory first) |
| `macro-four-step.traffic.json` | The Macro level end to end, not a study: four zones around a town (two-way radial arterials through a centre, a faster ring road), owning its trip rates, gravity deterrence, transit minutes, car/transit logit, AM period factors and a `four-step` procedure. Run it with `multra-cli --project data/projects/macro-four-step.traffic.json --procedure four-step`, or *Procedure* in Results; congestion splits West → East between the arterials and the ring | Left-hand | `tools/macro_example_network.hpp` → `multra-macro-example` (takes the data directory first) |
| `signalised-node-analysis.traffic.json` | The two letters the program gives, not a study: a signalised four-leg built from two Street segments and the installed template `signal-4-p3-major-far-pockets` (far-turn pockets on the major, D513), with turning counts on its Node for *Analyse* (HCM 7 Ch. 19, analytical) and one signalised travel-time section per movement for LOS from simulated section delay after *Run seeds*. One routeless input and one routing decision per approach; evaluation window 300–1200 s (D500). `multra-cli --project data/projects/signalised-node-analysis.traffic.json --node-analysis` prints the table | Left-hand | `tools/node_example_network.hpp` → `multra-node-example` (takes the data directory first) |

The editor's *File → Open example* menu reads `data/examples.json`: add a row there, with its
name and purpose in English and Thai, for every project added here (`example-menu-ui` fails otherwise).

Regenerate from the repository root, for example
`./build/headless/bin/multra-four-leg-fixture data/projects/four-leg-signalised.traffic.json`.
