# Braking onsets before a stop on NGSIM Lankershim (D666). Usage: python3 -I queue-approach-ngsim-onset.py lank.csv [smooth_s] [onset_acc]
import sys, numpy as np, pandas as pd
path = sys.argv[1]
SMOOTH = float(sys.argv[2]) if len(sys.argv) > 2 else 1.0   # centred moving-average window (s)
ONSET = float(sys.argv[3]) if len(sys.argv) > 3 else -0.3   # onset: last time before the stop the smoothed acc >= this
FT = 0.3048; DT = 0.1
STOP_V, FROM_V = 0.5, 5.0                                    # a stop: below 0.5 m/s, after being above 5 m/s
df = pd.read_csv(path)
df = df[df.v_class == 2]                                     # cars
df.sort_values(["vehicle_id", "global_time"], inplace=True)
# split a vehicle id into tracks at time gaps (the two periods reuse ids)
gap = (df.global_time.diff() != 100) | (df.vehicle_id.diff() != 0)
df["track"] = gap.cumsum()
w = max(1, int(round(SMOOTH / DT)))
df["v"] = df.groupby("track").v_vel.transform(lambda s: s.rolling(w, center=True, min_periods=1).mean()) * FT
df["a"] = df.groupby("track").v.transform(lambda s: s.diff().shift(-1).fillna(0) / DT)
# leader's smoothed speed and length at the same global_time
key = df.set_index(["vehicle_id", "global_time"])
lead_v = key.v; lead_len = key.v_length
idx = pd.MultiIndex.from_arrays([df.preceding.values, df.global_time.values])
df["lv"] = lead_v.reindex(idx).values
df["llen"] = lead_len.reindex(idx).values
rows = []
for tr, g in df.groupby("track"):
    v = g.v.values; a = g.a.values; n = len(v)
    i = 1
    while i < n:
        if v[i] < STOP_V <= v[i - 1]:
            j = i - 1
            while j > 0 and a[j - 1] < ONSET: j -= 1              # back to the start of the deceleration phase
            if v[j] >= FROM_V and g.int_id.values[i] == 0 and g.lane_id.values[i] > 0:
                d = float(np.sum(v[j:i]) * DT)
                if d > 1:
                    pre = g.preceding.values[i]
                    lv_stop = g.lv.values[i]; lv_on = g.lv.values[j]
                    hw = g.space_headway.values[i] * FT - (g.llen.values[i] * FT if pre else 0)
                    kind = "line" if pre == 0 else ("queue" if lv_on < STOP_V else ("joining" if lv_stop < STOP_V else "moving"))
                    rows.append(dict(kind=kind, v_on=v[j], d=d, req=v[j] ** 2 / (2 * d), peak=-a[j:i].min(),
                                     mean=(v[j] - v[i]) / ((i - j) * DT), dur=(i - j) * DT, rest_gap=hw if pre else np.nan))
            i += 1
        else:
            i += 1
r = pd.DataFrame(rows)
def q(s): return " / ".join(f"{s.quantile(p):.2f}" for p in (.1, .25, .5, .75, .9))
print(f"smooth {SMOOTH} s, onset acc >= {ONSET}; percentiles 10/25/50/75/90")
for k, g in r.groupby("kind"):
    print(f"{k:8s} n={len(g):4d} | v_on {q(g.v_on)} | d {q(g.d)} | req {q(g.req)} | peak {q(g.peak)} | mean {q(g['mean'])} | dur {q(g.dur)}"
          + (f" | rest gap {q(g.rest_gap.dropna())}" if g.rest_gap.notna().any() else ""))
r.to_csv(sys.argv[1].replace(".csv", f".events.{SMOOTH}.{ONSET}.csv"), index=False)
