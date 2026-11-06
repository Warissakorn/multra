# Ortúzar and Willumsen Ch. 5 against TRIP_DISTRIBUTION — 2026-11-06 (D0, D472)

Slice D0 of [TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md) §12: read Chapter 5 of the
owner's copy and record what the contract may cite. Source: Ortúzar, J. de D. and Willumsen, L. G.
(2011), *Modelling Transport*, 4th ed., Wiley. Copy `Warissakorn/Modelling-Transport-4-edition-Markdown`
at commit `1b3d41d`; book p. = PDF p. − 22. Every table number below was read from the page image
named, not from the automated transcription (BOOKS §1). No book text is copied beyond the
numbers and one-line paraphrases. The replays are a scratch script outside the repository
(double precision, `exp` unrounded); D1's rows repeat them in C++.

## 1. Bookings checked

| BOOKS §3.3 booking | Page (book / PDF) | Image read | Finding |
|---|---|---|---|
| Example 5.3, doubly constrained growth factor (Furness) | 181 / 203 | `assets/p203-table-053.png` (Table 5.6), `p203-table-054.png` (Table 5.7) | **Correct.** A prior matrix, target O_i and D_j, and the solution "after three iterations on rows and columns" |
| Example 5.4, gravity model | 189–190 / 211–212 | `p211-table-055.png` (5.8), `p211-table-056.png` (5.9), `p212-table-057.png` (5.10), `p212-table-058.png` (bands) | **Correct.** Costs, β = 0.10, the same trip ends as 5.3, the resulting matrix, a_i, b_j and a trip-length table |

Unlike Ch. 4 (Example 4.5), both bookings hold.

## 2. What the chapter gives the contract

| Contract point | Book | Page (book / PDF) | Finding |
|---|---|---|---|
| §3 Furness | §5.2.3, Eq. (5.7) `T_ij = t_ij a_i b_j`; steps: b_j = 1, solve a_i, then b_j, repeat | 180 / 202 (`p202-math-0232.png`) | Rows are corrected first. Example 5.3's replay confirms that order (§3) |
| §3 condition | Eq. (5.8): Σ_i O_i = Σ_j D_j = T is the most important condition for convergence | 181 / 203 (`p203-math-0233.png`) | M10.2's balancing (TRIP_GENERATION §4) gives exactly this |
| §3 gravity | Example 5.4: `T_ij = a_i b_j exp(−β c_ij)`, the deterrence matrix scaled by T / Σ exp(−β c) and then balanced "the same as Furness iterations" | 189–190 / 211–212 (`p211-math-0268.png`) | The engine is one Furness over the deterrence matrix |
| §3 factors | a_i and b_j are unique only to a multiplicative constant; A_i = a_i / O_i, B_j = b_j / D_j | 190 / 212 | The outputs name their normalisation (§8 of the contract) |
| §4 stopping | The method gives "within 3 to 5 %" in a few iterations; a tighter convergence "may be important" for model-system consistency (Ch. 11) | 180 / 202 | **No number.** Ch. 11 gives none for Furness either (searched). Example 5.3 is printed "within 1 %". D158's 0.1 % therefore has no book citation: an owner value (choice 4) |
| §5 intrazonal | §5.8.3: intrazonal costs are poorly represented by centroid connectors; it is preferable to remove intrazonal trips from the synthetic model and take them as a fixed proportion of the trip ends | 201–202 / 223–224 | Choice 2 |
| §11 constraint | §5.8.4: the journey to work is normally doubly constrained, and most other purposes singly constrained, because their attractions are hard to estimate | 202 / 224 | Differs from D158 ("doubly constrained" for every purpose): choice 9 |
| §5 externals | §5.8.2: external trips are taken out of the synthetic model, and their trip ends subtracted from its constraints | 201 / 223 | Gate zones stay out with TRIP_GENERATION §9 |
| §11 zeros | §5.6.1 (Example 5.5): zeros in the prior can leave no feasible solution, and the bi-proportional algorithm then oscillates | 193–194 / 215–216 | Applies to a gravity run only through unreachable pairs: choice 5 |

## 3. Example 5.3 replayed (Table 5.6 → Table 5.7)

Prior `t` (rows 1–4): `5 50 100 200 / 50 5 100 300 / 50 100 5 100 / 100 200 250 20`;
O = 400, 460, 400, 702; D = 260, 400, 500, 802; T = 1962.

- **Three passes, rows first** (each pass corrects rows, then columns) reproduce every printed cell
  of Table 5.7 to two decimals: row 1 `5.25 44.12 98.24 254.25` (sum 401.85), row 2
  `45.30 3.81 84.78 329.11` (462.99), row 3 `77.04 129.50 7.21 186.58` (400.34), row 4
  `132.41 222.57 309.77 32.07` (696.82); columns exactly 260, 400, 500, 802.
- Columns first does not reproduce it (row 1, cell 2 gives 44.19).
- The largest relative trip-end error after three passes is 0.74 % (row 2). The book says "within 1 %".
- Passes needed to bring it to ≤ 0.1 %: 5. To ≤ 0.01 %: 7.

## 4. Example 5.4 replayed (Tables 5.8–5.10)

Costs (minutes): `3 11 18 22 / 12 3 12 19 / 15.5 13 5 7 / 24 18 8 5`; the trip ends of Example 5.3;
β = 0.10.

- Σ exp(−β c) = 5.7423 (printed 5.74); expansion 1962 / 5.7423 = 341.67 (printed 341.67). The
  base matrix matches Table 5.9's second part to two decimals (e.g. 253.12, 113.73, 56.48, 37.86).
- **Three passes, rows first**, reproduce Table 5.10:
  - every cell (e.g. row 1: `155.73 99.00 64.46 74.17`);
  - the row sums 393.36, 402.81 and 710.37;
  - a_i = 1.17, 1.07, 0.68, 1.28, the products of the row corrections;
  - b_j = 179.17, 253.50, 332.37, 570.53, the products of the column corrections × 341.67.
- **One printed misprint:** row 2's sum is printed 455.56, but its printed cells add to 455.47. The
  replay gives 455.46.
- **Trip-length bands** at three passes: 355.95, 965.67, 263.27, 72.88, 209.21, 95.03 for
  1.0–4.0, 4.1–8.0, 8.1–12.0, 12.1–16.0, 16.1–20.0 and 20.1–24 min. Printed: 355.9, 965.7, 263.3,
  72.9, 209.2, 95.0. The bands are closed ranges over the costs as given.
- **Unrounded exp:** rounding exp(−β c) to the printed two decimals first gives 155.99 for cell
  (1,1), not 155.73. So the book computed with unrounded values, and so must a fixture.
- **Converged** (largest error < 10⁻¹²):
  - row 1: `157.04 100.36 66.14 76.46`;
  - a_i = 1.2004, 1.0807, 0.6739, 1.2626;
  - b_j × 341.67 = 176.59, 251.16, 333.33, 574.86;
  - mean cost 8.65 min;
  - six passes reach ≤ 0.1 %.

## 5. What this does not do

- It checks arithmetic, not behaviour. β = 0.10 is the book's illustration, not a calibrated value,
  and no number here becomes a default (BOOKS §1, rule 4).
- §5.5's calibration (Hyman's method) and §5.6's tri-proportional method were not read for
  numbers. The contract keeps both out (choice 8).

## 6. Singly constrained (D3 contract, D475)

Read for [TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md) §15.

| Point | Book | Page (book / PDF) | Image read | Finding |
|---|---|---|---|---|
| Model | §5.3.2, Eq. (5.16): one set of balancing factors set to 1; origin constrained: B_j = 1, A_i = 1 / Σ_j D_j f(c_ij) | 183 / 205 | `p205-math-0239.png` | T_ij = O_i · D_j f_ij / Σ_k D_k f_ik. One row normalisation; no iteration |
| Fixture | Example 5.2, Tables 5.4 and 5.5: an origin-constrained growth factor; each row scaled once to its target O_i | 179–180 / 201–202 | `p201-table-051.png`, `p202-table-052.png` | The same arithmetic as one row pass with the prior in place of D_j f_ij |
| Purposes | §5.8.4: work trips normally doubly constrained, most others singly | 202 / 224 | (text, §2 above) | Choice 9's later slice |

- **Example 5.2 replayed** (prior of Table 5.4 = Table 5.6's; O = 400, 460, 400, 702):
  - every printed cell of Table 5.5 is reproduced to one decimal, e.g. row 1 `5.6 56.3 112.7 225.4`
    and row 4 `123.2 246.3 307.9 24.6`;
  - the rows equal O_i exactly.
- **Two printed column sums differ:**
  - Column 4 is printed 701.2. Its printed cells add to 710.2 and the replay gives 710.1, so the
    printed sum is a digit transposition.
  - Column 1 is printed 257.7 and the unrounded replay gives 257.8. The printed cells add to
    257.7.
- **No printed singly-constrained *gravity* example.** A grep of Ch. 5's text and exercises for
  "singly", "origin-constrained" and "destination-constrained" finds only definitions and
  remarks: §5.1 (notation), §5.2.2 (growth factors), §5.3.2, §5.4 (dropping one entropy
  constraint), §5.7 (destination choice) and §5.8.4. The other rows are hand-computed.
- **Scaling invariance.** Eq. (5.16) is unchanged when every D_j is multiplied by one constant.
  TRIP_GENERATION §4 balances by such a constant (A*_i = A_i · P / A), so raw and balanced
  attractions give the same trips.
