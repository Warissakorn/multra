# LINE_SETBACK — a set-back at holding lines (contract)

Written 2026-11-26 (D669). It is a contract with measured options; **nothing is implemented**. The owner asked
whether `lineRest` should stop a suitable distance short of the line, then chose "ตามแนะนำ": sources first (D668), then
this contract for a key with default 0. The measurements are in [line-setback-options](../evidence/line-setback-options.md).

## 1. Today

- Under `lineRest`, every model rests with its front exactly at a holding line ([LINE_STOP](LINE_STOP.md) §2, D652).
- **Other programs (D668):**
  - Vissim stops 0.5 m before a signal head by default, using a static-obstacle standstill distance;
  - SUMO's `jmStoplineGap` is 1 m at signal and priority stop lines, and 0 at minor ones;
  - neither cites field data.
- An NGSIM look, on an unverified stop-bar stand-in, puts first cars at a median of about 1 m short.

## 2. The rule proposed

1. **A distance `s` (m) short of the line** becomes the target of the line law. Only where the vehicle comes to rest
   changes. The cap, the line finish, the amber and red decisions, red-running and every observer keep the real
   line, so a vehicle resting short of its line has not passed it.
2. **Default 0, written only when not 0.** Every existing file, project and result is then byte-identical: M0,
   `lineRest` off, and every project without the key. No value enters without the owner or a study setting it
   (D178).
3. **Range 0–10 m**, Vissim's upper bound (D668). Above 0 there is no lower bound: SUMO accepts 0, and so does the
   rule.
4. **Only under `lineRest`.** With the switch off, the models' own standstill gaps apply as before (LINE_STOP §1).

## 3. Options

| Choice | Options | Measured (evidence) |
|---|---|---|
| **Where the key lives** | **H1, the driver behaviour** (as Vissim and SUMO): it can differ by type, for example buses or trucks. H2, the project's run settings, one value for every vehicle | Not a measurement question. Under H1 the behaviour dialog gains one field (all three models), under H2 *Run settings* does |
| **Which lines** | **C1, signal heads only.** C2, every holding line: signals, give-way and Stop lines, zones and gates | **C2 costs 2–22 % of delay at give-way lines** (T-junction W74 at 1 m +22 %), because a vehicle at rest short of the line must start again to reach it. Under C1 the give-way labs are identical |

**At signal heads, s = 0.5 and 1 m** (per run):
- every arm passes no red;
- delay changes by under 0.4 %, except W74 on M2.6 (+4–6 %);
- startup lost time rises by about 0.1 s per 0.5 m (the four-leg prototype −0.59 → −0.46 → −0.35 s), still
  below HCM's 2.0 s;
- headway falls by 0.01–0.03 s;
- clamps change by under 2 per run.

## 4. The session's recommendation

**H1 + C1:** a behaviour key `lineSetback`, applied at signal heads only.
- **H1** follows both programs read, and allows a per-type value.
- **C1** follows SUMO's distinction (`jmStoplineGapMinor` 0) and avoids C2's measured cost at give-way lines.
  Extending to other lines would need its own source.

## 5. Rows (for the code session, once chosen)

| Row | What | Pass |
|---|---|---|
| BA273 | One car at a red, `lineSetback` 0.5 / 1 / 2 | Rests with its front `s` short (±0.01 m); no clamp; crosses the line after green |
| BA274 | The key absent | `multra-cli 42`, and every shipped project's summary, byte-identical |
| BA275 | Give-way, Stop, zone and gate lines (C1) | Rest at the line, as today |
| BA276 | Amber and red under a set-back | The decision and red-running use the real line; the four-leg passes no red |
| BA277 | Codec and dialog | Round trip at the next schema; absent when 0; range 0–10 refused outside; the field fits in Thai and English |

The next schema step is read-compatible: an old file has no key, which means 0.

## 6. Outside this contract

- **Resting exactly at the line on `signalised-node-analysis`** (found by these runs). Today's engine clamps 4.7
  times per run for the prototype there, with +7 s of delay. A set-back of 0.01 m removes it, which suggests a
  front resting exactly at a connector's start is read as inside the node.
  - It is a defect of today's default, separate from this key. It is to be diagnosed on its own.
  - A set-back would hide it, not fix it.
- A non-zero default needs a field source (D668).

## 7. The owner's choice and the implementation (D670)

The owner chose **H1 + C1** ("ตามแนะนำ").
- **Engine.** A behaviour's `lineSetback` (m) moves the line law's target that far short of a signal head
  (`setBackHead`, `src/core/line_stop.cpp`).
  - Give-way, Stop, zone and gate lines are not set back.
  - The tick cap, the line finish, the amber decision, red-running and the observers keep the real line.
  - It acts only under `lineRest`.
  - Absent or 0, nothing changes: M0 and every shipped project are byte-identical.
- **File.** The key is optional, written only when present, and needs schema 88. Below 88 it is refused at its path
  (`EDIT_UNSUPPORTED_FIELD`). Outside [0, 10] it is `INVALID_BEHAVIOUR_PARAMETER`.
- **Dialog.** The *Driving behaviours* dialog has a `lineSetback` checkbox and field on its junction page, for
  every model.
- **Rows** BA273–BA277 (`linesetback`).
- **The exact-line clamps** on `signalised-node-analysis` (§6) are not touched. Today's default is unchanged.
  - **Diagnosis so far:** the clamped car goes straight on a 2.73 m connector. A turning car on a 22 m connector
    passes its give-way line under `give-way/connector-44/to/connector-42` (gap 3 s, headway 7 m) and enters the
    same lane 0 m ahead of it. The leader cap binds at 0. There are no conflict zones in this project.
  - Why the rule lets the turner go, and how that ties to first cars resting exactly at a connector's start, is not
    proven.

## 8. A lab for the owner's checks (D671)

The owner asked for W74 and W99 test profiles so that no model key is typed by hand. The program has no behaviour
import, so the profiles ship as a project.
- **`data/projects/line-stop-lab.traffic.json`** (*File → Open example*, "Line-stop lab") is the four-leg with its
  catalogs owned and three more behaviours:
  - `w74-lab`: BA27's W74 fixture;
  - `w74-sumo`: the SUMO-defaults preset;
  - `w99-lab`: BA231's W99 fixture.
- Every vehicle type starts on `w99-lab`. Switching a type to another behaviour in the behaviour library is one
  choice, with nothing typed.
- These are test values, not presets for studies, and nothing is validated (rule 4, D178).
- **Builder:** `tools/line_stop_lab_network.hpp` → `multra-line-stop-lab-fixture data <file>`. The `linesetback`
  group checks that the committed file is its output.

