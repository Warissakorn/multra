# PROGRESS archive — 2026-10-09 — Three modelling levels (D145)

Moved whole from [PROGRESS](../PROGRESS.md) on 2026-10-14. Relative links gained `../`.

## 2026-10-09 — Three modelling levels designed and booked (D145)

The owner asked for more capability chosen by what studies need — parking lots, dynamic
assignment, then Micro, Meso and Macro levels, Macro as a full four-step model, after the D143
queue. Docs only, no code: [design](../plans/MULTI_LEVEL_MODELLING.md) (study → output matrix, one
authoring network compiled to three views, shared Zone/OD demand, parking lots as zones with
capacity, between-run dynamic assignment, pure `src/meso/` and `src/macro/` engines, subarea
cut), ROADMAP M8–M12, PROBLEM §1–§6 amended (the assignment non-goal replaced), ARCHITECTURE
planned rows, OWNER_SITTING B7 for the design's open parameters. Parking and regional modelling
left *Later*. Reasoning in [D145](../decisions/RECORD.md#d145). The D124/D125 entries moved whole
to `archive/`. Checks: `trafficsim-check-docs` and `trafficsim-check-file-sizes` built directly
with GCC 13.3 (the headless preset lacks nlohmann/json on this machine); no engine build claimed.
