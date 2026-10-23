# PROGRESS log archive — 2026-10-22

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

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
