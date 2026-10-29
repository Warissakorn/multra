# NODE_ANALYSIS — analytical junction capacity (HCM 7), contract (M13, D370)

Contract before code (D145, D370). The owner asked whether MULTRA can analyse a junction the way
Vistro, Synchro or SIDRA do. This page fixes what that means here: the level, what it reuses, the
first method, its inputs and outputs, the owner choices, the slices and the acceptance rows.
**A1 (§10, D372) and A2 (§11, D373, D374) are implemented, with D383's fixes (§13); the rest is not.** [ROADMAP](../ROADMAP.md) M13 owns status, [NEXT](../NEXT.md) owns
the queue. Owner choices (§8) were answered on 2026-10-27 with every recommendation (D371).

## 1. What it is and where it sits

- **Analytical, deterministic and flow-based.** A capacity procedure takes demand flow rates by
  lane group over an analysis period and computes saturation flow, capacity, v/c, control delay,
  LOS and queue. It uses formulas: there are no vehicles, no seed and no run clock. By how it
  represents traffic it is **macroscopic, at the scale of one junction**, unlike M10's
  network-wide assignment.
- **A fourth method beside Micro, Meso and Macro (D145).** It reads the same authoring network and
  demand (hard rule 3), and its unit is the **Node** ([NODES](NODES.md)). It is not a Micro feature:
  Micro's movement delay stays simulated travel-time delay. `data/los/hcm.json` already says that is
  "not HCM control delay", and this method computes the HCM quantity itself.
- **What the comparable tools do:**
  - Synchro: HCM signalised intersections, ICU, and cycle/split/offset optimisation.
  - Vistro: the HCM chapters 19–22 plus signal optimisation and TIA trip steps.
  - SIDRA: a proprietary lane-based analytical model.

  MULTRA follows **published methods only**: HCM 7 (the owner's copy, [BOOKS](BOOKS.md) §2, commit
  `74f4bc3`), Webster (1958, TP39) for timing. **SIDRA's own capacity and delay formulas are not
  reproduced.**
- **Not validation.** Reproducing an HCM example checks arithmetic, not behaviour (BOOKS §1).
  Every result carries the not-yet-validated marker until an M6-style benchmark passes (rule 4).

## 2. What exists and is reused

| Need | Already in MULTRA |
|---|---|
| Junction object, legs, turns, control type, per-turn g/C | Node, derived from Micro detail (NODES §3); coarse values on a Node without detail: turns, control, major/minor, stage signal (STREET_SEGMENTS_IMPLEMENTED §19–20) |
| Signal timing | Fixed-time programs, heads, groups (M2.7, D359/D360). **Read, never copied** |
| Demand by period | Counted volumes and 15-minute intervals (M2.2), compositions and PCU (S1.2), StudyPeriod (S1.1) |
| Base saturation flow | HCM 7 Exhibit 19-11: 1,900 pc/h/ln, or 1,750 below 250,000 metro population (PDF p. 1430, already cited by [STREET_SEGMENTS](STREET_SEGMENTS.md) §6) |
| Local saturation flow | Micro's queue-discharge measurement (M3.3.1) as an override with provenance ([MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) §6) |
| Results, comparison, export | Results tabs, scenario comparison (M5.8), `csv_format.hpp`, the marker line |

## 3. The first method: HCM 7 Chapter 19, core motorized vehicle methodology

**Slice A1's scope:**
- pretimed control;
- protected movements in exclusive-lane lane groups;
- no initial queue;
- an isolated intersection.

These are the conditions under which Chapter 19 applies Step 4 directly and Eq. 19-19 estimates
uniform delay (Ch. 19 Step 8A, PDF p. 1475). Shared or permitted lane groups, actuated control and
initial queues need Chapter 31's procedures and come in later slices (§7).

Cited by equation, with the copy's PDF page. ✓ means the formula image was read on 2026-10-27;
the rest are short transcriptions that A1 reads before coding.

| Step (Ch. 19 §3) | Quantity | Source |
|---|---|---|
| 1 | Movement groups and lane groups | Step 1, PDF p. 1462 |
| 2–3 | Demand flow rate per movement group and lane group (hourly volume ÷ PHF) | Steps 2–3, PDF p. 1463–1464 |
| 4 | Adjusted saturation flow, `s = s0 fw fHVg fp fbb fa fLU fLT fRT fLpb fRpb fwz fms fsp` | Eq. 19-8 ✓, PDF p. 1465; factors Eqs. 19-9 to 19-14, PDF p. 1467–1470 |
| 5 | Proportion arriving during green `P` (= g/C for random arrivals, platoon ratio 1) | Eq. 19-15, PDF p. 1472 |
| 6 | Phase duration: pretimed, from the program | Step 6, PDF p. 1472 |
| 7 | Capacity `c = N s g/C`; `X = v/c` | Eqs. 19-16, 19-17, PDF p. 1473 |
| 8 | Control delay `d = d1 + d2 + d3` | Eq. 19-18, PDF p. 1474 |
| 8A | Uniform delay `d1 = PF · 0.5 C (1 − g/C)² / (1 − min(1, X) g/C)` | Eq. 19-19 ✓, PDF p. 1475 |
| 8A | Progression factor `PF = (1 − P)/(1 − g/C) × (1 − y)/(1 − min(1, X) P) × [1 + y (1 − P C/g)/(1 − g/C)]`, with `y = min(1, X) g/C` | Eqs. 19-20 ✓, 19-21, PDF p. 1475 |
| 8B | Initial queue delay `d3 = 0` (no initial queue in A1) | Step 8B, PDF p. 1475 |
| 8C | Incremental delay factor `k = 0.50` for pretimed phases | Step 8C, PDF p. 1476 |
| 8D | Incremental delay `d2 = 900 T [(XA − 1) + √((XA − 1)² + 8 k I XA / (cA T))]`, `XA = v/cA`, `cA = c` without initial queue, `I = 1` isolated | Eqs. 19-26 ✓, 19-27, PDF p. 1479 |
| 8F | Approach and intersection delay: flow-weighted means of lane-group delay | Eqs. 19-28, 19-29, PDF p. 1480 |
| 9 | LOS by control delay, F when X > 1 for a lane group; approach and intersection by delay only | Exhibit 19-8 ✓, PDF p. 1422 |
| 10 | Back of queue and queue storage ratio | Step 10, PDF p. 1481 → Ch. 31 §4; slice A2 |

**Units.**
- MULTRA stores metres and seconds.
- HCM factors with thresholds in feet (lane width, Eq. 19-8's `fw`) compare after converting
  exactly: 1 ft = 0.3048 m.
- Output delay is in s/veh, flow in veh/h, saturation flow in veh/h/ln.

## 4. Inputs, and who owns each

**Rule:** a field is stored only once something reads it (D272). Anything already in the document
is read, never copied.

- **From the Node (derived, NODES §3):**
  - legs, turns, control;
  - each turn's heads, and through them its program, g/C, cycle, yellow and red.

  A Node without detail uses its coarse stage signal.
- **Lane groups, lane counts and lane widths are derived, not stored (D371).** For each lane of an
  in-leg, A1 collects the set of turns whose paths start on it; lanes serving one turn form that
  turn's exclusive lane group, and widths come from the Link's lanes. A1 refuses a shared or
  permitted lane group with a named advisory. A Node without detail is refused with
  `NODE_ANALYSIS_NEEDS_DETAIL`; stored lane groups for coarse Nodes belong to a later slice.
- **New on the Node, schema 58 with A1:**
  - **Analysis volumes:**
    - per turn: hourly volume, PHF and %HV;
    - optionally, per-15-minute rows, the same structure as M2.2 intervals (a later slice).
  - **Approach adjustments:** grade (%), parking manoeuvres per hour, buses stopping per hour; and
    area type (CBD or not) for the Node.
  - **Base saturation flow:**
    - default from Exhibit 19-11;
    - an override with a source string, such as "measured, M3.3.1, <date>".
  - **Analysis period T** (h, default 0.25).
- **Lost time and effective green** come from the program's intervals, with HCM's definitions:
  start-up lost time and extension of effective green (Ch. 19 §2; Eq. 19-1 and its definitions, PDF p. 1415). A1 reads
  their defaults from the image before use.

## 5. Outputs

- **Per lane group:**
  - v, s, c, X, g/C;
  - d1, d2, d3, d;
  - LOS.
- **Per approach:** d and LOS.
- **Per intersection:**
  - d and LOS;
  - critical v/c, from Chapter 19's critical-lane-group definition, in A2 (§11).
- **Per lane group, from A2:** its turns and turn shares (P_L, P_R); average and 95th-percentile
  back of queue and storage ratios (§11).
- **Each result names:**
  - the method ("HCM 7 Ch. 19, analytical") and the document revision;
  - every default used, and every refused lane group with its advisory.
- **Format.** JSON and CSV through the shared formatters, with the not-validated marker line.
  Results → *Node analysis* table, and the Node's own row. A result is stale when the revision
  changes (WORKFLOW §5).
- **No Micro, Meso or Macro output changes.** A project without analysis inputs keeps its bytes.

## 6. Workflow in the editor (target)

1. Draw the junction or use a Street segment-generated Node. A Node with detail derives lanes and turns.
2. Node → **Analysis**: lane groups, adjustments, base saturation flow.
3. Demand:
   - enter turning counts (15-minute, PHF, %HV);
   - or *Fill from model* (Micro routes and inputs through the Node's turns, later Macro), with
     provenance and one Undo.
4. Control: the existing signal program (fixed-time), or the Node's coarse stage signal.
5. **Analyse.** Instant, deterministic, on the UI thread: a pure function of the document.
6. Optional **timing proposal** (A6): Webster cycle and splits shown before/after, and *Use*
   writes the program in one Undo.
7. Compare scenarios and export (M5.8 tables, CSV).
8. Optional: run the same timing in Micro, 10 seeds, and show analytical against simulated delay
   side by side, labelled as different quantities.

## 7. Slices

| Slice | Scope |
|---|---|
| A0 | This contract; owner choices (§8); D370 |
| A1 | Engine `src/analysis/` (pure, imports nothing). Ch. 19 Steps 1–9 for pretimed protected exclusive lane groups. Node fields and codec, CLI `--node-analysis`, rows NA1–NA8 |
| A2 | Shared lane groups (Ch. 31 §2), protected only; critical v/c; back of queue and storage ratio (Ch. 31 §4); Exhibit 19-41. Contract §11 (D373), rows NA9–NA14 |
| A3 | Permitted and protected-permitted turns: Ch. 31 §3's QAP procedure for selected lane groups, Steps 1–7 (PDF p. 2986–3015), with Ch. 31 §2's pedestrian and bicycle factors (from PDF p. 2975). Contract [NODE_ANALYSIS_A3](NODE_ANALYSIS_A3.md) (D376) |
| A4 | Two-way stop control (Ch. 20) |
| A5 | All-way stop (Ch. 21) |
| A6 | Timing proposal: Webster cycle and splits (TP39), isolated |
| A7 | Roundabouts (Ch. 22), using merged Nodes (STREET_SEGMENTS RD15) |
| A8 | Actuated control (Ch. 19 Step 8C, Ch. 31); then corridor offsets (Ch. 16/18) as its own contract |
| A9 | Editor table, *Fill from model*, Micro side-by-side |

Each slice begins with its rows. Its gate is the rows passing, the marker kept, and other levels'
bytes unchanged.

## 8. Owner choices (answered 2026-10-27, D371: 1a–9a, "ใช้ตามแนะนำทั้งหมด")

1. **Placement.**
   - **(a, recommended)** Milestone M13 with its own engine `src/analysis/`.
   - (b) An extension of M5 evaluation.
2. **A1 scope.**
   - **(a, recommended)** Pretimed, protected, exclusive-lane, isolated, no initial queue (§3).
   - (b) Start with two-way stop control.
3. **Demand source.**
   - **(a, recommended)** Turning volumes stored on the Node, plus *Fill from model* with provenance
     (A9).
   - (b) Derived only, from Micro routes and inputs.
4. **Signal timing.**
   - **(a, recommended)** Read the existing fixed-time program through the Node's heads. A Node
     without detail uses its coarse stage signal, and no second timing copy is kept.
5. **Units.**
   - **(a, recommended)** Store metric and convert at HCM's foot thresholds (§3).
6. **Base saturation flow.**
   - **(a, recommended)** Exhibit 19-11's default by area population, with a local override that
     records its source.
7. **LOS bounds.**
   - **(a, recommended)** A new data pack for HCM 7 Exhibit 19-8, which applies X > 1 → F to lane
     groups. It is used by this method only, and `data/los/hcm.json` (6th edition, Micro) is
     unchanged.
8. **Acceptance.**
   - **(a, recommended)** A1 uses hand-computed rows per equation, plus a reproduction of Exhibit
     19-41's illustrative service volumes (pretimed, defaults stated in its notes, PDF p. 1570–1571)
     within a stated tolerance.
   - Chapter 31's Example Problem 1 (PDF p. 3132–3145) is actuated with protected-permitted left
     turns, so it becomes the acceptance case of A3 + A8.
9. **Marker and label.**
   - **(a, recommended)** Every output carries the not-validated marker and the label "analytical
     (HCM 7), not simulated".

## 9. Acceptance rows (A1; passing, D372)

| Row | Checks |
|---|---|
| NA1 | `s` from Eq. 19-8 with every factor 1 equals `s0`; each A1 factor moves `s` as its equation says, one hand-computed case each |
| NA2 | `c = N s g/C` and `X = v/c` on a hand case |
| NA3 | With `P = g/C` (random arrivals), `PF = 1` to 1e-12, and `d1` matches a hand value |
| NA4 | `d2` matches a hand value for X = 0.5, 1.0 and 1.2 with T = 0.25, k = 0.5, I = 1 |
| NA5 | Approach and intersection delay are flow-weighted (Eqs. 19-28, 19-29) |
| NA6 | LOS letters at Exhibit 19-8's bounds (10, 20, 35, 55, 80 s; X > 1 → F) |
| NA7 | A shared or permitted lane group is refused with a named advisory and no number |
| NA8 | Every Micro, Meso and Macro output, and every project without analysis inputs, keeps its bytes |

**Exhibit 19-41 (choice 8a) is carved out to A2.** Its notes (PDF p. 1570–1571) set "separate
right-turn lane: no", so every service volume needs a shared through/right lane group. That is
Chapter 31 §2's shared-lane procedure, outside A1. A2 reproduces the exhibit with that procedure.

## 10. A1 as implemented (D372)

- **Engine** `src/analysis/signalised.*`: pure, imports nothing (architecture check). Factor values,
  each read from the copy's image or text layer on 2026-10-27:

  | Factor | Value | Source |
  |---|---|---|
  | `fw` | 0.96 below 10.0 ft, 1.00 from 10.0 to 12.9 ft, 1.04 above; refused below 8 ft | Exhibit 19-20, PDF p. 1466 |
  | `fHVg` | `(100 − 0.79 P_HV − 2.07 P_g)/100` downhill, `(100 − 0.78 P_HV − 0.31 P_g²)/100` otherwise; P_HV ≤ 50 %, −4 ≤ P_g ≤ 10 % | Eqs. 19-9, 19-10, PDF p. 1467 |
  | `fp` | `(N − 0.1 − 18 N_m/3,600)/N ≥ 0.050`; 1.00 without parking; N_m ≤ 180/h | Eq. 19-11, PDF p. 1468 |
  | `fbb` | `(N − 14.4 N_b/3,600)/N ≥ 0.050`; N_b ≤ 250/h | Eq. 19-12, PDF p. 1469 |
  | `fa` | 0.90 in a CBD, else 1.00 | PDF p. 1469 |
  | `fLU` | Through 1.000 / 0.952 / 0.908; left (far) 1.000 / 0.971; right (kerb) 1.000 / 0.885, by lanes; more lanes take the smallest | Exhibit 19-15, PDF p. 1444 |
  | `fRT`, `fLT` | 1/1.18 kerb turn, 1/1.05 far turn (protected) | Eqs. 19-13, 19-14, PDF p. 1470 |
  | Others | `fLpb`, `fRpb`, `fwz`, `fms`, `fsp` = 1 (protected, no work zone or spillback) | Eq. 19-8 |

  HCM's right turn is the kerb turn and its left turn the far turn on either driving side. A turn
  within 45° of straight on is through.
- **Defaults** (Exhibit 19-11, PDF p. 1430–1431), listed in each result's `defaults`:
  - s0 1,900 pc/h/ln;
  - T 0.25 h;
  - PHF 0.92 at ≥ 1,000 veh/h entering, else 0.90 (1.00 when T is not 0.25 h);
  - 3 % heavy vehicles.

  Effective green is g = G − l1 + e with l1 = e = 2.0 s (Eq. 19-1's definitions, PDF p. 1415), so
  g = G. P = g/C (Eq. 19-15, PDF p. 1472, platoon ratio 1); k = 0.50 and I = 1.
- **Inputs:** `network.nodes[i].analysis` at schema 58 (PROJECT_SCHEMA).
  - Turns are named by in-leg and out-leg Link id.
  - A turn without a row gives `NODE_ANALYSIS_NO_VOLUME`.
  - Approach rows carry grade, parking and buses.
  - `setNodeAnalysis` stores an all-default value as none.
- **Lane groups and refusals** (`compileNodeAnalysis`): as §4. The codes are listed in
  `node_analysis_compile.hpp`, and a method range gives `ANALYSIS_*` on the lane group with no
  number.
- **Run and outputs:**
  - `runNodeAnalysis`: per lane group, approach and Node; the `hcm7-signalised` pack; the input
    manifest.
  - CLI `multra-cli --project F --node-analysis [--csv F]`.
  - JSON and CSV carry `validated: false` and the method label.
- **Rows:** NA1–NA8 in `tests/node_analysis_tests.cpp` (group `nodeanalysis`), plus an end-to-end run
  on the shipped four-leg's pocket turns (`tests/node-analysis-four-leg.traffic.json`, CLI test).
- **Not in A1:** the editor tab and *Fill from model* (A9), and coarse Nodes (stored lane groups,
  later). The other slices are in §7.

## 11. A2: contract (D373) and as implemented (D374)

The owner's answers on 2026-10-27:
- "ทำครบใน PR #200" ("do it all in PR #200");
- "ห่วงโซ่ไม่ซ้อนทับ" ("a non-overlapping chain", for critical v/c);
- "เก็บ speed limit ต่อ approach" ("store a speed limit per approach", for queues).

A2 stays **pretimed and protected**: permitted turns are A3 and actuated control is A8. Formula
images were read on 2026-10-27; pages are the copy's PDF pages.

**Lane groups become lane-based** (Ch. 19 Step 1, PDF p. 1462–1464).
- Per in-leg, adjacent lanes serving the same single turn form one exclusive lane group, and each
  lane serving two or more turns is its own shared lane group.
- **Analysed layouts** are Exhibit 31-6's (PDF p. 2952): exclusive l, t, r; shared sl, sr, lr; and a
  one-lane approach serving every turn. Any other layout gives `NODE_ANALYSIS_LANE_LAYOUT` for the
  approach.
- **Refusals for a shared lane:**
  - every turn in it must be protected and share one green interval;
  - a permitted turn in a shared lane refuses the whole approach with `NODE_ANALYSIS_PERMITTED`;
  - two programs in one shared lane give `NODE_ANALYSIS_SIGNAL`.

**Approach flow distribution** (Ch. 31 §2, Eqs. 31-46 to 31-66, PDF p. 2949–2960).
- **Inputs:**
  - `P_lc = 1 − (2 v_app/s_lc − 1)² ≥ 0`, with `s_lc = 3,600/3.7` and `v_app` per Eq. 31-51 (31-52 for
    an lr lane);
  - `E_L,m = (E_L − 1) P_lc + 1` with E_L 1.05, and `E_R,m = (E_R − 1) P_lc + 1` with E_R 1.18;
  - fLpb = fRpb = 1 ("no pedestrian or bicycle conflict", a listed default).
- **Iteration:** Steps B–H repeat until the shared lane flows change by less than 0.1 veh/h. A cap
  of 100 iterations gives `NODE_ANALYSIS_NO_CONVERGENCE`.
- **Which lane groups exchange flow:** the flow-ratio equalisation (Eq. 31-64) runs over the lane
  groups that can trade vehicles: the through-serving lanes, plus the exclusive lanes of a turn that
  also uses a shared lane. A turn without a shared lane keeps all its flow in its exclusive lanes;
  that is Eq. 31-55 with `v_sl,lt = 0`.
- **Saturation flow per lane, before Eq. 19-8's other factors:**
  - sr: Eq. 31-61, `s_th/(1 + P_R (E_R,m − 1))`;
  - lr: Eq. 31-63;
  - sl (protected, split phasing): Eq. 31-63's form with `P_R = 0` (MULTRA's reading; Eq. 31-59 is
    for permitted operation);
  - t: Eq. 31-62 with `f_s = 1` (protected).
- **Exclusive lane groups keep A1's factors exactly**, including fLU (Exhibit 19-15) for more than one
  lane. A shared lane group has fLU = 1 (Ch. 19, PDF p. 1469) and its shared factor in place of fLT/fRT.
- **Capacity and delay:** `c = N s g/C`. Uniform delay is Exhibit 31-11's polygon, which Ch. 31 §3
  says also applies to split phasing and to shared through-and-right lane groups (PDF p. 3002,
  3028). For one green it equals Eq. 19-19; NA10 integrates the polygon to show that.

**Critical v/c** (Eqs. 19-30, 19-31, PDF p. 1485), generalised to signal-group programs.
- Each analysed lane group has `y = v/(N s)` and its displayed green interval.
- **Critical chain:** the set of lane groups whose displayed greens are pairwise non-overlapping
  around the cycle, with the largest Σy (on a tie, the larger Xc).
- **Lost time and Xc:** `L = C − Σ g` over the chain, which is Σ lt when the chain's phases tile the
  cycle, as a dual-ring critical path does. Then `Xc = C/(C − L) · Σ y`.
- **Status:** this is MULTRA's generalisation and equals HCM's result on a ring/barrier program. It
  is not HCM text.
- A Node with any refused turn has no Xc (`NODE_ANALYSIS_PARTIAL`).

**Back of queue and storage ratio** (Ch. 31 §4, PDF p. 3016–3039): basic arrival–departure polygon
(Exhibit 31-25), which PDF p. 3028 applies to every A2 lane group.

| Quantity | Formula | Source |
|---|---|---|
| `d_a` | `[1.47 (S_a − S_s)]²/(2 · 1.47 S_a) · (1/r_a + 1/r_d)`, S_s 5 mi/h, r_a 3.5, r_d 4.0 ft/s² | Eq. 31-131, p. 3022 |
| `S_a` | `0.90 (25.6 + 0.47 S_pl)`, S_pl the posted limit, mi/h | Eq. 31-132, p. 3023 |
| `t_f` | Eq. 31-137 if `d_a ≤ (1 − P) g X`, else Eq. 31-138 | p. 3033 |
| `N_f`, `Q1` | Eq. 31-139 or 31-140 (same branch); `Q1 = N_f` | p. 3034 |
| `Q2` | `c_A d2/(3,600 N)`, `c_A = c` | Eq. 31-142, p. 3035; "conditions for use: none" (Exhibit 31-56, p. 3108) |
| `Q3` | 0 (no initial queue) | Eqs. 31-143 to 31-148 with `Q_b = 0` |
| `Q95` | `(Q1 + Q2) f_B% + Q3`, z 1.64, I 1 | Eqs. 31-150, 31-151 (v ≥ c), 31-153 (v < c), p. 3037–3038 |
| `R_Q`, `R_Q95` | `L_h Q/L_a`, `L_h = 25 (1 − 0.01 P_HV) + 0.01 · 45 · P_HV` ft | Eqs. 31-154 to 31-156, p. 3038–3039 |

- **Image check:** Eq. 31-151's image is clipped at its right edge after `(1.0 − e^(2−2X_A)`. The
  copy's PDF text layer closes it with `)`, and that reading is used.
- **Flow cap:** `q` is capped at the lane group's capacity (PDF p. 3024), so `min(1, X)`.
- **Inputs:**
  - **Speed limit:** the in-leg Link's posted `speedLimit` (km/h, J1, D390, schema 60). Schemas
    58–59 stored it on the approach row; reading such a file moves it to the Link. An in-leg without
    one has no queue, and its lane groups are listed under `NODE_ANALYSIS_NO_SPEED`; every other
    result stays.
  - **Storage `L_a`:** the in-leg Link from its start to where the lane group's first turn leaves it
    (its Connector's `from` station, else the Link's end), the stop line. Lanes run the whole Link.

J3a stores compass overrides independently on the Node (schema 61, D394); `approachLayout` reads
them for labels. HCM inputs and storage still read the drawing as before, never the template recipe.

J2 (D391, D392) adds a separate derived `approachLayout` for junction templates and UTDF
([JUNCTION_TEMPLATES §6](JUNCTION_TEMPLATES.md#6-the-lane-layout-derived-and-never-stored-d272)).
It recognizes a pocket's widening Connector and its reference length separately from full-width
storage, including across an ordinary later split. Its pocket walk stops at a Connector inside
any Node, even straight through (D391 review). This slice keeps HCM's lane-group, movement,
storage and refusal derivation unchanged: adoption needs byte parity on NA1–NA23, and B29 (e)'s
HCM storage choice still waits on the owner.

**Exhibit 19-41** (PDF p. 1570–1571).
- **Inputs,** from its notes: 0 % HV, PHF 0.92, 12 ft, 0 % grade, pretimed C 90 s, an exclusive
  protected left lane at g/C 0.10, right turns in a shared lane, 10 % left and 10 % right, no
  parking or buses, and non-CBD. N through lanes means an approach of N − 1 exclusive through lanes
  plus one sr lane.
- **Service volume:** the largest total approach volume whose approach delay is within the letter's
  bound and whose through lane groups have X ≤ 1. The note gives "v/c > 1.0" as an E threshold. The
  exclusive left lane caps at 1,665 veh/h, which the exhibit exceeds, so its X is not applied.
- **Prototype (2026-10-27):** 29 of 36 cells are within ±5 %.
- **The seven exceptions,** computed value against the exhibit:

  | Cell (g/C, lanes, LOS) | Computed | Exhibit | Deviation |
  |---|---|---|---|
  | 0.40, 1, B | 142 | 130 | +9 % |
  | 0.40, 3, E | 2,193 | 2,000 | +10 % |
  | 0.45, 3, D | 2,289 | 2,000 | +14 % |
  | 0.45, 3, E | 2,482 | 2,250 | +10 % |
  | 0.50, 2, D | 1,885 | 1,790 | +5 % |
  | 0.50, 3, D | 2,490 | 2,000 | +25 % |
  | 0.50, 3, E | 2,773 | 2,500 | +11 % |

  Three of them are the exhibit's "2,000" entries, which look capped.
- **NA13** checks the 29 cells to ±5 % and pins the seven at their computed values. Nothing is tuned
  to close them. The C++ engine gives the prototype's values to 0.5 veh/h.

**Rows (A2):**

| Row | Checks |
|---|---|
| NA9 | Lane-flow procedure by hand: t+sr converges to equal v/s; a one-lane approach; P_lc at v_app 0, s_lc/2 and above s_lc |
| NA10 | Eqs. 31-61 and 31-63 by hand; exclusive groups keep A1's numbers exactly; Exhibit 31-11's polygon integrated equals Eq. 19-19 |
| NA11 | Xc on a tiled four-stage program by hand; an overlapping program takes the larger chain; a refused turn gives no Xc |
| NA12 | d_a, t_f (both branches), Q1, Q2, Q95 (v < c and v ≥ c), R_Q by hand |
| NA13 | Exhibit 19-41: 29 cells within ±5 %, the seven above pinned |
| NA14 | Refusals: permitted far turn in a shared lane, layout outside Exhibit 31-6, two programs in a shared lane, no speed limit |

NA8 still holds. `cli-node-analysis`'s expected numbers change, because the four-leg's shared
through lanes are now analysed.

**As implemented (D374).**
- **Engine** (pure, imports nothing):
  - `src/analysis/lane_flow.*`: `approachLaneFlows`, Ch. 31 §2. Eq. 31-64's numerator is the demand
    the trading lane groups carry between them, so every movement is conserved; convergence needs
    each shared lane's flow and its turns to move by less than 0.1 veh/h.
  - `src/analysis/critical.*`: `criticalRatio`.
  - `src/analysis/queue.*`: `approachSpeed`, `accelerationDelay`, `backOfQueue`, `storageRatio`.
  - `signalised.*`: a shared group's `sharedShare` is `fShared` in Eq. 19-8.
- **Compile:** lane groups carry their turns, kind, `coupled`, green start and storage. An approach
  with a shared lane is analysed whole or refused, every turn with the same code.
- **Run and outputs:**
  - Per lane group: `kind`, `turns`, `flowRate`, `farTurns`, `kerbTurns`, `fShared` and `queue`
    (t_f, Q1, Q2, Q3, Q, Q95, storage in m, R_Q, R_Q95).
  - Per Node: `critical` (Xc, Σy, L, lane group names).
  - The CSV adds `farShare`, `kerbShare`, `criticalRatio`, `queue`, `queue95`, `storageRatio` and
    `storageRatio95`.
  - New advisories: `NODE_ANALYSIS_LANE_LAYOUT`, `NO_CONVERGENCE`, `PARTIAL` and `NO_SPEED`.
- **Four-leg** (`tests/node-analysis-four-leg.traffic.json`: 150 far, 400 through and 100 kerb veh/h
  per approach, 50 km/h):
  - West sr group: s 1,735.9, v 262.7 veh/h, v/s equal to the t group's 0.1513.
  - Approaches 44.29 and 75.44 s; node 59.86 s (E).
  - Xc 0.726, over the four approaches' sr groups, with L 20 s.
- **Rows:** NA9–NA14 pass in `tests/node_analysis_a2_tests.cpp`.

## 12. Outlines of later slices

Each later slice is outlined here with the questions its owner choices must settle. No formula,
value or row is written until the slice's own contract, which follows the owner's answers and a
reading of the formula images (BOOKS rule). Only the next slice's questions are put to the owner
([OWNER_SITTING](../plans/OWNER_SITTING.md) B25 for A3, answered in D375); the others wait here.

**A3: permitted and protected-permitted turns.** A turn that yields during its green to a
conflicting movement is analysed instead of refused with `NODE_ANALYSIS_PERMITTED`, by Ch. 31 §3's
procedure for selected lane groups: permitted green, time before the first left-turn vehicle
arrives (HCM's left turn is the far turn), permitted saturation flow, through-car equivalent, proportion of turns in a shared lane,
lane group saturation flow and its QAP (Steps 1–7, PDF p. 2986–3015). Decided 1a–5a on the owner's
delegation (D375, B25); contract [NODE_ANALYSIS_A3](NODE_ANALYSIS_A3.md) (D376):
1. **Scope.** (a, recommended) Exclusive and shared lanes, far and kerb turns (HCM's left and right,
   on either driving side as §10 maps them), pretimed.
   (b) Exclusive lanes first, shared lanes later.
2. **Recognising protected-permitted operation.** (a, recommended) Derived from the program and the
   Node's conflicts: the part of a turn's green that overlaps the green of a movement it yields to
   is permitted, the rest protected; no stored field. (b) A per-turn operation field.
3. **Opposing flow.** (a, recommended) The movements the turn yields to, from Node membership's
   conflicts. (b) Entered per approach.
4. **Pedestrians and bicycles.** (a, recommended) Still none (fLpb = fRpb = 1), stated as a default.
   (b) Ch. 31 §2's factors, with new pedestrian and bicycle volume inputs.
5. **Acceptance.** Ch. 31 Example Problem 1 is actuated (A8). (a, recommended) Hand rows per
   equation; Example Problem 1 stays the A3 + A8 case. (b) Also Example Problem 1 with its reported
   average phase durations as pretimed inputs, labelled an approximation.

**A4: two-way stop control (Ch. 20).** Capacity and delay of the minor movements at a Node whose
control is Stop or give-way on the minor approaches. Questions: is the major street read from the
Node's control or stored; are flared minor approaches and two-stage gap acceptance in or refused?

**A5: all-way stop control (Ch. 21).** Questions: only the geometries the chapter supports, with
the others refused; where the all-way control is read from.

**A6: timing proposal (Webster, TP39), isolated.** Questions: is the proposed cycle and split
written back as a new program (one Undo step) or only shown; where lost time per phase comes from.

**A7: roundabouts (Ch. 22).** Needs merged Nodes (STREET_SEGMENTS RD15). Questions: single-lane and two-lane
entries only; how circulating flow is derived from the Node's turns.

**A8: actuated control (Ch. 19 Step 8C, Ch. 31).** Questions: which controller and detector
inputs are stored, and whether they reuse the Micro signal controller's fields. Corridor offsets
(Ch. 16/18) stay a contract of their own.

**A9: editor table and *Fill from model*.** Questions: where the table lives in the editor, and
what provenance a filled volume shows.

## 13. Review fixes (D383)

A review of 2026-10-28 found three ways ordinary edits left the inputs wrong without saying so; each
row failed before its fix.

| Row | Checks |
|---|---|
| NA21 | Splitting the A1 fixture's in-leg `link-4` keeps all 12 lane groups and the Node delay; turn `in` and approach rows follow the end at the Node to the downstream piece, as destination zone connectors do. An out-leg split keeps its id |
| NA22 | A turn row naming no turn of the Node is `NODE_ANALYSIS_STALE_TURN` at `analysis.turns[k]`, an approach naming no in-leg `NODE_ANALYSIS_STALE_APPROACH` at `analysis.approaches[k]`; neither changes a lane group |
| NA23 | A polygon taking in the pockets (±70 m) is advised under its membership code (`NODE_CONNECTOR_CROSSES`), not `NODE_ANALYSIS_NEEDS_DETAIL`, which stays for a Node with no turn |

Still open: approach and Node delay and LOS are reported under `NODE_ANALYSIS_PARTIAL`
([OWNER_SITTING](../plans/OWNER_SITTING.md) B29 (d)).
