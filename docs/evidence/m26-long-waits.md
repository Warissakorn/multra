# The long m2.6 lane-change waits — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `dfeaeaf` (D448), no code change. Local host: Linux,
GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only.** Switched on as in the
[lateral-run audit](lateral-run-audit.md): keys 0.5/1.0/1 on the catalog behaviour of a copied
`data/`. Follows NEXT's lateral residue. Nothing here is validated (rule 4).

## Question

After D444–D448, the change-duration probe (target set → crossing, switched on) still showed waits
of up to 91 s on `m2.6-study-template`: seed 43 vehicle 1198, seed 47 vehicle 1199, seed 49 vehicle
534. Are they a lateral-model defect, or congestion that exists switched off too?

## Method (scratch probes, not committed)

- **`stubwait`** measures a time both runs share: from a vehicle's first tick on a route that still
  needs a change (`remainingOfRoute > 0`) to the tick its route changes. Seeds 42–51, off and on.
  Its arrival totals equal the CLI batches (off 19,595, on 19,582), as a cross-check.
- **`refusal`** classifies, each tick from the start to the crossing, why the changer has not
  crossed, by replicating the start's tests with public functions:
  - **no window:** no span holds the body (`spanHolds`);
  - **target place occupied:** a vehicle is alongside the mapped place;
  - **leader** or **follower on target lane too close:** that vehicle fails `acceptsAsLeader`;
  - **target clear.**

  It also records whether the changer is moving (> 2 m/s) or queued.

## Stub waits, all seeds, off → on

| Route | n | Mean (s) | p95 (s) | Max (s) | Over 30 s |
|---|---|---|---|---|---|
| All | 3000 → 2999 | 4.55 → 6.58 | 16.0 → 18.5 | 110.9 → 119.1 | 61 → 74 |
| `link-1/path-4` | 546 → 546 | 6.67 → 8.88 | 33.9 → 39.1 | 105.6 → 109.7 | 30 → 33 |
| `link-13/path-4` | 569 → 568 | 5.91 → 8.59 | 22.7 → 37.9 | 110.9 → 119.1 | 24 → 32 |
| `link-1/path-5` | 475 | 4.35 → 6.22 | 17.2 → 19.6 | 42.2 → 50.9 | 2 → 4 |
| `link-13/path-5` | 456 | 3.83 → 5.63 | 15.0 → 17.3 | 32.3 → 34.0 | 1 → 1 |
| `link-25/path-4/5`, `link-37/path-4/5` | 954 | 2.7–3.2 → 4.4–4.9 | 5.3–11.1 → 7.0–12.8 | ≤ 65.6 → ≤ 67.3 | 4 → 4 |

- **The long waits exist switched off.** The maximum is 110.9 s and 61 waits exceed 30 s, 54 of them
  on the two `path-4` approaches. They are congestion, not a lateral defect.
- **Switching on adds about 1.7–2.7 s to the mean on every route.** That is the physical crossing time:
  the centre must travel 1.75 m at 1 m/s. Switched off, the change is instantaneous.
- **The tail grows on the two `path-4` approaches:** 13 more waits over 30 s in total. On
  `link-13/path-4` the p95 rises from 22.7 to 37.9 s.

## The three long waits, by phase (switched on)

| Seed / vehicle | Moving: leader on target lane too close | Moving: occupied / no window / clear | Queued: target place occupied | Queued: no window | Other |
|---|---|---|---|---|---|
| 43 / 1198 | 18.4 s | 3.4 / 3.5 / 0.6 s | **65.3 s** | 0.8 s | 1.2 s |
| 47 / 1199 | 4.2 s | 5.2 / 4.2 / 0.7 s | **74.0 s** | 0.7 s | 1.1 s |
| 49 / 534 | 7.8 s | 4.2 / 4.9 / 1.3 s | **21.1 s** | **39.7 s** | 0.3 s |

- **Same course in each.** The change starts upstream at 7–14 m/s. About 1 s later the body reaches
  the line, and the cover is refused while moving, mostly because the target lane's leader is too
  close. The vehicle passes the first window's end (220 m) and the 20 m Connector stretch without a
  span (220–240 m), then joins its own lane's queue behind a red signal.
- **Most of the wait is that queue.** The target lane alongside is a standing queue too, so the
  target place stays occupied until green.
- **534's "no window" (39.7 s) is the same queue.** It creeps from 240 m to 244.5 m, where its rear
  enters the window [240, 277]; the switched-off start uses the same window rule (`spanHolds`).

## Finding

- **Congestion, not a defect.** The long m2.6 waits are signal-queue congestion that exists switched
  off too. None comes from a disagreement between two tests, the pattern D444–D448 fixed.
- **What lateral adds:** the crossing time (≈ 2 s per change), plus a modest tail on the `path-4`
  approaches. There, a change that would be instantaneous switched off is refused for a few seconds
  while moving, and the vehicle then ends up in the red queue.
- **Gap seeking would reach only the moving phase** (4–18 s of these waits), not the queue. Whether
  to add it is the owner's choice; this measurement does not require it.
