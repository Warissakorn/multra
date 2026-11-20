# Realism and efficiency gaps against Vissim, SUMO and Aimsun

The owner's request, 2026-11-20 (verbatim): "ศึกษาส่วนอื่นที่ต้องเพิ่มอย่างละเอียด เพื่อให้ ฐาน M0 สมบูรที่สุดด้วย" and
"ลองศึกษาจาก Vissim SUMO Aimsun มาต่อยอดเพื่อให้พฤติกรรมสมจริงและมีประสิทธิภาพมากที่สุด". Read from vendors' public help,
agency calibration guides and the SUMO mailing list on 2026-11-20; nothing was run in another program.
A row marked *unverified* was named in a source but its meaning was not confirmed. No verdict, no code:
§4 is a proposed order for the owner.

## 1. What MULTRA already has (do not rebuild)

From [BEHAVIOUR_PARAMETERS](../reference/BEHAVIOUR_PARAMETERS.md) and the contracts it cites:
prototype and W74 car-following; cooperative braking (D90); mandatory and discretionary lane changes
with accepted decelerations (D95/D101, D221); kerb-lane bias; amber stop-or-go and red running (D147,
D238); give-way commitment (D219), Stop dwell (D231), visibility (D607); perception keys including
look-ahead, observed vehicles and an action delay `reactionTime` (0–2 s); lateral occupancy; curve
speed; desired-speed decisions, areas and distribution curves; lane preference by type; driver-factor
spread; lane locks at dead ends (D578–D582).

## 2. M0's own evidence: the one figure off the reference

[m0-plausibility](m0-plausibility.md) measures the bare M0 crossing: saturation headway 1.83 s
(1,967 veh/h) against HCM's base 1.9 s, but a **start-up lost time of 0.89 s** against HCM's default
2.0 s. Queued vehicles move off too soon after green. This is the M0 gate's own metric, so it is the
first gap below.

## 3. Gaps, by what a study would see

| # | Gap | Vissim | SUMO | Aimsun | MULTRA today | M0 scope? |
|---|---|---|---|---|---|---|
| G1 | **Start-up delay after a stop** | Through W74/W99 standstill behaviour (not read in detail) | vType `startupDelay`: a fixed extra delay when starting after having stopped, e.g. at a light | *Reaction time at traffic light*: added to the first stopped vehicle's reaction when green starts; default 1.6 s, 2.8 s suggested for urban ~50 km/h | `reactionTime` delays every action, moving or not; nothing specific to moving off; M0's lost time 0.89 s | **Yes** (discharge at green) |
| G2 | **Acceleration that falls with speed** | Desired/maximum acceleration as functions of speed; W74 calibration names acceleration from standstill and at 80 km/h (TxDOT) | Model-dependent (not read) | Gipps (manual), whose acceleration term falls as speed nears the desired speed | One `maxAcceleration` per type at every speed | **Yes** (acceleration from the line; heavy vehicles) |
| G3 | **Stuck vehicles** | *Waiting time before diffusion*: a vehicle waiting at the emergency stop position for a lane change longer than this (60 s on arterials, Louisiana DOTD) is removed, with a warning | `time-to-teleport` *unverified* | Not read | `missedExitAfter` (D581) at dead ends only; any other gridlock stays to the run's end | No (lane changes are outside M0) |
| G4 | **Safety distance while changing lanes** | *Safety distance reduction factor*: 0.6 by default during a change, restored after | `lcAssertive` *unverified* | Gap acceptance and cooperation (manual headings, values not read) | Accepted decelerations only; the full desired gap applies | No |
| G5 | **Cooperative lane change** (moving over to let a merging vehicle in) | Cooperative lane change; cooperative braking up to 50 % of desired deceleration | `lcCooperative` (speed adaptation; 0 or −1 none, 1 optimal) | Cooperation in target-gap choice | Cooperative braking only (D90) | No |
| G6 | **Strategic look-ahead for lane changes** | Lane-change distance per connector | `lcStrategic` (default 1.0; higher changes earlier; −1 off) | Look-ahead zones (headings only) | Routing decisions and lane-change spans; no per-driver earliness | No |
| G7 | **Impatience at give-way** | Not in the pages read | `impatience` | Maximum give-way time | None | No |
| G8 | **Stop dwell distribution** | Dwell-time distribution on stop signs | `jmStopSignWait` (fixed) | Not read | Fixed `stopDwellTime` | No |
| G9 | **Motorway car-following** | W99 | IDM and others (not read) | Gipps | W99 does not exist; queued as M3.3.3b | No |

**Efficiency.** None of the pages read gives another program's run-time design in enough detail to
copy. MULTRA's own measured lever stands: `closestVehicle` is 15.7 % of the oversaturated study, and an
order kept across ticks is untried ([ENGINEERING_NOTES](../reference/ENGINEERING_NOTES.md)). A
behaviour change above is costed by callgrind before and after, as the 2026-11-19 pass did.

**Since (2026-11-20):** G1 and G2 are D608 ([START_UP](../reference/START_UP.md)), as Vissim, absent by default.

## 4. Proposed order (the owner's choice; each needs a contract and failure-first rows)

1. **G1 start-up delay** — closes M0's one off-reference figure. A behaviour key, absent by default
   (rule 5), first-mover only or every stopped vehicle as SUMO, measured against HCM's 2.0 s.
   Changing shipped data to use it would move the M0 baseline and reopen M0 (D369); the key alone does not.
2. **G2 speed-dependent acceleration** — a curve on the vehicle type, like `desiredSpeed.curve`.
3. **G3 stuck vehicles**, then **G5/G4** with M3.4's zipper, **G7/G8** with give-way work,
   **G9** as M3.3.3b, **G6** when a study shows late changes.

## Sources

- PTV Vissim help: [Lane change behavior](https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/4_BasisdatenSim/FahrverhaltensparameterFahrstreifenwechsel_bearb.htm);
  [TxDOT Vissim driving behavior](https://www.txdot.gov/manuals/des/tsp/chapter-13-microsimulation-analysis/13-5-calibration/13-5-3-calibration-parameters-and-best-practices/13-5-3-1-vissim-calibration/13-5-3-1-3-driving-behavior.html);
  [Louisiana DOTD Vissim calibration](https://dotd.la.gov/media/ggmgprf1/vissim-calibration.pdf).
- SUMO: [vehicle types](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html),
  [LC2013 source](https://sumo.dlr.de/doxygen/db/d77/_m_s_l_c_m___l_c2013_8cpp_source.html),
  mailing list on [`startupDelay`](https://www.eclipse.org/lists/sumo-user/msg13087.html) and
  [LC2013 parameters](https://www.eclipse.org/lists/sumo-user/msg11914.html).
- Aimsun: [vehicle movement (Gipps)](https://docs.aimsun.com/next/latest/UsersManual/MicrosimulationModellingVehicleMovement.html),
  an Aimsun tech note found by search for *reaction time at traffic light* (its page was not recorded;
  re-check before citing a number), [control plan editing](https://docs.aimsun.com/next/24.0.1/UsersManual/ControlPlanEditing.html),
  [Levitate report (UWE)](https://uwe-repository.worktribe.com/OutputFile/12111859).
