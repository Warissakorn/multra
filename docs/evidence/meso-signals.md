# M11.3d signals — averaged green from Tampère §5.2, Yperman §5.2.1 and HCM Eq. 19-1 (D299, 2026-10-22)

M11.3c deferred signals (D298), because explicit green under FIFO holds a whole in-link while any of
its turns is red. The owner chose averaged green (2026-10-22), which replaces choice 4a by 4b. This
note records the sources and what was derived from them.

## Sources

- **Tampère et al. (2011), §5.2 (pp. 304–305):** re-read from the owner's PDF (sha256
  `9f94a313…2928417`, as in [tampere-node-model](tampere-node-model.md)).
  - The paper itself takes the averaged option: "the effects of the traffic lights are averaged to
    allow larger time steps".
  - **Eq. 33**, an internal node supply per in-link i and phase p: `Σ_{j∈U_i^p} q_ij ≤ N_i^p = a^p
    b_i^p C_i`. Here `a^p` is the green fraction of the cycle and `b_i^p` the fraction of `C_i`
    available, "e.g. based on the number of turning lanes".
  - **Eq. 34**, its level: `a_i^p = min(1, N_i^p / Σ_{j∈U_i^p} C_ij)`.
  - **Eq. 35**, the claims on out-links reduced: `C̃_ij = a_i^p C_ij`. **Eq. 36** computes the out-link
    levels with C̃.
  - The solution is analogous to Eq. 28 by the same algorithm. The paper assumes each turn belongs
    to one phase.
- **Yperman (2007), §5.2.1:**
  - Eq. 5.13: `q_n,ij = (g_ij / c) q_M,ij`.
  - Eq. 5.15, the shared-lane split: `q_M,ij = (S_ij / S_i) q_M,i`. The thesis calls it "the only one
    that is consistent with the FIFO principle".
  - The thesis's prototype also assumes FIFO on every link: an oversaturated turn holds the others.
- **HCM 7th edition, Chapter 19, Equation 19-1** (PDF pp. 1415–1416; the owner's Markdown copy
  `Warissakorn/highway-capacity-manual-7edition-markdown` @ `74f4bc3`):
  - The equation is `l_t = l1 + Y + Rc − e`, with "l1 = start-up lost time = 2.0 (s)" and "e =
    extension of effective green = 2.0 (s)".
  - The effective green is the phase duration less its lost time: `g = (G + Y + Rc) − l_t = G + e − l1`.
  - With those values, g equals the displayed green G. The HCM adds that "longer times may be
    appropriate for congested conditions, higher speeds, or heavy vehicles".

## What was built and what was derived

- **g/C per program:**
  - a controller group's displayed green, `greenEnd − greenStart` wrapped past the cycle's end, over
    its cycle;
  - a legacy program's green phases over its total duration;
  - amber counts as not green, as HCM's `g = G` implies.
- **Heads to turns:**
  - a turn takes the one program shown by the heads its vehicles pass: on its path's Connectors, on
    the in-leg lanes its first Connector leaves from, or on a Link inside the Node;
  - a turn under two programs is advised `MESO_SIGNAL_AMBIGUOUS` and left unsignalised. The paper
    assumes disjoint phases, so it gives no rule for this case.
- **b by Yperman's Eq. 5.15:** `b_i^p = S_i^p / S_i`. It makes Tampère's `a_i^p` equal g/C exactly.
  The engine passes `N = (g/C) C_i S_p / S_i`.
- **Derived here: CTF across phases.** Under CTF, a supply holds the whole in-link at `a_i^p C_i`.
  - The in-link therefore enters the §5.1 algorithm with capacity `min_p a_i^p · C_i`, and a demand
    above it is capped with the same fractions (the invariance principle, requirement 7).
  - This equals the paper when an in-link's turns share one phase.
  - Across phases, a first version that reduced claims per movement broke `q_j ≤ R_j` on generated
    cases. The paper's Eq. 36 does not say how CTF holds there; the effective capacity keeps both.
- **The saturation flow** is the in-link's link-type capacity (Tampère's `C_i`). HCM adjusted
  saturation flows are not modelled.
- **Not modelled:** permitted left turns yielding within green (priority stays
  `MESO_PRIORITY_IGNORED`), and the queue cycle within a cycle, which is Micro's.

## Rows and results (Linux, 2026-10-22)

- **ME7** (`mesoloading`):
  - an approach of 3600 veh/h green 30 s of 120 s passes 900 veh/h at 1800 veh/h demand, and the
    vertical queue grows at 900 veh/h;
  - it passes the demand at 600 veh/h;
  - two turns in 30 s and 60 s groups pass 900 veh/h in all;
  - the wrap and legacy g/C are exact.
- **ME26** (`mesoloading`):
  - the four-leg's 12 Node turns carry their groups, six at 30/120 and six at 20/120;
  - a second West head of group 2 gives `MESO_SIGNAL_AMBIGUOUS`;
  - the T-junction's turns carry none.
- **ME27** (`mesonode`): 300 generated nodes with supplies meet `q_j ≤ R_j`, `Σ q ≤ N`, CTF and
  maximality, and over 50 are held by a supply alone. A shared out-link reproduces Eq. 35's reduced
  claim (60 against 240).
