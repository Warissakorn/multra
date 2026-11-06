# Ortúzar and Willumsen Ch. 4 against TRIP_GENERATION — 2026-11-06 (G0, D469)

Slice G0 of [TRIP_GENERATION](../reference/TRIP_GENERATION.md) §12: read Chapter 4 of the owner's
copy and record what the contract may cite. Source: Ortúzar, J. de D. and Willumsen, L. G.
(2011), *Modelling Transport*, 4th ed., Wiley. Copy `Warissakorn/Modelling-Transport-4-edition-Markdown`
at commit `1b3d41d` (the commit BOOKS §2 surveyed); book p. = PDF p. − 22. Every number below
was read from the page image named, not from the automated transcription (BOOKS §1). No book
text is copied beyond the numbers and one-line paraphrases.

## 1. What the chapter gives the contract

| Contract point | Book | Page (book / PDF) | Image read | Finding |
|---|---|---|---|---|
| §3 cross-classification | §4.3.1.3, Eq. (4.16): productions = Σ over household types of (households of type h in zone i) × (trip rate of type h, purpose p) | 158 / 180 | `assets/p180-math-0209.png` | Same form as §3's `T_i = Σ_c N_ic · t_c`. Rates are per household type and purpose (Eq. 4.15, p. 157) |
| §3 regression, zonal totals | §4.2.5: a household-based linear model is aggregated by putting the zone's mean of each variable into the equation and multiplying by the zone's households | 156 / 178 | `assets/p178-math-0204.png`, `p178-math-0205.png` | Expressible in §3's form with zone **totals** as attributes and `a_0 = 0`: the household intercept becomes a term on the household count (§2 below) |
| §4 balancing | §4.2.6, Eqs. (4.13)–(4.14): origins and destinations need not sum alike; normal practice takes the sum of origins as the correct total T and multiplies every destination by f = T / Σ_j D_j | 156–157 / 178–179 | `assets/p179-math-0207.png` | Exactly choice 3a. The book's reason: production models are household-based and "better"; attraction models are at best zonal |
| §1 out of scope | §4.1.4 growth factors (Eq. 4.1–4.2), §4.4 accessibility, §4.5 frequency-choice logit, §4.7 updating rates | 143–172 | — | None needed for M10.2; all stay out |
| §9 seam | §4.3.3.3: attractions and NHB productions "require ad hoc methods" at zone level | 163 / 185 | — | No book rule for NHB; the contract keeps NHB as an ordinary purpose of the set |

## 2. Fixtures with printed answers

**Example 4.1** (book pp. 143–144; PDF 165–166; images `p166-math-0179.png`, `p166-math-0182.png`).
A zone of 250 car-owning households at 6.0 trips/day and 250 non-car-owning households at
2.5 trips/day:

- today: 250 × 2.5 + 250 × 6.0 = **2125 trips/day**;
- with all 500 households owning a car and the rates held: 500 × 6 = **3000 trips/day**.

This is cross-classification with two categories, and the fixture for TG12.

**Example 4.4's zonal form** (book p. 156; PDF 178; images `p177-math-0203.png`,
`p178-math-0205.png`). The household model Y = 0.84 + 1.41 X₁ + 0.75 Z₁ + 3.14 Z₂ (R² 0.387;
X₁ workers, Z₁ one car, Z₂ two or more cars) aggregates to

```
T_i = H_i (0.84 + 1.41 X̄_1i) + 0.75 H_1i + 3.14 H_2i
    = 0.84 H_i + 1.41 W_i + 0.75 H_1i + 3.14 H_2i      (W_i = H_i X̄_1i, the zone's workers)
```

so a regression with `a_0 = 0` and terms on the attributes H, W, H₁, H₂ reproduces it. The book
prints no zone values, so TG4b uses these coefficients with hand-chosen zone totals; the printed
part is the coefficients, not an answer. Table 4.3's third step (p. 153, `p175-table-035.png`),
Y = 0.91 + 1.44 X₁ + 1.07 X₂, aggregates the same way (`p178-math-0204.png`).

## 3. The booking that was wrong

BOOKS §3.3 named **Example 4.5 (p. 161)** as the category-analysis fixture. It is not one: it
shows Table 4.7, the 1977 Santiago sample's 15 853 households in 13 income and car-ownership
categories (p. 162), and reports that per-category regressions were satisfactory (R² 0.401 to
0.682) without printing their coefficients or any zone result. There is nothing to reproduce.
BOOKS §3.3 and §4 are corrected to Example 4.1, Example 4.4's zonal form and Eq. (4.14).

## 4. Not used

- The chapter's exercises (pp. 172–173) print no answers.
- No trip rate from the book is shipped: the examples are illustrations, and choice 8a ships none.
