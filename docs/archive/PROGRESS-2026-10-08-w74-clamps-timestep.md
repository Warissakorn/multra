# PROGRESS archive — W74 clamp trace and timestep record

Moved whole from [`PROGRESS.md`](../PROGRESS.md) on 2026-10-13. Nothing here is current.

## 2026-10-08 — The extra W74 clamps traced: no W74 defect

Evidence only; no engine change. `tools/w74_clamp_trace.cpp` attributes every safety clamp
of the BA27 runs (2,053, all moving) to the obstacle setting the smallest allowance, from the
tick's rebuilt snapshot, and records each amber onset. All 240 runs match BA27's counts; none
is unexplained. 83–96 % are amber heads, held as red with no commitment test: W74's slower
fixture discharge leaves a queue at 16–19 % of amber onsets (prototype under 0.1 %), so more
onsets catch a vehicle that cannot stop. The rest are the standstill cap treating a moving
leader as standing (D105's recorded case), met by W74's gentle emergency braking near `AX`.
Neither rule changed (BA18); NEXT §2 holds it as an owner decision. The sweep now shares its
document setup with the trace and stays byte-identical. See [evidence](../evidence/w74-clamps.md).

---

## 2026-10-08 — W74 timestep record, BA27, and the courtesy fixture

Evidence only; no engine change. `tools/w74_discharge_sweep.cpp` runs the four-leg project,
prototype and `w74` (fixture values in `evidence/w74-discharge-behaviour.json`: three are
PTV's W74 defaults, fifteen are uncited), at dt 0.1/0.25/0.5, seeds 42–81, with the CLI's
discharge spec. Its prototype arm at dt 0.1 seed 42 equals the Debug CLI in all 90 cycles,
and a rerun is byte-identical. Mean headway rises with dt in both models, about twice as
much for W74; W74 runs clamp more (9.4 → 14.8 per run), which NEXT now asks to trace before
any preset. A `w74run` case checks that a winning courtesy hold stores its own state; a
mutation keeping the leader's state fails it. See [evidence](../evidence/w74-discharge.md).
