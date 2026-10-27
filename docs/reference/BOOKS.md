# BOOKS — the owner's textbook copies and where each serves MULTRA

The owner supplied five books as Markdown repositories on GitHub. This page says how to attach
them, how to cite them, how far to trust each conversion, and which chapters serve which part of
MULTRA. It is navigation: status stays in [NEXT](../NEXT.md) and [ROADMAP](../ROADMAP.md), and
the checks made against the copies are dated in
[book-copies-check](../evidence/book-copies-check.md). Surveyed 2026-10-23 at the commits below.

## 1. Rules for every book

- **Attach, do not vendor.** `add_repo` with the owner and name below, then a shallow clone
  beside this repository (`git clone --depth 1`). Never copy book text, tables or page images
  into this repository: the copies are the owner's, and MULTRA is GPL-3.0-or-later (D285).
- **Cite the book, not the copy.** Chapter or section, equation, table or exhibit number, and the
  printed page where the book has one (§2 gives each book's page rule). The copy's own prose,
  README and AI guides are conversion notes, not sources.
- **Read the image before using a number.** Every copy transcribes equations and tables
  automatically. Each links the source image of a formula or table; read it before a value goes
  into `data/`, a fixture or a contract. Text inside `OCR_UNVERIFIED` or `text` fences is for
  search only.
- **A book value is not a validation.** Reproducing a textbook example checks arithmetic, not
  behaviour, and never removes the not-validated marker (rule 4). Which benchmark an M6 or
  milestone gate uses stays the owner's choice (rule 8).
- **Edition matters.** Say which edition a value comes from. `data/los/hcm.json` and LOS cite
  the HCM 6th edition; the copy here is the 7th (§3.2).

## 2. The five copies

| Book | Repository (`Warissakorn/…`) | Commit surveyed | How to cite a page | Conversion caveats |
|---|---|---|---|---|
| Sheffi, Y. (1985). *Urban Transportation Networks: Equilibrium Analysis with Mathematical Programming Methods.* Prentice-Hall | `Urban-Transportation-Networks-Markdown` | `b9f0767` | `book p.`; PDF p. = book p. + 16 for Arabic pages | Equations marked `transcription: visually checked` may be used; `OCR_UNVERIFIED` may not. **Ten page images are empty files**, among them book p. 114 (Figure 5.1), so OWNER_SITTING B21 still needs the print ([check](../evidence/book-copies-check.md#1-sheffi-empty-page-images)) |
| Transportation Research Board (2022). *Highway Capacity Manual, 7th Edition.* | `Highway-Capacity-Manual-7edition-Markdown` | `74f4bc3` | Chapter, Exhibit or Equation number, and the **PDF page** of the copy; the printed page is chapter-page (`19-17`) and is not the PDF page | `EXHIBITS.md` and `EQUATIONS.md` index every caption with its PDF page. Tables are rebuilt from column positions with `Column N` headers and rows that drift across merged cells: read `assets/tables/pNNNN-table-NN.png`. Some formulas are cut at the right edge of the source PDF (`SOURCE_ISSUES.md`) |
| Ortúzar, J. de D. and Willumsen, L. G. (2011). *Modelling Transport,* 4th ed. Wiley | `Modelling-Transport-4-edition-Markdown` | `1b3d41d` | `book p.`; book p. = PDF p. − 22 from PDF p. 23 | `TOC.md` gives every section's PDF and book page. The repository also holds the source PDF (`source/`), the authority for any damaged formula |
| Meyer, M. D. (ed.) (2016). *Transportation Planning Handbook,* 4th ed. ITE / Wiley | `Transportation-Planning-Handbook-Markdown` | `9b38959` | `book p.` and PDF p. from the nearest source-page comment; **no fixed offset** (chapter 1 is +15, chapter 19 is +2) | Mostly prose from the PDF text layer; diagram and OCR fences are search aids. The edition, unchecked until now, is the 4th |
| Yperman, I. (2007). *The Link Transmission Model for Dynamic Network Loading.* PhD thesis, KU Leuven | `the-link-transmission-model-for-dynamic-network-loading-markdown` | `ecd6bfe` | **No page numbers:** cite chapter, section and equation, e.g. Eq. (4.31) | Chapter 6 is split into four files (6a–6d). `w` is a signed backward wave speed, `w < 0` |

## 3. Where each book serves MULTRA

Chapter rows name what the book offers a milestone; they do not book work. A row marked
**used** is already cited by a contract.

### 3.1 Sheffi (1985) — Macro assignment (M10, S2) and M9

| Chapter (book pages) | Offers | MULTRA |
|---|---|---|
| 3 Formulating assignment as a mathematical program (56–80) | Beckmann's UE program, Table 3.1's notation | [MACRO_NETWORK](MACRO_NETWORK.md) Frank–Wolfe objective |
| 5 Solving for user equilibrium (111–132) | Frank–Wolfe, Figure 5.1 and Tables 5.1–5.4 | **used:** MA15 (D294, [macro-sheffi-benchmark](../evidence/macro-sheffi-benchmark.md)) |
| 6 UE with variable demand (134–163) | Elastic demand, modal split inside the equilibrium | M10.4 mode choice if it feeds back to assignment |
| 7 Trip distribution and assignment (164–202) | Doubly constrained gravity with assignment, Evans's algorithm | M10.3 distribution; a combined-model fixture |
| 8 Equilibrium with link interactions (203–230) | Asymmetric costs, diagonalisation | Junction delay that depends on crossing flows in Macro |
| 11–12 Stochastic loading and SUE (286–344) | STOCH, Monte Carlo loading, MSA | M9's path-cost loop: MSA as the averaging rule, SUE as an alternative to deterministic UE |
| 13 Analysis inputs (346–376) | BPR curve, signal delay as a link cost, estimation | The VDF defaults' provenance (D158 cites the 1964 BPR manual) |

### 3.2 HCM 7 (2022) — capacities, LOS, M6 benchmarks, Node evaluation

| Chapter | Offers | MULTRA |
|---|---|---|
| 4–5 Capacity and LOS concepts | Definitions of capacity, demand, control delay | [LOS](LOS.md), [NODES](NODES.md) wording |
| 6 HCM and alternative tools; Exhibits 6-1, 6-6 | How microsimulation and the HCM model the same phenomena; performance measures by tool | [PROBLEM](../PROBLEM.md) §1 reports; what a Micro result may be compared with |
| 7 Interpreting results; Exhibits 7-8, 7-9, 7-12, 7-13 | Trajectory-based delay terms, run-to-run variability, HCM against simulated delay when oversaturated | [SIMULATION](SIMULATION.md) results semantics; [BATCH](BATCH.md) seed counts; M6 S2's delay definition |
| 10, 12 Freeway facilities and basic segments; Exhibits 10-7, 12-4, 12-6 | Jam density, base capacities and speed–flow curves | **used:** S3.2's link types (D304, [ROADS](ROADS.md) §8) |
| 13–14 Weaving, merge and diverge | Segment capacities and lengths | Future Macro and Meso link types for ramps |
| 15, 18 Two-lane highways, urban street segments | Segment capacity and speed | ROADS §8 cites Ch. 18 for why urban street types are not shipped: their free speed is site-specific |
| 19 Signalised intersections; Exhibits 19-8, 19-11; Eqs. 19-18, 19-19 | Base saturation flow, control delay `d = d1 + d2 + d3`, LOS | **used:** ROADS (Exhibit 19-11), LOS bounds; M6 S1-a and S2-a; Meso signals' `g = G` (D299); [NODE_ANALYSIS](NODE_ANALYSIS.md) Steps 1–9 with Eqs. 19-1, 19-8 to 19-21, 19-26 to 19-29 and Exhibits 19-8, 19-11, 19-15, 19-20 (D370–D372); A2: Eqs. 19-30, 19-31 and Exhibit 19-41 (D373, D374) |
| 20 Two-way stop control; Exhibits 20-2, 20-17, 20-18; Eq. 20-18 | Conflicting flows by movement, base critical and follow-up headways, potential capacity | M6 U-a; **a cited `t_ij` source** for Meso priority inside a Node (MESO_NETWORK §5, D297) |
| 21 All-way stop; Exhibit 21-8 | LOS bounds, AWSC capacity | LOS's unsignalised bounds |
| 22 Roundabouts; Exhibits 22-8, 22-16 | Entry capacity against circulating flow | A roundabout Node in Macro or Meso; a Micro check |
| 31 Signalised supplemental §2, §4, §6; Exhibits 31-6, 31-11, 31-25, 31-47, 31-56 | Lane group flow on shared-lane approaches (Eqs. 31-46 to 31-66), back of queue and storage ratio (Eqs. 31-131 to 31-156); field methods for control delay and saturation flow | **used:** NODE_ANALYSIS A2 (§11, D373, D374); `--discharge` rank convention (M6 S1); M6.0 sheet |
| 32 Stop-controlled supplemental | Worked TWSC and AWSC examples | Hand-computed fixtures for a gap-acceptance check |
| 36 Concepts supplemental §5 | Vehicle trajectory analysis: stops, back of queue, accumulated delay by definition (Exhibits 36-40 to 36-44) | M5 queue and stop measures from the event stream (`src/eval/`) |
| 38 Network analysis | Queue spill-back between facilities | Meso spill-back reading; S4 Node proposals from spill-back |

### 3.3 Ortúzar and Willumsen (2011) — the four steps (M10.2–M10.4) and M9

| Chapter (book pages) | Offers | MULTRA |
|---|---|---|
| 4 Trip generation (139–172) | Regression and cross-classification; Examples 4.1–4.6 | M10.2 generation: Example 4.5 (p. 161, category analysis) as a fixture |
| 5 Trip distribution (176–206) | Growth factors, Furness, gravity, entropy, calibration; Examples 5.1–5.7 | M10.3: Example 5.3 (p. 181, doubly constrained growth factor) for Furness balancing, Example 5.4 (p. 189) for the gravity model |
| 6 Modal split (207–226) | Logit modal split, hierarchical structure; Examples 6.1–6.4 | M10.4: Example 6.2 (p. 214, car and bus shares under logit, the red bus/blue bus case) and Example 6.3 (p. 216, its hierarchical form) as fixtures |
| 7 Discrete choice (227–) | MNL (§7.3), nested logit (§7.4) | M10.4's multinomial logit and its parameters |
| 10 Assignment (349–390) | AON, Dial's stochastic loading, congested assignment; Examples 10.1–10.6 | M10.1 cross-check; Example 10.4 (p. 367) sets up the two-route network below |
| 11 Equilibrium and dynamic assignment (391–) | UE and system optimum, traffic dynamics, departure time; Examples 11.1–11.5 | **M9's gate fixture candidate:** Example 11.1 (p. 394), two routes with costs `15 + 0.005 V` and `10 + 0.02 V`, 2000 vehicles, equilibrium 1400 / 600 at 22 min; Example 11.2 (p. 396) gives the system optimum ([check](../evidence/book-copies-check.md#5-ortúzar-and-willumsen-a-two-route-equilibrium)) |
| 15 Key parameters | Planning variables, value of time | Later economic outputs; not booked |

### 3.4 Transportation Planning Handbook (2016) — the study around the model

| Chapter (book pages) | Offers | MULTRA |
|---|---|---|
| 2 Travel characteristics and data (17–73) | Counts, surveys, peak factors | Demand entry; M6 field data |
| 6 Travel demand and network modelling (205–235) | Where four-step, activity-based and simulation tools fit | [MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) framing; M12 subarea cuts |
| 9 Road and highway planning §VII | System performance and capacity measures | Readiness and Node proposals ([READINESS](READINESS.md)) |
| 10 TSM&O | Operations strategies, performance data | Scenario comparison (M5.8) use cases |
| 11 Planning for parking §VI, §VIII | Parking demand, shared parking | M8 lots ([ZONES_AND_PARKING](ZONES_AND_PARKING.md)) |
| 19 Site planning and impact analysis (891–943) | Access and impact study scope, analysis procedures, report organisation | The audience of PROBLEM §1; `src/report/` (reserved) |

### 3.5 Yperman (2007) — the Meso engine (M11)

| Chapter | Offers | MULTRA |
|---|---|---|
| 3 LTM framework; Eq. (3.4); §3.2 "FIFO conditions" | Time step `Δt ≤ L / v_f`, cumulative counts, route counts by FIFO | **used:** D298; §3.2's route counts for the Meso cut, D330 |
| 4 Link model; Eqs. (4.31), (4.35); Example 4.5 | Sending and receiving flows, comparison with CTM | **used:** D298, rows ME3–ME6 ([yperman-ltm](../evidence/yperman-ltm.md)) |
| 5 Node models; Eqs. 5.2, 5.4, 5.15 | Turn splits, origins as point queues, shared-lane split | **used:** D298, D299 |
| 6a–6d Intersection delays | Point queues that add signal and priority delay without physical length | **Not used.** Meso signals are capacity only (averaged green, D299), so a signalised Meso route has no red-time delay below capacity. A Meso delay slice would start here |
| 7 Case studies | LTM against DQM | **used:** §7.1 within its figures (D298); candidates in [meso-benchmark-options](../evidence/meso-benchmark-options.md) |

## 4. Open items a book can serve

| Open item ([NEXT](../NEXT.md), [OWNER_SITTING](../plans/OWNER_SITTING.md)) | Book | What it gives, and what it does not |
|---|---|---|
| B21: Sheffi Figure 5.1's capacities | Sheffi | Nothing new: book p. 114's image is empty in this copy; the print is still needed |
| B9 / M6: benchmark choice | HCM 7 Ch. 19, 20, 31 | The M6.0 sheet's HCM values, which it says were not read, now checked against the 7th edition ([check](../evidence/book-copies-check.md#2-hcm-7-against-the-m60-option-sheet)). The choice stays the owner's |
| Meso priority inside a Node (`t_ij`, D297) | HCM 7 Ch. 20 | Movement-specific `t_c` and `t_f` and conflicting-flow rules, citable; Brilon and Wu (2001) itself is not in these books |
| M9 gate: analytic two-route fixture | Ortúzar Example 11.1 | A published two-route UE with closed-form costs; Sheffi Ch. 12 for MSA |
| M10.2–M10.4 hand-computed fixtures | Ortúzar Ch. 4–7 | Worked examples with printed answers for generation, Furness, gravity and logit |
| LOS pack edition | HCM 7 Exhibits 19-8, 20-2, 21-8 | The same bounds as `data/los/hcm.json`'s 6th-edition values; a citation update is possible, not needed |
| A Meso delay at signals | Yperman Ch. 6 | The point-queue method; a slice would need its own contract and owner choice |
| Boyles et al. 2025 (D315) | None | Not in these books; still waiting on the owner's copy |
