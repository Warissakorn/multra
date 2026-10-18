# Red-light running share (B-9)

Written 2026-10-18 (D237); implemented the same day (D238,
[evidence](../evidence/red-running.md)). This is tier B-9 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), the
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
- **At a red head.** A vehicle whose effective behaviour (D127) carries the keys is not held by that
  head when all of these hold at the start of the tick:
  1. it is a runner under that behaviour's share;
  2. the head has been red for less than `redRunningTime` (`redElapsed`, below);
  3. its speed is at least `kStoppedSpeed` (0.1 m/s);
  4. its front is not past the line.
- **Otherwise the red rule is unchanged,** including commitment. Amber is unchanged.
- **Every tick.** The test runs on each tick, like the amber check. A runner still short of the line
  when the window ends is held again, as any vehicle is. Without `amberDeceleration`, a hold that
  then cannot be met gives a safety clamp, as today.
- **`redElapsed`.** The time since the head last turned red, at the start of the tick. Consecutive
  red phases in a program count as one red, measured across the cycle boundary.
- **The event.** On the tick when a vehicle exempted by this rule moves its front past the head's
  line, the run emits `RedRunningEvent{time, vehicleId, headId}`, at most once per vehicle and
  head.
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

## 6. Rows (BA138–BA145)

| Row | Case | Asserts |
|---|---|---|
| BA138 | No keys | Red holds as AMBER §1; shipped outputs byte-identical |
| BA139 | Share 1, time 2 s; a vehicle that can stop meets red onset | It is not held, crosses, and one event is emitted; without the keys it is held |
| BA140 | The same, arriving after 2 s of red | Held |
| BA141 | A runner standing at the line when red begins | Stays until green |
| BA142 | The draw | About the share over 2000 ids; the same id gives the same answer |
| BA143 | The random stream | Arrivals, desired speeds and factors unchanged with the keys |
| BA144 | Behaviour by road | Only the head under a behaviour carrying the keys is run |
| BA145 | Codec, validation, outputs, editor | Schema 44 round-trips; 43 refused; the pair and ranges refused at their paths; the event JSON; `redRunning` only with the keys; the dialog rows write them |
