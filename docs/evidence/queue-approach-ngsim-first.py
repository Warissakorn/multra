# The first deceleration episode toward a vehicle at rest, NGSIM Lankershim (D666). Usage: python3 -I queue-approach-ngsim-first.py lank.csv
import sys, numpy as np, pandas as pd
FT, DT, ONSET, STOP_V = 0.3048, 0.1, -0.3, 0.5
df = pd.read_csv(sys.argv[1]); df = df[df.v_class == 2].sort_values(["vehicle_id", "global_time"])
df["track"] = ((df.global_time.diff() != 100) | (df.vehicle_id.diff() != 0)).cumsum()
df["v"] = df.groupby("track").v_vel.transform(lambda s: s.rolling(10, center=True, min_periods=1).mean()) * FT
df["a"] = df.groupby("track").v.transform(lambda s: s.diff().shift(-1).fillna(0) / DT)
key = df.set_index(["vehicle_id", "global_time"])
idx = pd.MultiIndex.from_arrays([df.preceding.values, df.global_time.values])
df["lv"] = key.v.reindex(idx).values; df["llen"] = key.v_length.reindex(idx).values
out = []
for tr, g in df.groupby("track"):
    v, a, lv = g.v.values, g.a.values, g.lv.values
    pre, hw, ll, iid, lane = g.preceding.values, g.space_headway.values * FT, g.llen.values * FT, g.int_id.values, g.lane_id.values
    i, n = 1, len(v)
    while i < n:
        if a[i] < ONSET <= a[i - 1] and v[i] >= 5 and pre[i] and lv[i] < STOP_V and iid[i] == 0 and lane[i] > 0:
            k = i
            while k < n - 1 and a[k] < ONSET: k += 1
            gap = hw[i] - ll[i] - 2.8                        # to the leader's rear less the median rest gap measured
            stopped = v[i:k + 1].min() < STOP_V
            moved = np.nanmax(lv[i:k + 1]) >= 1.0 if not np.all(np.isnan(lv[i:k + 1])) else False
            # within 30 s, does it stop behind the same leader, and does that leader stay at rest until then?
            end = min(n, i + 300); later = None
            for t in range(i, end):
                if pre[t] != pre[i]: break
                if v[t] < STOP_V: later = t; break
            still = later is not None and np.nanmax(lv[i:later + 1]) < 1.0
            if gap > 1: out.append(dict(stopped=stopped, leader_moved=moved, v_on=v[i], req=v[i] ** 2 / (2 * gap), vmin=v[i:k + 1].min(), tr=tr, leader=pre[i], stop_at=(later if later is not None else -1),
                                        later=('stop, leader still' if still else 'stop, leader moved' if later is not None else 'no stop')))
            i = k
        i += 1
r = pd.DataFrame(out)
first = r[r.later == 'stop, leader still'].drop_duplicates(["tr", "leader", "stop_at"], keep="first")
print("approaches ending at rest behind a leader at rest throughout (first deceleration episode):", len(first))
print("  req at first onset 10/25/50/75/90 " + "/".join(f"{first.req.quantile(p):.2f}" for p in (.1,.25,.5,.75,.9)) + f" | v_on med {first.v_on.median():.1f}")
print("  share with req < 0.8: %.0f%%, < 1.0: %.0f%%, >= 2.0: %.0f%%" % (100*(first.req<0.8).mean(), 100*(first.req<1).mean(), 100*(first.req>=2).mean()))
print("episodes behind a standing leader:", len(r))
for l, g in r.groupby("later"):
    print(f"30 s outcome: {l:20s} n={len(g)} | req at onset 10/25/50/75/90 " + "/".join(f"{g.req.quantile(p):.2f}" for p in (.1,.25,.5,.75,.9)) + f" | v_on med {g.v_on.median():.1f}")
for (s, m), g in r.groupby(["stopped", "leader_moved"]):
    print(f"stopped={s} leader_moved={m} n={len(g)} | req 10/50/90 {g.req.quantile(.1):.2f}/{g.req.median():.2f}/{g.req.quantile(.9):.2f} | v_on med {g.v_on.median():.1f} | vmin med {g.vmin.median():.1f}")
