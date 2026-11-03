# UTDF files (M14)

Data for the UTDF export ([UTDF_EXPORT](../../../docs/reference/UTDF_EXPORT.md)). Kept byte for byte
(CRLF, no BOM, as Synchro writes), so `.gitattributes` turns off line-ending conversion here.

| File | What it is |
|---|---|
| `u0-handwritten.csv` | What MULTRA would write for the U0 reference junction ([UTDF_PROBE](../../../docs/plans/UTDF_PROBE.md)), written by hand in U0 (2026-10-28). MULTRA-authored, GPL like the rest. Imported into Vistro 2026 on 2026-11-03: geometry, lanes and volumes came through, timing was refused (UTDF_PROBE §6 row 6) |
| `u0-handwritten-v2-empty.csv` | The same junction, with `[Timeplans]` and `[Phases]` carrying every record Synchro 10.1 writes, in its order; records MULTRA does not author are empty. Probes whether Vistro needs those rows present (UTDF_PROBE §6 row 13). **Refused by Vistro 2026 (2026-11-03): empty cells are invalid** |
| `u0-handwritten-v2-filled.csv` | As v2-empty, but the unauthored records get neutral pretimed values (VehExt 0, MinSplit = split, Yield170/Local* = Yield/Start, ActGreen = green). **Imported by Vistro 2026 with the timing exact (2026-11-03).** Its values are what the writer emits for unauthored records (D439) |
| `u0-vistro-draft1.csv` | The owner's first Vistro 2026 export of the U0 junction (metric and left-hand on; *File → Export → Synchro…*), committed with permission on 2026-11-03. **Draft, not the golden file:** cycle 60 with phases 2/4/6/8, PHF 1.00, HV 2 %, and NB/SB lanes differ from the sheet. Kept for Vistro's dialect: written in feet, far turn under `EBR` |
| `u0-vistro-draft2.csv` | The owner's second Vistro 2026 export of the U0 junction (2026-11-03). Matches the sheet's lanes, pockets, names, volumes, PHF 0.92 and HV 5 %. **Draft, not the golden file:** the timing is still 60 s with phases 2/4/6/8. Shows Vistro writes pocket storage on the through column and `[Links] Lanes` without pockets |
| `u0-handwritten-v3.csv` | `u0-handwritten-v2-filled.csv` with the junction legs' `[Links] Lanes` set to 3 (full lanes plus the pocket, D440). Probes how Vistro reads it (UTDF_PROBE §6 row 17). **Imported by Vistro 2026 with the pockets as pockets and the same result as v2-filled (2026-11-03)** |

`u0-vistro.csv` is the owner's Vistro 2026 export of the U0 junction (metric and left-hand on), the
golden file for UT0 (D438), committed with permission on 2026-11-03. It matches the sheet's
geometry and volumes, with a 100 s cycle and protected-only far turns (EBR phase 3, WBR phase 7).
Its phase numbers and splits are Vistro's own, not UTDF_PROBE §3's.