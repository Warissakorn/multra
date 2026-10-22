# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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

The owner chose S3 next, with values from HCM 7. [ROADS](reference/ROADS.md) fixes the following:
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
- **[meso-benchmark-options](evidence/meso-benchmark-options.md)** has two kinds: M1, a corridor
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

## 2026-10-22 — M11.3c: Yperman's link model and the loading engine (D298)

The owner supplied Yperman's thesis as a Markdown repository, and its chapters 3–5 and §7.1 were read.
- **Confirmed:** §4's sending and receiving volumes, the storage, the origin queue and the
  destination. The thesis prints only `L/v` as the step limit; `L/w` is added, derived.
- **Owner choices:**
  - turn splits follow the vehicles' routes (Eq. 5.2), replacing shares at time t;
  - signals move to M11.3d, because explicit green under FIFO holds the whole in-link. Signalised
    Nodes are advised `MESO_SIGNAL_IGNORED`.
- **`loadMeso`** joins the link model and `mesoNodeFlows` per step, with spill-back and the
  boundary volumes.
- **Against the thesis:** Example 4.5 is exact. §7.1's diverge network gives `k_B` = 166.67 veh/km,
  r2m clear at 44 min (thesis 46) and r1n queued 26.25–82.75 min (thesis 28–82).
- **Rows:** ME3–ME6, ME8, ME9 and ME24–ME26 in `mesoloading`. Six mutations are caught.

## 2026-10-22 — M11.3's node model from Tampère et al. (2011) (D297)

The owner uploaded the paper, and it was read in full.
- **`mesoNodeFlows`** is its §5.1 instance and algorithm: oriented capacity proportional, exact,
  and unique. It reproduces the paper's Table 8.
- **Changed from the contract:** the paper has no instance of conflict constraints (its Eq. 17 is
  provisional). By the owner's choice, priority inside a Node is advised `MESO_PRIORITY_IGNORED`
  instead of guessed.
- **Signals** are explicit green, as the paper's §5.2 allows.
- **Rows:** ME20–ME23 in `mesonode`. Three mutations are caught; two had no effect, and both are
  explained.
- **Still blocked:** the link model needs Yperman (B22).

## 2026-10-22 — M11.3 blocked on its sources; nothing built

The owner asked for M11.3. The contract makes checking §4 against Yperman (2007) and §5 against
Tampère et al. (2011) the first step, but the environment refused every host that holds them
(proxy 403: arxiv.org, lirias.kuleuven.be, ideas.repec.org). The owner chose to wait for the sources,
so no engine code was written. OWNER_SITTING B22 asks for the hosts or the PDFs.

## 2026-10-22 — M11.3b: routeless inputs and placed decisions as Meso paths (D296)

The owner asked for M11.3b. Meso now takes Micro's own expansion of routeless inputs and placed
decisions: `expandRouteless` after `resolveCatalogs`, so lane splits, slice shares and typed routing
are not copied.
- **Lanes to objects:** `mesoObjectChain` maps lane sections and connector paths back to Links and
  Connectors, then `mesoPath` as in M11.3a.
- **Stubs:** a stub's flow goes to the full routes its vehicles change lanes onto, because the trip
  does not end there.
- **Type parts:** a composition's type parts sum back into its class.
- **The M2.6 template** (four placed decisions) now loads its whole demand, conserved over time.
- **Rows:** ME17–ME19 in `mesodemand`. ME13 now checks the walk's own issues instead of the old
  refusal. Four mutations are caught.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
