# Archived progress — 2026-10-25

Entries moved whole from the live log; dates and evidence describe their own session.

## 2026-10-25 — Refined motorcycle appearance

- **Asked:** improve the motorcycle's looks. Rounded cowl, tank shoulders and tail replace
  the arrow-shaped body; a separate handlebar crosspiece defines its width. Dark tyre tread,
  grips, windscreen and saddle contrast with the highlighted tank and filler cap. The headlamp
  sits on the front cowl rather than the tyre tip.
- Physical dimensions and runtime behaviour retain the existing data. Checks cover connected
  body picking, dark tyres, and distinct tank/saddle detail alongside the existing LOD checks.
- **Verification:** Linux/Qt 6.8.2 offscreen: rendering build and all four targeted display
  suites pass (appearance, canvas LOD, run presentation, lane-change display). Production
  painter preview inspected at working and overview scales; repository checks pass.

## 2026-10-25 — Refined heavy-vehicle appearance

- **Asked:** a more attractive, realistic heavy vehicle. The shared rigid-truck archetype
  now has a rounded cab, wide windscreen, distinct raised cab roof and side glass,
  an exposed chassis neck, softened box corners, roof corrugations and a rear-door header.
  Both heavy-vehicle and rigid-truck select this archetype through existing appearance data.
- Dimensions, axle pose, engine parameters and picking retain the same rigid-body contract.
  Regression checks cover the solid chassis hit area and recessed cargo-roof detail.
- **Verification:** Linux/Qt 6.8.2 offscreen: rendering targets build; appearance, canvas LOD,
  run presentation and lane-change display suites pass. Production gallery inspected at two
  scales. Architecture, file-size and documentation checks pass.

## 2026-10-25 — More realistic van plan view

- **Asked:** refine the van's appearance. Its silhouette now has a short cab-over nose,
  parallel passenger-compartment sides and upright rear doors, independently of the pickup.
  A continuous raised roof, shallow bonnet, divided cab/sliding-door/rear-quarter windows
  and narrow tailgate glass replace the earlier car-like cabin layout.
- Physical dimensions, axles, traffic parameters and the existing display LOD are preserved.
  The appearance suite checks the distinct silhouette, window pillars, solid roof and rear glass.
- **Verification:** Linux/Qt 6.8.2 offscreen: desktop rendering targets build; appearance,
  canvas LOD, run presentation and lane-change display suites pass. File-size, documentation
  and architecture checks pass. Production-painter gallery inspected at two scales.
