# Nested logit in mode choice (M10.4 N)

Written 2026-11-06 as a contract (D480). The owner answered its three choices in the same session
(§4), and N1 is implemented (D481, §7). It extends [MODE_CHOICE](MODE_CHOICE.md), whose choice 1a
left nested logit to a later slice.

Slice N0 read Ortúzar and Willumsen Ch. 7 from the page images
([evidence](../evidence/ortuzar-ch6-7-mode-choice.md) §2, §4).

Rule 4 applies:
- Every result keeps the not-validated marker.
- Reproducing Example 6.3 checks arithmetic, not behaviour ([BOOKS](BOOKS.md) §1).
- No nest parameter ships as a default.

## 1. Scope

- **In:**
  - a two-level nested logit (NL) per purpose in a mode-choice set, using the alternatives,
    costs, availability, P-A to OD and occupancy of MODE_CHOICE §3–§5 unchanged;
  - the codec, schema 71, the run, the JSON echo;
  - a *Nests* dialog in the editor.
- **Out:**
  - more than two levels, and cross-nested or other GEV forms (§7.4.2.2, §7.7);
  - estimating φ (Ch. 8);
  - logsum feedback into distribution (§6.5.2);
  - the heuristic hierarchical calibration of §6.5.5.

## 2. Model (Eq. 7.19, Eq. 7.23 with β = 1)

For purpose p, pair (i, j) and the available alternatives (finite cost), with
V_k = ASC_k + θ_k · c_k(i, j) as in MODE_CHOICE §3:

```
W_n  = m_n + φ_n · ln Σ_{k∈n, available} exp((V_k − m_n) / φ_n)    m_n = max V_k in nest n
W_k  = V_k                                                            for an alternative in no nest
P(n) = exp(W_n) / Σ_branches exp(W)                                   over branches with an available member
P(k) = P(n) · exp((V_k − m_n)/φ_n) / Σ_{k'∈n, available} exp((V_k' − m_n)/φ_n)
```

- **The parameter.** φ_n is Williams' structural parameter φ = β/λ (Eq. 7.17–7.18, p. 238). It
  equals McFadden's μ_j of Eq. 7.23 with β = 1, because 1/λ_j = φ_j = μ_j (p. 239). The existing
  utilities already have scale 1 (MODE_CHOICE §3), so φ is the one new number per nest.
- **Its range.** φ must lie in (0, 1]:
  - φ > 1 is incompatible with utility maximisation (p. 238, Eq. 7.20, and μ_j ≤ 1 on p. 239);
  - Eq. 7.35 is equivalent to 0 < φ ≤ 1 (§7.4.4.3, p. 245).
- **The MNL as a special case.** At φ = 1 the NL collapses to the MNL (p. 238). A nest with one
  member has W = V, so its φ has no effect.
- **Numerics.** The shifts by m_n and by the largest W (`logitShares`) keep a small φ from
  overflowing or underflowing.
- **Availability.**
  - An unavailable member takes no part, and its nest renormalises over the rest.
  - A nest with no available member drops out.
  - A pair with trips and no available alternative is still `MODE_CHOICE_NO_ALTERNATIVE`.
- **Example 6.3 in this form** (p. 216):
  - Use θ = −λ₁ for car, red bus and blue bus, every constant 0, the bus nest with φ = λ₁/λ₂, and
    costs of 50.
  - Then V/φ = −λ₂C and exp(W) = exp(−λ₁C_B), with C_B = 49.23 at λ₂ = 0.9.

## 3. Data

A purpose in a mode-choice set (MODE_CHOICE §6) may carry `nests`:

```json
{ "id": "HBW", "alternatives": [ … ],
  "nests": [ { "id": "bus", "phi": 0.5, "alternatives": ["red-bus", "blue-bus"] } ] }
```

The values illustrate the shape only.

Each fault is reported at its path:

| Fault | Code |
|---|---|
| `nests` present but empty | `MODE_CHOICE_INVALID_SET` |
| unknown key | `MODE_CHOICE_INVALID_SET` |
| nest id blank | `INVALID_ID` |
| nest id repeated | `DUPLICATE_ID` |
| `phi` not finite, or not in (0, 1] | `MODE_CHOICE_INVALID_SET` at `.phi` |
| members empty | `MODE_CHOICE_INVALID_SET` at `.alternatives` |
| a member that names no alternative of the purpose | `MODE_CHOICE_INVALID_SET` at `.alternatives[m]` |
| an alternative in two nests | `DUPLICATE_ID` at the second member |

- **Not in a nest.** An alternative in no nest is its own branch at the root.
- **The writer** adds `nests` only when a purpose has some, so MNL sets keep their bytes.
- **Schema.** A project's own sets (`modeChoiceSets`) with nests select **schema 71**. Older
  versions refuse `modeChoiceSets[i].purposes[k].nests` with `EDIT_UNSUPPORTED_FIELD`. Catalog sets
  (`data/mode-choice/`) carry no schema version, as before.

## 4. Owner choices (answered 2026-11-06, D480)

The owner chose 1a and 2a, and 3b against the recommendation.

1. **Levels.**
   - **(a) Two levels (Eq. 7.19): root → nests → alternatives; an alternative in no nest is its
     own branch (recommended, chosen).**
   - (b) General trees of any depth.
2. **Parameter.**
   - **(a) φ per nest in (0, 1], refused outside (recommended, chosen).**
   - (b) Allow φ > 1 with an advisory (Börsch-Supan, §7.4.4.3).
   - (c) Store Williams' λ_j and a root β.
3. **Editor.**
   - (a) Nests only through Import of the set's JSON (recommended).
   - **(b) A nest editor in the Mode choice sets dialog (chosen).**

## 5. Outputs and the editor

- **Run.** `runModeChoice` splits a nested purpose by §2; every later stage is unchanged.
- **JSON.** It echoes a nested purpose's `nests` (`id`, `phi`, `alternatives`); the CSV is
  unchanged. MNL outputs keep their bytes. No new CLI flag: `--mode-choice` reads nested sets.
- **Editor.**
  - The Mode choice sets dialog gains *Nests…* for the selected set.
  - The *Nests* dialog holds a table of purpose, nest id, φ and members (comma-separated ids),
    with *Add nest* and *Remove*.
  - A note lists each purpose's alternatives.
  - **OK** validates the set with the codec. A refusal shows its code and path and keeps the dialog
    open.
  - The sets dialog's *Confirm* commits the list in one History step, as before.

## 6. Slices and acceptance rows

| Slice | Content | Rows |
|---|---|---|
| N0 | Read Eq. 7.16–7.23 (PDF 260–261) and §7.4.4.3 (PDF 267); this contract. **Done (D480)** | — |
| N1 | `nestedLogitShares` and `splitModes(…, ModeNests)`; the codec; schema 71; the run and the JSON echo; the *Nests* dialog. **Implemented (D481)** | NL1–NL8 |

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| NL1 | Example 6.3 | C_B = 49.2298; all ten P_C rows of the printed table to three decimals, and to 1e-12 against 1/(1 + exp{−λ₁(C_B − C_C)}) |
| NL2 | Collapse | φ = 1, or every alternative its own nest, equals the MNL to 1e-15; λ₁ = λ₂ gives 1/3 |
| NL3 | Availability | An unavailable member gets 0 and its nest renormalises; an empty nest drops out; no alternative is an issue |
| NL4 | Numerics and IIA | φ = 10⁻³ at costs of 10⁴ min stays finite and sums to 1; red bus / blue bus in a tight nest keeps car near ½ |
| NL5 | Codec | Round trip; each fault of §3 refused at its path; φ = 1 allowed |
| NL6 | Schema 71 | An owned nested set round-trips at 71; 70 refuses `…purposes[k].nests`; a set without nests stays at 70 |
| NL7 | Run and CLI | Shares follow §2 on the run's own costs; persons conserved; the JSON echoes `nests` and an MNL output has none; `cli-mode-choice-nested` |
| NL8 | Editor | *Nests…* adds a nest; φ = 1.5 refused in place at its path; *Confirm* is one Undo step; Cancel changes nothing; saving writes 71 |

## 7. As implemented (N1, D481)

- **Files:**
  - `src/macro/mode_choice.*`: `nestedLogitShares` and `ModeNests`. `splitModes` takes the nests
    as a defaulted last argument; with none it keeps the `logitShares` path.
  - `src/project/mode_choice_codec.*`: `ModeChoiceNest` and `ModeChoicePurpose::nests`.
  - `src/project/document.cpp`: schema 71, with `kLatestSchema` 71.
  - `src/project/mode_choice_run.cpp`: maps member ids to alternative indices.
  - `src/project/mode_choice_output.cpp`: the echo.
  - `src/shell/mode_choice_nests_dialog.*`: the dialog.
  - `src/shell/owned_list_dialog.*`: an optional in-place edit action, set only by the mode-choice
    sets.
- **Test data:** `tests/data/trip-distribution/mode-choice/nested.json`: car, plus walk and jog in
  the nest `foot` with φ 0.5. Its values are illustrations.
- **Rows:**
  - `tests/nested_logit_tests.cpp` (group `nestedlogit`) covers NL1–NL7;
  - `cli-mode-choice-nested` covers NL7's CLI part;
  - `nested-logit-ui` covers NL8;
  - MC1–MC14 pass unchanged.
