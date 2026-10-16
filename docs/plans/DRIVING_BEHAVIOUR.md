# Driving behaviour delivery and acceptance

Owner authorized the staged improvement plan on 2026-10-06 (D120). P0
wrote the interface/capability/acceptance design first; subsequent measurement
slices add diagnostics and CLI settings without changing engine behavior or schema. Read [the contract](../reference/DRIVING_BEHAVIOUR.md),
[ROADMAP](../ROADMAP.md#m33--driving-behaviour-library-and-models) and
[NEXT](../NEXT.md) for authoritative scope, gates and live work.

## 1. Delivery slices

| Slice | System and result | Prerequisite / closing evidence |
|---|---|---|
| M3.3.0 (P0) | Assignment contract, capability map and acceptance design | Docs/navigation guards; baseline provenance; no runtime completion inference |
| M3.3.1a (P1) | [Discharge observer/CLI](../reference/DISCHARGE.md) delivered | BA01/BA02/BA04 focused evidence; existing StopLine output unchanged |
| M3.3.1b1 (P1) | Vehicle-type follower selection and declared CLI windows/ranks delivered | BA03 type/window evidence; no behavior classes or PCU inference |
| M3.3.1b2a (P1) | Captured input hashes and shared-prefix recognition delivered | [Provenance/passage evidence](../evidence/discharge-provenance.md); raw diverted sink inference suppressed |
| M3.3.1b2b1 (P1) | Proven lateral/insertion-tick passages reconstructed | [Passage evidence](../evidence/discharge-passage.md) |
| M3.3.1b2b2 (P1) | Rank-scoped remap invalidation and same-tick source-sink type identity delivered | [Identity evidence](../evidence/discharge-identity.md); BA05 focused evidence; native CI per PR |
| M3.3.2a (P2a) | Project library/class/behaviour-type contracts and codec delivered (D126) | [Library evidence](../evidence/behaviour-library.md): BA06–BA09 focused evidence; Run refuses assigned roads until 2b |
| M3.3.2b (P2b) | Compile assignment and select prototype per road/type delivered (D127) | [Selection evidence](../evidence/behaviour-selection.md): BA10–BA18 focused |
| M3.3.2c (P2c) | Library/assignment editor delivered (D128) | [Editor evidence](../evidence/behaviour-editor.md): BA19/BA20 automated Linux evidence; Windows CI per PR; owner visual review open |
| M3.3.3a (P3) | W74: [contract](../reference/W74.md) written (D129); pure function (D135), codec (D136), traits (D137) and composition (D138) delivered; BA27 recorded and its clamps traced | [Pure-function](../evidence/w74-pure-function.md), [codec](../evidence/w74-codec.md), [trait](../evidence/w74-traits.md) and [composition](../evidence/w74-composition.md) evidence: BA21–BA26, BA28, BA29 focused; BA27 [recorded](../evidence/w74-discharge.md), clamps [traced](../evidence/w74-clamps.md) (no W74 defect); cited preset [recorded](../evidence/w74-preset.md) (D176); no validation before code; analytic transition/replay and timestep cases; M0 observation remains independent |
| M3.3.3b (P3) | W99 | Own equations/state/traits contract before code, after M3.3.3a |

Later plan portions belong to their owning systems: signal behavior P4 to M4;
physical-mouth/anticipation P5 to M3.2.4e/M3.2.8a.1; extended lane changes P6 to
M3.2.8c; lateral infrastructure and motorcycle filtering P7 (now M3.7/M3.8, D177; perception M3.5, speed control M3.6, intersection extensions M3.9 — [review](../audits/DRIVING_BEHAVIOUR_REVIEW.md)) need numbered slices
and body-occupancy contracts before code. Presets P8 ship only after their used
capabilities pass. Batches/export P9 belong to M5; independent calibration P10
belongs to M6. Do not combine these into a single implementation session.

## 2. Measurement definition before implementation

`src/eval/stop_lines.*` currently measures crossing ticks, mean gaps within one
green, waiting and residual queues. This is not yet a steady-discharge estimator
or startup-lost-time result; never relabel the existing meanHeadway as saturation
headway. It sees tick snapshots, not exact interpolated crossing times.

The M3.3.1 contract must define head/lane identity, vehicle class, green cycle,
crossing timestamp policy and measurement/warm-up window. For the first fixture,
accept a declared crossing-record stream independently of simulation. Declare a
steady headway interval by queued vehicle rank before observing results. When
that interval lacks enough crossings or its queue is not sustained, return an
unavailable estimate with a reason; do not return zero or silently include free
arrivals. Several physical lanes must not interleave into one fictitious headway.

For declared steady mean headway h, the reported discharge-rate estimate is
3600/h vehicles/h for that lane/window, not a universal approach capacity.
Define startup lost time as the declared initial queue headways' sum minus their
count times h, with the first headway measured from effective Go to first crossing.
Report sign, rank range, sample count, time quantization and excluded cycles.
Negative startup estimates remain visible for diagnosis, not silently clamped.
Effective Go is green under today's D36; future red-amber behavior needs its own
definition. Confirm cycle-start queued membership with snapshots/explicit flags.

PCU conversion, empirical target ranges, warm-up rules for real studies and
confidence-interval precision need chosen sources/data. No universal 1,700–1,900
or 2,100–2,300 PCU/h gate follows from the owner's suggested parameter sheet.

## 3. Failure-first acceptance rows

BA01/BA02/BA04 have focused Linux evidence in [M3.3.1a](../evidence/discharge-measurement.md).
BA03 has [type/window evidence](../evidence/discharge-controls.md), with future class assignment deferred.
BA05 has [proven-motion](../evidence/discharge-passage.md) and [source-sink identity](../evidence/discharge-identity.md)
evidence. BA06–BA09 have [library/codec evidence](../evidence/behaviour-library.md). BA10–BA18 have
[selection evidence](../evidence/behaviour-selection.md).
BA19/BA20 have automated [editor evidence](../evidence/behaviour-editor.md); the owner's visual verdict stays open. BA21–BA29 are the [W74 contract](../reference/W74.md)'s rows (D129); BA21/BA22 have focused [pure-function evidence](../evidence/w74-pure-function.md) (D135), BA29 [codec evidence](../evidence/w74-codec.md) (D136), BA26 [trait evidence](../evidence/w74-traits.md) (D137), BA23–BA25/BA28 [composition evidence](../evidence/w74-composition.md) (D138), BA27 a [timestep record](../evidence/w74-discharge.md). BA30–BA38 are the [perception contract](../reference/PERCEPTION.md)'s rows (D179), BA30 and BA37 with [codec evidence](../evidence/perception-codec.md) (D181), BA31 and BA32 with [motion evidence](../evidence/perception-motion.md) (D182), BA33 and BA34 with [lane-change evidence](../evidence/perception-lane.md) (D183), BA35 and BA36 with [action-delay evidence](../evidence/perception-delay.md) (D184), BA38 with [reaction-time discharge evidence](../evidence/perception-discharge.md) (D185); BA39–BA46 the [lateral contract](../reference/LATERAL.md)'s (D180), BA45 with [codec and compile evidence](../evidence/lateral-codec.md) and BA39 kept in it (D187), BA40–BA42 with [span and predicate evidence](../evidence/lateral-spans.md) (D188), BA43–BA44 with [motion evidence](../evidence/lateral-motion.md) (D189), BA46 with [display evidence](../evidence/lateral-display.md) (D203), BA47 with [zone evidence](../evidence/lateral-zones.md) (D202); BA48–BA53 the [lane-change duration contract](../reference/LANE_CHANGE_DURATION.md)'s (D204), with [duration evidence](../evidence/lane-change-duration.md) (D206); BA54 with [gap evidence](../evidence/lane-change-gap.md) (D208). All later rows are pending. P0 writing a row does not pass it. Record exact fixture/input hashes, commit, compiler/flags, commands,
assertions and platform in evidence when the corresponding slice lands.

| Row | Fixture / forcing condition | Required outcome |
|---|---|---|
| BA01 | Queued lane: crossing times 3, 5.5, 7.5, 9.5, 11.5 s after Go; steady ranks 3–5 | h=2 s, discharge estimate=1800 veh/h; startup ranks 1–2 give 1.5 s; metadata identifies windows |
| BA02 | Empty/one-crossing window, interrupted queue, partial first/last green | Estimates unavailable with reasons; no division by zero or biased silent inclusion |
| BA03 | Two lanes, two types; boundary timestamps and warm-up exclusions | Independent lane headways; declared class filters; half-open windows with no duplicate records |
| BA04 | Same seeded run with and without measurement | Identical full trajectory/RNG/accounting; observer does not mutate state |
| BA05 | Copied snapshots, route changes, sinks, timestep variants | No duplicate/lost crossings; documented tick error; crossings at sink handled explicitly |
| BA06 | Duplicate/edit set with references from two roads | New independent ID; editing original affects only its declared users; one Undo restores everything |
| BA07 | Duplicate class membership, bad units/model/IDs, unused invalid set | Reject candidate/load atomically; preserve published document and revision |
| BA08 | Delete referenced set/class/type and explicit reassignment | Simple deletion rejected; validated reassignment is one transaction; Undo restores references |
| BA09 | Save/reopen old and new owned catalogs; unsupported future fields | Portable ownership; old schema/bytes preserved without new features; unsupported fields rejected |
| BA10 | Unassigned road, assigned default, assigned class override | Exact precedence; unclassified vehicle uses assigned default; missing IDs do not silently fallback |
| BA11 | Multi-lane/section-cut Link and derived Connector paths | Road assignments survive all compiler expansion; no route-specific disagreement on a shared segment |
| BA12 | Front just before, exactly at and just after a join; front/rear span two roads | Boundary convention holds; trailing length does not change selected front profile |
| BA13 | One tick crosses multiple/short roads and same-profile boundaries | Document selection lag; no new movement cap; identical-set fixture trajectory unchanged |
| BA14 | D119 source-zero, mid-Link and consecutive decision lines; alternate suffix | Select after actual prefix/route updates; no early behavior from unknown suffix; routing draws unchanged |
| BA15 | Mandatory/discretionary changer and distinct trailing profiles | Follower uses own physical location/set; recompute after accepted remap; preserve hard safety |
| BA16 | Source record blocked by first-step clearance; new profile at entry | Same sampled pending vehicle waits/releases; no resample/false departure; equality case covered |
| BA17 | Served Stop while profile changes; denied conflict and receiving queue | No reset at same served line; physical constraints compose; no suppressed genuine clamp |
| BA18 | Legacy prototype fixtures, immutable copy and same-build replay | Existing exact/tolerance contracts pass; baselines never regenerated to hide failures. M4.2 (D147) leaves them untouched: M0 scenarios keep amber as red |
| BA19 | Dialog cancel/no-op/edit, bulk road assignment, rename/delete | Atomic History and revision invalidation; effective/inherited values distinguishable; local changes not leaked |
| BA20 | EN/TH, save/reopen, Linux/Windows UI and owner appearance | Parameter names preserved; automated platform evidence separate from owner's visual verdict |
| BA21 | W74 pure function: inputs placed at each regime row of contract §4 and on both sides of every equality | Regime and acceleration match hand-computed values; `g = ABX` emergency, `dv = CLDV` following, `dv = OPDV`/`dv = SDV`/`g = DMAX` free |
| BA22 | Emergency inside `AX`, approaching below comfortable, free above `maxAcceleration` | Bounds hold exactly; type limits, not behaviour keys, cap acceleration |
| BA23 | Standing queue, its discharge when the head leaves, and a moving platoon held for 600 s | Oscillation stays inside `±bNull`; D105 keeps a stopped vehicle with `g ≤ ax` still; a queued vehicle inside `ABX` starts only once `g > ABX` or while its leader accelerates (emergency `dv²` and `aL` terms, recorded; D159); no clamp in an uncongested platoon |
| BA24 | Prototype→`w74`→prototype→`w74` along one route; front exactly at each join | Selection per D127; state initialised on entry, cleared on exit, re-initialised on re-entry; same-model boundary keeps state |
| BA25 | `w74` follower behind prototype leader and the reverse; static obstacle | Follower reads only gap/speed/acceleration; leader model never changes the result |
| BA26 | Trait hash: copied state, replayed run, two compilers, mixed scenario | Bit-identical traits; in a mixed scenario the `randomState` draw sequence is unchanged; a prototype-only scenario is byte-identical to the build without W74 |
| BA27 | Queue discharge fixture at dt 0.1/0.25/0.5 via `--discharge` | Sensitivity recorded with seeds 42–81; no claimed bound, no calibration inference |
| BA28 | Forced red, conflict hold, dead end, Stop and source D108 with a `w74` behaviour; vehicle braking to rest in the following band before a Stop line; courtesy/cooperative second obstacle winning and tying | Hard cap, counted clamps, Stop service and first-step rule unchanged; the standstill `s = +1` rule brings it within `stopLineReach` and the Stop is served; stored regime is the kept result's, the vehicle ahead on a tie; every `Leader` carries its acceleration (0 when static) |
| BA29 | Missing/extra/out-of-range keys, prototype keys on `w74`, `w74` in schema 21, unused invalid behaviour | Atomic rejection with path; schema 25 only when used (22 is M5.3's evaluation period, D132; 23 is M5.4's sections, D133; 24 is M5.5's control type, D134); older bytes preserved |
| BA30 | Every [perception](../reference/PERCEPTION.md) key absent (M3.5, D179) | CLI seed 42 and frozen baselines byte-identical; prototype-only and `w74` runs unchanged |
| BA31 | `lookAheadDistance` 100 m behind a standing queue first 150 m ahead (assert it really is beyond 100 m first) | Not a perceived leader until within 100 m; the physical cap still applies; any late stop is a counted clamp, never an overlap |
| BA32 | `observedVehiclesAhead` 2; the second leader brakes hard before the first | The follower brakes before the first leader does; with 1 the run equals the key-absent run; on equal accelerations the nearer leader's mode/regime is stored |
| BA33 | `lookBackDistance`: a target-lane follower just outside, then just inside | Outside: the change is made and the follower brakes or clamps (counted, no overlap); inside: today's rear test decides; something alongside always refuses |
| BA34 | `rearTimeToCollision` with a closing follower below, at and above the key | Below refuses; equal and above accept; a follower not closing passes |
| BA35 | `reactionTime` 1.0 s at dt 0.1, then at dt 0.25 and 0.5 | First deceleration exactly 10 ticks after the leader's; `d = floor(τ/dt + 0.5)`; a copied `SimState` with full FIFOs replays exactly; insertion starts after `d` ticks |
| BA36 | `reactionTime` with forced red, Stop, dead end, D108 source, zone hold, courtesy, a road boundary changing `d`, and a Lab retune | Hard cap, counted clamps, Stop service and admission unchanged; hypothetical tests undelayed; FIFO trimmed/padded as specified; a retune changing `d` refused |
| BA37 | Codec: non-number, out-of-range and non-integer values, a key in an older schema, an unused invalid behaviour, external catalog | Atomic rejection with path; the new schema only when used; older bytes preserved |
| BA38 | Four-leg discharge with `reactionTime` 0 / 0.5 / 1.0 s, seeds 42–81, dt 0.1 | Headway, startup lost time and clamps recorded; no bound, no calibration inference |
| BA39 | Every [lateral](../reference/LATERAL.md) key absent (M3.7, D180) | Scenario equal to today's; CLI seed 42, frozen baselines, prototype, `w74` and perception runs byte-identical |
| BA40 | A body overlapping the adjacent lane by just over, then exactly, `kLateralOverlap` (assert the overlap first) | Over: a span on that lane, and it is the leader of that lane's follower; at or under: no span there |
| BA41 | Two narrow bodies side by side in one wide lane, lateral clearance above, at and below `dLat(v)` | Above: neither is the other's leader; at (touching the widened body) and below: the rear one follows; no 2D overlap at any published tick |
| BA42 | `dLat` at 0, 25, 50 and 70 km/h | Exact interpolation; constant at and above 50 km/h |
| BA43 | Two lateral moves into the same space in one tick; a move into a published body | The lower vehicle id is admitted, the other keeps its `lateral` with `lateralSpeed` 0; replay of a copied `SimState` exact |
| BA44 | A target across the lane boundary with and without a `LaneChangeSpan`; a target past the carriageway edge | With a span: route remap at the crossing and `lateral` re-expressed; without: clamped inside the lane; never past the edge |
| BA45 | Codec and compile: missing one of the three keys, out-of-range values, an older schema, an unused invalid behaviour; left- and right-hand networks | Atomic rejection with path; cross-sections and `drivingSide` compiled only when switched on; lane centres equal `laneGeometry`'s offset |
| BA46 | Display of a body with `lateral` ≠ 0 on both driving sides | Drawn at lane centre + `lateral`; physics never reads display data |
| BA47 | A straddler's body over the next lane's conflict area (major side, minor side), approaching it on the chain, sweeping into it in one tick; covering no more than `kLateralOverlap`; the switch off (assert the other-lane span first) | Major: the minor approach waits at its line, is anticipated by headway/gapTime, a request is capped; minor: the major vehicle waits at entry; within the tolerance or switched off: route positions alone decide |
| BA48 | Every lateral key absent, a lane-change project (`lane-change-lab`) | Changes stay instantaneous; CLI seed 42, frozen baselines and project runs byte-identical |
| BA49 | Switched on, `maxLateralSpeed` 1, a 3.5 m lane, a mandatory change (assert it was accepted first) | The route does not remap at acceptance; the move takes ≥ 35 ticks at dt 0.1; one `LaneChangeEvent`, at the crossing; ends centred with `lateralTarget` cleared |
| BA50 | During that change, a follower on each lane | Each treats the changer as its leader while the body covers its lane; no 2D overlap at any tick |
| BA51 | The next zone's line just under, then at, `travel` ahead on either route | Under: the change does not start; at: it starts; it never straddles into a zone |
| BA52 | A change whose crossing would fall past the span's `fromEnd` | It does not start; a vehicle with `lateralTarget` set starts no second change |
| BA53 | The target lane's space taken mid-change | The move is refused, `lateralSpeed` 0, `lateral` kept; it resumes when free; display draws the engine position (D203) |
| BA54 | Mid-change, a vehicle on the target lane alongside, close ahead or close behind; a body already over the line with one ahead (D208) | The body does not cover that lane until rules 3–5 hold there; over the line, it steps back; no clamp, no braking beyond comfortable, no 2D overlap; a far leader does not slow the move |

Future W74/W99 rows must force each transition/limit, mixed-model following,
profile/model boundary state transfer, distribution sampling and physical safety.
Future lane/lateral/conflict rows must check occupancy during the maneuver and
rear clearance, not just final positions or rendered appearance.

## 4. Baseline and integration recipe

Record the clean base commit, source/catalog hashes, compiler/version/flags,
timestep and seed list before experiment code. Compare whole outputs for the
four supplied projects and T-junction stress fixture when changing engine motion;
use at least seeds 42–81 for movement comparisons (D88), retain gridlocked and
unfinished seeds, and record generated/completed/active/pending/clamps.
This seed count is a development rule, not statistical sufficiency for all studies.

Prototype baseline fixtures stay frozen. Validate no-feature default outputs
before enabling assignment. Add analytic estimators/forced runtime cases before
implementation, then perform relevant native/Qt checks. Benchmark any new
neighbor search/table/state cost on the same toolchain; no unmeasured performance
claim. Use documented [BUILDING](../BUILDING.md) commands and report missing tools
honestly. Headless checks do not establish desktop appearance or Windows behavior.

## 5. Scope and validation limits

The target does not authorize vehicle deletion at a diffusion timeout, geometry
distortion to hit throughput, 3D or articulated dynamics. Scalar clearance is
not swept-body evidence. New presets remain experimental until their capability
and validation gates pass. Keep the not-yet-validated marker and disclose that
Vissim-calibrated parameter values need independent recalibration here.

P0 is documentation delivery only; P1–P10 are subsequent systems. This does not
close M0, M2/M3 owner exercises, M6 or the existing owner review queue.
