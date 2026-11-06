# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-06 — Docs: the decision record split every 200 IDs (D486)

- **Owner:** ran /optimize-program on docs and AI context, and chose the split by 200 IDs, with the
  decision index split by the same ranges.
- **D486:** `RECORD.md` (500 lines, ~134k tokens) became `RECORD-D001-D199.md` and
  `RECORD-D200-D399.md` (frozen) and `RECORD.md` (D400 onward, 97 lines). The rows moved unchanged.
  527 anchored links in 24 files now name the file that holds the row. AGENTS says how to cut the
  next range.
- **Verified:** `multra-check-docs` and `multra-check-file-sizes`; no code changed.

## 2026-11-06 — Procedure sequence P2: Procedures dialog and Procedure page (D485)

- **D485 ([PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md) §9):**
  - *Procedures…* edits the list and its steps, refused in place, as one History step;
  - the twelfth Results tab runs a procedure on a worker with Cancel and shows each step's status
    and summary, with stale marking and a marked CSV.
- **Rows:** `procedure-ui` (PS10); `workspace-ui` counts 12 tabs.
- **Note:** `docs/decisions/RECORD.md` reached 500 lines with D485; splitting it is the owner's call
  (NEXT).
- **Verified:** Linux only (desktop and headless). Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — Procedure sequence P1: engine, schema 73, CLI (D484)

- **Owner:** approved the plan after D483.
- **D484 ([PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md) §8):**
  - `procedures` at schema 73, and `runProcedure` (nearest earlier active input, an in-memory
    assignment demand, a refusal stops the rest, Cancel);
  - `procedureJson` and `--procedure`;
  - `generatedOdMatrices` moved into the project layer.
- **Rows:** `procedure` (PS1–PS8) and `cli-procedure*` (PS9). Found by the rows: a `{}` initialiser
  had set every step to *done*; fixed. Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless). Windows not checked.

## 2026-11-06 — Procedure sequence contract (D483)

- **Owner:** "ลำดับขั้นตอนการรัน"; answered §6 in the session: a general step list (1b, against
  the recommended fixed chain), in-memory matrices (2a), stored procedures (3a), feedback later
  (4a).
- **D483 ([PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md)):** steps `tripGeneration`,
  `tripDistribution`, `modeChoice` and `assignment`, each taking the nearest earlier active input;
  a refused step stops the later ones; schema 73; `--procedure`; rows PS1–PS11. Docs only.

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
