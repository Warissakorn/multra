# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-18 — Give-way line stop implemented (D240)

D239's contract ([evidence](evidence/give-way-stop.md)).
- **Rule:** on a tick that would clamp at a binding give-way line, a stop within the type's maximum
  brakes at `v²/(2·gap)` instead. BA147–BA149 failed first; the guards were proved by mutation.
- **Measured:** the shipped Yield T-junction's 6 clamps at seeds 42–81 go, with delays unchanged.
  Every other shipped output is byte-identical. The 11 replaced ticks braked at 2.5–7.4 m/s². Ir
  +0.11–0.15 %.
- **Correction:** D232's "none at seeds 42–51" was wrong (seed 46 has 2).

`check` 178/178, Linux only.

## 2026-10-18 — Give-way line stop contract (D239, docs only)

The owner asked for the easiest open item: M3.2.8a.1's give-way line clamps (D232)
([GIVE_WAY_STOP](reference/GIVE_WAY_STOP.md)).
- **The owner's choice:** brake to stop at the line.
- **The contract:** only on a tick that today clamps at a binding give-way line, and only when the
  stop fits the type's maximum, the vehicle brakes at `v²/(2·gap)` and no clamp is reported.
  Everything else stays byte-identical. Rows BA146–BA151.

## 2026-10-18 — B-9 red-light running implemented (D238); tier B complete

D237's contract ([evidence](evidence/red-running.md)).
- **Rule:** a hashed share of drivers pass a red head in its first seconds while moving; a
  `red-running` event when they pass the line; `redRunning` in reports only with the keys. BA139
  and BA144 failed first against a step that ignored the keys.
- **File and editor:** schema 44, refused below; the pair and ranges validated on both models; two
  behaviour-dialog rows. Every hook proved by mutation.
- **Measured:** shipped runs byte-identical. At a 5 % share runs are rare and rise steeply with the
  window (four-leg 2, 8, 24 in 40 runs for 1–3 s); delay falls within its half-width. Ir −0.44 %
  (layout).

`check` 177/177, Linux only.

## 2026-10-18 — B-9 red-light running contract (D237, docs only)

NEXT's item, the last of tier B ([RED_RUNNING](reference/RED_RUNNING.md)). Today red holds
every vehicle that can still stop.
- **The owner's choices:** only in the first T seconds of red and only while moving; a fixed share
  of drivers decided once per vehicle by a hash; no new interaction with crossing traffic, but each
  run is counted.
- **The contract:** `redRunningShare` and `redRunningTime` as a pair at schema 44; W74's hash at
  index 15; a `red-running` event and a `redRunning` count written only with the keys; rows
  BA138–BA145.

## 2026-10-18 — B-4 driver-factor spread implemented (D236)

D235's contract ([evidence](evidence/driver-factor-spread.md)).
- **Draw:** `samplePending` takes the standard deviation from the type's own behaviour, 0.15 without
  the key; the random stream is unchanged. Rows BA132, BA133, BA135 and BA136 failed first against
  the 0.15 stub.
- **File and editor:** schema 43, refused below; `INVALID_BEHAVIOUR_PARAMETER` on both models; a
  behaviour-dialog row. Codec, validation and the row proved by mutation.
- **Measured:** shipped runs byte-identical. Spreads of 0, 0.3 and 0.5 leave mean delay inside its
  half-width on all four projects; clamps rise with a wide spread. Ir ±0.015 %.

`check` 176/176, Linux only.

## 2026-10-18 — B-4 driver-factor spread contract (D235, docs only)

NEXT's item ([DRIVER_FACTOR](reference/DRIVER_FACTOR.md)). Today every driver's factor is drawn
with a fixed spread of 0.15.
- **The owner's choices:** on the driver behaviour, from the type's own behaviour when the vehicle
  is made; the standard deviation only (the mean would repeat `additiveSafetyDistance`).
- **The contract:** `driverFactorSpread` in [0, 0.5] at schema 43; the same two draws, so the random
  stream is unchanged; drawn once, never redrawn by road; rows BA131–BA137.

## 2026-10-17 — B-7 lane preference by vehicle type implemented (D234)

D233's contract ([evidence](evidence/lane-preference.md)).
- **Split:** `laneSplit` takes the type's `kerbLanes` in compile, routeless paths and the editor's
  input table. Rows BA125–BA129 failed first against a stub that ignored it.
- **File and editor:** schema 42, refused below; `INVALID_KERB_LANES`; a type-dialog row (failed
  first). Codec and hooks proved by mutation.
- **Measured:** shipped runs byte-identical. On the lane-change lab, heavy vehicles on the kerb lane
  add 0.3–0.4 s on their movements. M2.6 does not move: its entry decisions choose lanes by
  destination, which the contract now names. Ir −0.35 % (layout).

`check` 175/175, Linux only.

## 2026-10-17 — B-7 lane preference by vehicle type contract (D233, docs only)

NEXT's item ([LANE_PREFERENCE](reference/LANE_PREFERENCE.md)). Today every type gets the same
per-lane entry split, so heavy vehicles spread across all lanes.
- **The owner's choices:** entry lane by type, on the vehicle type, the kerb-most N lanes.
- **The contract:** `kerbLanes` (1–16) at schema 42 renormalises the split over lanes 0 to N−1;
  mandatory changes and D95 unchanged; rows BA124–BA130.

## 2026-10-17 — Give-way line clamps traced (D232, no code change)

NEXT's item ([evidence](evidence/give-way-line-clamps.md)). D231's Stop-line clamps are not the
Stop rule's: the shipped Yield `t-junction-priority` clamps the same way (6 at seeds 42–81).
- **What fires:** phase 1, on a minor vehicle creeping off its line at walking pace when a major
  vehicle comes inside the zone's headway or gap time. It can still stop at its maximum, so it is
  not committed (D69) and is held again at the line. The braking law overshoots by millimetres to
  centimetres. A few standing cases at dt 0.5 are the standstill cap (D201 B5).
- **Booked:** these are M3.2.8a.1's moving minor clamps. Its gate wants a contract and rows before
  any motion change, and each option moves shipped output, so nothing was changed.

## 2026-10-17 — B-8 Stop dwell time implemented (D231)

D230's contract ([evidence](evidence/stop-dwell.md)).
- **Engine:** the Stop service entry records whole ticks of rest from the vehicle's behaviour when it
  comes to the line; one without the key. Rows BA118–BA123 failed first against a one-tick stub.
- **File and editor:** schema 41, refused below; validation on both models; a behaviour-dialog row
  (failed first). Codec hooks proved by mutation.
- **Measured:** shipped runs byte-identical. On a T-junction copy with Stop controls, a 2 s dwell
  raised minor delay by 13–18 s. Every clamp there sits at a Stop line, with or without the key
  (traced next). +0.25 % Ir on four-leg, +0.41 % on the Stop copy.

`check` 174/174, Linux only.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
