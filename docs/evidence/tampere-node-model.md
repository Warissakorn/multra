# M11.3 node model — Tampère et al. (2011) read against MESO_NETWORK §5 (D297, 2026-10-22)

MESO_NETWORK §5 (D291, D292 3a) named this paper from search summaries. It required reading the
paper before M11.3. This note records what the paper says, and what changed in §5 as a result.

## Source

- **Citation:** C. M. J. Tampère, R. Corthout, D. Cattrysse, L. H. Immers (2011), "A generic class
  of first order node models for dynamic macroscopic simulation of traffic flows", *Transportation
  Research Part B* 45(1), 289–309, doi:10.1016/j.trb.2010.06.004.
- **Copy read:** the owner uploaded the publisher's PDF (21 pages) into the session. Its sha256 is
  `9f94a3136c83a86d55e55417ae633b2fb4ec36e92d36b18e1681ce3c72928417`. The PDF is not kept in the
  repository (publisher copyright).

## What it says (page numbers are the journal's)

- **Requirements, §3.1 (pp. 294–295):**
  1. general applicability;
  2. maximising flows;
  3. non-negativity;
  4. conservation of vehicles;
  5. demand and supply constraints;
  6. conservation of turning fractions (CTF, FIFO per in-link);
  7. the invariance principle.

  §2.3–2.4 shows that a demand-proportional distribution cannot satisfy invariance (Bliemer's model,
  Tables 1–4).
- **The generic class, Eq. 15 (p. 296):** maximise Σ q_ij subject to the constraints, plus a supply
  constraint interaction rule (SCIR), which is mandatory, and node supply constraints, which are
  optional.
- **Node supply constraints, §4.1 (pp. 296–297):** Eq. 16 is the general form. For a conflict point,
  Eq. 17, `Σ q_ij t_ij ≤ 1`, is "provisionally suggested (based on Brilon and Wu, 2001)", and "a
  thorough analysis ... is a topic of future research".
- **The unsignalised instance, §5.1 (pp. 298–302):**
  - **The SCIR:** oriented capacity proportional, `C_ij = (S_ij/S_i) C_i` (Eq. 19).
  - **The flows:** `q_ij = S_ij` for a demand-constrained in-link (Eq. 21), otherwise
    `q_ij = a_{j*(i)} C_ij` (Eq. 24). Together they give `q_ij = min(S_ij, a_{j*(i)} C_ij)` (Eq. 28).
  - **The levels of reduction:** Eqs. 25–27, and Eq. 29 for the smallest.
  - **The exact algorithm:** §5.1.2, Eqs. 30–32, in five steps.
  - "No internal node supply constraints are considered yet" (p. 298).
- **Merge and diverge, §5.1.3 (p. 302):** the instance reduces to Ni and Leonard's (2005) and
  Daganzo's (1995) merge, and to Daganzo's diverge.
- **The numerical example, §5.1.4 (pp. 302–304):** Fig. 1, with Table 1 (`S_ij`, `R_j`) and `C_i` of
  Table 5, gives Table 8 after three iterations, with `a_7 = 0.685`.
- **Signals, §5.2 (pp. 304–305):** "If the green and red phases are explicitly simulated, the node
  model of Section 5.1 can be applied unaltered for every green phase separately". Eqs. 33–36 average
  the green for long steps instead.
- **Proofs:**
  - App. A: the requirements hold, and the solution is unique.
  - App. B: the algorithm is exact, in at most n iterations.

## What changed in MESO_NETWORK §5

- **Confirmed:**
  - the capacity-proportional distribution;
  - FIFO by CTF;
  - an exact algorithm exists.
- **Changed: conflicts.** The contract said "conflicts inside a Node as node supply constraints",
  but the paper gives no instance for them. The owner chose (2026-10-22) to leave them out and advise
  `MESO_PRIORITY_IGNORED` until Brilon and Wu (2001) is read and a cited `t_ij` exists. This amends
  D292 3a in part.
- **Specified: signals.** Choice 4a (explicit green) is the paper's own first option, so the node
  model needs no signal term. The engine withholds a red turn's demand.

## Row and result

- **Row:** ME20 reproduces Table 8 within 0.5 veh/h. Exactly, `a_7 = 850/1241 = 0.68493`, while the
  paper prints `a_7 = 0.685` and, for example, `q_47 = 644.5` against 644.6 computed.
- **Other rows:** ME21 covers merge and diverge, and ME22 the requirements on 300 generated nodes.
- **Result:** all pass on Linux, 2026-10-22 (`tests/meso_node_model_tests.cpp`, ctest `mesonode`).
