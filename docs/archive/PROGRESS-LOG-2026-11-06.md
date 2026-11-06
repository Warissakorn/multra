# PROGRESS archive — 2026-11-06

Whole entries moved from the live PROGRESS log.

## 2026-11-06 — M10.2 owner answers and G0, the book read (D469)

- **Owner:** "เอาตามแนะนำ 1a–8a อนุญาตแนบหนังสือ เริ่ม G0".
- **G0 ([evidence](../evidence/ortuzar-ch4-trip-generation.md)):** Ortúzar Ch. 4 attached and read
  from the page images. §3's two forms match Eq. (4.16) and §4.2.5's aggregation; §4.2.6's
  Eq. (4.14) is the same rule as choice 3a.
- **Correction:** Example 4.5 (p. 161), booked as the fixture, prints only sample counts. The
  fixture is Example 4.1 (2125 and 3000 trips/day), with Example 4.4's zonal form as TG4b;
  BOOKS §3.3 corrected.
- **Contract:** answers recorded in §11; household-based models enter on zone totals with no
  second regression form (§3).
- **Verified:** `check_docs` and `check_file_sizes` pass (compiled with g++; the CMake preset still
  lacks nlohmann/json here). Docs only.

## 2026-11-06 — M10.2 trip generation contract (D468)

- **Owner:** asked what could come next, then "เริ่มเขียนสัญญา M10.2 trip generation".
- **D468 ([TRIP_GENERATION](../reference/TRIP_GENERATION.md)):** trip-rate sets as data, one method
  per purpose side (cross-classification or zonal regression), attractions balanced to productions,
  the set's unit carried with no period or mode conversion, outputs, CLI and editor surface, seams
  to M10.3/M10.4, slices G0–G2 and rows TG1–TG14. Docs only; no code.
- **Measured:** `Zone::attributes` is read and written by the codec only; no editor surface or
  command edits it, so choice 7a includes attribute editing.
- **Not read:** attaching the owner's Ortúzar copy was refused by the session's permission check,
  so Example 4.5 (p. 161) is booked as slice G0, and §3–§4 rest on the standard formulation until
  that reading cites Ch. 4.
- **Open:** eight owner choices, OWNER_SITTING B42.
- **Verified:** `check_docs` and `check_file_sizes` pass, compiled directly with g++; the headless
  preset did not configure here (nlohmann/json not installed), so no CMake build or ctest ran. Docs only.
