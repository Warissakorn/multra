# Junction templates

Structural recipes for M14, [JUNCTION_TEMPLATES §3](../../docs/reference/JUNCTION_TEMPLATES.md#3-the-junction-template-data).
Each JSON file is a catalog entry; the loader reads filenames in order. Project-owned
`definition.junctionTemplates` replaces this whole catalog, including when it is empty.

- Four legs, signalised: P1, P2, P3 and P4, each with shared turn lanes or major far-turn pockets.
- Three and four legs, two-way stop: each with shared turn lanes or major far-turn pockets.
- Three-leg roles are major-far, major-kerb and minor; four-leg roles are major and minor.

P2 separates the major far turn; P3 separates every far turn, as required by the one-protected-group
per lane rule (D396). A non-pocket F row is full-width; TK remains shared.

Rows run far to kerb and use F (far turn), T (through), K (kerb turn), independent of driving side.
Every file describes its own structure. No storage, taper or timing number ships; those are site
values on the Node. The phasing preset selects structure, never a cycle or green time. All-way
stop is accepted for owned templates; its fill remains listed but disabled until A5 or U1 (D399).
J3b generates junction geometry; J4 applies the required site timing to the four-leg signal
presets (D404). Transitions and their catalog structure are J5; the editor is J6.
