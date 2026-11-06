# Trip generation (M10.2)

Written 2026-11-06 (D468) as a contract with eight owner choices (§11). The owner accepted every
recommendation the same day (D469: 1a–8a), and slice G0 checked §3–§4 against the book
([evidence](../evidence/ortuzar-ch4-trip-generation.md)). G1 (D470) and G2 (D471) are implemented (§14). This is the first of the
four-step stages that MACRO_NETWORK left out of M10.1 ([MACRO_NETWORK](MACRO_NETWORK.md) §1,
§15; [MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §3.4 step 1): productions and
attractions per zone and purpose, from zone attributes and a trip-rate set held as data.

Rule 4 applies: every generation result carries the not-validated marker. Reproducing a textbook
example checks arithmetic, not behaviour, and removes no marker ([BOOKS](BOOKS.md) §1).

## 1. Scope

- **In M10.2:**
  - the trip-rate set, a data catalog (`data/trip-rates/`) and project-owned sets;
  - two methods per purpose side: cross-classification (category analysis) and zonal linear
    regression (choice 1);
  - productions and attractions per zone and purpose, and their balancing (choice 3);
  - outputs in JSON and CSV, the CLI and one editor surface (choice 7).
- **Out:**
  - distribution (M10.3), mode choice and person-to-vehicle conversion (M10.4);
  - anything that writes an OD matrix or feeds an assignment: generation output is trip ends,
    not a trip table, until M10.3 consumes it (§9);
  - gate zones, external trips and special generators (choice 6);
  - estimating rates or coefficients from survey data: a set states values, it is not fitted
    here;
  - the procedure sequence (MACRO_NETWORK §19 c), which waits on M10.3–M10.4;
  - growth factors, elasticities and future-year forecasting beyond "change the attributes".

### Already decided (cited, not asked again)

| Item | Decision | Where |
|---|---|---|
| Purposes | HBW, HBO, NHB by default; purposes are named by the data, not by code | D158 (B7 answer 3) |
| Where rates live | `data/trip-rates/`, per purpose; attraction totals balanced to productions | D145 design §3.4 step 1 |
| Zone input | `Zone::attributes`, finite numbers, "read by M10 trip generation only; unknown names kept, not interpreted" | ZONES_AND_PARKING §2.1 (D153) |
| Determinism | No seed; fixed iteration order (zone id, then file order); the same input gives the same bytes | D145 design §3.4 |
| Module | Pure arithmetic in `src/macro/` (imports nothing); `src/project/` resolves zones and catalogs | D145 design §3.4, §4; MACRO_NETWORK §1 |
| Shipped values | None until a source names them, as PCU (D267) and link types (D272 2a) | choice 2's pattern; this contract asks again only for rates (choice 8) |
| Zone kinds | `internal`/`gate` come with M10's boundary flows | STUDY_PERIODS §1 |

## 2. Today (measured from the code on `5a63039`)

- **Zones** (`src/model/demand/zones.hpp`): `Zone{id, name, polygon, attributes}`, with
  `attributes` a `std::map<std::string, double>`. The codec checks names and finiteness
  (`ZONE_ATTRIBUTE_INVALID` at `zones[i].attributes.<name>`). Nothing reads the values, and no
  editor surface or command edits them.
- **No purpose, rate set or trip-end type exists.** `data/` has no `trip-rates/` folder.
- **Macro** (`src/macro/`, MACRO_NETWORK §16–§21) assigns OD matrices of vehicle trips per
  study period. It has no notion of person trips or purposes.
- **Schema:** the newest feature-selected save is 65 (UTDF numbering, D442). A file that uses no
  M10.2 field keeps its schema and bytes (§6).

## 3. The two methods (choice 1)

Notation: zone `i`, purpose `p`, side `s` ∈ {productions, attractions}. Each side of each purpose
names exactly one method. Source: Ortúzar and Willumsen (2011), *Modelling Transport* 4th ed.,
Ch. 4, read in G0 ([evidence](../evidence/ortuzar-ch4-trip-generation.md)).

- **Cross-classification** (category analysis; §4.3.1, Eq. 4.16, book p. 158). The set lists
  categories `c`, each with the
  zone attribute `attr_c` that holds the zone's count of units in that category (usually
  households) and a rate `t_c` in trips per unit:

  ```
  T_i = Σ_c N_ic · t_c,    N_ic = attributes[attr_c] of zone i
  ```

- **Zonal linear regression** (§4.2; aggregation §4.2.5, book p. 156). A constant `a_0` and
  terms `k`, each an attribute and a coefficient:

  ```
  T_i = a_0 + Σ_k a_k · x_ik,    x_ik = attributes[attr_k] of zone i
  ```

- **A household-based model enters on zone totals.** The book aggregates `Y = b_0 + Σ b_k X_k`
  per household to `T_i = H_i (b_0 + Σ b_k X̄_ki)` (§4.2.5). With the zone's household count `H`
  and variable totals (for example workers) as attributes, that is `a_0 = 0`, a term `b_0` on `H`
  and a term `b_k` on each total; dummies become counts of households in their class (Example
  4.4's zonal form, book p. 156). No second regression form is needed.
- **Sums run in file order** (categories, then terms), zones by id. No method reads the polygon,
  the network or another zone.
- **A negative trip end is refused**, `TRIP_GENERATION_NEGATIVE` at the zone and purpose side
  (a regression with a negative constant can produce one). It is never clamped to 0: clamping
  invents a total the data does not state.
- **A zero is valid.** A zone whose terms are all 0 generates 0.

## 4. Balancing (choice 3a)

Ortúzar and Willumsen §4.2.6, Eqs. (4.13)–(4.14), book pp. 156–157: distribution needs equal
totals, and normal practice takes the productions' sum as the correct total and scales every
attraction by `f = T / Σ_j D_j`, because production models are usually the better ones.
Per purpose, with `P = Σ_i P_i` and `A = Σ_i A_i` (raw):

- Attractions are scaled to productions, `A*_i = A_i · P / A`. The factor `f_p = P / A` is
  reported. Productions are never scaled.
- `A = 0` with `P > 0` cannot be balanced: `TRIP_BALANCE_IMPOSSIBLE` at the purpose.
- `P = 0` and `A = 0`: the purpose generates nothing; `f_p` is absent, not 1, and every `A*_i`
  is 0.
- Raw attractions stay in the output beside the balanced ones; a reader can see how far apart
  the two totals were.

## 5. Units and period (choice 4)

- A rate set states one `unit` string (for example "person trips per day"). It is carried into
  every output unchanged and is not interpreted.
- M10.2 does no time or mode conversion (choice 4a). A day-based or person-based set stays
  in its unit; M10.3–M10.4 own the period factor and the person-to-vehicle step, because both
  need distribution and modes. Generation never names a study period.

## 6. Data and storage

### 6.1 A trip-rate set

One file per set in `data/trip-rates/<id>.json`, or a project-owned row (choice 7):

```json
{
  "id": "example",
  "name": "Example set",
  "source": { "citation": "…", "table": "…" },
  "unit": "person trips per day",
  "purposes": [
    {
      "id": "HBW",
      "productions": {
        "method": "crossClassification",
        "categories": [ { "attribute": "hh.cars0", "rate": 0.9 } ]
      },
      "attractions": {
        "method": "regression",
        "constant": 0,
        "terms": [ { "attribute": "jobs", "coefficient": 1.1 } ]
      }
    }
  ]
}
```

The values above illustrate the shape and are not a source. Rules:

- `id` non-blank and unique among shipped and owned sets (`INVALID_ID`, `DUPLICATE_ID`); a
  project set with a shipped set's id is `DUPLICATE_ID`, never a silent override.
- `source.citation` is required for a shipped set and optional for a project set; it is copied
  into every output.
- Purpose ids are non-blank and unique within the set. At least one purpose.
- `method` is `crossClassification` (needs ≥ 1 category, no `constant` or `terms`) or
  `regression` (needs `constant`, ≥ 0 terms, no `categories`). `rate`, `constant` and
  `coefficient` are finite numbers; `rate` is ≥ 0. Attribute names are non-blank; one attribute
  may appear once per side.
- Any other shape is `INVALID_TRIP_RATE_SET` at its JSON path; unknown fields are refused, not
  dropped (AGENTS model invariants).

### 6.2 Zone attributes (choice 2)

- Category counts are ordinary zone attributes (choice 2a), named by the set's
  `attribute` strings (for example `hh.cars0`). This needs no new zone field and no schema step.
  Before G2 only the file carried them. G2 edits them in the **Edit Zone dialog** (owner, D471):
  a name/value table with Add and Remove; Confirm is one Undo step for the whole zone, and a blank
  name, a duplicate name or a value that is not a finite number keeps the dialog open. The Zones
  table shows each zone's attribute count.
- A zone that lacks an attribute named by the selected set is `TRIP_RATE_ATTRIBUTE_MISSING` at
  `zones[i].attributes.<name>` (choice 5); the run is refused, every missing name listed.

### 6.3 Project schema

- Project-owned sets (choice 7a) are an optional root `tripRateSets`, saved at schema **66**; an
  empty list is omitted, so a file without owned sets keeps its schema and bytes. Below 66 the
  field is `EDIT_UNSUPPORTED_FIELD`. A present list must be a non-empty array; rows follow §6.1
  with the citation optional; ids are unique (`DUPLICATE_ID`).
- **Editing (owner, D471):** *Trip-rate sets…* lists the owned sets (id, name, unit, purposes) with
  Import (a set's JSON file, refused at its first faulty path), Export and Remove. Values are
  edited outside MULTRA. Confirm is one Undo step; Cancel changes nothing.
- A run looks in the project's sets first, then the catalog; an owned set sharing the chosen id
  with a catalog set is `DUPLICATE_ID` at `tripRateSets[i].id`, never a silent override.
- No result, selection or generated trip end is stored in the project.

## 7. Engine (`src/macro/`) and project layer

- **`src/macro/trip_generation.hpp`** (pure, imports nothing):
  - inputs: zone ids in order, an attribute table resolved to dense values per zone, and the
    purposes with their two sides as plain structs;
  - `generateTrips(...)` returns, per purpose, `P_i`, raw `A_i`, `A*_i`, the totals and `f_p`,
    or the issues of §3–§4.
- **`src/project/trip_generation_run.*`:** loads the catalog and owned sets, resolves the
  selected set, checks attributes (§6.2), calls the engine and formats JSON and CSV. It never
  touches an OD matrix or a Macro run.
- **Run stages**, as MACRO_NETWORK §19: Inputs (hashes) → Rate set (`TRIP_RATE_SET_UNKNOWN`) →
  Zones (`TRIP_GENERATION_NO_ZONES`, §6.2) → Generation (§3) → Balancing (§4) → Outputs. A stage
  that refuses stops every later stage.

## 8. Outputs

- **JSON:** the marker line; `rateSet {id, name, source, unit}`; input hashes; per purpose
  `{id, factor?, totals {productions, attractionsRaw, attractions}, zones[{zoneId, productions,
  attractionsRaw, attractions}]}`, zones in id order.
- **CSV:** the marker line, one identification line, then one row per purpose and zone:
  `purpose,zoneId,productions,attractionsRaw,attractions,factor` (factor blank) and a totals row
  per purpose whose unquoted `totals` cell is distinct from any quoted zone id.
- Numbers are formatted as the Macro link outputs format theirs (JSON full precision, CSV two
  decimals); the same input gives the same bytes.
- **CLI:** `--trip-generation <rateSetId>` with `--project`, taking only `--csv` and `--data-dir`.
  It cannot be combined with `--macro`, `--macro-run`, `--meso`, `--node-analysis`,
  `--export-utdf`, seeds or the single-run diagnostics.
- **Editor (choice 7a, as built D471):** a *Trip generation* page under Results: a rate-set choice
  (owned, then catalog), *Run trip generation* on a worker with its four stages, *Trip-rate sets…*,
  and a table (purpose, zone, P, raw A, A, factor) with a totals row per purpose; Copy and Export
  use the Results actions. A run is no History step; a later document revision marks it stale and
  the rows stay.

## 9. Seams

- **M10.3 distribution** reads `P_i` and `A*_i` per purpose and the set's `unit`; it, not M10.2,
  turns them into trip tables. Its doubly-constrained gravity needs `ΣP = ΣA*`, which §4 gives.
- **M10.4 mode choice** owns person-to-vehicle conversion and the study-period factor (§5).
- **The procedure sequence** (MACRO_NETWORK §19 c) gains a Generation stage that calls the same
  run function; nothing here assumes one.
- **Gate zones and boundary flows** (choice 6b's later slice; S5): a gate zone's trip ends come
  from a cut or a count, never from rates.
- **Calibration:** fitting rates to surveys or counts is a separate milestone; a fitted set is
  still just a set.

## 10. Benchmark and sources

Read in G0 (2026-11-06, D469; [evidence](../evidence/ortuzar-ch4-trip-generation.md)), each
number from its page image:

- **Fixture with a printed answer: Example 4.1** (book pp. 143–144). 250 households at 2.5 and
  250 at 6.0 trips/day give 2125 trips/day; 500 at 6.0 give 3000. Row TG12.
- **Published coefficients, hand-chosen zones: Example 4.4's zonal form** (book p. 156),
  `0.84 H + 1.41 W + 0.75 H₁ + 3.14 H₂`. The book prints no zone result, so row TG4b states its
  zone totals and its hand result.
- **Balancing:** Eq. (4.14), book p. 157 (§4).
- **Example 4.5 (book p. 161), booked by BOOKS §3.3 before G0, is not a fixture:** it prints a
  sample's category counts and no rates or results. BOOKS is corrected.
- Hand-computed fixtures (TG3–TG7) need no book.

## 11. Owner choices (answered 2026-11-06, D469: 1a 2a 3a 4a 5a 6a 7a 8a)

The owner: "เอาตามแนะนำ 1a–8a". The options are kept for the record.

1. **Methods.**
   - **(a) Both, per purpose side (recommended).** Production by categories and attraction by
     regression is the common pairing, and Example 4.5 is category analysis.
   - (b) Cross-classification only.
   - (c) Regression only.
2. **Category counts.**
   - **(a) Ordinary zone attributes named by the set (recommended),** §6.2; no schema step.
   - (b) A new per-zone household table field.
3. **Balancing.**
   - **(a) Attractions scaled to productions per purpose (recommended),** §4.
   - (b) Both scaled to the mean of the two totals.
   - (c) No balancing in M10.2; M10.3 balances.
4. **Units.**
   - **(a) The set's unit carried, no conversion (recommended),** §5.
   - (b) A per-purpose period factor in the set now.
5. **A missing attribute.**
   - **(a) Refuse the run, listing every missing name (recommended),** §6.2. A silent zero hides a
     data error in exactly the zone a study is about.
   - (b) Read it as 0 with an advisory per zone.
6. **Which zones.**
   - **(a) Every zone, gate zones later (recommended).** A gate zone's ends are not rate-driven
     (§9).
   - (b) Add zone kinds now and skip `gate` zones.
7. **Storage and surface.**
   - **(a) Project-owned sets at the next schema, CLI, a Trip generation table and zone-attribute
     editing in the Zones table (recommended).** A study's local rates live in its file, as link
     types do, and a study enters its zone data in the editor rather than in JSON.
   - (b) Catalog sets only and the CLI; no schema change and no editor surface yet (attributes
     stay file-only).
   - **Refined by the owner for G2 (D471):** attributes in the Edit Zone dialog rather than Zones
     table columns; owned sets in a list dialog with Import/Export JSON and Remove rather than a
     form editor.
8. **Shipped rates.**
   - **(a) Ship none (recommended),** as PCU and link types: rates are local and dated, and a
     shipped number reads as a default.
   - (b) Ship one cited set the owner names (title, edition, table).

## 12. Slices

| Slice | Content | Rows |
|---|---|---|
| G0 | Read Ortúzar Ch. 4 into an evidence file; cite §3–§4. **Done (D469)** | — |
| G1 | Rate-set codec and catalog, `src/macro/trip_generation`, balancing, the run function, JSON/CSV and `--trip-generation` | TG1–TG10, TG4b, TG12 |
| G2 | Owned sets at schema 66 with the *Trip-rate sets…* dialog; zone attributes in the Edit Zone dialog; the *Trip generation* Results page and stale marking (choice 7a; D471). **Implemented** | TG11, TG13, TG14 |
| Fixture | Example 4.1 reproduced (G1's tests) | TG12 |

## 13. Acceptance rows (implemented: G1 D470, G2 D471)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| TG1 | Shipped projects with no set selected | Saved bytes, schema and every Micro, Meso and Macro output byte-identical |
| TG2 | Rate-set codec | Round-trip; every bad shape of §6.1 refused at its path; a shipped/owned id clash is `DUPLICATE_ID` |
| TG3 | Cross-classification, two zones, three categories | `P_i` equals the hand sum Σ N · t |
| TG4 | Regression, three zones | `A_i` equals `a_0 + Σ a_k x_ik` by hand; a zero zone gives 0 |
| TG4b | Example 4.4's zonal form (§10), one zone with H = 100, W = 150, H₁ = 40, H₂ = 10 | 84 + 211.5 + 30 + 31.4 = 356.9 trips |
| TG5 | Balancing | `A*` sums to `P` within 1e-12 relative; `f_p` = P / A; raw A kept |
| TG6 | Edge totals | `A = 0, P > 0` is `TRIP_BALANCE_IMPOSSIBLE`; both 0 gives no factor and zeros |
| TG7 | Negative end | A negative constant producing `T_i < 0` is `TRIP_GENERATION_NEGATIVE` at zone and side; nothing is clamped |
| TG8 | Missing attribute | `TRIP_RATE_ATTRIBUTE_MISSING` at every missing `zones[i].attributes.<name>`; the run writes no result |
| TG9 | Determinism | Same bytes twice; zones and attributes permuted in the file give the same bytes |
| TG10 | Data swap and CLI | Another set changes the output with no code edit; the marker line in JSON and CSV; unknown id is `TRIP_RATE_SET_UNKNOWN`; selector conflicts refused |
| TG11 | Owned sets | Schema-66 round-trip; no owned sets keeps old bytes; older schemas refuse the field; accept is one Undo step |
| TG12 | Ortúzar Example 4.1 (§10) | Two categories, 250 households each at 2.5 and 6.0, give 2125; one category of 500 at 6.0 gives 3000, as printed |
| TG13 | Editor | Trip generation is no History step; a table row per purpose and zone; an attribute edit marks it stale |
| TG14 | Zone attributes in the Edit Zone dialog | Add, edit and remove an attribute; each is one Undo step; a non-number, a non-finite value or a blank name keeps the dialog open and changes nothing |

## 14. As implemented (G1, D470)

- **Files:** `src/macro/trip_generation.*` (pure: `tripEnd`, `generateTrips`, index-located
  `TripIssue`s); `src/project/trip_rate_codec.*` (`parseTripRateSet`, `tripRateSetJson`,
  `resolveTripRateSets`); `src/project/trip_generation_run.*` (`runTripGeneration`, stages
  rateSet → zones → generation → balancing; inputs and outputs are the caller's, as Macro's);
  `src/project/trip_generation_output.*`; `--trip-generation` in `tools/run_simulation.cpp`.
- **Paths:** a missing attribute is `zones[i].attributes.<name>` and a negative end
  `zones[i].<purpose>.productions|attractions`, with `i` the document's zone index; an unbalanceable
  purpose is `<file>.purposes[p]`; an unknown set `tripRateSets`; shape faults
  `catalog/trip-rates/<file>.<json path>`.
- **Catalog:** `data/trip-rates/` does not exist (choice 8a), which reads as an empty catalog. The
  rows use a test-only data root, `tests/data/trip-generation/`, with Examples 4.1 and 4.4 as sets,
  and `tests/trip-generation.traffic.json` (the two-route fixture with zone attributes).
- **Rows:** `tests/trip_generation_tests.cpp` (group `tripgeneration`) and the CLI tests
  `cli-trip-generation`, `-conflict`, `-unknown`. TG11, TG13 and TG14 are G2's (below). Every row passed on
  first run; four engine mutations (inverted factor, unrefused negative, rate without count, a
  partial result kept on refusal) were each caught by at least one row.

### G2 as implemented (D471)

- **Files:** `ProjectDocument::tripRateSets`, `kLatestSchema` 66 and `parseOwnedTripRateSets` /
  `addOwnedTripRateSetsJson` / `ownedTripRateSetIssues` (`src/project/trip_rate_codec.*`, called
  from `document.cpp`); `setTripRateSets` (`src/commands/trip_rate_commands.*`);
  `editTripRateSets` (`src/shell/trip_rate_dialog.*`, file choice injected); the self-owned
  `TripGenerationPanel` (`src/shell/trip_generation_panel.*`), installed in `editor_results.cpp`
  with no `EditorWindow` member (its header is at the 500-line limit); the Edit Zone dialog in
  `editor_zones.cpp`.
- **Rows:** `tests/trip_rate_owned_tests.cpp` (group `triprateowned`: TG11 codec, command,
  owned-first lookup and the clash) and `tests/trip_generation_ui_tests.cpp` (`trip-generation-ui`:
  TG11 save, TG13, TG14, the dialog's Import/Export/Remove/Cancel).
