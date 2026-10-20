# Red-light running share (B-9)

Written 2026-10-18 (D237); implemented the same day (D238,
[evidence](../evidence/red-running.md)). §3 was revised the same day by the owner's answers to
D241 (D246) and implemented by D247. This is tier B-9 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), the
last of tier B: a level-4 key under D216, so driver behaviour, local by road. It extends the red
rule of [AMBER](AMBER.md) §1. Rule 4 applies: nothing here is validated, and no value ships (D178).

## 1. Scope and the owner's choices

Today a red head holds every vehicle that can still stop. The only exception is commitment: a
vehicle that cannot stop even at its type's `maxDeceleration` goes, and only when its behaviour has
`amberDeceleration` (AMBER §1). Deliberate red-light running is not modelled, though its rate varies
strongly by country and a study may need it.

The owner chose on 2026-10-18:
- **When.** Only in the first T seconds of red, and only while the vehicle is still moving. A
  standing vehicle never starts on red.
- **Who.** A fixed share of drivers. Each vehicle is decided once, by a hash, so the random stream is
  unchanged.
- **Conflicting traffic.** No new interaction. Conflict areas and priority rules that the network
  already has still apply; the shipped signalised four-leg has none. Each run through red is
  recorded as an event and counted, so a study can count violations. It is not a collision model.

After D241's trace, the owner chose on 2026-10-18 (D246):
- **A runner who can stop gently stops.** The rule lets a runner through only when stopping before
  the line would need more than a gentle deceleration, so only a driver caught near the line runs.
  A driver already braking for amber, who can still stop gently, stays stopped (D241's vehicle 1945).
- **A runner who is let through keeps going.** The head does not hold it again during that red, so
  the window is the time in which a driver may decide to run, not the time by which it must cross.
  No late second stop forces its follower into a clamp (D241's vehicle 1788).
- **Every crossing the rule started is counted,** including one that falls after the window (D241's
  seed 51 at 2 s).

After D247's measurements, the owner kept both on 2026-10-18 (D248):
- **The window stays an upper bound.** It is not widened to drivers who could stop gently.
- **A runner's committed crossing still counts.** It is not removed when commitment would also
  have let the vehicle through.

## 2. The keys

- **`redRunningShare`** is the share of drivers who may run, in (0, 1].
- **`redRunningTime`** is how long after red begins they may still run, in seconds, in (0, 30].
- **Both or neither.** One without the other is refused with `INCOMPLETE_RED_RUNNING` at the missing
  key's path, as D95's pair is. A value outside its range, or a non-number, is refused with
  `INVALID_BEHAVIOUR_PARAMETER` at its path. Both models carry them.
- **Schema 44.** The keys are written only when an owned behaviour carries them. An older schema
  carrying either is refused with `EDIT_UNSUPPORTED_FIELD`.
- **Absent means today:** every run is byte-identical. The catalog ships no value.

## 3. The rule

- **The runner draw.** `u = mix(mix(seed) ^ id · 0x9E3779B97F4A7C15 ^ 15) >> 11 · 2⁻⁵³`.
  - This is W74's trait hash ([W74 §5](W74.md#5-parameters-and-traits)) at index 15; W74 uses 0–14.
  - A vehicle is a runner where `u < redRunningShare`.
  - It is computed when needed, never stored, and never drawn from the run's random stream, so
    arrivals, desired speeds and driver factors stay the same.
  - The same driver gives the same answer at every head. Whether a share applies there is set by
    the behaviour in force at that head.
- **At a red head.** A vehicle whose effective behaviour (D127) carries the keys is *let through*
  by that head, which then does not hold it, when at the start of the tick its speed is at least
  `kStoppedSpeed` (0.1 m/s), its front is not past the line, and either:
  1. **it was let through by this head on the previous tick** (the decision holds, D246); or
  2. **it is let through anew:**
     - it is a runner under that behaviour's share;
     - the head has been red for less than `redRunningTime` (`redElapsed`, below);
     - **it cannot stop gently** (D246): `v² > 2 · g · toLine`, where `g` is the behaviour's
       `amberDeceleration` or, without it, the type's `comfortableDeceleration`. Equality stops, as
       at amber.
- **The decision lapses** on the first tick that starts with any of these:
  - the head is not red (it turned green before the vehicle crossed: no event);
  - the vehicle's front is past the line;
  - the vehicle is standing, for example behind a queue. A standing vehicle never starts on red, so
    the head holds it again until green;
  - its behaviour in force no longer carries the keys;
  - the vehicle has left the network.
- **Otherwise the red rule is unchanged,** including commitment. Amber is unchanged.
- **Every tick.** The test runs on each tick, like the amber check. A vehicle let through is not
  held again when the window ends: the window bounds only the start of a run. Before D246 a runner
  short of the line at the window's end was held again, and D241 traced the clamps that caused on
  M2.6 ([evidence](../evidence/red-running.md#d241-the-added-m26-clamps)).
  - **What the window means in practice (D247, D248).** A vehicle sees a red head from any
    distance, so one that meets red after onset has usually been stopping gently since amber. With
    `amberDeceleration`, a run therefore almost always starts at red onset, by a driver who went on
    amber; on the shipped projects the 1, 2 and 3 s windows give identical outputs
    ([evidence](../evidence/red-running.md#d247-the-owners-answers-to-d241-2026-10-18)). Do not
    calibrate `redRunningTime` expecting it to scale the count.
- **The state.** `SimState::redReleases` lists the (vehicle, head) pairs let through on the last
  tick, sorted by vehicle id and, within a vehicle, in its route's head order. It is empty
  without the keys.
- **`redElapsed`.** The time since the head last turned red, at the start of the tick. Consecutive
  red phases in a program count as one red, measured across the cycle boundary.
- **The event.** On the tick when a vehicle that this rule let through moves its front past that
  head's line, the run emits `RedRunningEvent{time, vehicleId, headId}`, at most once per vehicle
  and head. Because the decision holds, a crossing after the window is counted too (D246).
  - The rule is tested before commitment, so a runner's crossing counts even when commitment would
    also have let it through. A non-runner's committed crossing is not a run (D248).
  - The event stream writes it as kind `red-running`.
  - The run summary counts it as `redRunning`.
  - Batch JSON, the per-seed rows and the CSV carry `redRunning` only when a behaviour of the run
    carries the keys, so outputs without them keep their bytes.
- **After the line.** A runner moves as any vehicle does: following, conflict areas and receiving
  space all apply. No crossing vehicle reacts to it.

## 4. Editor

The behaviour dialog gains two off-until-ticked rows, `redRunningShare` and `redRunningTime`. They
are labelled with the key names and sit beside `stopDwellTime`. Ticking one without the other is
refused on confirm with the pair's issue. Editing is one Undo step with the library.

## 5. What stays out

- Yielding to crossing traffic. That needs conflict areas at signalised junctions.
- A collision or near-miss model.
- Starting from standstill on red, and running at amber (amber has its own rule).
- A share by vehicle type, or one that changes with the time of day.
- Calibration: rates by country are for a study to set.
- A camera-style count of every entry on red, whatever lets the vehicle through. That is a
  measure for `src/eval/`, to add when a study needs it (D248).

## 6. Rows (BA138–BA145, BA152–BA157)

BA152–BA157 and BA139's amended case are D246's, implemented by D247.

| Row | Case | Asserts |
|---|---|---|
| BA138 | No keys | Red holds as AMBER §1; shipped outputs byte-identical |
| BA139 | Share 1, time 2 s; at red onset a vehicle that can stop only harder than gently (10 m/s, 15 m short: 3.3 m/s² against `amberDeceleration` 3) | It is not held, crosses, and one event is emitted; without the keys it is held |
| BA140 | The same, arriving after 2 s of red | Held |
| BA141 | A runner standing at the line when red begins | Stays until green |
| BA142 | The draw | About the share over 2000 ids; the same id gives the same answer |
| BA143 | The random stream | Arrivals, desired speeds and factors unchanged with the keys |
| BA144 | Behaviour by road | Only the head under a behaviour carrying the keys is run |
| BA145 | Codec, validation, outputs, editor | Schema 44 round-trips; 43 refused; the pair and ranges refused at their paths; the event JSON; `redRunning` only with the keys; the dialog rows write them |
| BA152 | Share 1, time 2 s; at red onset a runner that can stop at `amberDeceleration` (D241's 1945: 32 m short at 9 m/s) | Held; no event. The same runner 15 m short at 10 m/s is let through |
| BA153 | The same, on a behaviour without `amberDeceleration` | The type's `comfortableDeceleration` decides: held when it fits, let through when it does not |
| BA154 | A runner let through at red onset, still short of the line when the window ends (D241's 1788 and seed 51 at 2 s) | Not held again; it crosses after the window and exactly one event is emitted |
| BA155 | A runner let through that comes to a standstill behind a standing leader short of the line | The leader leaves; the runner is held until green; no event |
| BA156 | The head turns green before a vehicle that was let through crosses | No event; `redReleases` is empty after green. Without the keys `redReleases` stays empty; two runs of one seed are identical |
| BA157 | Two red heads on one route; a runner let through at the first | Decided afresh at the second: held there when it can stop gently |
