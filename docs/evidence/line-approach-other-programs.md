# Approaching and stopping at a give-way line: Vissim, SUMO, Aimsun and MULTRA

The owner's question for OWNER_SITTING A2 (M3.2.7d attempt 2), 2026-11-20 (verbatim): "ตรวจสอบว่าการวิ่งมาหยุดที่เส้น
มีรายละเอียดมากกว่านี้หรือไม่ ลองศึกษาจากโปรแกรมอื่น". Read from the vendors' public help and the SUMO
documentation and mailing list on 2026-11-20; nothing here was measured, and no model was run. Where a
source is a forum answer rather than documentation, the row says so. No verdict and no code: the
choices at the end are the owner's.

## 1. What each program does

| Aspect | Vissim (PTV help, 2020–2025) | SUMO (docs, mailing list) | Aimsun Next (tech notes) | MULTRA today |
|---|---|---|---|---|
| **Where the front stops** | A yielding vehicle "plans to stop as close as possible" to an imaginary stop line just upstream of the conflict area | Stop-line offset 1 m by default, vType `jmStoplineGap` (a 2020 mailing-list answer, for signals; not checked in the reference) | Not found | Front at the line (D596); a derived line stands `frontStopDistance` (1 m) short of the area's entry |
| **Moving the line** | *Additional stop distance* per conflict area, default 0 m | `jmStoplineGap` per vehicle type | Not found | One `frontStopDistance` per project, or a placed stop line anywhere (D595) |
| **Approach** | The minor vehicle recalculates every time step whether it can filter in; if the gap is too small it decelerates as if it had to stop before the area | Vehicles slow down until they can see foe traffic (next row) | Gap acceptance applies once within the visibility distance | Recalculated every tick (`zoneHold`); held like a red head, so it slows only when the gap test fails |
| **Visibility** | *Visibility link A/B* per conflict area: the furthest an approaching vehicle can "see" vehicles on the other link (older help: up to 100 m) | "Clear visibility of foe traffic" only 4.5 m before the stop line by default, "which will make them slow down before that point" (developer's answer); a junction's `visibility` raises it | *Visibility distance*: a give-way vehicle starts gap acceptance when this close to the section end | **None.** The test sees every major vehicle on its route at any distance; a minor vehicle with a clear gap keeps its speed through the line |
| **Stop sign** | A stop sign holds a vehicle at least one time step; optionally a dwell-time distribution; a regular stop sign is placed at the same position as a priority rule's stop line | Minimum stop `jmStopSignWait`, default one simulation step (development build after 1.20.0) | Stops completely, then applies gap acceptance | Stop serves one complete tick at the line; optional fixed `stopDwellTime` per driver behaviour (D231), no distribution |
| **Gap conditions** | Front gap (default 0.5 s), rear gap, safety distance factor on merges (default 1.5) | `jmTimegapMinor`, default 1 s ("no calibration effort" behind it, per the developer) | Give-way time and safety margins (meso note: initial safety margin 10 s) | `gapTime` and `headway` per area; a standing major vehicle beyond the headway does not block |
| **Not blocking the area** | *Avoid blocking*, on by default: a yielding vehicle does not enter or stop inside unless it can drive through in one go | Not read | Not found | Chain admission and receiving space (D66): a turn with nowhere to go waits at its line |
| **Waiting longer, accepting less** | Not in the pages read | `impatience` makes a waiting driver more aggressive when merging from a minor road | Maximum give-way time: after waiting long enough the vehicle goes (meso note) | **None:** the same gap test however long the wait |

## 2. What MULTRA lacks, in order of what the owner would see at the line

1. **Slowing on the approach to a give-way with nothing coming.** In all three programs a yielding
   vehicle cannot judge the major road until it is close (a visibility distance), so it approaches at
   a speed from which it can still stop at the line. In MULTRA a minor vehicle whose gap is clear
   keeps its speed and crosses the line without slowing, and one whose gap closes late must brake from
   full speed. This is the largest visible difference.
2. **A stop-sign dwell that varies by driver.** Vissim draws the dwell from a distribution; MULTRA's
   Stop is one tick or one fixed `stopDwellTime` for every driver of a behaviour.
3. **Impatience.** A long wait in MULTRA never lowers the accepted gap; SUMO and Aimsun let it.
4. **A stop offset per area.** Vissim's *additional stop distance* is per area; MULTRA covers this
   with a placed stop line (D595), so nothing is missing in practice.

## 3. Choices for the owner (none is booked)

- **V (visibility and approach speed):** a visibility distance per conflict area (and per priority
  rule marker, PRIORITY_RULES) inside which the minor vehicle sees major traffic; beyond it, the
  vehicle drives so it can still stop at the line, as if the line were held. Needs a contract with a
  default (Vissim's 100 m and SUMO's 4.5 m are far apart; no source in the repository yet), rows,
  and a measurement of delay and capacity before and after, since it raises minor delay.
- **D (dwell distribution):** `stopDwellTime` drawn per vehicle from a distribution, seeded like the
  driver factor.
- **I (impatience):** the accepted gap time falls with time waited, to a floor; needs a cited source
  for the rate before any default.
- **None:** keep D596's rule; the attempt 2 verdict is given on today's behaviour.

## Sources

- PTV Vissim help: [Using conflict areas](https://cgi.ptvgroup.com/vision-help/VISSIM_2024_ENG/Content/5_Netzbearbeiten/Konfliktflaechen_modellieren.htm),
  [Attributes of conflict areas](https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/5_Netzbearbeiten/Konfliktflaechen_Attr.htm),
  [Modeling stop signs](https://cgi.ptvgroup.com/vision-help/VISSIM_2023_ENG/Content/5_Netzbearbeiten/StoppschilderundAbf_mod.htm),
  [Attributes of stop signs](https://cgi.ptvgroup.com/vision-help/VISSIM_2023_ENG/Content/5_Netzbearbeiten/StoppschilderundAbf_Attr.htm).
- SUMO: [Intersections](https://sumo.dlr.de/docs/Simulation/Intersections.html),
  [Traffic Lights](https://sumo.dlr.de/docs/Simulation/Traffic_Lights.html),
  [Safety](https://sumo.dlr.de/docs/Simulation/Safety.html); mailing list on
  [visibility and `jmTimegapMinor`](https://www.eclipse.org/lists/sumo-user/msg01726.html) and
  [`jmStopSignWait`](https://www.eclipse.org/lists/sumo-user/msg14192.html).
- Aimsun tech notes [hybrid micro–meso](https://www.aimsun.com/tech_notes/hybrid-micro-meso/) and
  [hybrid micro–meso 2](https://www.aimsun.com/tech_notes/hybrid-micro-meso-2/): give way against stop,
  the visibility distance and the give-way time factor (which note holds which was not recorded).
