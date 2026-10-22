#include "loading.hpp"
#include "node_model.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace multra {
namespace {
struct Commodity {
    std::size_t cls{}, path{};
    double pcu{};
    const MesoPath* p{};
    std::vector<std::size_t> slot; // its slot on each of its links, by position
};
// PCU demanded by a path over [0, t]: its piecewise-constant veh/h integrated, times the PCU.
double cumulative(const MesoPath& p, double pcu, double t) {
    double v = 0;
    for (const auto& i : p.flow) v += std::max(0.0, std::min(t, i.end) - i.start) * i.vehiclesPerHour;
    return v * pcu / 3600.0;
}
bool loadable(const MesoGraph& g, const MesoPath& p) {
    if (p.links.empty() || p.turns.size() + 1 != p.links.size()) return false;
    for (std::size_t k = 0; k < p.links.size(); ++k) {
        if (p.links[k] >= g.links.size()) return false;
        for (std::size_t e = 0; e < k; ++e) if (p.links[e] == p.links[k]) return false;
        if (k + 1 < p.links.size()) {
            if (p.turns[k] >= g.turns.size()) return false;
            const auto& t = g.turns[p.turns[k]];
            if (t.in != p.links[k] || t.out != p.links[k + 1]) return false;
        }
    }
    return true;
}
// One row of a node's step: an in-link (or an origin queue) and the commodities it holds.
struct Row {
    std::size_t link{};    // the in-link, or the first link for an origin
    bool origin{};
    double capacity{};
    std::vector<std::pair<std::size_t, double>> parts; // (commodity, PCU in the sending volume)
};
}
MesoLoading loadMeso(const MesoGraph& g, const std::vector<MesoClass>& classes, double step, double horizon,
                     const std::function<bool(std::size_t, std::size_t)>& progress) {
    MesoLoading out;
    out.step = step;
    const auto steps = static_cast<std::size_t>(std::ceil(horizon / step - 1e-9));
    const auto nl = g.links.size();
    // Commodities, and per link the commodities on it with their entrance counts.
    std::vector<Commodity> cs;
    std::vector<std::vector<std::size_t>> onLink(nl);    // commodity ids by slot
    for (std::size_t c = 0; c < classes.size(); ++c)
        for (std::size_t p = 0; p < classes[c].paths.size(); ++p) {
            const auto& path = classes[c].paths[p];
            if (!(classes[c].pcu > 0) || !loadable(g, path)) { out.refused.push_back({c, p}); continue; }
            Commodity x{c, p, classes[c].pcu, &path, {}};
            for (const auto l : path.links) { x.slot.push_back(onLink[l].size()); onLink[l].push_back(cs.size()); }
            cs.push_back(std::move(x));
        }
    std::vector<std::vector<MesoCurve>> upC(nl);         // per link and slot
    std::vector<std::vector<double>> downC(nl);          // per link and slot, the exit count now
    for (std::size_t l = 0; l < nl; ++l) {
        upC[l].assign(onLink[l].size(), MesoCurve{step, {0}});
        downC[l].assign(onLink[l].size(), 0);
    }
    out.up.assign(nl, MesoCurve{step, {0}});
    out.down.assign(nl, MesoCurve{step, {0}});
    out.sending.assign(nl, {});
    out.receiving.assign(nl, {});
    out.turned.assign(g.turns.size(), {0});
    std::vector<double> entered(cs.size()), arrived(cs.size());
    for (const auto& x : cs) out.commodities.push_back({x.cls, x.path, {0}, {0}, {0}});
    // Rows and columns per node: in-links by `to`, out-links by `from`.
    std::vector<std::vector<std::size_t>> ins(g.nodes.size()), outs(g.nodes.size());
    for (std::size_t l = 0; l < nl; ++l) {
        if (g.links[l].to < g.nodes.size()) ins[g.links[l].to].push_back(l);
        if (g.links[l].from < g.nodes.size()) outs[g.links[l].from].push_back(l);
    }
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> turnOf; // (in, out) -> turn
    for (std::size_t u = 0; u < g.turns.size(); ++u) turnOf.try_emplace({g.turns[u].in, g.turns[u].out}, u);
    for (std::size_t k = 0; k < steps; ++k) {
        if (progress && !progress(k, steps)) { out.cancelled = true; break; }
        const double t = static_cast<double>(k) * step;
        std::vector<double> S(nl), R(nl);
        for (std::size_t l = 0; l < nl; ++l) {
            S[l] = mesoSending(g.links[l], out.up[l], out.down[l], t, step);
            R[l] = mesoReceiving(g.links[l], out.up[l], out.down[l], t, step);
            out.sending[l].push_back(S[l]);
            out.receiving[l].push_back(R[l]);
        }
        // Increments of step k, applied after every node (the state at t only).
        std::vector<double> dUp(nl), dDown(nl), dTurn(g.turns.size());
        std::vector<std::vector<double>> dUpC(nl), dDownC(nl);
        for (std::size_t l = 0; l < nl; ++l) { dUpC[l].assign(onLink[l].size(), 0); dDownC[l].assign(onLink[l].size(), 0); }
        for (std::size_t n = 0; n < g.nodes.size(); ++n) {
            std::vector<Row> rows;
            for (const auto l : ins[n]) {
                if (!(S[l] > 0)) continue;
                // Eq. 5.2: the routes of the vehicles numbered N_down(t) to N_down(t) + S at the entrance.
                const double tau = out.up[l].time(out.down[l].value(t) + S[l]);
                Row r{l, false, mesoStepCapacity(g.links[l], step), {}};
                double total = 0;
                for (std::size_t s = 0; s < onLink[l].size(); ++s) {
                    const double v = std::max(0.0, upC[l][s].value(tau) - downC[l][s]);
                    if (v > 0) { r.parts.push_back({onLink[l][s], v}); total += v; }
                }
                if (!(total > 0)) continue;
                for (auto& [c, v] : r.parts) v *= S[l] / total;
                rows.push_back(std::move(r));
            }
            // Origins (Eq. 5.4): each first link's queued demand, demand-constrained (C = S).
            std::map<std::size_t, Row> origins;
            for (std::size_t c = 0; c < cs.size(); ++c) {
                const auto first = cs[c].p->links.front();
                if (g.links[first].from != n) continue;
                const double v = cumulative(*cs[c].p, cs[c].pcu, t + step) - entered[c];
                if (!(v > 1e-12)) continue;
                auto& r = origins[first];
                r.link = first; r.origin = true; r.capacity += v;
                r.parts.push_back({c, v});
            }
            for (auto& [l, r] : origins) rows.push_back(std::move(r));
            if (rows.empty()) continue;
            // Columns: the node's out-links, then one destination receiving without limit (Eq. 5.5).
            const auto& cols = outs[n];
            const auto sink = cols.size();
            const auto column = [&](const Row& r, std::size_t c) {
                const auto& p = *cs[c].p;
                if (r.origin) return static_cast<std::size_t>(std::find(cols.begin(), cols.end(), p.links.front()) - cols.begin());
                const auto pos = static_cast<std::size_t>(std::find(p.links.begin(), p.links.end(), r.link) - p.links.begin());
                if (pos + 1 == p.links.size()) return sink;
                return static_cast<std::size_t>(std::find(cols.begin(), cols.end(), p.links[pos + 1]) - cols.begin());
            };
            MesoNodeStep ns{std::vector<std::vector<double>>(rows.size(), std::vector<double>(sink + 1)), {}, {}};
            for (std::size_t i = 0; i < rows.size(); ++i) {
                ns.capacity.push_back(rows[i].capacity);
                for (const auto& [c, v] : rows[i].parts) ns.demand[i][column(rows[i], c)] += v;
            }
            for (const auto j : cols) ns.receiving.push_back(R[j]);
            ns.receiving.push_back(std::numeric_limits<double>::infinity());
            // Signals as averaged green (D299): one internal supply per in-link and program, Tampère
            // Eq. 33 with Yperman's shared-lane split (Eq. 5.15), N = (g/C) C_i S_p / S_i.
            for (std::size_t i = 0; i < rows.size(); ++i) {
                if (rows[i].origin) continue;
                double si = 0;
                for (std::size_t j = 0; j < sink; ++j) si += ns.demand[i][j];
                si += ns.demand[i][sink];
                std::map<std::string, MesoNodeSupply> bySignal;
                std::map<std::string, double> share, sp;
                for (std::size_t j = 0; j < sink; ++j) {
                    if (!(ns.demand[i][j] > 0)) continue;
                    const auto t = turnOf.find({rows[i].link, cols[j]});
                    if (t == turnOf.end() || g.turns[t->second].signal.empty() || !(g.turns[t->second].green > 0)) continue;
                    const auto& turn = g.turns[t->second];
                    auto& s = bySignal[turn.signal];
                    s.in = i; s.outs.push_back(j);
                    share[turn.signal] = turn.green; sp[turn.signal] += ns.demand[i][j];
                }
                for (auto& [id, s] : bySignal) {
                    s.supply = share[id] * rows[i].capacity * sp[id] / si;
                    ns.internal.push_back(std::move(s));
                }
            }
            const auto q = mesoNodeFlows(ns);
            for (std::size_t i = 0; i < rows.size(); ++i) {
                const auto& r = rows[i];
                double si = 0, qi = 0;
                for (std::size_t j = 0; j <= sink; ++j) { si += ns.demand[i][j]; qi += q[i][j]; }
                if (!(qi > 0)) continue;
                const double ratio = qi / si; // CTF: every part of the row moves in this ratio
                if (!r.origin) dDown[r.link] += qi;
                for (const auto& [c, v] : r.parts) {
                    const double moved = v * ratio;
                    const auto& p = *cs[c].p;
                    const auto pos = r.origin ? 0 : static_cast<std::size_t>(std::find(p.links.begin(), p.links.end(), r.link) - p.links.begin());
                    if (r.origin) {
                        entered[c] += moved;
                        dUp[r.link] += moved; dUpC[r.link][cs[c].slot[0]] += moved;
                        continue;
                    }
                    dDownC[r.link][cs[c].slot[pos]] += moved;
                    if (pos + 1 == p.links.size()) { arrived[c] += moved; continue; }
                    const auto next = p.links[pos + 1];
                    dUp[next] += moved; dUpC[next][cs[c].slot[pos + 1]] += moved;
                    dTurn[p.turns[pos]] += moved;
                }
            }
        }
        for (std::size_t l = 0; l < nl; ++l) {
            out.up[l].at.push_back(out.up[l].at.back() + dUp[l]);
            out.down[l].at.push_back(out.down[l].at.back() + dDown[l]);
            for (std::size_t s = 0; s < onLink[l].size(); ++s) {
                upC[l][s].at.push_back(upC[l][s].at.back() + dUpC[l][s]);
                downC[l][s] += dDownC[l][s];
            }
        }
        for (std::size_t u = 0; u < g.turns.size(); ++u) out.turned[u].push_back(out.turned[u].back() + dTurn[u]);
        for (std::size_t c = 0; c < cs.size(); ++c) {
            auto& o = out.commodities[c];
            o.demand.push_back(cumulative(*cs[c].p, 1, t + step));
            o.entered.push_back(entered[c] / cs[c].pcu);
            o.arrived.push_back(arrived[c] / cs[c].pcu);
        }
    }
    for (std::size_t l = 0; l < nl; ++l) { out.sending[l].push_back(0); out.receiving[l].push_back(0); }
    return out;
}
}
