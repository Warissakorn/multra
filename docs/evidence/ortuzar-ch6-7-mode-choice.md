# Ortúzar and Willumsen §5.4.4 and Ch. 6–7 against MODE_CHOICE — 2026-11-06 (M0, D477)

Slice M0 of [MODE_CHOICE](../reference/MODE_CHOICE.md) §12: read the parts of the owner's copy
that M10.4 may cite. Source: Ortúzar, J. de D. and Willumsen, L. G. (2011), *Modelling
Transport*, 4th ed., Wiley. Copy `Warissakorn/Modelling-Transport-4-edition-Markdown`
(book p. = PDF p. − 22).
- **Images:** every number below was read from the page image named (a crop in `assets/`, or the
  PDF page rendered at 80–90 dpi), not from the automated transcription (BOOKS §1).
- **Copying:** no book text is copied beyond numbers and one-line paraphrases.
- **Replays:** a scratch script outside the repository, in double precision.

## 1. Bookings checked (BOOKS §3.3)

| Booking | Page (book / PDF) | Image read | Finding |
|---|---|---|---|
| Example 6.2, red bus / blue bus under an N-way logit | 214 / 236 | PDF p. 236 | **Correct.** Car, red bus and blue bus at equal cost: Eq. (6.8) gives P_C = 0.33, not the expected 0.5 |
| Example 6.3, the hierarchical form | 216 / 238 | PDF p. 238 | **Correct.** P_C = 1/(1 + exp{−λ₁(C_B − C_C)}); P_R/B binary in λ₂; composite C_B = −(1/λ₂) log Σ exp(−λ₂ C). A printed table of P_C against λ₁ at costs 50, λ₂ = 0.9 |

Both bookings hold. BOOKS gives Ch. 6 as pp. 207–226; its examples sit on pp. 213–221.

## 2. What the reading gives the contract

| Contract point | Book | Page (book / PDF) | Image | Finding |
|---|---|---|---|---|
| P-A to OD | §5.4.4 | 190–191 / 212–213 | PDF p. 213 | For 24 h, each P-A trip is made once in each direction. For a shorter period, survey shares of each purpose's P-A matrix travel P→A and A→P. "A typical morning peak" of 70 % P→A and 15 % A→P is an illustration, not a value. **No default:** owner data (MODE_CHOICE choice 4) |
| Mode choice in P-A form | §5.4.4 | 191 / 213 | PDF p. 213 | There is a case for mode choice in P-A form, because the mode depends on the whole tour |
| Logit share | Eq. (6.7)–(6.8) | 211 / 233 | `p233-math-0300.png` (crop) | P^k_ij = exp(−β C^k_ij) / Σ_m exp(−β C^m_ij). Shares depend only on cost differences |
| MNL | §7.3, Eq. (7.9) | 232 / 254 | `p254-math-0355.png` | P_i = exp(β V_i) / Σ_{A_j ∈ A(q)} exp(β V_j) over the *available* alternatives, β normalised to 1 |
| Constants | §7.3.1, Example 7.1 | 232–233 / 254–255 | (text) | Alternative-specific constants: only differences are identified, so one alternative's is fixed at 0 |
| Nested logit condition | §7.4.2.1, Eq. (7.17)–(7.18) | 238 / 260 | `p260-math-0382.png` | φ = β/λ ≤ 1. At φ = 1 the NL collapses to the MNL; φ > 1 is incompatible with utility maximisation |
| Structure | §6.5.2, Eq. (6.10)–(6.11) | 213–214 / 235–236 | PDF p. 236 | With a logsum composite cost, G/D/MS/A needs β ≤ λ. The book suggests G/D/MS/A for work trips and G/MS/D/A for other purposes |
| Occupancy | §11.3, Eq. (11.18)–(11.19) | 407 / 429 | (text) | Assignment carries vehicles while distribution and mode choice carry person trips. Vehicle occupancy η converts them. **No number:** owner data (choice 5) |
| Calibration | §6.5.4, Example 6.4, Table 6.1 | 217–218 / 239–240 | (text) | Berkson–Theil regression for λ and δ (λ ≈ 0.72, δ ≈ 3.15). Out of M10.4 (choice 8), so not replayed |

## 3. Example 6.2 replayed

With equal costs and any β, three alternatives give P_C = 1/3 = 0.333, printed as 0.33.

## 4. Example 6.3 replayed (PDF p. 238)

- **Composite cost.** Costs are 50 generalised minutes for car, red bus and blue bus, with
  λ₂ = 0.9. C_B = 50 − (log 2)/0.9 = 49.2298, printed 49.23.
- **P_C against λ₁.** All ten rows of the printed table are reproduced to three decimals:

| λ₁ | Replay | Printed |
|---|---|---|
| 0.001 | 0.4998 | 0.500 |
| 0.005 | 0.4990 | 0.499 |
| 0.010 | 0.4981 | 0.498 |
| 0.050 | 0.4904 | 0.490 |
| 0.100 | 0.4808 | 0.481 |
| 0.500 | 0.4049 | 0.405 |
| 0.600 | 0.3865 | 0.386 |
| 0.700 | 0.3684 | 0.368 |
| 0.800 | 0.3507 | 0.351 |
| 0.900 | 0.3333 | 0.333 |

- **Collapse to the MNL.** At λ₁ = λ₂ the model collapses to Example 6.2's 1/3.

## 5. Searched and not found

- **No printed mode-choice example over a zone matrix.** The Ch. 6 exercises (pp. 223–225) and
  the Ch. 7 exercises (pp. 266–268) print data and questions, with no answers; Ch. 7's Examples 7.2–7.4 are specification examples. The MC rows of
  M10.4 beyond Examples 6.2–6.3 are hand-computed.
- **No occupancy, no peak-period share and no P→A / A→P split as a number to adopt.** §5.4.4's
  70 %/15 % is marked "for example". Rule 4 keeps each one owner data, with no shipped default.

## 6. What this does not do

It checks arithmetic, not behaviour. λ = 0.9 and the costs of 50 are the book's illustration, and
no number here becomes a default (BOOKS §1, rule 4). Chapters 8–9 (estimation, aggregation) were
not read.
