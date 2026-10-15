# Evidence

Measurement records behind decisions and acceptance rows. Each `.md` note summarises its
measurement, names the command that reproduces it and cites its raw data. **Read the note, not
the raw `.json`/`.jsonl`/`.csv` files** (≈1.5 MB); open raw data only to re-check a number.
Tests read the `m3.2.7-*` files, and `src/` cites the `vehicle-pose-original-*` files: do not move
or edit evidence in place. A new measurement gets a new note.

| Note | What it measured | Decisions |
|---|---|---|
| [b11-seed-mixing.md](b11-seed-mixing.md) | B11 — seed mixing (D159), 2026-10-14 | D159, D88 |
| [behaviour-editor.md](behaviour-editor.md) | M3.3.2c behaviour library editor evidence — 2026-10-07 | D128 |
| [behaviour-library.md](behaviour-library.md) | M3.3.2a behaviour library and codec evidence — 2026-10-07 | D126 |
| [behaviour-selection.md](behaviour-selection.md) | M3.3.2b front-segment behaviour selection evidence — 2026-10-07 | D127 |
| [connector-equation.md](connector-equation.md) | Direct Connector equation — D107, 2026-10-04 | D107, D106, D80 |
| [connector-lane-centres.md](connector-lane-centres.md) | Connector runtime lane-centre verification — 2026-10-04 (D106) | D106 |
| [discharge-controls.md](discharge-controls.md) | M3.3.1b1 local evidence — 2026-10-06 | — |
| [discharge-identity.md](discharge-identity.md) | M3.3.1b2b2 rank scope and source-sink identity evidence — 2026-10-07 | D125 |
| [discharge-measurement.md](discharge-measurement.md) | M3.3.1a local measurement evidence — 2026-10-06 | — |
| [discharge-passage.md](discharge-passage.md) | M3.3.1b2b1 proven passage evidence — 2026-10-07 | — |
| [discharge-provenance.md](discharge-provenance.md) | M3.3.1b2a local evidence — 2026-10-06 | — |
| [m3.2.7-sweep.md](m3.2.7-sweep.md) | M3.2.7b — T-junction diagnostic sweep | — |
| [m3.2.8a-clamps.md](m3.2.8a-clamps.md) | M3.2.8a — the five near-standing minor-road clamps (D105) | D105, D80, D69 |
| [m3.2.8a-commitment.md](m3.2.8a-commitment.md) | M3.2.8a — commitment at a waiting line | D69 |
| [m3.2.8b-mandatory.md](m3.2.8b-mandatory.md) | M3.2.8b — mandatory lane changing, with minimal cooperation | D90, D71 |
| [m3.2.8c-arrival-phases.md](m3.2.8c-arrival-phases.md) | M3.2.8c step 4: the right-turn rise was a five-seed sample, not a mechanism | — |
| [m3.2.8c-cooperative-braking.md](m3.2.8c-cooperative-braking.md) | M3.2.8c — cooperative braking with look-ahead (D90) | D90, D89, D71 |
| [m3.2.8c-discretionary.md](m3.2.8c-discretionary.md) | M3.2.8c — discretionary lane changes (D95) and the hold (D101): A53 evidence | D95, D101 |
| [m3.2.8c-east-approach.md](m3.2.8c-east-approach.md) | M3.2.8c step 5: the East-approach rise is the left-turn dead-end waits | — |
| [m3.2.8c-pocket-discharge.md](m3.2.8c-pocket-discharge.md) | M3.2.8c step 3: the right-turn rise is red time, not missed greens | D88 |
| [m3.2.8c-right-turn-stages.md](m3.2.8c-right-turn-stages.md) | M3.2.8c step 2: where the South/North right-turn delay grows | D88 |
| [m3.2.8c-right-turns.md](m3.2.8c-right-turns.md) | M3.2.8c step 1 — where lane changes happen, and the right-turn rise | D88 |
| [m3.2.8c-wait-causes.md](m3.2.8c-wait-causes.md) | M3.2.8c — why the remaining dead-end waits wait (D92) | D92, D90 |
| [m4.2-amber.md](m4.2-amber.md) | M4.2 amber stop-or-go — gate evidence (D147, 2026-10-09) | D147, D88 |
| [m5.9-cooldown.md](m5.9-cooldown.md) | M5.9 cool-down on `four-leg-signalised` (D146, 2026-10-09) | D146 |
| [m6-benchmark-options.md](m6-benchmark-options.md) | M6 benchmark option sheet (M6.0, 2026-10-10) | D143, D34 |
| [source-first-step.md](source-first-step.md) | Source first-step clearance — D108, M3.2.8a.1a | D108, D107 |
| [perception-motion.md](perception-motion.md) | M3.5 slice 2: look-ahead and multi-anticipation evidence — 2026-10-15 | D182 |
| [perception-delay.md](perception-delay.md) | M3.5 slice 4 — reaction-time FIFO, safety, replay and performance — 2026-10-15 | D184, D179 |
| [perception-lane.md](perception-lane.md) | M3.5 slice 3: look-back, rear TTC and perceived lane-change evidence — 2026-10-15 | D183 |
| [perception-codec.md](perception-codec.md) | M3.5 slice 1: perception codec evidence — 2026-10-15 | D179, D181 |
| [w74-clamps.md](w74-clamps.md) | M3.3.3a: where the extra W74 safety clamps come from — 2026-10-08 | — |
| [w74-codec.md](w74-codec.md) | M3.3.3a W74 schema-25 codec evidence — 2026-10-08 | D135, D136 |
| [w74-composition.md](w74-composition.md) | M3.3.3a W74 composition evidence — 2026-10-08 | D137, D138 |
| [w74-discharge.md](w74-discharge.md) | M3.3.3a BA27: W74 queue-discharge timestep sensitivity — 2026-10-08 | — |
| [w74-preset.md](w74-preset.md) | M3.3.3a: a cited, uncalibrated `w74` preset and its discharge and clamps — 2026-10-15 | D176 |
| [w74-pure-function.md](w74-pure-function.md) | M3.3.3a W74 pure-function evidence — 2026-10-08 | D135 |
| [w74-traits.md](w74-traits.md) | M3.3.3a W74 driver-trait evidence — 2026-10-08 | D136, D137 |
| [windows-session-check-2026-10-02.md](windows-session-check-2026-10-02.md) | Windows session check of the owner's items — 2026-10-02 | — |
