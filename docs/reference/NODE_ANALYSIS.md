# NODE_ANALYSIS — analytical junction capacity (HCM 7), contract (M13, D370)

Contract before code (D145, D370). The owner asked whether MULTRA can analyse a junction the way
Vistro, Synchro or SIDRA do. This page fixes what that means here: the level, what it reuses, the
first method, its inputs and outputs, the owner choices, the slices and the acceptance rows.
**Nothing here is implemented.** [ROADMAP](../ROADMAP.md) M13 owns status, [NEXT](../NEXT.md) owns
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
| Junction object, legs, turns, control type, per-turn g/C | Node, derived from Micro detail (NODES §3); coarse values on a Node without detail: turns, control, major/minor, stage signal (ROADS_IMPLEMENTED §19–20) |
| Signal timing | Fixed-time programs, heads, groups (M2.7, D359/D360). **Read, never copied** |
| Demand by period | Counted volumes and 15-minute intervals (M2.2), compositions and PCU (S1.2), StudyPeriod (S1.1) |
| Base saturation flow | HCM 7 Exhibit 19-11: 1,900 pc/h/ln, or 1,750 below 250,000 metro population (PDF p. 1430, already cited by [ROADS](ROADS.md) §6) |
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
  - critical v/c, from Chapter 19's critical-lane-group definition, in A2.
- **Each result names:**
  - the method ("HCM 7 Ch. 19, analytical") and the document revision;
  - every default used, and every refused lane group with its advisory.
- **Format.** JSON and CSV through the shared formatters, with the not-validated marker line.
  Results → *Node analysis* table, and the Node's own row. A result is stale when the revision
  changes (WORKFLOW §5).
- **No Micro, Meso or Macro output changes.** A project without analysis inputs keeps its bytes.

## 6. Workflow in the editor (target)

1. Draw the junction or use a Road-generated Node. A Node with detail derives lanes and turns.
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
| A2 | Back of queue and storage ratio (Ch. 31 §4); critical v/c; shared lane groups (Ch. 31 §2) |
| A3 | Permitted and protected-permitted left turns (Ch. 31 §2) |
| A4 | Two-way stop control (Ch. 20) |
| A5 | All-way stop (Ch. 21) |
| A6 | Timing proposal: Webster cycle and splits (TP39), isolated |
| A7 | Roundabouts (Ch. 22), using merged Nodes (ROADS RD15) |
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

## 9. Acceptance rows (planned, A1)

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
