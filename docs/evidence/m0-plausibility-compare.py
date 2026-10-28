# M0 plausibility evidence (D369): the same metrics from MULTRA traces and SUMO FCD output.
# Usage: python3 -I m0-plausibility-compare.py DIR, with DIR holding tr_<seed>.csv (MULTRA),
# fcd_<Krauss|IDM>_<seed>.xml and ti_<Krauss|IDM>_<seed>.xml (SUMO) for seeds 42-51.
import csv, sys, statistics as st, collections, xml.etree.ElementTree as ET
D = sys.argv[1]
SEEDS = range(42, 52)
LINE = 148.0                                  # crossing.json head position on its 150 m Link
LANE = {'west_0': 150.0, 'south_0': 146.5}    # SUMO approach lane lengths (net.xml)

def phase(app, t):
    c = t % 64
    if app == 'EW': return 'G' if 32 <= c < 56 else 'A' if 56 <= c < 59 else 'R'
    return 'G' if c < 24 else 'A' if c < 27 else 'R'

def multra(s):  # rows: (t, approach, distance to line, speed, acceleration)
    rec = collections.defaultdict(list)
    for r in csv.DictReader(open(f'{D}/tr_{s}.csv')):
        rec[r['id']].append((float(r['t']), 'EW' if r['route'] == '0' else 'NS',
                             LINE - float(r['dist']), float(r['speed']), float(r['acc'])))
    return rec

def sumo(m, s):
    rec = collections.defaultdict(list)
    for ts in ET.parse(f'{D}/fcd_{m}_{s}.xml').getroot():
        t = float(ts.get('time'))
        for v in ts:
            lane = v.get('lane')
            rec[v.get('id')].append((t, 'EW' if v.get('id').startswith('w') else 'NS',
                                     LANE[lane] - float(v.get('pos')) if lane in LANE else -1.0,
                                     float(v.get('speed')), float(v.get('acceleration'))))
    return rec

def metrics(rec):
    out = collections.defaultdict(list); greens = collections.defaultdict(list)
    out['clamp'] = [0]; out['full'] = [0]; out['amber'] = [0]; out['red'] = [0]
    for tr in rec.values():
        tr.sort()
        out['amax'].append(max(x[4] for x in tr))
        out['clamp'][0] += sum(x[4] < -8.05 for x in tr)
        out['full'][0] += sum(-8.05 <= x[4] < -7.9 for x in tr)
        cr = next((x for x in tr if x[2] < 0), None)
        if not cr: continue
        ph = phase(cr[1], cr[0])
        if ph == 'R': out['red'][0] += 1
        if ph == 'A': out['amber'][0] += 1
        if ph != 'G': continue
        queued = any(x[3] < 0.5 and 0 <= x[2] < 80 for x in tr if x[0] < cr[0])
        start = 32 if cr[1] == 'EW' else 0
        greens[(cr[1], (cr[0] - start) // 64)].append((cr[0], queued, start))
    for (app, k), crossings in greens.items():
        crossings.sort(); prev = k * 64 + crossings[0][2]
        for i, (t, queued, _) in enumerate(crossings):
            if not queued: break
            out[f'h{min(i + 1, 6)}'].append(t - prev); prev = t
    for app, end in (('EW', 31.9), ('NS', 63.9)):   # standing queue just before green
        for t in sorted({x[0] for tr in rec.values() for x in tr if abs(x[0] % 64 - end) < 0.05}):
            ds = sorted(x[2] for tr in rec.values() for x in tr
                        if x[0] == t and x[1] == app and x[3] < 0.1 and 0 <= x[2] < 100)
            out['jam'] += [b - a for a, b in zip(ds, ds[1:])]
    return out

for name, load in (('MULTRA', multra), ('SUMO Krauss', lambda s: sumo('Krauss', s)),
                   ('SUMO IDM', lambda s: sumo('IDM', s))):
    agg = collections.defaultdict(list)
    for s in SEEDS:
        for k, v in metrics(load(s)).items(): agg[k] += v
    hs = st.mean(agg['h6'])
    lost = sum(st.mean(agg[k]) for k in ('h1', 'h2', 'h3', 'h4')) - 4 * hs
    print(f'== {name}')
    print('  max acceleration %.2f; per run: clamps %s, full-braking ticks %s, amber passes %s, red passes %s'
          % (max(agg['amax']), agg['clamp'], agg['full'], agg['amber'], agg['red']))
    print('  headways h1-h5 ' + ' '.join('%.2f' % st.mean(agg[f'h{i}']) for i in range(1, 6)))
    print('  saturation headway %.2f s (%.0f veh/h), start-up lost time %.2f s' % (hs, 3600 / hs, lost))
    print('  standing front-to-front spacing median %.2f m [%.2f-%.2f], n=%d'
          % (st.median(agg['jam']), min(agg['jam']), max(agg['jam']), len(agg['jam'])))
