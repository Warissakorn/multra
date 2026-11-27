# Where the first car of a queue rests relative to the intersection boundary (int_id 0 -> k) on NGSIM Lankershim.
# Usage: python3 -I ngsim_restline.py lank.csv
import sys, numpy as np, pandas as pd
FT = 0.3048
df = pd.read_csv(sys.argv[1])
df.sort_values(["vehicle_id", "global_time"], inplace=True)
df["track"] = ((df.global_time.diff() != 100) | (df.vehicle_id.diff() != 0)).cumsum()
df["v"] = df.groupby("track").v_vel.transform(lambda s: s.rolling(10, center=True, min_periods=1).mean()) * FT
df["y"] = df.local_y * FT
# 1. boundary per (direction, lane, intersection): where tracks first enter int_id k from int_id 0
prev_int = df.groupby("track").int_id.shift(1); prev_y = df.groupby("track").y.shift(1); prev_lane = df.groupby("track").lane_id.shift(1)
ent = df[(prev_int == 0) & (df.int_id > 0) & (prev_lane > 0)].copy()
ent["yb"] = (ent.y + prev_y[ent.index]) / 2; ent["lane"] = prev_lane[ent.index]
b = ent.groupby(["direction", "lane", "int_id"]).yb.agg(["median", "count", lambda s: s.quantile(.9) - s.quantile(.1)])
b.columns = ["yb", "n", "spread90"]; b = b[b.n >= 30]
print("boundaries (m along local_y): n, median, p90-p10 spread"); print(b.round(2).to_string())
# position of every vehicle per time, to find who is ahead
key = df.set_index(["vehicle_id", "global_time"])
rows = []
for tr, g in df[df.v_class == 2].groupby("track"):
    v = g.v.values; ii = g.int_id.values; n = len(v)
    for i in range(1, n):
        if not (v[i] < 0.5 <= v[i - 1]) or v[max(0, i - 80):i].max() < 5: continue
        nxt = ii[i:][ii[i:] > 0]
        if ii[i] == 0 and len(nxt) == 0: continue
        k = ii[i] if ii[i] > 0 else nxt[0]
        if ii[i] > 0 and ii[i - 1] == 0: pass
        d = g.direction.values[i]; lane = g.lane_id.values[i]
        if lane <= 0 and ii[i] == 0: continue
        if ii[i] > 0:  # stopped already past the boundary: use the lane it came in on
            lanes = g.lane_id.values[:i][g.int_id.values[:i] == 0]
            if len(lanes) == 0: continue
            lane = lanes[-1]
        if (d, lane, k) not in b.index: continue
        sgn = 1 if d in (2,) else -1 if d in (4,) else 0
        if sgn == 0:
            # infer travel sense from the track itself
            yy = g.y.values; sgn = 1 if yy[i] > yy[max(0, i - 50)] else -1
        yb = b.loc[(d, lane, k), "yb"]; gap = sgn * (yb - g.y.values[i])   # >0: front short of the boundary
        if gap > 15: continue          # too far back to be first in queue at the line
        pre = g.preceding.values[i]; first = True
        if pre:
            t = g.global_time.values[i]
            if (pre, t) in key.index:
                py = key.loc[(pre, t), "y"] * FT / FT if False else key.loc[(pre, t), "y"]
                if sgn * (yb - py) > -2: first = False   # a leader still upstream of / at the boundary
        if first: rows.append(dict(direction=d, lane=lane, int_id=k, gap=gap))
r = pd.DataFrame(rows)
q = lambda s: " / ".join(f"{s.quantile(p):.2f}" for p in (.1, .25, .5, .75, .9))
print(f"\nfirst cars at rest near a boundary: n={len(r)}; gap (m, >0 short) p10/25/50/75/90: {q(r.gap)}")
print(f"share past the boundary (gap<0): {(r.gap < 0).mean():.0%}; within 0-1 m: {((r.gap>=0)&(r.gap<1)).mean():.0%}")
for (d), g in r.groupby("direction"): print(f"direction {d}: n={len(g)} median {g.gap.median():.2f}  {q(g.gap)}")
r.to_csv(sys.argv[1].replace(".csv", ".restline.csv"), index=False)
