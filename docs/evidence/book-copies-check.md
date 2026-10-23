# The owner's book copies, checked against MULTRA's citations — 2026-10-23

The owner asked for the five book repositories to be analysed and added for development
([BOOKS](../reference/BOOKS.md) is the maintained map). This note records what was checked
against the copies on 2026-10-23, at the commits BOOKS §2 lists. Docs only: no engine, data or
test changed, and no owner gate moved. Values were read from the copies' **source images**, not
their transcriptions, wherever an image exists.

**Reproduce:** attach each repository with `add_repo`, `git clone --depth 1`, then open the files
named below.

## 1. Sheffi: empty page images

`find assets -type f -size 0` in `Urban-Transportation-Networks-Markdown` finds 10 of 415 page
images empty: PDF pages 30, 130, 133, 151, 173, 183, 201, 209, 210 and 331, which are book pages
14, 114, 117, 135, 157, 167, 185, 193, 194 and 315 (PDF p. = book p. + 16). The other four copies
have no empty asset.

- Book p. 114 holds Figure 5.1, MA15's network. The copy gives no new evidence on its capacities,
  so OWNER_SITTING B21 still needs the print. The derivation (c = 2, 4, 3) stands as recorded in
  [macro-sheffi-benchmark](macro-sheffi-benchmark.md).
- Any later use of the other nine pages must check that the text it needs is not on the image only.

## 2. HCM 7 against the M6.0 option sheet

[m6-benchmark-options](m6-benchmark-options.md) (2026-10-10) says the HCM text was not read and
its HCM values came from agency and software documents. The 7th edition copy now answers those
points. The sheet is dated evidence and stays as written; this section is the check.

| Sheet's statement | HCM 7 copy (PDF page) | Result |
|---|---|---|
| U-a: base `t_c / t_f` for a major street of fewer than four lanes: major left 4.1 / 2.2, minor right 6.2 / 3.3, minor through 6.5 / 4.0, minor left 7.1 / 3.5 s | Exhibit 20-17 (1616) and Exhibit 20-18 (1617), column "Two Lanes", one-stage values; read from `assets/tables/p1616-table-01.png` and `p1617-table-01.png` | **Confirmed.** The column is the number of major-street lanes, two; four and six lanes have their own values (e.g. minor right 6.9 s at four lanes). Two-stage critical headways at two lanes: minor through 5.5 s in each stage, minor left 6.1 s in each stage |
| U-a: `c_p = v_c·e^(−v_c·t_c/3600) / (1 − e^(−v_c·t_f/3600))` | Equation 20-18 (1618); adjustments Eq. 20-16 (`t_c`) and Eq. 20-17 (`t_f`) for heavy vehicles, grade and three-leg geometry | **Confirmed** in the text order; the equation image is the authority for the layout |
| U-a: rank-2 capacity equals potential capacity | Eqs. 20-22 and 20-23 (1621–1622), for major lefts and minor rights | **Confirmed** |
| S1-a: 1,750 pc/h/ln below 250,000 population "not checked in the 7th edition" | Exhibit 19-11 (1430): metro population ≥ 250,000: 1,900; otherwise 1,750 pc/h/ln | **Confirmed** (ROADS §8 already cites it) |
| S1-a: base conditions (12 ft lanes, no heavy vehicles, level grade, no parking, no stopping buses, even lane utilisation, no turns) | Chapter 19, §3, saturation flow rate (PDF part 2 of §3) | **Confirmed**, the same list |
| S1-a: the 1985 HCM's "headways after the fourth vehicle"; the 7th edition's field procedure not read | Chapter 31, §6, Field measurement of saturation flow rate (3090–3091); worksheet Exhibit 31-47 (3092) | Saturation flow is "usually achieved after 10 to 14 s of green, which corresponds to the front axle of the fourth to sixth queued passenger car". Front axle, stop line and **the time the fourth queued vehicle crosses** are the procedure's three reference points. `--discharge-steady-first 5` matches it only if MULTRA's crossing reference is the same point: to be checked before pre-registration |
| S2-a: whether the 7th edition keeps `PF` | Eq. 19-18 (`d = d1 + d2 + d3`), Eq. 19-19 (1475): `d1 = PF · 0.5·C·(1 − g/C)² / (1 − min(1, X)·g/C)`; `k` by Eqs. 19-22 to 19-25 (1476) | **Keeps `PF`** for a lane group with one movement and no permitted operation. Shared lanes, permitted operation and initial queues use incremental queue accumulation (§4, Eqs. 19-34 to 19-36) instead |

Nothing here chooses a benchmark or a tolerance: OWNER_SITTING B9 stays open.

## 3. HCM 7 LOS bounds against `data/los/hcm.json`

| Control | `hcm.json` (cites the 6th edition) | HCM 7 | Result |
|---|---|---|---|
| Signalised | 10, 20, 35, 55, 80 | Exhibit 19-8 (1422): ≤10, >10–20, >20–35, >35–55, >55–80, >80 | Equal |
| Two-way stop | 10, 15, 25, 35, 50 | Exhibit 20-2 (1592–1593): 0–10, >10–15, >15–25, >25–35, >35–50, >50 | Equal |
| All-way stop | 10, 15, 25, 35, 50 | Exhibit 21-8 (1698): the same bounds | Equal |

A delay equal to a bound takes the better letter in both (LOS's rule). HCM 7 also sets F whenever
v/c > 1.0; LOS deliberately does not apply that rule. Updating the pack's description to cite
the 7th edition is optional and would change no number.

## 4. Meso priority: a citable `t_ij`

MESO_NETWORK §5 defers conflicts inside a Node until Brilon and Wu (2001) is read and a `t_ij` is
cited (D297). Brilon and Wu (2001) is not in any copy; HCM 7 cites Brilon's TWSC work (Chapter 20
references). Exhibits 20-17 and 20-18 with Chapter 20's conflicting-flow rules (Exhibits 20-7 to
20-16) are a citable, movement-specific source of headways. Whether they serve as `t_ij` in
Tampère's Eq. 17 is a modelling decision, not settled here.

## 5. Ortúzar and Willumsen: a two-route equilibrium

Example 11.1 (book p. 394, figures p. 395–396) is the town-centre and bypass network of Example
10.4 (p. 367): bypass cost `15 + 0.005 V_b`, town-centre cost `10 + 0.02 V_t` in minutes,
2000 vehicles. The printed equilibrium is 1400 via the bypass and 600 via the town centre, both at
22 min, 44 000 vehicle-minutes; the costs check by hand (15 + 7 = 10 + 12 = 22).
Example 11.2 (p. 396–397) gives the system optimum: 1500 and 500, 43 750 vehicle-minutes, with a
2.5 min town-centre toll to reach it.

This is the kind of analytic two-route fixture the M9 gate names. Using it is M9's own contract
decision; it would equally serve as a second Macro check beside MA15.

## 6. Editions and page rules

- The Transportation Planning Handbook copy is the **4th edition** (ITE, ed. Meyer, Wiley 2016).
  NEXT called it unchecked.
- Page rules per copy are in BOOKS §2. The Handbook has no fixed PDF offset (chapter 1 +15,
  chapter 19 +2); the HCM's printed pages are chapter-page numbers distinct from the PDF page.
