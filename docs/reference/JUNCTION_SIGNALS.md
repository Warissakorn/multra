# Junction template signals and NEMA numbering (M14 J4/J4T)

B32 answered as recommended in all three choices (D404). J4 implements the shipped four-leg
P1–P4 set (§3). **A group's number is its NEMA
phase**; existing controllers/groups/heads remain the only stored signal model. The pure
`nemaCheck(controller, layout)` reports `NEMA_*` failures for a derived network/layout view.
Hand-made controllers keep running in Micro/HCM independently of that export check.

- Ring 1 is 1,2 | 3,4 and ring 2 is 5,6 | 7,8, in cyclic order. The barriers separate those
  two pairs, after 2/6 and after 4/8; both rings must clear a half before the other half opens.
- Every used phase requires the Node's green, yellow and allRed, with no defaults (D395).
  **allRed is entered clearance.** Red waiting for a barrier or P4's other approach is derived
  separately from the other phases; it does not replace or inflate the entered clearance.
- Each lane may carry only one protected group and at most one permitted group. A permitted
  turn uses its approach's through group and the existing yielding Conflict areas. Operation
  remains derived from actual green/conflict overlap (NODE_ANALYSIS_A3), never stored.
  Protected far turns use the odd group immediately before the opposing through group;
  D396's separate lanes are required for P2 major / P3 all approaches.

| Preset | Ring 1 | Ring 2 | Far turns |
|---|---|---|---|
| P1 Two-phase | 2 \| 4 | 6 \| 8 | permitted with opposing through |
| P2 Protected far turns on the major | 1,2 \| 4 | 5,6 \| 8 | major leading protected; minor permitted |
| P3 Protected far turns, all legs | 1,2 \| 3,4 | 5,6 \| 7,8 | protected leading |
| P4 Split phasing on the minor | 2 \| 4, then wait | 6 \| wait, then 8 | major permitted; each minor approach alone |

The vertical bars within the ring cells are barriers.

**Numbering and anchor (D404).** For major E–W: 2 WB, 6 EB, 4 SB, 8 NB. For major N–S,
rotate the whole mapping: 2 NB, 6 SB, 4 WB, 8 EB. The major pair must be opposite compass
approaches with reciprocal geometric through movements; direction overrides label the layout
and cannot create physical opposition (§6). Far means right in left-hand traffic and
left in right-hand traffic, with the same odd phase numbers. **2 and 6 start at cycle time 0**;
leading 1/5 run at the end of the preceding cycle. The controller offset is zero by this
reference convention, not a site timing default.

**Timing construction (D404).** Let S_n = entered green + yellow + allRed for phase n, with
an absent preset phase contributing zero to these structural expressions only. The through
half lasts T = max(S_2,S_6). The minor half lasts M = max(S_4,S_8) for P1/P2,
max(S_3+S_4,S_7+S_8) for P3, and S_4+S_8 for P4. The leading tail lasts
L = max(S_1,S_5). **Require cycle = T+M+L**, refusing a mismatch rather than assigning spare time.
2/6 start at 0; 1/5 start at cycle−S_1/S_5 so both have completed clearance at 0.
P1/P2 minor greens start at T. P3 starts 3/7 at T and 4/8 after its ring's 3/7 slot.
P4 starts 4 at T, then 8 at T+S_4; the other ring waits red. Green lengths and yellows stay
exactly the entered values. The difference between clearance completion and the next green
is derived waiting. All numbers must lie on the project's time-step grid for a fill.

U0's answer table remains empty: internal NEMA checks do not establish Synchro/Vistro
interchange or left-hand support there. UTDF_EXPORT §6 still waits on the probe; no writer
is included in J4.

## Signalised T (J4T, B33/D405 and B37/D416)

Owner selected one installed `signal-3-protected` entry: `legs=3`, `control=signalised`,
`phasing=P2`. Incoming rows run far to kerb: major-far F | T, major-kerb T | K,
minor F | K, all full-width. Owned/per-leg pocket overrides retain required site lengths.
No new phasing enum, stored field or schema is introduced.

Keep 2/6 for the reciprocal major through pair. The physical major far turn uses 1 or 5
(the odd phase before the opposing through); the existing minor uses 4 or 8 for both turns.
The literal compass checks (stem is the arm pointing away from the Node):

| Major axis | Stem | Minor group | Left-hand protected far | Right-hand protected far |
|---|---|---|---|---|
| E–W | North | SB = 4 | WBR = 5 | EBL = 1 |
| E–W | South | NB = 8 | EBR = 1 | WBL = 5 |
| N–S | East | WB = 4 | NBR = 5 | SBL = 1 |
| N–S | West | EB = 8 | SBR = 1 | NBL = 5 |

Other phases are absent, never renumbered or given entered zero timing. This is the sole
T preset in this slice; other three-leg phasing presets refuse `NEMA_PRESET` on Fill.
Compass labels and physical turn angles retain their separate roles; overrides cannot make
adjacent major arms oppose. Both numbered rings remain, including a ring with only one phase.

Let T=max(S2,S6), M=S4 or S8 for the existing minor, L=S1 or S5 for the physical major far.
Require cycle=T+M+L. Major through/kerb movements start at zero; minor alone starts at T;
major far alone starts at cycle-L and completes entered yellow/allRed at zero. A ring with
no movement in a stage waits red. Entered allRed remains clearance; barrier waiting is derived.
Every used phase still requires green/yellow/allRed, with cycle/grid/applicability refusals.
Missing and unused phase values remain editable draft values, never silently filled or dropped.
The Node form lists the same derived required numbers as Fill, including incomplete timing.

## T template cut measurement (JT19, B37/D416)

For any three-leg junction template, including two-way stop, extend the stem's inner endpoints
only in the full-width measuring copy until they reach the far major pavement edge, projected
along the stem's terminal tangent. Major painted boundary endpoints determine that extent;
there is no cut-distance number. Run the existing surface-overlap cuts on that copy.
Map cut stations back to the actual Link before checking references. Stored cut geometry must
remain on the original Link: no measuring extension is published. Street segment centrelines
and ordinary F4/nodeLegCuts remain unchanged; B38's polygon fit (§5) is separate. Existing outside/reference
refusals and generated/refine/one-Undo rules still apply. This replaces D400's zero-gap
limitation for T template fills, with no change to legacy F3/F4 or their frozen JT13 bytes.

## Acceptance

- **JT18:** installed structural entry without site defaults; both driving sides and four stem
  directions, literal U0 compass bindings, four used groups, exact site timing and single-phase
  ring waiting; missing/extra/grid/cycle/protected-lane guards, NEMA corruptions, drafts,
  generated replacement/reference guards, save/reopen and full JSON Undo/Redo. HCM reads all
  six drawn movements' greens and Micro runs seed 42. Node UI shows only the four required
  phases on both sides/locales. **Passing.**
- **JT19:** orthogonal wide-median T, signalised and two-way stop, both driving sides; extended
  measuring copy only, retained base footprints/references, unchanged preview/Street segments,
  generated flags, repeat Fill and exact Undo/Redo; legacy JT13 unchanged. **Passing.**

J4T is implemented and awaits owner review. Native junctionsignals has 40 passing cases
(nine JT9, seven JT18, three JT19, 21 JT20); junctionfill has 25 including the updated JT19
case and frozen JT13. Catalog/recipe Qt tests pass on both driving sides/locales, including
later first choice and real Fill for all three template families. U0 has no answers; no writer
or Synchro/Vistro interoperability claim is included.

## Natural polygon workflow (JT20, B38/D417)

The owner extended the [§5 polygon-fit policy](JUNCTION_TEMPLATES.md#junction-polygon-fit-b38d417)
to every junction template, including four legs. JT20 uses untouched Find crossings polygons
for stop-4-shared, stop-4-major-far-pockets and signal-3-protected on both driving sides,
through the crossing column and later Node form. First template choice retains the current
flag; false flags use explicit previewed Fit polygon + Fill and are never restored to true.
Capture/overlap/refinement/reference guards, saved-state preservation and one Undo remain.
JT20 is **passing**, including automatic/explicit fitting, no flag resurrection, exact Undo/Redo,
save/reopen and remaining Connector refusals with Node/Connector paths.
