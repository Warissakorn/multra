# Lateral lab (D457)

The owner asked (2026-11-04) for a project to test lateral behaviour. The lateral lab is a shipped
project, `data/projects/lateral-lab.traffic.json`, with four small scenes and the lateral switch on.

It is a development bed, not a study and not an acceptance fixture for any gate. Nothing measured in
it is a claim about a real road (rule 4), and its results stay "not yet validated" until M6.

## 1. The exception to D178

- **The only shipped project with the keys:** every behaviour in the project carries the lateral
  keys of [LATERAL](LATERAL.md) §9, so `ScenarioIndex::lateral` is on. No other shipped project
  does, and no `data/` behaviour does.
- **The values are lab placeholders:** uncited, not a preset, and never to be copied into a study.

  | Key | Value | Source |
  |---|---|---|
  | `lateralMinDistanceStanding` | 0.5 m | the lateral run audit's value |
  | `lateralMinDistanceAt50` | 1.0 m | the lateral run audit's value |
  | `maxLateralSpeed` | 1 m/s | the lateral run audit's value |

- **One source:** `tools/lateral_lab_keys.hpp` (`labLateralKeys()`), which `multra-weaving-sweep`
  also uses.
- **Keep-left scene behaviour** (`lateral-lab-keep-left`): it also carries D95's pair and the
  kerb-lane bias, with the values of a passing `kerbbias` case:
  - `discretionaryLaneChangeThreshold` 0.5;
  - `acceptedDecelerationTrailingVehicle` 1;
  - `kerbLaneBias` 1.

  These are placeholders too.
- **Why an exception:** D178 asks for a cited preset before any shipped behaviour carries keys. The
  owner chose, for this lab only, to ship the values used in the evidence, so that lateral motion can
  be watched in the editor.

## 2. Scenes

All scenes use left-hand traffic and 3.5 m lanes, with travel along +x and lane 0 as the kerb lane.
They lie 60 m apart, so each scene gives its own movement rows. Inputs run 0–600 s, and the run
lasts 900 s. Volumes are plausible round numbers, not counts. The geometry is copied from the other
labs, not shared, so their files keep their bytes.

| # | Scene | Layout | Inputs (veh/h) | What to watch |
|---|---|---|---|---|
| 1 | Timed lane change | The lane-change lab's diverge: 3 lanes, 600 m, splitting into a 1-lane exit from lane 0 and a 2-lane exit from lanes 1–2 | Car 400 to exit A; car 1000 and heavy vehicle 150 to exit B | Changes take time (D204); the body straddles two lanes (LATERAL §5); lateral clearance by speed |
| 2 | Lane drop | 2 lanes, a 1-lane Connector, then 2 lanes | Car 900 | Mandatory changes before a dead end, switched on (D450–D456) |
| 3 | Short weave | The weaving lab's 150 m Type A weave, at ×0.8 of its volumes | Mainline 1680 and ramp 640, 30 % of each crossing | Two-way swaps and partner holds (D207) |
| 4 | Keep-left | One 2-lane Link, 1000 m, with its own behaviour type | Car 900, heavy vehicle 200, motorcycle 150 | Kerb-lane bias (D215); bodies of different widths (motorcycle 0.8 m, car 1.8 m) |

**Not modelled:**
- motorcycle filtering, lane sharing and forming at the stop line (M3.8);
- overtaking inside a lane (M3.7.1+).

The lab shows only what LATERAL and [LANE_CHANGE_DURATION](LANE_CHANGE_DURATION.md) implement.

## 3. Builder and rows

- **Builder:** `tools/lateral_lab_network.hpp`, which uses the editor's own commands.
- **Regenerate:** `multra-lateral-fixture data data/projects/lateral-lab.traffic.json`. Never edit
  the JSON by hand. The builder reads `data/` to capture the catalogs the project owns.
- **Rows:** BA198–BA203 in [DRIVING_BEHAVIOUR](../plans/DRIVING_BEHAVIOUR.md) §3, in the test
  `laterallab`.
- **Evidence:** [lateral-lab](../evidence/lateral-lab.md).
