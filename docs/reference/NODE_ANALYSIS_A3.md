# M13 A3: permitted and protected-permitted turns (contract, D376)

Slice A3 of [NODE_ANALYSIS](NODE_ANALYSIS.md). It removes most `NODE_ANALYSIS_PERMITTED` refusals by
running HCM 7 Ch. 31 §3's QAP procedure for selected lane groups (Steps 1–7, PDF p. 2986–3015),
with the lane-flow terms Ch. 31 §2 adds for permitted turns. The scope follows the choices decided
1a–5a on the owner's delegation (D375):
- exclusive and shared lanes, far and kerb turns, pretimed;
- operation derived from the program;
- opposing flow from Node membership;
- no pedestrians or bicycles;
- acceptance by hand rows.

HCM's left turn is MULTRA's far turn, and its right turn the kerb turn (NODE_ANALYSIS §10). Every
value below was read from its formula or table image in the owner's copy on 2026-10-27, with the
page cited. Results stay analytical, not simulated, and not validated.

## 1. Operation per turn (derived, never stored)

A2 marks a turn *permitted* when its displayed green overlaps the green of a conflicting movement
it yields to (`node_analysis_compile.cpp`). A3 splits the turn's green into two parts:
- the **permitted window**: the times when a movement it yields to also shows green;
- the **protected** part: the rest of its green.

Operation is then:
- **protected**: no permitted window (A1 and A2, unchanged to the bit);
- **permitted**: no protected part;
- **protected-permitted**: both parts. The turn's displayed green may be one interval, or two
  intervals separated by red (HCM's lead–lead with a gap).

What happens to a yielding turn, by movement:

| Movement | Treatment |
|---|---|
| Far turn yielding to the opposing in-leg's through and/or kerb movements | Analysed (sections 2–5) |
| Far turn yielding to anything else | Refused with `NODE_ANALYSIS_PERMITTED`. This covers a crossing approach whose green overlaps, or an opposing far turn alone; neither is an HCM case |
| Kerb turn that yields | Analysed with protected numbers, plus advisory `NODE_ANALYSIS_KERB_YIELD` (see note) |
| Through movement that yields | Refused with `NODE_ANALYSIS_PERMITTED` |

Note on the kerb turn: in HCM a kerb turn yields only to pedestrians, through f_Rpb. With no
pedestrians (D375 4a), Eqs. 31-104 and 31-105 give the protected values exactly (PDF p. 2996).
MULTRA's own vehicle yield for a kerb turn is not an HCM case, which is why the advisory is given.

## 2. Permitted window times (Step 1, Exhibit 31-12, PDF p. 2987–2990)

**Exhibit 31-12 as one rule.** Exhibit 31-12 prints G_U for nine phase sequences. Phase 2 is the
opposing through and phase 6 the subject's adjacent through. Every printed row equals:

**G_U = E − max(S_a, S_o + G_q2) ≥ 0**, where:
- E is the earlier end of the subject's and the opposing through's displayed greens;
- S_a is the start of the subject's permitted window;
- S_o is the opposing through's green start.

G_p is the same expression with G_q2 = 0, which is the length of the window. MULTRA applies this
rule directly to signal-group intervals. It is the table restated, not a generalisation, and NA16
checks it against every row.

**Lost time and extension.**
- The permitted start-up lost time l_1,p is:
  - l_1 if the window opens the subject's displayed green;
  - 0 if it continues the protected green;
  - note b's form `clamp(gap + l_1 − e, 0, l_1)` after a red gap of that length.
- The permitted extension e_p is:
  - e if the subject's display ends with the window;
  - 0 if it continues into the protected green.
- Both use l_1 = e = 2.0 s.
- The one difference from the table is Lag–Lag's partial e1* (note c). A MULTRA display either
  continues (0) or ends (e), so that partial value cannot arise.

**Effective greens.**
- Eq. 31-94: `g_p = G_p − l_1,p + e_p ≥ 0`.
- Eq. 31-95: `g_u = G_u + e_p ≤ g_p`.

**The opposing queue time G_q2** (Exhibit 31-12 note a) is computed per opposing lane, excluding
an opposing shared far-turn lane, and the longest is used.
- For an exclusive opposing lane, G_q2 = g_q + l_1, where g_q is that lane group's queue service
  time g_s from its own polygon (section 4).
- For a shared opposing lane, G_q2 = g_p − g_e + l_1.

**Iteration across the Node.** A Node whose opposing approaches both have permitted far turns
depends on itself. MULTRA iterates every lane group's polygon until every G_q2 changes by less than
0.1 s. A cap of 100 iterations gives `NODE_ANALYSIS_NO_CONVERGENCE`. This iteration is MULTRA's
solution method; the formulas are the book's.

## 3. Saturation flows (Steps 2–6, PDF p. 2990–3002; Ch. 31 §2, PDF p. 2953–2959)

**Permitted saturation flow** (Eq. 31-100): `s_p = v_o e^(−v_o t_cg/3600) / (1 − e^(−v_o t_fh/3600))`
with t_cg 4.5 s and t_fh 2.5 s.
- v_o is Case 1, the book's default: the flows of the opposing through and kerb movements the
  turn yields to.
- v_o is not adjusted for lane use.
- v_o = 0 is replaced by 0.1 veh/h.

**Through-car equivalents.**
- Eq. 31-101: `E_L1 = s_o/s_p`.
- Eq. 31-102: `E_L2 = (1 − (1 − P_lto)^n_q)/P_lto ≥ E_L`, with E_L = 1.05.
- Eq. 31-103: `n_q = 0.278 (g_p − g_u − g_f) ≥ 0`.
- Eq. 31-48: `E_L1,m = (E_L1/f_Lpb − 1) P_lc + 1`.
- Eq. 31-49: `E_L2,m = (E_L2/f_Lpb − 1) P_lc + 1`.
- f_Lpb = 1 (D375 4a).
- P_lto is the opposing far-turn share of the opposing lane.

**When the opposing approach counts as one lane** (PDF p. 2994): it has one lane serving through
traffic, its far turn shares that lane, and it either has no kerb turn or its kerb turn shares the
lane. An exclusive opposing kerb lane is assumed to influence gap acceptance, which is the book's
default, so such an approach has more than one lane.

**Time before the first far-turning vehicle blocks a shared lane** (Eqs. 31-96 to 31-99):
- Eq. 31-96: `g_f,max = (1 − P_L)/(0.5 P_L) (1 − (1 − P_L)^(0.5 g_p)) − l_1,p ≥ 0`.
- Eq. 31-97 (one-lane approach): `g_f = max(G_p e^(−0.860 LTC^0.629) − l_1,p, 0) ≤ g_f,max`.
- Eq. 31-98 (otherwise): the same with 0.882 and 0.717.
- Eq. 31-99: `LTC = v_lt C/3600`.

**Saturation flow for each lane group:**

| Lane group | Saturation flow |
|---|---|
| Exclusive permitted far turn | Period 1 (opposing queue): `s_l1 = 0` with two or more opposing lanes, else `s_l/E_L2` (Eq. 31-111). Period g_u: `s_l = s_p f_w f_HVg f_p f_bb f_a f_LU f_Lpb f_wz f_ms f_sp` (Eq. 31-110) |
| Exclusive protected-permitted far turn | Protected `s_lt` (Eq. 31-112, A1's value), then permitted `s_l` |
| Shared far turn and through, permitted | Period g_f at s_th. Period g_diff (Eq. 31-107: `g_p − g_u − g_f ≥ 0`) at `s_sl2 = s_th/(1 + P_L(E_L2/f_Lpb − 1))` (Eq. 31-108), or 0 with two or more opposing lanes. Period `min(g_p − g_f, g_u)` at `s_sl3 = s_th/(1 + P_L(E_L1/f_Lpb − 1))` (Eq. 31-109) |
| Shared far turn and through, protected-permitted | The three periods above, plus `s_sl4 = s_th/(1 + P_L(E_L − 1))` in the protected period (Eq. 31-113) |
| Through lanes beside a shared permitted far-turn lane | ×0.91 (`f_s`, Eq. 31-62, when N_sl = 1) |
| Kerb turns | Eqs. 31-104 to 31-106 with f_Rpb = 1, which are the protected values |

**Lane-flow distribution** (Ch. 31 §2, A2's procedure).
- A permitted shared far-turn lane uses Eq. 31-59 as its average saturation flow:
  `s_sl = s_th/g_p (g_f + g_diff/(1 + P_L(E_L2,m − 1)) + min(g_p − g_f, g_u)/(1 + P_L(E_L1,m − 1))
  + 3600 n_s* f_ms f_sp/s_th)`.
- Eq. 31-60: `n_s* = P_L/(1 − P_L)(1 − P_L^n_s)` if P_L < 0.999, else `n_s P_L`.
- Eq. 31-59's image is clipped at the right. Its last term is completed from the PDF text layer and
  the equation's variable list (n_s*, f_ms, f_sp), as was done for Eq. 31-151 in A2. f_ms = f_sp = 1.
- An exclusive permitted lane uses Eq. 31-59 with P_L = 1, g_diff = g_f = 0 and s_th replaced by
  s_lt (PDF p. 2958).

## 4. Polygon, uniform delay and capacity (Step 7, PDF p. 3002–3015)

**Polygon.**
- Each lane group's queue accumulation polygon is built over one cycle from its periods:
  - arrival rates q_r and q_g, from P as in A2 with Rp = 1;
  - each period's service rate from section 3;
  - zero service while the opposing queue blocks (two or more opposing lanes).
- Exhibits 31-13 to 31-18 are the shapes this produces (images read).
- If a queue remains at the end of the permitted period, it drops by the sneakers:
  - n_s f_ms f_sp = 2 for an exclusive lane;
  - (1 + P_L) f_ms f_sp for a shared lane.
- The book gives sneakers when "the phase extends to max-out". A pretimed phase always ends at its
  fixed time, so A3 counts them every cycle (MULTRA's reading, stated).
- The polygon starts at a time when its queue is zero. The book requires lane volume no greater
  than capacity for these polygons (PDF p. 3007). A lane group over capacity keeps Eq. 19-26's d2
  with X > 1, and its polygon is solved at capacity.

**Uniform delay.**
- Eq. 31-115: `d_1 = 0.5 Σ (Q_i−1 + Q_i) t_t,i / (q C)`.
- Eq. 31-116: `t_t,i = min(t_d,i, Q_i−1/w_q)`.
- This replaces Eq. 19-19 for permitted and protected-permitted lane groups only.
- The queue service time g_s is where the queue first reaches zero after the start of service.

**Capacity** (Step 7 B):

| Lane group | Equation |
|---|---|
| Exclusive permitted | Eq. 31-119: `(g_u s_l + 3600 n_s f_ms f_sp)/C · N_l` |
| Exclusive protected-permitted | Eq. 31-124: `(g_l s_lt/C + (g_u s_l + 3600 n_s f_ms f_sp)/C) N_l` |
| Shared permitted | Eq. 31-121: `(g_p s_sl + 3600(1 + P_L) f_ms f_sp)/C`, with s_sl from Eq. 31-122 (whose g_diff term is 0 with two or more opposing lanes) |
| Shared protected-permitted | Eq. 31-126: `g_l s_sl4/C + (g_p s_sl + 3600(1 + P_L) f_ms f_sp)/C` |

Available capacities (Eqs. 31-118, 120, 123, 125, 127, 129) need G_max and belong to A8.

**Incremental delay.** d2 is Eq. 19-26 with this capacity.

## 5. Critical v/c and back of queue

**Critical v/c** (Ch. 19 §4, PDF p. 1486–1487).
- A permitted lane group's flow ratio uses its average permitted saturation flow (Eq. 31-59 with
  E_L1 and E_L2).
- A protected-permitted lane group gives two intervals to A2's chain:
  - the first displayed takes min(v, its capacity);
  - the second takes the remainder.

**Back of queue** (Ch. 31 §4, Exhibits 31-26 to 31-31).
- Q1 is built from the same polygon by Step 3's two rules:
  - stopped arrivals lag arrivals by d_a/2;
  - departure initiation leads departures by d_a/2.
- Q2 is c_A d2/3600N as in A2.
- NA19 requires this construction to equal Eqs. 31-137 to 31-140 on Exhibit 31-25's basic polygon.
  If it does not, Q1 is omitted for these lane groups with advisory `NODE_ANALYSIS_NO_QUEUE`, and
  every other result stays.

## 6. Rows

| Row | Checks |
|---|---|
| NA15 | s_p (Eq. 31-100) at several v_o, including v_o = 0 → 0.1; E_L1, E_L2, n_q; E_L1,m and E_L2,m — each by hand |
| NA16 | G_p and G_u from intervals reproduce each Exhibit 31-12 row (Perm–Perm, Perm–Lead, Perm–Lag, Lead–Lead, Lead–Lag, Lag–Lead, Lag–Perm, Lag–Lag), with l_1,p and e_p; Eqs. 31-94 and 31-95 |
| NA17 | The polygon's d_1 equals a numerical integral and Eq. 19-19 for a protected group; Eqs. 31-115 and 31-116 by hand on Exhibit 31-13's shape; capacities by Eqs. 31-119, 31-121, 31-124 and 31-126 by hand |
| NA18 | Shared lane with a permitted far turn: g_f, g_diff, P_L, s_sl by Eq. 31-59 and Eq. 31-122, f_s 0.91, conservation of approach flow |
| NA19 | Xc with permitted and protected-permitted groups; Q1 against Eqs. 31-137 to 31-140 on the basic polygon (section 5) |
| NA20 | Refusals and advisories (section 1); a fully protected Node's results unchanged to the bit (the `cli-node-analysis` fixture stays 59.86 s, Xc 0.726); NA14's permitted shared far-turn case now analysed |
