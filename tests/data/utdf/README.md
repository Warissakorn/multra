# UTDF files (M14)

Data for the UTDF export ([UTDF_EXPORT](../../../docs/reference/UTDF_EXPORT.md)). Kept byte for byte
(CRLF, no BOM, as Synchro writes), so `.gitattributes` turns off line-ending conversion here.

| File | What it is |
|---|---|
| `u0-handwritten.csv` | What MULTRA would write for the U0 reference junction ([UTDF_PROBE](../../../docs/plans/UTDF_PROBE.md)), written by hand in U0 (2026-10-28). MULTRA-authored, GPL like the rest. Imported into Vistro 2026 on 2026-11-03: geometry, lanes and volumes came through, timing was refused (UTDF_PROBE §6 row 6) |
| `u0-handwritten-v2-empty.csv` | The same junction, with `[Timeplans]` and `[Phases]` carrying every record Synchro 10.1 writes, in its order; records MULTRA does not author are empty. Probes whether Vistro needs those rows present (UTDF_PROBE §6 row 13) |
| `u0-handwritten-v2-filled.csv` | As v2-empty, but the unauthored records get neutral pretimed values (VehExt 0, MinSplit = split, Yield170/Local* = Yield/Start, ActGreen = green). **Probe-only**: it tells whether Vistro needs values, not just rows; it is not what the writer will emit (UT4) |

The owner's Vistro 2026 export of the same junction (`u0-vistro.csv`, the golden file for UT0;
Vistro since D438, as Synchro cannot create UTDF) comes here only if the owner agrees that it may
be redistributed; otherwise only its field names and units are recorded in UTDF_EXPORT §3.
