# UTDF files (M14)

Data for the UTDF export ([UTDF_EXPORT](../../../docs/reference/UTDF_EXPORT.md)). Kept byte for byte
(CRLF, no BOM, as Synchro writes), so `.gitattributes` turns off line-ending conversion here.

| File | What it is |
|---|---|
| `u0-handwritten.csv` | What MULTRA would write for the U0 reference junction ([UTDF_PROBE](../../../docs/plans/UTDF_PROBE.md)), written by hand in U0 (2026-10-28). MULTRA-authored, GPL like the rest. Not yet imported by either program |

The owner's Synchro 12 export of the same junction (the golden file, UT0) comes here only if the
owner agrees that it may be redistributed; otherwise only its field names and units are recorded
in UTDF_EXPORT §3.
