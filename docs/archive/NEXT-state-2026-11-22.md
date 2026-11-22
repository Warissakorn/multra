# NEXT state cut on 2026-11-22

Text cut verbatim from [NEXT](../NEXT.md) when it was compacted to open items again (D319). Nothing here is
current; NEXT keeps each section's open lines. Links are rewritten for this folder.

## M3.2.7d passed on attempt 2 (D612); G3 and priority rules slice 1 done

Done before it (2026-11-19; history in [the archived NEXT](../archive/NEXT-state-2026-11-19.md)): M3.2.4e
([MERGE_MOUTH](../reference/MERGE_MOUTH.md), D593), [STOP_LINES](../reference/STOP_LINES.md) (D591, D594–D595),
the line approach (D596, [LINE_APPROACH](../reference/LINE_APPROACH.md) §9), areas opaque (D592), D587–D590.

- **Visibility done (D607, [VISIBILITY](../reference/VISIBILITY.md)):** 100 m default changes no shipped result
  (M0 stays closed); the approach slowdown starts below about 50 m, entered per area in the dialog.
- **G1/G2 done as Vissim (D608, [START_UP](../reference/START_UP.md)):** a type's `accelerationCurve` and a behaviour's
  attention pair, absent by default (M0 unchanged). Measured on saturated M0: Vissim-like values shorten lost
  time (1.53 → 0.96 s); a standstill acceleration of 2.0 m/s² gives 2.16 s (HCM 2.0). Owner, "ตามแนะนำ" (D609):
  no shipped curve, each study sets it; order: A2 attempt 2, then G3, then PRIORITY_RULES slice 1.
- **A2 step 3 answered (D610, D611):** gap/headway plausible; areas green, red and olive as Vissim with a centre
  arrow; pointing shows a road's edges in black with arrows, selection in blue, a selected area its roads.
- **M3.2.7d passed (D612):** step 4 and the verdict, owner (verbatim): "ทดลองแล้วสมจริง". M3.2 still closes only with
  its diverge acceptance-fixture row (ROADMAP M3.2).
- **G3 done as Vissim (D613, [DIFFUSION](../reference/DIFFUSION.md)):** a behaviour's `diffusionTime` removes a vehicle
  stuck at its dead end and counts it as `removed`; off by default (M0 unchanged). PRIORITY_RULES slice 1
  done (D614, below); the owner's remaining Windows items are in [OWNER_SITTING_WINDOWS](../plans/OWNER_SITTING_WINDOWS.md).


## Priority rules like Vissim (D604, D606, D614–D617) — all slices done; the owner's look next

[PRIORITY_RULES](../reference/PRIORITY_RULES.md): a stop line with conflict markers, no conflict area. **Slice 1
done (D614):** schema 83 marker form, compile per marker to the core `PriorityRule`, `maxSpeed` and class
filters in the engine; rows `priority_rule` PR1–PR6, PR10–PR13; M0 unchanged. **Slice 1b done (D615):** a Stop control
on the line (it may name no area) is a full stop before the gap test (PR14–PR16). **Slice 2 done (D616):** with a line picked,
Ctrl+right-click on another lane adds a marker; drag and Delete; markers follow their roads (PR7, PR8). **The owner's look (item 51) passed 2026-11-22** ("51 ok") on the
`priority-rule-lab` example. **Slice 3 done (D617):** headway bands, links
while picked, table rows with a dialog, the unreached-marker advisory. **Left:** a class
picker in place of typed class ids if the owner asks.


## G7 waiting acceptance (D618, D619) — done; the owner judged it plausible (2026-11-22)

[YIELD_IMPATIENCE](../reference/YIELD_IMPATIENCE.md): Aimsun's gap reduction (a behaviour's per-vehicle `yieldTime`; a
rule's or marker's `finalGapTime`, `gapReductionStart`, `gapReductionEnd`, in the area and rule dialogs) and SUMO's
impatience at conflict areas (the follow-on behind a standing major vehicle), schema 84, rows YI1–YI12; off by default,
M0 unchanged. **Item 52 passed 2026-11-22** ("สมจริง", with a licensing question answered in PROGRESS); its findings on
conflict-area display are D620 (arrows on the lane centre, only on the selected area; Vissim's yellow and red-red).
**The other realism gaps** ([evidence](../evidence/realism-gaps-vissim-sumo-aimsun.md)): G8 needs nothing (matches all
three programs); G4, G5, G6 are named in ROADMAP M3.4; G9 is M3.3.3b.

