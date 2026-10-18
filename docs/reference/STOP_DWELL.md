# Stop dwell time (B-8)

Written 2026-10-17 (D230); implemented the same day (D231,
[evidence](../evidence/stop-dwell.md)). This is tier B-8 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), a
level-4 key under D216: driver behaviour, local by road. It extends the Stop rule in
[SIMULATION](SIMULATION.md) (M3.2.5a, D62). Rule 4 applies: nothing here is validated, and no value
ships (D178).

## 1. Scope and the owner's choices

At a Stop line a minor vehicle must serve the line before it may cross. Today the Stop finishes the
stop itself: the vehicle rests at zero for the tick it came to the line and one whole tick more
(`restsAtStop`). So a stop lasts 0.1–0.2 s at `timeStep` 0.1 and 0.5–1 s at 0.5. The result depends
on the time step, and either value is far shorter than a real stop of about 1–3 s.

The owner chose on 2026-10-17:
- **A driver-behaviour key,** so the value follows the behaviour assigned by road and class
  (M3.3.2b), like the other level-4 keys.
- **Absent means today.** Without the key the old tick rule runs, and every existing file runs
  byte-identically. The catalog ships no value. No shipped project has a Stop (`t-junction-priority`
  uses Yield).
- **Fixed seconds,** the same for every driver of the behaviour.

## 2. The key

- **`stopDwellTime`** is an optional behaviour key, in seconds, on either model. Its range is
  (0, 60]. A value outside it, or a non-number, is refused with `INVALID_BEHAVIOUR_PARAMETER` at its
  path.
- **Schema 41.** The key is written only when an owned behaviour carries it. An older schema carrying
  it is refused with `EDIT_UNSUPPORTED_FIELD`.

## 3. The rule

- **Coming to the line** is unchanged: below walking pace, with the front within `stopLineReach`
  (`atStopLine`).
- **Dwell ticks.** When the service entry is made, it records `ticks = max(1, ceil(stopDwellTime / dt))`
  from the vehicle's effective behaviour at that moment (its road and class, D127). A tolerance of
  1e-9 keeps an exact multiple from rounding up. Without the key, `ticks = 1`.
- **Rest.** The vehicle rests at zero speed for the tick it came to the line and then `ticks` whole
  ticks more. Only after that has it served the line. With `ticks = 1` this is exactly today's rule.
- **Waiting runs alongside.** The dwell counts while the vehicle also waits for a gap, a red head or
  receiving space. Serving the line never lets it cross a blocked gap, and a gap that opens during
  the dwell does not cut the dwell short.
- **Time step.** The dwell is at least `stopDwellTime` and less than one tick longer, so runs at
  different time steps agree to within a tick.
- **Retune.** A live retune that changes the key leaves entries already made with their ticks; the
  new value applies from the next arrival at a line.
- **Not a clamp.** Resting at a Stop stays ordinary braking (SIMULATION §6), as now.

## 4. Editor

The behaviour dialog gains an off-until-ticked row for `stopDwellTime` among the keys either model
carries, beside `commitmentDeceleration`. Like its neighbours, it is labelled with the key name.
Editing it is one Undo step with the library.

## 5. What stays out

- A dwell set on each Stop line (a network-object value), or a per-driver range.
- Stop signs on signal-controlled lines beyond today's composition: green removes the head's hold
  only, and the Stop still applies, as now.
- Any change to how a vehicle comes to the line, or to the gap test.

## 6. Rows (BA117–BA123)

| Row | Case | Asserts |
|---|---|---|
| BA117 | No key | Rests the tick it came to the line plus one; shipped outputs byte-identical |
| BA118 | 2 s at dt 0.1, clear major road | Rests 20 whole ticks after coming to the line, then crosses |
| BA119 | 2 s at dt 0.1, 0.25 and 0.5 | The time from coming to the line to moving off agrees within one tick (time-step independence) |
| BA120 | 0.05 s at dt 0.1 | Rests one tick: the minimum |
| BA121 | 1 s, with the major road blocked for longer | Still held until the gap opens; moves off on the first tick it may, with no further dwell |
| BA122 | Behaviour by road | The behaviour assigned to the minor approach sets the dwell |
| BA123 | Codec, validation, editor | Schema 41 round-trips; 40 → `EDIT_UNSUPPORTED_FIELD`; 0, 61 and a string refused at the path; the dialog row writes it |
