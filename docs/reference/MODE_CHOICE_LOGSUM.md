# Logsum composite cost into distribution — G/D/MS/A (MODE_CHOICE §3, D548)

Written 2026-11-13 as a contract, before code. MODE_CHOICE §3 left out logsum feedback:
"no logsum is fed back, so Eq. (6.11)'s β ≤ λ is not tested". The owner chose it as the next M10
slice and answered §7 in the same session, each with the recommended option. Rule 4 holds:
- every output keeps the not-validated marker;
- reproducing a book number checks arithmetic, not behaviour.

**Source.** Ortúzar and Willumsen (2011), *Modelling Transport* 4th ed., §6.5.2 (book pp. 213–214;
repository `Modelling-Transport-4-edition-Markdown`, commit `1b3d41d`). Paraphrased:
- In a sequential distribution and mode-split model, the only composite cost consistent with
  rational choice (Williams 1977) is the logsum:
  K_ij = −(1/λ)·log Σ_k exp(−λ·C^k_ij) (Eq. 6.10).
- It must satisfy β ≤ λ (Eq. 6.11), where β is distribution's and λ mode choice's cost
  parameter.
- The logsum is at most the cheapest mode's cost. With β > λ, the G/D/MS/A structure "will produce
  pathological results".
- G/MS/D/A (Eq. 6.12–6.13) is the alternative structure. It is out of this slice (§7, 1).

## 1. Scope

- **In:**
  - a per-purpose switch in a mode-choice set;
  - the composite cost from the purpose's own alternatives;
  - the distribution of that purpose on the composite;
  - Eq. 6.11 refused;
  - JSON echo;
  - the editor switch.
- **Out:**
  - G/MS/D/A;
  - alternatives with different cost coefficients in a logsum purpose;
  - estimating λ or β;
  - a logsum over purposes.

## 2. Data (schema 76)

A mode-choice purpose gains an optional key:

```json
{ "id": "all", "distributionCost": "logsum", "alternatives": [ … ], "nests": [ … ] }
```

**Absent keeps today's distribution cost**, the skim of the distribution's `skimMode` and its cost
matrix (TRIP_DISTRIBUTION §5).

**Shape faults** are `MODE_CHOICE_INVALID_SET` at `<set path>.purposes[p].distributionCost`:
- a value other than `"logsum"`;
- the purpose's alternatives do not all share one `timeCoefficient`;
- that coefficient is 0, because λ = −θ must be > 0. A positive θ is already refused
  (MODE_CHOICE §3).

**Where the key is read:**
- Catalog sets (`mode-choice/*.json`) read it at any time.
- An owned set (`modeChoiceSets`) holding it selects schema **76**. Below 76 it is
  `EDIT_UNSUPPORTED_FIELD` at `modeChoiceSets[s].purposes[p].distributionCost`.
- A file without it keeps its bytes and its schema.

## 3. Composite cost

For a logsum purpose with λ = −θ, pair (i, j), and the alternatives available there (finite cost,
MODE_CHOICE §3–§4):
- V_k = ASC_k − λ·c_k(i, j);
- LS_ij = ln Σ over the branches of exp(W), with W as in NESTED_LOGIT §2. With no nests every
  alternative is its own branch, which gives the MNL;
- K_ij = −LS_ij / λ, in minutes. A constant enters as an offset of −ASC/λ minutes.

**No available alternative** gives K_ij = +∞, so f = 0 and the pair gets no trips. The diagonal
follows the same rule.

**Properties** (§6.5.2):
- K ≤ min_k(c_k − ASC_k/λ);
- with one alternative, K = c − ASC/λ;
- at φ = 1 the NL's K equals the MNL's.

## 4. Run

`runModeChoice` (and so every procedure, page and CLI run of the set):

1. **Distribution, then costs.** The run goes as today: the distribution with its skim, then each
   alternative's costs. Both follow the run's `linkTimes`, so D547's feedback gives congested
   composites without change.
2. **Composite costs.** For each logsum purpose, K is built from that purpose's alternative costs.
3. **Eq. 6.11 check, before any trip is re-balanced.** A logsum purpose whose distribution β (its
   deterrence purpose's `beta`) exceeds λ is `MODE_CHOICE_STRUCTURE_INVALID` at
   `<set path>.purposes[p]`. β = λ passes.
4. **Distribution again.** The distribution runs again with each logsum purpose's K replacing
   the skim. Its intrazonal share and constraint are unchanged. The other purposes keep the skim,
   so their results are byte-identical. This distribution replaces the run's.
5. **The mode split** is unchanged and works on the new P-A tables.

**In a procedure,** the separate distribution step still shows its skim-based result. Only the
mode-choice step uses the composite. Under D547's feedback, the distribution step takes the mode
choice's distribution, as it already does.

## 5. Outputs

- **Distribution JSON:** a purpose that used the composite gains `"costSource": "logsum"`.
- **Mode-choice JSON:** the set echo carries `distributionCost`.
- **Byte-identity:** both appear only when used, so every existing output is byte-identical.

## 6. Hand values and rows

**(a) Ortúzar Example 6.3** (p. 216).
- **Set-up:** a logsum purpose holding only red bus and blue bus at 50 minutes each. They share
  θ = −λ₁ = −0.1 and both sit in one nest with φ = λ₁/λ₂, λ₂ = 0.9.
- **Result:** K = 50 − ln 2 / 0.9 = **49.2298**, the book's printed C_B of 49.23.

**(b) A small G/D/MS/A case** (fixture `tests/data/mode-choice-logsum/`).
- **Zones and demand:**
  - zone O produces 100 trips; D1 and D2 attract 100 each;
  - origin-constrained exponential gravity, β = 0.05.
- **Alternatives,** both from owned cost matrices, sharing θ = −0.1:
  - car: O→D1 10 min, O→D2 20 min, ASC 0;
  - walk: O→D1 15 min, O→D2 60 min, ASC −0.5.
- **By hand:**

| | K (min) | T (P-A) | car | walk | P(car) |
|---|---|---|---|---|---|
| O→D1 | 6.867383 | 65.725987 | 48.049547 | 17.676440 | 0.731059 |
| O→D2 | 19.889523 | 34.274013 | 33.897446 | 0.376567 | 0.989013 |

  With the car skim as the distribution cost (today's form), T is 62.245933 / 37.754067. That is
  the forcing: the composite moves trips towards D1, where walk is a real alternative.

| Row | Asserts |
|---|---|
| LS1 | Codec and schema 76: round trip for an owned set; each §2 fault at its path; absent keeps the bytes and schema; < 76 refuses |
| LS2 | Differing or zero θ in a logsum purpose is refused at `…distributionCost` |
| LS3 | The composite equals (a) 49.2298 to 1e-4, and (b)'s K to 1e-6 |
| LS4 | (b)'s distribution and split equal the table to 1e-6, against the forcing's skim-only T |
| LS5 | β = 0.2 > λ = 0.1 is `MODE_CHOICE_STRUCTURE_INVALID` at its path; β = λ passes |
| LS6 | A pair with no available alternative gets K = +∞ and no trips. With nests at φ = 1, K equals the MNL's |
| LS7 | With link times passed (D547), a network alternative's composite follows the congested skim |
| LS8 | `costSource` and the set echo appear only when used: shipped projects and existing outputs are byte-identical |
| LS9 | Editor: the switch is one Undo step and saves 76; differing θ is refused in place |

Each row first asserts that its forcing worked.

## 7. Owner choices (answered 2026-11-13, D548; each the recommended option)

1. **Structure.**
   - **(a) G/D/MS/A with the composite cost (chosen).**
   - (b) G/MS/D/A.
   - (c) both.
2. **Where.**
   - **(a) Per purpose in the mode-choice set (chosen).** Every run of the set agrees, and
     purposes may differ: work and non-work.
   - (b) An option on a procedure's mode-choice step.
3. **When Eq. 6.11 fails.**
   - **(a) Refuse (chosen).**
   - (b) An advisory, and run.
4. **λ.**
   - **(a) λ = −θ, with one coefficient per logsum purpose (chosen).**
   - (b) A separate λ per purpose.

## 8. Slices

| Slice | Content | Rows |
|---|---|---|
| L0 | This contract | — |
| L1 | Codec at 76, `logsum` in `src/macro/mode_choice`, the composite and the re-run in `runModeChoice`, JSON | LS1–LS8 |
| L2 | The switch in the *Nests* dialog | LS9 |
