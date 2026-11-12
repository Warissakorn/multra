# FCTL_REFERENCE — the fixed-cycle traffic-light queue reference (D529)

Contract for the script M6's S2-c row needs ([m6-benchmark-options](../evidence/m6-benchmark-options.md)
§4, §6): the mean delay of the fixed-cycle traffic-light (FCTL) queue with Poisson arrivals,
computed by [vanLeeuwaarden06]'s procedure. It is an **analytic reference**, not an engine figure,
and validates nothing by itself (rule 4). Written before code; rows FC1–FC6 fail first.

Source read: the author's preprint of [vanLeeuwaarden06] (16 pages, supplied by the owner on
2026-11-12, page images read). Page numbers below are that PDF's; equation numbers are the paper's.
No text of the paper is copied here.

## 1. The model (paper §2, PDF p. 4)

- Time is divided into **slots**; one slot is the time one queued vehicle needs to depart. Green
  `g` and red `r` are whole numbers of slots, cycle `c = g + r` (Assumption 1).
- The number of arrivals per slot is i.i.d. (Assumption 2); here **Poisson** with mean `μ` per
  slot, `Y(z) = exp(μ(z − 1))`. Stability needs `c·μ < g` (Eq. 3.1, p. 5).
- A vehicle arriving during the residual green, after the queue has cleared, passes with no delay
  (Assumption 3, the FCTL assumption).
- **Delay** (Definition 1, p. 7): the slots from the start of the slot after the arrival slot to
  the end of the departure slot. **The departure slot counts** (the "+1" of Eq. 4.5, p. 9). The
  part of the arrival slot after the arrival, `D^R ∈ [0, 1]`, is **not** included (§3.2, p. 8).

## 2. The procedure

1. **Roots** (Eq. 5.2, p. 11): for `k = 0 … g − 1`, `w_k = e^{2πik/g}`, iterate
   `z ← w_k · exp(cμ(z − 1)/g)` from `z = 0` until `|Δz| < 1e−13` (the paper's criterion, p. 12).
   A root that has not converged after 100,000 iterations is a refusal.
2. **Unknowns** (Eq. 3.11, p. 7): with `ζ(z) = z / Y(z)` and `η = (g − cμ)/(1 − μ)` (Eq. 3.9, p. 6),
   solve the `g × g` complex system whose first row is all ones (right side `η`) and whose row `k`
   is `ζ(z_k)^j`, `j = 0 … g − 1` (right side 0). Gaussian elimination with partial pivoting.
   Each `q_j = P(X_j = 0)` must be real within `1e−9` and lie in `[0, 1]` within `1e−9`;
   otherwise the result is refused.
3. **Mean overflow queue** `E X_g` by Eq. 3.13 (p. 7), with `σ² = μ` for Poisson.
4. **Mean delay** `E D` by Eq. 3.17 (p. 7), in slots.

## 3. The residual `D^R`

§3.2 (p. 8) states that McNeill's extra term `r / (2cμ(1 − μ))` equals `E D^R`. At the S2-c
settings this term exceeds one slot (1.33 slots at `g = r = 16`, `μ = 0.25`), while `D^R` lies in
`[0, 1]` by definition, so the statement cannot be read as a per-vehicle mean and **is not used**.
The reference is `h · E D` in seconds (`h` the slot length); the residual is stated as a bound: a
delayed vehicle's in-slot wait adds between 0 and one slot, `h`.

## 4. The command

`multra-fctl-reference --saturation S --green-slots G --red-slots R --x X1,X2,…`

- `h = 3600 / S` s; for each `x`, `μ = x·G/(G + R)`, the demand `v = μ · 3600 / h` veh/h.
- Prints one JSON object: `{"model":"fctl-poisson","source":"vanLeeuwaarden06","slotSeconds":h,
  "greenSlots":G,"redSlots":R,"rows":[{"x","mu","flow","meanOverflow","meanDelaySlots",
  "referenceSeconds"}]}`. `referenceSeconds = h · meanDelaySlots`.
- Exit status 2 with a message on stderr for a missing or invalid option, or a refused row.

## 5. Acceptance rows

| Row | What must hold | Test |
|---|---|---|
| FC1 | Poisson, `g = r = 5`, `μ` = 0.30, 0.40, 0.45, 0.49: `E X_g` = 0.1800, 1.0971, 3.3998, 23.2249 (Table 1, p. 12) within 5e−4 | `fctl` |
| FC2 | The same settings: `E D` = 2.7245, 5.0634, 9.9675, 49.8805 slots (Table 2, p. 14) within 5e−4 | `fctl` |
| FC3 | An independent slot-by-slot simulation of the queue (recursion 3.2, p. 6, with Assumption 3 and Definition 1), fixed seed, 10⁶ cycles at `g = r = 5`, `μ` = 0.40: its mean delay lies within 3 standard errors of `E D` | `fctl` |
| FC4 | `c·μ ≥ g`, `g < 1` or `r < 0` is refused | `fctl` |
| FC5 | `g = r = 16`, `μ` = 0.25, 0.35, 0.425 converge, every `q_j` lies in `[0, 1]`, and `E D` rises with `μ` | `fctl` |
| FC6 | The command prints the JSON keys of §4 and refuses a missing `--saturation` | `cli-fctl-reference`, `cli-fctl-reference-refused` |

Table 2's geometric `Var D` column repeats Table 1's `Var X_g` digits (p. 12, p. 14), apparently a
typesetting copy; geometric arrivals are out of scope here.

## 6. References

- **[vanLeeuwaarden06]** van Leeuwaarden, J.S.H. (2006). Delay analysis for the fixed-cycle
  traffic-light queue. *Transportation Science* 40(2), 189–199. Author's preprint read.
