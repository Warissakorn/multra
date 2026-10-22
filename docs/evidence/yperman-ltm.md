# M11.3c link model — Yperman (2007) read against MESO_NETWORK §4–§6 (D298, 2026-10-22)

MESO_NETWORK §4 (D291) took the LTM's sending and receiving volumes and its step limit from search
summaries, and required the thesis before the engine. This note records what the thesis says, what
changed in the contract, and what the rows measured.

## Source

- **Citation:** I. Yperman (2007), *The Link Transmission Model for Dynamic Network Loading*, doctoral
  thesis, Katholieke Universiteit Leuven.
- **Copy read:** the owner's Markdown edition,
  `Warissakorn/the-link-transmission-model-for-dynamic-network-loading-markdown`, commit `ecd6bfe`.
  - It was converted from a Word file whose sha256 is
    `5ffa66410f10f214133de0875fce0c07d8407898c18221a888e51732e95a93ee`.
  - Four numbered equations are marked SOURCE_UNCLEAR in the copy: (5.16), (6.9), (6.15) and (6.16).
    None of them is used here.
  - The copy has no page numbers, so citations name the thesis's sections and equations.
- **Read:** chapters 3 and 4, §5.0–§5.2.1, §7.1, the notation and the conversion notes.

## What it says

- **The algorithm, §3.2:** per step and per node, the steps are:
  1. sending and receiving volumes;
  2. transition flows by the node model;
  3. the cumulative counts updated.

  The exit counts by route are inherited from the entrance through the inverse count function, which
  needs link FIFO. Linear interpolation between steps is Eq. 3.2–3.3.
- **The step, Eq. 3.4:** `Δt ≤ L_a / v_f,a` (CFL). Nothing about the backward wave.
- **Sending, Eq. 4.31:** `S_i = min{N(x⁰, t+Δt−L/v_f) − N(xᴸ, t), q_M Δt}`.
- **Receiving, Eq. 4.35:** `R_j = min{N(xᴸ, t+Δt+L/w) + k_jam L − N(x⁰, t), q_M Δt}`, with w signed
  negative.
- **Volumes:** `S`, `R` and `G` are vehicle increments per step, not rates (§3.1).
- **Turn split, Eq. 5.2:** `S_ij` is the route composition of the vehicles numbered `N(xᴸ,t)` to
  `N(xᴸ,t) + S_i` when they passed the entrance.
- **Origins and destinations:**
  - an origin is a point queue, `G = min{N_r(t+Δt) − N(x⁰,t), R_j}` (Eq. 5.4);
  - a destination takes `G = S` (Eq. 5.5).
- **Exact FIFO at a diverge, §5.1.4:** a sub-flow procedure by entrance interval. It "may be skipped"
  for practical applications, averaging each interval.
- **Urban nodes, §5.2:** the thesis treats signals and priority on average (g/C capacities, Eq. 5.13),
  not by explicit green. It notes that FIFO holds a whole in-link when one turn is oversaturated.
- **Example 4.5 (§4.8):** a 10 km link (3600 veh/h, 120 km/h, 225 veh/km, Δt = 5 min) is blocked
  downstream. The shock reaches the entrance at 37.5 min, and the LTM restricts it "only 2,5 minutes"
  early.
- **§7.1, the diverge network:** Table 7.1 is split at 500 m, with Δt = 15 s and 90 min.
  - Routes p1 (r1→s1) and p2 (r1→s2) each carry 3000 veh/h; p3 (r2→s1) carries 3000 veh/h until 30 min.
  - Fig. 7.5: the queue state on nm is 2000 veh/h at ≈ 167 veh/km.
  - The text: r2m clears at 46 min, and the queue on r1n lasts from 28 min to 82 min. These are read
    from screenshots 6 minutes apart (Fig. 7.4).
- **Multiple classes** are further research (§8.3.1).

## What changed in MESO_NETWORK

- **Confirmed:**
  - §4's sending and receiving (with w taken positive);
  - the storage `k_jam L`;
  - linear interpolation;
  - the origin queue and the destination.
- **Derived here (not printed in the thesis):** the step limit becomes `min(L/v, L/w)`. The receiving
  volume reads the downstream count at `t+Δt−L/w`, which is known only if it is no later than t.
  `mesoStepLimit` and `mesoShortestLink` now use it.
- **Changed by owner choice:** turn shares come from Eq. 5.2, replacing §6's "shares at time t, not
  shifted" (amends D295 in part).
- **Not built:**
  - the exact-FIFO sub-flow procedure, which the thesis allows to be skipped. A row's transfers are
    split by its `S_ij` composition, so vehicles and `R_j` are conserved exactly;
  - Yperman's merge (Eq. 5.7): the node model stays Tampère (D297).
- **Ours:** a commodity is a (class, path) pair counted in PCU (D272).
  - An origin whose first link also receives turns enters the node model as a demand-constrained
    row (`C = S`). On an entry link this is exactly Eq. 5.4.
- **Deferred by owner choice:** signals, to M11.3d. Explicit green under CTF holds the whole in-link,
  while the thesis averages green instead. A signalised Node was advised `MESO_SIGNAL_IGNORED`.
  **Later (D299):** averaged green is built, and the advisory retired ([meso-signals](meso-signals.md)).

## Rows and results (Linux, 2026-10-22, `tests/meso_loading_tests.cpp`, ctest `mesoloading`)

| Row | Thesis | Measured |
|---|---|---|
| ME6 | Example 4.5: restricted 2.5 min early; `k_jam L` = 2250 veh stored | Entrance 300 veh per step to 35 min, R = 150 in the step from 35 min, 2250 at 40 min, R = 0 after |
| ME8 | Fig. 7.5: `k_B` ≈ 167 veh/km at 2000 veh/h | 166.67 veh/km; 2000 veh/h from each in-link |
| ME8 | r2m clears at 46 min | 44 min (last step above critical density) |
| ME8 | r1n queued from 28 to 82 min | 26.25 to 82.75 min |

- **ME5's shockwave:** the back of the queue crosses each 500 m piece in 275 s, exactly the 275 s that
  `(q₂ − q₁)/(k₂ − k₁)` gives.
- **The thesis used a demand-proportional merge for §7.1.** Here it equals Tampère's
  capacity-proportional merge, since both in-links have the same capacity and send equally.
- **The tolerance:** ME8's times hold within the 6 minutes between screenshots. Those minutes are
  the resolution of the thesis's own figures, not a fit.
