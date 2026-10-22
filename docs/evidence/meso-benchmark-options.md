# Meso benchmark option sheet (M11.6a, 2026-10-22)

Prepared by a session at the owner's request (2026-10-22). Rule 4 keeps the not-validated marker on
every Meso result until M6's Meso benchmark passes ([MESO_NETWORK](../reference/MESO_NETWORK.md);
[MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §3.7 names the kind: "analytic queue
and shockwave cases"). This sheet lists published options by the rules of the
[M6.0 sheet](m6-benchmark-options.md). The owner answers by letter in the
[owner sitting](../plans/OWNER_SITTING.md) B23. Nothing here is decided until then.

## 1. Rules of this sheet

- **No engine numbers.** No MULTRA figure appears below. Every number is a published value or an
  input a comparison would set.
- **One disclosure.** Three published cases already have engine numbers in the test rows, written
  while the formulas were checked against their sources:
  - Yperman's Example 4.5 (ME6);
  - Yperman §7.1 (ME8);
  - Tampère et al.'s Table 8 (ME20).

  These cases are **not blind**. They also come from the papers our formulas were taken from, so
  matching them verifies the implementation against its own sources, not the model against an
  independent result. They are listed so that the owner can choose knowingly. The rows'
  values are in their evidence notes, not here.
- **No tolerance.** The tolerance is the owner's next decision. It is set after the options are
  chosen and before the first comparable run, then published in the docs and shown in the app.
- **Published sources only,** at least two per kind. A comparison with another simulator is not an
  option (D38).
- **Not validation.** This sheet validates nothing. The marker stays until the benchmark passes.
- **What this session could read.** The environment refuses arXiv, ScienceDirect and most article
  hosts (proxy 403). The only primary sources read are the owner's copies:
  - Yperman (2007), as Markdown;
  - Tampère et al. (2011), as a PDF;
  - HCM 7th edition, as Markdown.

  Every option below says whether it was read. An unread option needs a copy before its case,
  values and units can be checked.

The two kinds follow the M11 gate (ROADMAP) and §3.7: a queue and its shockwave on one corridor,
and spill-back across a network's nodes.

| Kind | Quantity |
|---|---|
| **M1** | A bottleneck on a corridor: queue growth and discharge, the backward shock's arrival times, and spill-back to the entrance at storage capacity |
| **M2** | Network loading through merges and diverges: the transfers when an out-link is full, spill-back to the upstream links, and when queues form and clear |

## 2. What Meso can measure today

These are the commands a comparison would run. This sheet runs none of them.

| Figure | Command | Definition | Gaps every comparison must state |
|---|---|---|---|
| Link flows, density, travel time, queue and spill-back by slice | `multra-cli --project F --meso [--csv out.csv]` ([MESO_NETWORK](../reference/MESO_NETWORK.md) §7) | See the notes below the table | See the notes below the table |
| Cumulative counts, sending and receiving volumes per step | A hand-built `loadMeso` in a test row (`src/meso/loading.hpp`) | Each link's `N_up`, `N_down` and `admitted` at k·Δt; `S` and `R` per step; per commodity, entered and arrived | Not exported by the CLI. A comparison on curves runs as a row or a tool, not on a drawn project |

The CLI row in detail:
- **Definitions:**
  - inflow and outflow, pcu/h;
  - mean density;
  - the mean travel time of the PCU entering in a slice, from the curves;
  - the vertical queue `N_up(t − L/v) − N_down(t)`;
  - spill-back when R = 0 in a step;
  - turn transfers;
  - VKT, VHT and delay.
- **Gaps every comparison must state:**
  - units are PCU, not veh;
  - the fixed step Δt and the linear interpolation between steps (Yperman Eq. 3.2);
  - the slice is a reporting choice;
  - turn time is a lag charged to the out-link (D301);
  - priority at Nodes is not modelled;
  - signals are averaged green (D299).

## 3. Kind M1 — a queue and its shockwave on a corridor

| Option | Source | Published values | Read? | Notes |
|---|---|---|---|---|
| **M1-a** | Yperman (2007), Example 4.5 (§4.8) | A 10 km link (3600 veh/h, 120 km/h, 225 veh/km, Δt = 5 min) blocked downstream. The shock reaches the entrance at 37.5 min, and the LTM restricts the entrance "only 2,5 minutes" early. Storage is `k_jam·L` | Read | **Not blind** (ME6). It comes from the source of our link model |
| **M1-b** | Daganzo (1994), the CTM paper's numerical example | Not checked | Not read | Its results follow kinematic-wave theory, so the same case applies to any first-order model. The case, values and printed results must be read first |
| **M1-c** | Newell (1993), Part II, "Queueing at freeway bottlenecks" | Not checked | Not read | The analytic cumulative-curve method behind the LTM. It may print no numerical case (to check) |
| **M1-d** | Boyles, Lownes and Unnikrishnan (2025), *Transportation Network Analysis* Vol. I, the link transmission model's worked examples (its index places them at about pp. 334–341 and 372–386) | Not checked | Not read | Open on arXiv, which this session cannot reach. It is independent of Yperman's text, and it is a textbook, as MA15's Sheffi was |

## 4. Kind M2 — network loading with merges, diverges and spill-back

| Option | Source | Published values | Read? | Notes |
|---|---|---|---|---|
| **M2-a** | Yperman (2007) §7.1, the diverge network (Table 7.1) | Fig. 7.5: about 167 veh/km at 2000 veh/h on nm. The text: r2m clears at 46 min, and r1n queues from 28 to 82 min, read from screenshots 6 minutes apart (Fig. 7.4) | Read | **Not blind** (ME8). Its numbers have figure resolution, and it comes from the source of our link model |
| **M2-b** | Daganzo (1995), Part II: network traffic | Not checked | Not read | Its junction rules (merges, diverges with FIFO) are those our node model generalises. Whether it prints a numerical network case must be checked |
| **M2-c** | Boyles et al. (2025), the network loading and node-model examples | Not checked | Not read | Independent of our sources, and open access. It is the same book as M1-d, so one copy serves both kinds |
| **M2-d** | Tampère et al. (2011), Table 8 (4 × 4 node, its Tables 1 and 5) | `q_ij` per movement, and `a_7` | Read | **Not blind** (ME20). It covers one node step only, with no links and no time, so it cannot stand alone for M2 |

## 5. What a comparison fixes before it runs

Whichever options are chosen, a later session writes these down before the first comparable run,
together with the tolerance:

- **the case as printed:** links, diagram values, demand, step and horizon. A value the source
  leaves implicit is stated with its derivation;
- **the units:** veh against PCU. A car-only case runs with PCU 1;
- **each published quantity, and how it is read from MULTRA's outputs:**
  - a time from the cumulative curves;
  - a queue as vertical or physical;
  - a shock arrival as the first step a link's inflow falls;
  - a throughput as a count per interval;
- **the tolerance's form:** absolute or relative, per quantity;
- **the source's own resolution.** A value read from a figure carries that figure's spacing, as ME8
  carries Fig. 7.4's 6 minutes;
- **where the run lives:** a test row (curves) or a benchmark tool (`tools/*_benchmark.cpp`, not in
  `check`, AGENTS).

## 6. Owner questions (answer by letter, sitting B23)

| # | Question | Options | Session's recommendation |
|---|---|---|---|
| QM | What kind of benchmark defines the Meso gate? | (a) published analytic and worked cases · (b) a published field dataset · (c) (a) now, (b) at M6 | **(c)**. §3.7 names analytic cases, and no field dataset was checked for this sheet. A field dataset would test the model, not just its arithmetic, so it belongs with M6's other kinds |
| M1 | Which corridor case, primary and secondary? | M1-a … M1-d | **M1-d** primary: it is independent of our sources and open access. **M1-b** secondary. M1-a stays verification (ME6) |
| M2 | Which network case, primary and secondary? | M2-a … M2-d | **M2-c** primary, from the same copy as M1-d. **M2-b** secondary. M2-a and M2-d stay verification (ME8, ME20) |
| NB | May a not-blind case (M1-a, M2-a, M2-d) count toward the gate? | yes · only as verification | **Only as verification.** Our formulas came from those papers, and their engine numbers already exist |
| CP | Which copies will the owner supply? | Boyles et al. 2025 (arXiv 2502.05182) · Daganzo 1994 · Daganzo 1995 · Newell 1993 Part II · allow `arxiv.org` in the environment's network access | **Boyles et al.** as a Markdown repository like the other books, or `arxiv.org` allowed. The rest only if chosen |

An answer here chooses sources only. It closes nothing, validates nothing and removes no marker.

## References

How these were checked: web searches on 2026-10-22 against TRID, IDEAS/RePEc, publisher and
arXiv index records. Only the two items marked read were opened; the others are cited from
their bibliographic records, and nothing is claimed about their contents beyond their titles and
abstracts.

- **[BLU25]** Boyles, S.D., Lownes, N.E., Unnikrishnan, A. (2025). *Transportation Network Analysis,
  Volume I: Static and Dynamic Traffic Assignment.* arXiv:2502.05182. **Not read.**
- **[Dag94]** Daganzo, C.F. (1994). The cell transmission model: a dynamic representation of highway
  traffic consistent with the hydrodynamic theory. *Transportation Research Part B* 28(4), 269–287.
  doi:10.1016/0191-2615(94)90002-7. **Not read.**
- **[Dag95]** Daganzo, C.F. (1995). The cell transmission model, part II: network traffic.
  *Transportation Research Part B* 29(2), 79–93. doi:10.1016/0191-2615(94)00022-R. **Not read.**
- **[New93]** Newell, G.F. (1993). A simplified theory of kinematic waves in highway traffic, part II:
  queueing at freeway bottlenecks. *Transportation Research Part B* 27(4). **Not read.**
- **[Tam11]** Tampère, C.M.J., Corthout, R., Cattrysse, D., Immers, L.H. (2011). A generic class of
  first order node models for dynamic macroscopic simulation of traffic flows. *Transportation
  Research Part B* 45(1), 289–309. **Read** ([tampere-node-model](tampere-node-model.md)).
- **[Ype07]** Yperman, I. (2007). *The Link Transmission Model for Dynamic Network Loading.* Doctoral
  thesis, KU Leuven. **Read** ([yperman-ltm](yperman-ltm.md)).
