Analytical junction capacity for M13 ([contract](../../docs/reference/NODE_ANALYSIS.md), D370, D371). Pure and Qt-free: it imports nothing outside this folder, reads no clock and draws no random number.

`signalised.*` is HCM 7th edition Chapter 19's core motorized vehicle methodology for one lane group under pretimed, protected operation in exclusive lanes with no initial queue (slice A1):
- the saturation flow factors of Eq. 19-8;
- capacity and v/c (Eqs. 19-16, 19-17);
- uniform delay with the progression factor (Eqs. 19-19 to 19-21);
- incremental delay (Eqs. 19-26, 19-27);
- the flow-weighted approach and intersection delay (Eqs. 19-28, 19-29);
- LOS by Exhibit 19-8's bounds.

Every formula cites its PDF page in the owner's copy ([BOOKS](../../docs/reference/BOOKS.md) §2).

`src/model/demand/node_analysis_compile.*` derives the lane groups from a Node's Micro detail and its signal programs. `src/project/node_analysis_run.*` analyses every Node of a document. These results are analytical, not simulated, and not validated (rule 4).
