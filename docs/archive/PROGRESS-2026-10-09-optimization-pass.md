# PROGRESS archive — moved whole 2026-10-14

## 2026-10-09 — Optimization pass: lld, D140, validate benchmark, NEXT

Measured first (Linux, 4 cores, Qt offscreen): the engine (0.15–0.30 µs per vehicle-tick, linear
to 96 crossings), the run view (~3.3 ms per Step) and redraw (3.6 ms at 40 crossings) are not
bottlenecks and were left alone. **Link with lld when the compiler accepts it**
(`TRAFFICSIM_LLD`, default ON, MSVC untouched): touching one source relinks some 40
executables, and GNU ld was most of an incremental Debug build. `trafficsim-tests` links in
4.5 s with ld and 0.57 s with lld; touching `src/shell/editor_demand.cpp` rebuilt in 31.3 s
before and 13.0–13.5 s after, `src/model/network/routing.cpp` 28.7 s → 10.7–11.1 s. CI's Linux
apt line installs `lld`; without it the default linker is kept silently.

**D140: Connector paths once per expansion.** `History` validates every command, and validating
the M2.6 study template took 21.6 ms against 1.0 ms for four-leg on the same network: callgrind
put 85 % of it in `connectorPaths`, recomputed by every `routeShortestChains`, `routeLaneFamily`
and `routelessChains` call inside one `expandRouteless`. `ConnectorPathTable` (lazy, local, never
stored) is now built once by `expandRouteless` and `routelessIssues` and passed down; the old
signatures build their own, so editor and compile callers are unchanged. Validation 21.6 → 5.0 ms
(median of 15; four-leg 1.03 → 0.91 ms). `--seeds 1-3` output of all four shipped projects is
identical apart from the build stamp; the ten-seed study batch and the engine benchmark are
unchanged within noise (an interleaved ld/lld engine run ruled out the linker).
`tools/validate_benchmark.cpp` (`trafficsim-validate-benchmark [project] [repetitions]`) keeps
that number measurable; like the other benchmarks it prints and is not in `check`.

**Route continuations find their tail once.** After D140, 43 % of study-template validation was
`continuations` rescanning every route object to find the tail, once per candidate. It now finds
it once (the last match, as before). Validation 4.97 → 2.75 ms (four-leg 0.85 → 0.72 ms); the
editor's route hover and gesture use the same function. CLI output identical.

**Destination chains once per expansion (D140's walk scope).** `placedDecisions` runs once per
time slice × vehicle type and resolved the same `routeShortestChains(decision Link, destination)`
each time, about 86 calls per study-template validation. A `DestinationChainMemo` beside the
table resolves each pair once. Same-session A/B: 3.32 → 2.46 ms median (five runs of 31 each);
four-leg unchanged (no destination decisions). CLI output identical.

**`appendStationRouting` takes the expansion's `runtimeSections`.** It rebuilt the table on every
call (≈5 per study-template validation, 23 % of the remaining instructions); `expandRouteless`
already holds that table and passes it to `appendLaneChanges` beside it. A/B: 2.60 → 1.72 ms
median (five runs of 31); four-leg unchanged (no positioned decisions). CLI output identical.
Validation overall this session: 21.6 → 1.7 ms. Left: `routelessChains` still walks once per
input × slice; at under 2 ms per edit it was not started.

**NEXT.md slimmed** (31.4 → 26.8 KB, −15 %): the five 2026-10-05/06 owner-instruction sections narrated
finished slices already recorded in ROADMAP (M2.8, M3.2.4, M3.3 status lines), PROGRESS and
RECORD (D115–D129, D135–D139, each checked before removal). One section keeps every open item
verbatim — the D128/D119/D116/D115–D118 Windows reviews, the Demand PR review order, W74's next
steps, BA18, the M3.2.4e contract rule and pending native CI. §1–§5 are live owner gates and
working notes and were not cut, so the planned −40 % was not reached.

---
