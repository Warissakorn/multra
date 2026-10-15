# VISSIM PARITY — 2026-09-14 sections

§1, §3 and §6 of [`VISSIM_PARITY.md`](../audits/VISSIM_PARITY.md), moved verbatim 2026-10-15.
They are the pre-completion assessment; their "today" columns describe an editor that no longer exists.

## 1. Working posture — what the hands do

This is the part that decides whether an engineer feels at home in the first ten minutes, and
it is where the editor diverges most.

**Source note.** The Vissim column below is the owner's reference material (2026-09-14),
not the PTV manual. Rows it states explicitly are marked ✔; rows carried over from the first
draft of this review and *not* confirmed by it are marked **?** and must be checked against
Vissim itself before M1.9 designs against them. An earlier revision of this file asserted
plain right-drag for creation and Ctrl+right-drag for panning — **that was backwards**, and it
invalidated the conclusion drawn from it. Do not restore it.

| | Vissim | Today | Gap |
|---|---|---|---|
| Choosing what you are about to create | Click a type in the **Network Objects** sidebar; the sidebar is the mode, and it stays visible ✔ | A `QComboBox` of six tools: `select, draw, split, measure, calibrate, connect` (`src/editor/canvas.hpp:10`, wired at `src/shell/editor_window.cpp:57-59`) | The current mode is a collapsed dropdown showing one line of text. Vissim's is a permanent list — you can see every object type you *could* be placing |
| Creating a link | **`Ctrl` + right-drag** from start to end on empty space, then a **Link Data** dialog for lane count and widths ✔ | Pick **Draw link**, set lane count and width in the inspector, click each centreline point, Enter or double-click to finish (`docs/reference/NETWORK_EDITOR.md` §Draw) | Ours is a polyline of clicks, Vissim's is one drag plus a dialog. Defensible: tracing an aerial image wants per-point placement. But the **creation chord** should still be `Ctrl`+right-drag so the reflex transfers |
| Adding curve points to a link | **`Ctrl` + right-click** on the link ✔ | Double-click the centreline (`docs/reference/NETWORK_EDITOR.md` §Draw) | Divergent, and it collides with the Vissim reflex below |
| Rotating a link | **`Alt` + left-drag** on the selection ✔ | Not possible at all | Absent. Cheap to add once group transforms exist; blocked by the same connector-reanchoring problem as group drag |
| Creating a connector | **`Ctrl` + right-drag** from the source link to the target link, then a **Connector** dialog choosing the lane range at each end; **left-click during the drag adds spline points** ✔ | Two clicks: an orange circle at a source lane end, then a teal circle at a target lane start (`src/editor/canvas_connectors.cpp`), or two combo boxes in **Properties → Connectors** | The real cost: a four-lane-to-four-lane movement is **one** `Ctrl`+right-drag in Vissim and **four** two-click pairs here. Vissim also shapes the curve in the same gesture; we require a separate editing pass. Owner's third priority |
| Adjusting lane count across a connector | **Corner drag points** on the connector when source and target lane counts differ ✔ | Not possible — a connector is one lane to one lane (`src/model/network/network.hpp`) | Structural, not cosmetic: our connector model has no lane *range*. M1.9 has to widen the model, not just the gesture |
| Panning | Plain right-drag is **not** spent on object creation — creation is `Ctrl`+right-drag ✔ | Right-button **or** middle-button drag (`src/editor/canvas_input.cpp`) | **No conflict.** Our right-drag pan can stay exactly as it is and `Ctrl`+right-drag creation can be added on top. This removes the only reason the two-click connector flow existed |
| Zooming | Wheel ✔ (implied) | Wheel, around the pointer | Matches |
| Opening an object's properties | Double-click the object **?** — unconfirmed by the reference, which shows dialogs opening on *creation* | Double-click **inserts a geometry point**; properties are a separate always-open inspector dock | Verify before acting. Our always-visible inspector may be the better design regardless |
| Adding to a selection | Not stated. The reference gives **`Ctrl` + left-click = duplicate the selection** ✔ | `Ctrl`+click or Shift+click adds/removes one object; rubber band on empty space (`src/editor/canvas_select.cpp`) | **Direct semantic collision on the same chord**: the gesture that extends a selection here *copies an object* there. A Vissim user reaching for `Ctrl`+click expects a duplicate. This is more dangerous than a missing feature |
| Cycling overlapping objects | **`Tab`** at the click position ✔ | Nothing — `hit()` returns the nearest object and there is no way to reach the one behind it (`src/editor/canvas.hpp:64`) | Absent, and genuinely needed: links and connectors overlap constantly at a junction. `hit()` already ranks by distance, so the ordered list this needs mostly exists |
| Deleting the selection | `Delete` **?** — not in the reference | `Delete` on the canvas removes a **geometry vertex** (`src/editor/canvas_input.cpp:135-137`); deleting objects is a toolbar button with a confirmation dialog | Divergent from near-universal convention regardless of Vissim. Worth fixing with M1.9 |
| Moving several objects | Drag the selection **?** | Not possible — "There is no group drag" (`docs/reference/NETWORK_EDITOR.md` §M1.5) | Known and documented. Reanchoring every attached connector is the reason; it is real work, not an oversight |

**Summary, corrected.** Vissim's creation verb is **`Ctrl` + right-drag**, uniformly, for every
network object, and adopting it cost nothing here because right-drag panning was never the
obstacle. What blocked it was the *model* — one Vissim gesture creates a connector across a lane
range, and `Connector { from, to }` held one lane pair — so M1.9 was a model change with a
gesture on top. Both were built. §1a is what the same rows measure today.

## 3. Window layout

**Dated 2026-09-14, like §1 and §6.** The sidebar (M1.9), levels and display types (M1.10, `data/levels/`,
`data/display-types/`) and in-editor Run (M1.8) exist now; the renderer is `src/editor/canvas_*.cpp`
(`src/render/` was removed in M1.24).

| | Vissim | Today |
|---|---|---|
| Network object palette | Permanent left sidebar, one row per object type, the active row is the edit mode | None. A toolbar dropdown (§1) |
| Properties | Modal dialog on double-click, per object type | A dockable, always-available inspector with **Links / Connectors / Image** tabs; the tab follows the selection (`src/shell/editor_inspector.cpp`, `editor_window.cpp:78-81`) — **better than Vissim for tracing work**, keep it |
| Object lists | Dockable list windows per object type, editable in place | **Objects and problems** dock, four tabs: Links, Connectors, Signal heads, Problems; two-way selection with the canvas; **read-only** (`src/shell/editor_tables.cpp`, `docs/reference/NETWORK_EDITOR.md` §M1.5) |
| Quick View | A small panel showing the selected object's key attributes | None; the inspector covers most of the need |
| Level selector | Levels order overlapping geometry (flyovers, underpasses) | None — no `level` anywhere in the model |
| Display types | Named, swappable draw styles per object | None — draw styles are fixed in `src/render/` and `src/editor/canvas.cpp` |
| Simulation controls | **In the network editor window** | A separate window entirely (§5) |

The tables being read-only is a deliberate M1.5 limit, not a defect — but in Vissim the list
window *is* a legitimate way to edit, and a user who tries will find nothing happens.

## 6. Ranked gaps

Ranked 2026-09-14 by how much each one cost a Vissim user per hour of drawing. **Closed
2026-09-22 (M1.27.3):** items 1–8 were all built, and `trafficsim-gesture-walkthrough` now
measures what is left rather than this list estimating it. They are collapsed to one line each
so the ranking stays readable as history; §1a is the current state.

| # | Gap | Closed by |
|---|---|---|
| 1 | No Run in the editor | M1.8, and M1.24 removed the second window |
| 2 | `Ctrl`+left-click extends the selection; in Vissim it **duplicates** | Not built — measured a dead end, and the owner ruled it stays one (§1a) |
| 3 | Connector creation is per-lane-pair, and the model has no lane range | M1.9 widened the model; M1.26 made the whole carriageway the default |
| 4 | No Network Objects sidebar | M1.9 (`editor_palette.cpp`) |
| 5 | `Ctrl+B` toggles the Objects dock; in Vissim it toggles the **background image** | M1.9 — `Ctrl+B` is the image, the dock moved to `Ctrl+Shift+O` |
| 6 | `Delete` deletes a vertex, not the selection | M1.9 — `Delete` is the selection, `Ctrl+Delete` the vertex |
| 7 | No tool shortcuts; no `Tab` to cycle overlapping objects | M1.9 — `S L C R V H X M K`, and `Tab` is `cycleOverlap` |
| 8 | No levels, no display types | M1.10 |
| 9 | Object tables are read-only | Open — M1.5 limit, revisit with M1.5.1 |
| 10 | No group drag, no `Alt`-drag rotate, no copy/paste | Group drag M1.16, rotate M1.22.2; copy/paste still unbooked (ID allocation) |
| 11 | Missing object types (nodes, priority rules, stop signs, reduced speed areas, parking lots, signal groups, …) | Open — each one needs engine behaviour first; see §4 and §2b |
Items 9–11 stay without a milestone deliberately. Booking work the engine cannot yet honour is
how a roadmap stops being true (`../ROADMAP.md` rule 2).

**Nodes are the one omission worth re-reading later.** They are absent here and unbooked, but in
Vissim they are how delay and queue are aggregated per junction — which is the output a traffic
impact study is actually paid for (`../PROBLEM.md`). They belong to M5 evaluation, not the editor,
but M5 should not rediscover them from scratch.
