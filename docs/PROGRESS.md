# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-06 — M10.3 D4: destination-constrained purposes (D482)

- **Owner:** "งานถัดไปตามแนะนำ"; the recommended options (all a) stand.
- **D482 ([TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md) §16):**
  - `constraint: "destination"` (A_i = 1, p. 183, re-read from the page image), computed as the
    origin path on the transposed problem;
  - T_ii = s · A*_i, and owned sets using it at schema 72;
  - `productionsModelled`, the CSV suffix and the page note.
- **Rows:** `tripdistributiondestination` (TD21–TD25: Example 5.2 transposed, weights, intrazonal,
  refusals, schema 72), `cli-trip-distribution-destination` and `trip-distribution-ui` (TD26).
  TD19's bad value is now `"both"`. Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless). Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.4 N: nested logit, contract and N1 (D480, D481)

- **Owner:** "ทำงานถัดไป" (nested logit, picked at the compaction). Choices answered in the
  session: two levels, φ in (0, 1] refused outside, and a nest editor in the dialog (3b, against
  the recommended Import only).
- **D480 ([NESTED_LOGIT](reference/NESTED_LOGIT.md)):** N0 read Eq. 7.16–7.23 (PDF 260–261) and
  §7.4.4.3 (PDF 267) from the page images; the contract.
- **D481 (NESTED_LOGIT §7):**
  - `nestedLogitShares` (Eq. 7.23 with β = 1, shifted logsums), `splitModes(…, ModeNests)`;
  - per-purpose `nests` in mode-choice sets, owned nested sets at schema 71;
  - the run and the JSON echo;
  - *Nests…* in the Mode choice sets dialog.
- **Rows:** `nestedlogit` (NL1–NL7: Example 6.3's ten rows, the MNL collapse, availability,
  numerics, the codec, schema 71, the run), `cli-mode-choice-nested`, `nested-logit-ui` (NL8);
  MC1–MC14 unchanged. Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless). An offscreen grab of the dialog was checked for
  layout. Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.4 E2: owned sets, Mode choice page, Write OD matrices (D479)

- **Owner:** "ทำงานถัดไป".
- **D479 ([MODE_CHOICE](reference/MODE_CHOICE.md) §13):**
  - owned `modeChoiceSets` and `odMatrices[i].generatedBy` at schema 70;
  - the *Mode choice* Results page, with *Mode choice sets…* and *Period factors…*;
  - *Write OD matrices*, one History step replacing only generated matrices, which Run Macro
    assigns.
  - **Fixed:** D476's note key shadowed the Trip distribution page's Origin column. Locale keys
    are now checked for repeats.
- **Rows:** `modechoiceowned` (MC11–MC12) and `mode-choice-ui` (MC13); `workspace-ui` counts 11
  Results tabs. Three mutations each failed a row.
- **Verified:** Linux only (desktop and headless). An offscreen grab of the page was checked for
  layout. Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.4 E1: mode choice engine, period factors, run and CLI (D478)

- **Owner:** "เอาตามแนะนำ" on MODE_CHOICE §10 (1a–8a).
- **D478 ([MODE_CHOICE](reference/MODE_CHOICE.md) §13):**
  - the MNL split (exp(V − max V), unavailable alternatives excluded);
  - P-A to OD by period factors (root `periodFactors`, schema 69);
  - vehicles by occupancy, summed per mode;
  - marked JSON/CSV, and `--mode-choice`/`--period`.
  - The zone skim moved to `zone_skim`, with distribution unchanged.
  - The four-step CLI runs moved to `tools/four_step_cli`.
- **Rows:** `modechoice` (MC1–MC9) and `cli-mode-choice*` (MC10). Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless); Windows not checked.

## 2026-11-06 — M10.4 contract: mode choice and OD conversion (D477)

- **Owner:** "ทำงานถัดไปตามแนะนำ".
- **M0:** read Ortúzar §5.4.4 and Ch. 6–7 from the page images
  ([evidence](evidence/ortuzar-ch6-7-mode-choice.md)):
  - Examples 6.2 and 6.3 reproduce, including every row of the λ₁ table;
  - MNL (7.9) and the NL condition φ ≤ 1;
  - no book value for occupancy or period shares.
- **D477 ([MODE_CHOICE](reference/MODE_CHOICE.md)), docs only:** eight owner choices, slices
  E1–E2, rows MC1–MC14.

## 2026-11-06 — M10.3 D3: origin-constrained purposes (D476)

- **Owner:** "เอาตามแนะนำ" on TRIP_DISTRIBUTION §15.2 (1a–5a).
- **D476 ([TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md) §15.4):**
  - a per-purpose `constraint: "origin"` in deterrence sets (schema 68 when owned);
  - one row normalisation of A_j f_ij (Eq. 5.16), with T_ii = s · P_i;
  - JSON, CSV and the page name the constraint and the modelled attractions;
  - doubly outputs keep their bytes.
- **Rows:** `tripdistributionorigin` (TD15–TD19; TD15 reproduces Example 5.2's Table 5.5),
  `cli-trip-distribution-origin` and `trip-distribution-ui` (TD20).
- **Verified:** Linux only (desktop and headless); Windows not checked.

## 2026-11-06 — M10.3 D3 contract: singly constrained purposes (D475)

- **Owner:** asked for an explanation of singly constrained models, then "ทำงานต่อไปตามแนะนำ".
- **D475 ([TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md) §15), docs only:**
  - Eq. (5.16) (p. 183) read from the page image;
  - Example 5.2 (Tables 5.4–5.5) replayed as the fixture, with a misprinted column sum (701.2 for
    710.2) ([evidence](evidence/ortuzar-ch5-trip-distribution.md) §6);
  - raw and balanced attractions shown to give the same trips;
  - five owner choices; rows TD15–TD20.
- **Not done:** no code. Slice D3 waits for the owner's answers.

## 2026-11-06 — M10.3 D2: owned deterrence sets and cost matrices, Trip distribution page (D474)

- **Owner:** "เริ่มเลย" on D2.
- **D474 ([TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md) §14):**
  - `deterrenceSets` and `costMatrices` at schema 67, with commands;
  - a run-chosen matrix overrides skim cells and is the only source of a diagonal;
  - G2's trip-rate dialog made a shared owned-list dialog;
  - the self-owned *Trip distribution* page;
  - `--cost-matrix`.
- **Rows:**
  - `tripdistributionowned` (TD12 and the override rows) and `trip-distribution-ui` (TD12 save,
    TD13, the dialogs).
  - `workspace-ui` updated deliberately for the tenth Results tab.
- **Verified:** Linux only (desktop and headless; counts in the session report). An offscreen grab
  of the page and the dialog was checked for layout. Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.3 D1: gravity distribution engine, skim, run, outputs and CLI (D473)

- **Owner:** "ตามแนะนำ" on TRIP_DISTRIBUTION §11 (1a–9a).
- **D473 ([TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md) §14):**
  - `src/macro/trip_distribution` (row-first Furness, exponential gravity seeded by T/Σf,
    intrazonal share, the stopping measure);
  - `freeFlowSkim`, with zone costs as the connector-share-weighted mean;
  - deterrence sets as data;
  - `runTripDistribution`, JSON/CSV and `--trip-distribution`.
- **Rows:**
  - TD1–TD10 (group `tripdistribution`) and four CLI tests (TD11).
  - Examples 5.3 and 5.4 reproduce the printed tables.
  - Four engine mutations each failed a row.
- **Found while building:** the Macro graph's centroids are per zone connector, not per zone, so
  the skim aggregates connector pairs by share (§5).

## 2026-11-06 — M10.3 contract after reading Ortúzar Ch. 5 (D472)

- **Owner:** "สำหรับงานถัดไปคือ M10.3", then "ทำงานถัดไป".
- **D0 ([evidence](evidence/ortuzar-ch5-trip-distribution.md)):** the page images of Tables
  5.6–5.10 and the band table were read. A scratch replay reproduces Table 5.7 and Table 5.10
  (cells, a_i, b_j, bands) with three row-first passes. Table 5.10's row-2 sum is misprinted. The
  book gives no Furness tolerance.
- **D472 ([TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md)):** gravity with exponential deterrence
  over M10.2's ends, row-first Furness, a free-flow skim, deterrence sets as data (none shipped),
  rows TD1–TD14, slices D0–D2, and nine owner choices. Docs only.
