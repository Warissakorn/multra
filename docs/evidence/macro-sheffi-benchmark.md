# MA15 — Sheffi (1985) Table 5.4 as the Macro benchmark (D294, 2026-10-22)

D272 (choice 7b) chose a textbook network with a published equilibrium. Its first candidate was
Sheffi's book. This note records what was checked, before row MA15 was written as code
([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §11).

## Source

- **Citation:** Yosef Sheffi, *Urban Transportation Networks: Equilibrium Analysis with
  Mathematical Programming Methods*, Prentice-Hall (1985).
  - Section 5.2, book p. 120, PDF p. 136, Table 5.4: the Frank–Wolfe iterations.
  - Section 5.1, book p. 114, PDF p. 130: Figure 5.1, the network.
  - Section 5.1, book p. 113, PDF p. 129: Table 5.1, capacity restraint on the same network.
- **Copy read:** the owner's Markdown transcription,
  `https://github.com/Warissakorn/urban-transportation-networks-markdown`, commit
  `b9f07679be607c0c358606f8427309d46f15665d` (2026-10-22).
  - Table 5.4 is marked `transcription: visually checked` there, and so is Table 5.1.
  - sha256 `2d0f4e285e021d92b8ad4c928ec602f33da964c51153b7233317b8f70a76f359` for
    `chapters/05-solving-for-user-equilibrium/02-applying-the-convex-combinations-method.md`
    (Table 5.4).
  - sha256 `f1088da8cab9db609cc4dfd06845900457a867e2dd939635c163f6c4a1d330b1` for
    `…/01-heuristic-equilibration-techniques.md` (Table 5.1, Figure 5.1's OCR).
- **No PDF was available**, so no PDF hash is recorded.

## The network (Figure 5.1)

- Three parallel links join one origin to one destination, with q = 10 flow units.
- tₐ = t₀ₐ (1 + 0.15 (xₐ / cₐ)⁴), with t₀ = (10, 20, 25) time units. This is the BPR form with
  α = 0.15 and β = 4, which is `data/vdf/bpr.json`'s.
- **Capacities c = (2, 4, 3) are derived, not read.**
  - `assets/pages/pdf-0130.png` (book p. 114) is a 0-byte file in that commit. The page's OCR keeps
    the free-flow times and drops each capacity.
  - Table 5.1 gives t₁(10) = 947: 10 (1 + 0.15 (10/c₁)⁴) = 947 gives (10/c₁)⁴ = 624.7, so c₁ = 2.0.
  - It also gives t₂(10) = 137: 20 (1 + 0.15 (10/c₂)⁴) = 137 gives (10/c₂)⁴ = 39, so c₂ = 4.0.
  - c₃ is fitted to Table 5.4: c₃ = 3 matches every printed number, and 2.5, 2.8, 3.2 and 3.5 each
    miss some (for example 3.2 gives step 4 = 0.021 and t₃ = 25.2 at iteration 4).
  - **Owner check:** confirm c = (2, 4, 3) against the printed figure (OWNER_SITTING).

## Replay

A scratch replay (Python, exact bisection of the Beckmann derivative, not kept) against the
printed table:

| Move k | x replayed | x printed | step replayed | step printed | z at next update replayed / printed |
|---|---|---|---|---|---|
| 1 | 4.0346, 5.9654, 0 | 4.04, 5.96, 0.00 | 0.59654 | 0.596 | 197.404 / 197.00 |
| 2 | 3.3845, 5.0042, 1.6113 | 3.39, 5.00, 1.61 | 0.16113 | 0.161 | 189.994 / 189.98 |
| 3 | 3.6197, 4.8263, 1.5541 | 3.62, 4.83, 1.55 | 0.03555 | 0.035 | 189.445 / 189.44 |
| 4 | 3.5458, 4.7278, 1.7264 | 3.54, 4.73, 1.72 | 0.02040 | 0.020 | 189.361 / 189.33 |
| 5 | 3.5922, 4.6938, 1.7139 | 3.59, 4.70, 1.71 | 0.00719 | 0.007 | 189.340 / 189.33 |

- **Largest differences:** 0.0064 in x, 0.16 in t (iteration 2, 34.84 against 35.0), 0.40 in z
  (iteration 2) and 0.0006 in the step. The book rounded its intermediates, which explains these.
- **Misprints:**
  - Iteration 2's direction is printed as link 1. The times (35.0, 35.0, 25.0) make link 3 the
    shortest, and the printed move to x₃ = 1.61 needs it.
  - Iteration 3's t₃ is printed as 35.3. Its own x₃ = 1.61 gives 25.3.
  - The transcription notes both as printed.
- **Equilibrium:** equal times give u = 25.456 at x = (3.5833, 4.6451, 1.7716), z = 189.3320. The
  book's last row prints t = (25.6, 25.7, 25.4) and z = 189.33. Its text says the flows are "close
  to equilibrium".

## Row and result

- **Row:** `tests/macro_benchmark_tests.cpp` (ctest `macrobenchmark`), with these tolerances:
  - (a) for capped runs of k = 1–5: x ±0.01, t ±0.2, z ±0.5, step ±0.001;
  - (b) at the 10⁻⁴ target: the flows and times within 10⁻³ (relative) of the equal-time solution,
    and z within 0.005 of 189.33.
- **Result:** on 2026-10-22 (Linux) the engine passed both, unchanged. The iterations and cost are in
  [ENGINEERING_NOTES](../reference/ENGINEERING_NOTES.md).
- **What this does not show.** It is one row of the M10 gate on a three-link network, in the book's
  units. It does not test the document compile (MA3–MA6, MA11 cover that) or validate any field
  flow. The not-validated marker stays until M6 (rule 4).
