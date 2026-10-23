# PROGRESS log archive — 2026-10-22

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

## 2026-10-22 — E1: tool pointers, plain clicks after the first point, solid previews (D306)

The owner asked for three canvas changes.
- **Pointers:** no tool shows the crosshair. Select is the arrow; every other tool shows the arrow
  with its toolbar glyph on a small badge, as a move shows the hand.
- **Multi-click tools** (Zone, Node, Queue counter, Section, the two-click Connector): only the first
  point needs Ctrl+right-click; later points take a plain left click. Ctrl+right-click still works,
  and with no draft a left click still only selects (amends D84).
- **Previews are never dashed:** a solid accent line on a translucent casing, and the edges still to
  come (a Zone's closing edge, the edge to the pointer) fainter. Dashes that mean something in saved
  work stay.
- **Rows:** E1-1 to E1-4 in `zone-ui`, `node-ui`, `section-ui`, `queue-counter-ui` and
  `editor-attachments`; four mutations caught.

## 2026-10-22 — S3.3: Roads, templates and *Apply template* (D305)

- **A Road** is a centreline with a cross-section template. *Apply template* generates one Link per
  direction, at median/2 + width/2 on its driving side, the backward one reversed.
  - A re-apply keeps the Link and lane ids and reanchors Connectors.
  - Dropping a direction is refused.
- **Detaching:** a generated Link edited by hand leaves its Road.
- **Templates** are owned or installed (none ships, B12). Roads, `roadId` and owned templates make a
  file schema 53.
- **Rows:** RD1's Road part and RD5–RD9 (`roads`).
- **Amended:** the editor surface is S3.3b; `shoulder` waits for a reader.

## 2026-10-22 — S3.2: HCM link types and the capacity basis (D304)

The owner answered S3's choices as recommended (1a–5a, 6b, 7a), and the catalog came first.
- **`data/link-types/`** ships eleven HCM 7 types:
  - freeway at FFS 55–75 mi/h and multilane at 45–70 mi/h, with Exhibit 12-4's capacities;
  - jam density 190 pc/mi/ln (Exhibit 10-7).
- **`capacityBasis`** (`practical` or `ultimate`) on link types and VDFs. `bpr` is practical and
  HCM's types are ultimate, so Macro refuses the pair (`MACRO_CAPACITY_BASIS_MISMATCH`) while Meso
  runs them. An owned type with a basis is schema 53.
- **The Link types tab** shows the basis.
- **Rows:** RD1 (basis), RD2 and RD3 (`roadcatalog`).
- **Amended:** `rank` moves to S3.6, where its reader lands.

## 2026-10-22 — S3.1: the Roads contract (D303, docs only)

The owner chose S3 next, with values from HCM 7. [ROADS](../reference/ROADS.md) fixes the following:
- **The Road:** a centreline with a template, whose generated Links stay ordinary Links with
  `roadId`, so Micro is unchanged.
- **Editing:** *Apply template*, detaching, *Join opposite Links* and *Assign template*.
- **Nodes:** *Find crossings* with junction or overpass, and *Merge nodes*.
- **S0.4:** coarse Node values that Meso and Macro read.
- **Seam checks** by level.
- **HCM values**, read in the owner's copy: freeway and multilane capacities (Exhibits 12-4, 12-6),
  saturation flow (19-11) and jam density 190 pc/mi/ln (10-7). BPR pairs with practical capacity,
  so a capacity basis is proposed: HCM types shipped as `ultimate`, which Macro refuses with BPR.
- Seven owner choices (B24) and rows RD1–RD22. Nothing is built yet.

## 2026-10-22 — M11.6a: the Meso benchmark option sheet (D302, docs only)

The owner chose to list published options before choosing a Meso benchmark, by the M6.0 rules: no
engine numbers and no tolerance.
- **[meso-benchmark-options](../evidence/meso-benchmark-options.md)** has two kinds: M1, a corridor
  queue and its shockwave; and M2, network spill-back. Each has four options.
- **Disclosed:** Yperman's Example 4.5 and §7.1, and Tampère's Table 8, are not blind. They already
  have engine numbers, and our formulas came from them.
- **Recommended:** Boyles et al. (2025) for both kinds, which is open access but unreachable from
  this environment, with Daganzo (1994, 1995) secondary. The owner answers OWNER_SITTING B23.

## 2026-10-22 — M11.4b: a turn's free-flow time in Meso (D301)

The turn free time that the compile recorded was not loaded, so a Node's interior took no time.
Yperman's nodes have no length, so the owner chose a design of ours:
- **The lag:** the node model decides at t as before; a vehicle enters the out-link τ later.
- **Storage:** until then it counts against the out-link's storage (the receiving volume reads the
  admitted count), so spill-back holds. No step limit or turn capacity is added; τ = 0 is unchanged.
- **Outputs:** turn rows carry `freeTime`; VHT includes node time with delay unchanged;
  `inNodesAtEnd` is new.
- **Rows:** ME28 (`mesoloading`) and ME10 extended. Four mutations are caught.

## 2026-10-22 — M11.4: Meso outputs, `--meso` and Run Meso (D300)

The owner chose the whole Micro run as Meso's horizon, with the same demand, so the CLI is `--meso`.
- **`runMeso`** feeds JSON and CSV outputs by slice, in PCU: link flows, density, travel time from the
  curves, queue and spill-back; turn transfers; VKT, VHT and delay; and vehicles per class.
- **The editor** has Run Meso and Cancel, the Meso links tab with the marker and a stale notice, and
  Export.
- **Rows:** ME10 (`mesooutput`), ME11 (`meso-ui`), and `cli-meso*`. Five mutations are caught.
- **Docs:** the as-implemented sections moved to MESO_NETWORK_IMPLEMENTED for the 500-line cap.
- **Known gap:** turn free-flow times are not loaded yet.

## 2026-10-22 — M11.3d: signals in Meso as averaged green (D299)

The owner chose averaged green (4b) over explicit green, which under FIFO would hold a whole approach
on any red turn.
- **Turns** take the program their heads show and its g/C. g is the displayed green, by HCM Eq. 19-1
  with its default lost times.
- **The node model** takes Tampère §5.2's internal supplies, with Yperman's shared-lane split. Under
  CTF they act as an effective capacity per in-link. A literal per-movement reading broke `q_j ≤ R_j`
  on generated cases and was replaced.
- **Rows:** ME7, ME26 and ME27. Five mutations are caught. `MESO_SIGNAL_IGNORED` is retired;
  `MESO_SIGNAL_AMBIGUOUS` is new.
