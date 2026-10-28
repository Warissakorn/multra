# Junction builder and the HCM study workflow (M14, D386)

Contract before code (D145), docs only, written 2026-10-28. The owner asked for a Network Editor
that builds a road network for HCM analysis first, and exports it to PTV Vistro 2026 and Synchro 12
(D386: scope approved, "อนุมัติ"). The export is [UTDF_EXPORT](UTDF_EXPORT.md); this page is the
editor side: the **junction builder**, the lane layout it derives, NEMA numbering of signal groups,
and the analysis panel. **Nothing here is implemented.** The owner choices (§11) are open.

Rule 4 applies: every analysis result keeps the not-validated marker. Building a junction quickly
is not calibration.

## 1. Why a builder

Drawing a four-leg signalised junction for HCM today takes six manual steps
([NETWORK_EDITOR](NETWORK_EDITOR.md)): Links and opposite carriageways, *Split + add downstream
lane* per pocket, one Connector per turn and lane, a controller and a head per lane, a Node, and
analysis inputs that only the file or `--node-analysis` reach. Synchro and Vistro do the same in
a lane table. The shortcut that exists, a Road with a template, *Find crossings*, *Coarse
values…* and F4 ([READINESS](READINESS.md) §13), fails HCM in two places:
- **Lanes:** F4 shares the in-leg's lanes kerb to far (§13.3) and draws no pocket. NODE_ANALYSIS
  lane groups need the lanes a study specifies.
- **Templates:** none ships (`data/cross-sections/` does not exist; OWNER_SITTING B12).

ROADMAP M1 says "junctions are not something the user places". The builder keeps that: it places
**no new object kind**. It writes ordinary Links, Connectors, a Node, right-of-way and signal
objects, all `generated` (READINESS §13.6), which every existing tool then edits (D386).

## 2. What it reuses

| Need | Existing piece |
|---|---|
| Leg cut at the stop line | `nodeLegCuts` (READINESS §13.2, F4-1a) |
| Connector geometry | `connectorEquation`'s default curve, as F4 |
| Priority between generated Connectors | HCM 7 Ch. 20 ranks and Stop on minor arms (§13.4, F4-3a) |
| Controller, groups, heads | `SignalController{cycle, offset, groups}`, `SignalGroup{number, greenStart, greenEnd, amber}`, heads with `controllerId`/`groupNumber` (D359, D360, §13.5) |
| Pockets | `splitLink` + add lane (median side only today) |
| Opposite direction and Road | `oppositeLink`, `joinOppositeLinks` (ROADS_IMPLEMENTED §17) |
| Lane groups and HCM | `src/analysis/` (NODE_ANALYSIS A1–A3), inputs in `Node::analysis` (schema 58) |
| One Undo step, `generated`, settle | `History::execute`, `settleGenerated`, `settleNodes` |

## 3. The builder dialog (tool **Junction**, key `J`)

Ctrl+right-click a centre point on the canvas (D84), or *Build junction…* on a Node row. One
dialog, three tabs; *Create* is one Command and one Undo step.

**Legs** (3 or 4 in the first slices; §11 JB-2):

| Field | Meaning | Default |
|---|---|---|
| Bearing | Direction from the centre to the leg's far end, degrees clockwise from north | 0/90/180/270, or 0/90/270 |
| Name | Street name, written to the Road and both Links' `name` | empty |
| Length | Drawn approach length from the centre, m | §11 JB-4 |
| Lane width | One width for the leg's lanes, m | §11 JB-5 |
| Out lanes | Lanes leaving the junction on this leg | the in-leg's through lanes |
| Posted speed | km/h, written to the Links (J1) | empty: refused for UTDF, allowed for HCM |

**Lanes** — one row per in-lane, **far side to kerb** (`drivingSide` decides which side is far):

| Field | Meaning |
|---|---|
| Movements | A contiguous subset of *far turn · through · kerb turn*: `F`, `T`, `K`, `FT`, `TK`, `FTK` |
| Storage | For an exclusive turn lane: pocket length, m; empty means a full lane |
| Taper | Pocket taper length, m; drawn as the pocket's first metres, exported to UTDF |

The table shows the geometric name beside each row (L/T/R), so "far turn" is *right* in Thailand
and *left* in the United States. Refused before *Create*: a movement whose out-leg does not exist,
a lane serving no movement, non-contiguous movements on one lane, crossing lane assignments (a
lane's movements must not lie farther than those of a lane on its far side), and a pocket longer
than the approach.

**Control:**
- **Signalised, pretimed:** a phasing preset (§6), cycle, and per phase green, yellow and all-red.
- **Two-way stop:** the major legs (two), Stop on the others (§13.4's ranks).
- **All-way stop:** geometry and Stop on every leg; Micro refuses it as F4 does
  (`FILL_NODE_ALL_WAY_STOP`), HCM waits for A5, UTDF exports the control type (§11 JB-6).
- Roundabouts are out (NODE_ANALYSIS A7).

## 4. What *Create* writes

All in one Command, every object `generated`:
1. **Per leg:** an in Link and an out Link along the bearing, joined into one Road with no template
   (`joinOppositeLinks`), named; lanes and widths from the tables.
2. **Pockets:** the in Link is split at storage + taper from its stop line; the downstream piece
   gains the lane on the far side (today's tool) or the **kerb side (new, J2)**. Thailand's
   exclusive left-turn lanes are kerb-side, so J2 is required before the builder.
3. **A Node:** a `generated` polygon around the cuts, as *Find crossings* draws it.
4. **Cuts** by `nodeLegCuts`.
5. **Connectors:** one per (lane row, movement), from that lane to the out-leg lanes; the far turn
   lands on the out-leg's far lanes and the others from the kerb (§13.3's out-end rule). Lane rows
   with the same movement and adjacent lanes make one multi-lane Connector.
6. **Right-of-way:** §13.4's areas and ranks; Stop on the minor arms for two-way stop.
7. **Signals:** one controller named after the Node, groups **numbered by NEMA phase** (§6), a head
   at the start of every Connector path, carrying its group.

Undo restores every byte. A second *Create* at the same place is refused while any `generated`
object of the first remains (`BUILD_JUNCTION_OCCUPIED`).

## 5. The lane layout, derived and never stored (D272)

`approachLayout(network, node)` (pure, `src/model/network/`) reads, for each in-leg:
- each lane's movements, from the Connectors leaving it;
- each exclusive lane's storage, the length from its pocket's start to the stop line;
- each in-leg's bearing, from its last segment before the cut.

Three readers share it, so none keeps a copy (rule 3): the builder opened on an existing Node
(§11 JB-3), NODE_ANALYSIS's lane groups (whose derivation it replaces only if byte-identical on
NA1–NA23), and UTDF's Lanes table.

**Movement names.** An approach's direction is the bearing of travel on its in-leg, nearest of
N/E/S/W; a Node where two in-legs take one direction is `JUNCTION_DIRECTION_AMBIGUOUS`, fixed by a
per-Node override (§11 JB-2). Turns are L/T/R by the turn angle, U-turns refused.

## 6. NEMA numbering of signal groups

UTDF and both target programs work in NEMA phases 1–8 (two rings, a barrier). MULTRA keeps its
model, a controller with groups (green start/end, amber): **a group's number is its NEMA phase**.
The builder writes them; for a hand-made controller the user renumbers groups in the existing
dialog. `nemaCheck(controller, layout)` (pure) states whether the numbering is consistent:
- each lane's heads carry one protected group, and at most one permitted group;
- groups 1–4 are ring 1 and 5–8 ring 2, each ring in order within the cycle;
- both rings cross the barrier at the same instant (between 1–2/5–6 and 3–4/7–8);
- all-red per group is the gap from its amber's end to the next group of its ring.

Failures are named (`NEMA_*`) and reach the UTDF export as refusals; Micro and HCM do not read them.

**Presets** (right-hand numbering; for left-hand traffic the far turn takes the left-turn numbers):

| Preset | Ring 1 | Ring 2 | Far turns |
|---|---|---|---|
| P1 Two-phase | 2 \| 4 | 6 \| 8 | permitted, with the opposing through |
| P2 Protected far turns on the major | 1, 2 \| 4 | 5, 6 \| 8 | major leading protected; minor permitted |
| P3 Protected far turns, all legs | 1, 2 \| 3, 4 | 5, 6 \| 7, 8 | protected leading |
| P4 Split phasing on the minor | 2 \| 4, 8 sequential | 6 \| — | each minor approach alone |

Phases 2 and 6 serve the major (bearings of the first two legs, editable), 4 and 8 the minor. P1–P3
are the NEMA convention as Synchro's quick setup names them; U0 confirms the numbers both programs
read (UTDF_EXPORT §6).

## 7. Posted speed on the Link (J1)

Booked by D379, confirmed by D380, shaped by D381: one optional `speedLimit` (km/h) on the Link,
Micro reads it as a default and never a cap. J1 implements it, and NODE_ANALYSIS's approach
`speedLimit` becomes derived from the in-leg (D379). UTDF's Links and Lanes need it.

## 8. Analysis panel (J5; NODE_ANALYSIS A9's first half)

The Nodes tab gains *Analysis…*: per turn hourly volume and %HV (named by direction and L/T/R
from §5), PHF, analysis period, CBD, base saturation flow with its source, and per approach
grade, parking manoeuvres and buses stopping. Confirm writes `Node::analysis` in one Command.
*Analyse* runs `src/analysis/` on the UI thread (a pure function of the document) and fills
Results → *Node analysis*: per lane group v, s, c, X, g/C, d, LOS, back of queue; per approach and
intersection d and LOS; refusals with their codes, which then need locale strings (D383). CSV
through `csv_format.hpp` with the marker line.

## 9. Slices

| Slice | Scope | Sessions |
|---|---|---|
| U0 | UTDF probe with the owner's Synchro 12 and Vistro 2026 (UTDF_EXPORT §8) | 1 + owner |
| J1 | Posted speed on the Link (§7) | 1 |
| J2 | Kerb-side pocket; `approachLayout` with direction names (§5) | 1 |
| J3 | Builder: legs, lanes, pockets, Node, Connectors, two-way stop (§3–§4) | 1–2 |
| J4 | Builder signals: presets, NEMA numbering, `nemaCheck` (§6) | 1 |
| J5 | Analysis panel and results table (§8) | 1–2 |
| U1–U3 | UTDF writer and round trip (UTDF_EXPORT §7) | 3–4 |

Order: U0 first, because its answers can change J4 and U1. Total about 9–12 sessions.

## 10. What stays out

Roundabouts, actuated control and coordination (A7, A8); channelised free kerb turns (slip lanes)
and bus bays, each a later slice when asked; five or more legs; pedestrians (ROADMAP "Later").

## 11. Owner choices (open)

1. **JB-1 Where the builder lives.**
   - **(a) A canvas tool `J` and *Build junction…* on a Node row (recommended).**
   - (b) Only a menu command at a typed coordinate.
2. **JB-2 Legs.**
   - **(a) 3 or 4 legs, directions by nearest compass point with a per-Node override when
     ambiguous (recommended).** UTDF's Lanes table is by direction.
   - (b) Up to 8 legs with diagonal directions now.
3. **JB-3 Editing after *Create*.**
   - **(a) Reopening on a Node shows the derived layout (§5); *Apply* replaces only objects still
     `generated`, and refuses when a person has edited one (recommended).**
   - (b) Create only; later edits with the ordinary tools.
4. **JB-4 Approach length.**
   - **(a) 150 m, labelled a drawing default with no traffic meaning, editable (recommended).**
     HCM's queue storage ratio needs the approach long enough for the back of queue.
   - (b) No default: required.
5. **JB-5 Lane width default.**
   - **(a) 3.6576 m = 12 ft, HCM 7's default lane width, editable (recommended)**, until a Thai
     template is chosen (B12). J3 reads the value and its exhibit from the owner's copy (BOOKS)
     before citing it; Exhibit 19-11, already cited for saturation flow, is the likely source.
   - (b) Required, no default.
6. **JB-6 All-way stop.**
   - **(a) Built as geometry and Stop controls; Micro refuses it, UTDF exports it (recommended).**
   - (b) Not offered until Micro and HCM A5 support it.
7. **JB-7 Yellow and all-red.**
   - **(a) Required inputs with no default (recommended).** No cited default exists in the project;
     amber's 3 s group default stays for hand-made groups.
   - (b) 3 s yellow and 1 s all-red as editable defaults, recorded as unsourced.

## 12. Acceptance rows (planned)

| Row | Slice | Asserts |
|---|---|---|
| JB1 | J1 | A Link's `speedLimit` round-trips; absent keeps the bytes; Node analysis reads the in-leg's |
| JB2 | J2 | A kerb-side pocket adds the lane on the kerb side for both `drivingSide`s; routes and heads survive |
| JB3 | J2 | `approachLayout` names a four-leg's 12 movements by direction and L/T/R; an ambiguous pair is refused |
| JB4 | J3 | *Create* on the Thai four-leg fixture (kerb pockets, shared TR) gives the lane table back from `approachLayout` exactly |
| JB5 | J3 | Every invalid lane table in §3 is refused with its code and changes nothing |
| JB6 | J3 | Undo restores every byte; a second *Create* there is `BUILD_JUNCTION_OCCUPIED` |
| JB7 | J3 | The built junction runs in Micro (seed 42) with no refusal and NODE_ANALYSIS finds every lane group |
| JB8 | J4 | Each preset's controller passes `nemaCheck`; breaking the barrier, a ring order or a head's group is named |
| JB9 | J4 | A pretimed built junction's HCM g/C per lane group equals the preset's greens over the cycle |
| JB10 | J5 | The panel writes `Node::analysis` in one Undo step; *Analyse* matches `--node-analysis` byte for byte |
