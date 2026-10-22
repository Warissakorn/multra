#include "meso_output.hpp"
#include "csv_format.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
constexpr const char* kMeasure = "Meso link-transmission loading (Yperman 2007; Tampere et al. 2011 node model): "
    "flows in pcu/h, density in pcu/km, times in seconds; signals as averaged green; no delay at nodes";
// The integral of N(t) dt over [t0, t1] for the piecewise-linear curve.
double area(const MesoCurve& c, double t0, double t1) {
    if (!(t1 > t0)) return 0;
    double sum = 0, t = t0;
    while (t < t1) {
        const double next = std::min(t1, (std::floor(t / c.step + 1e-12) + 1) * c.step);
        sum += (c.value(t) + c.value(next)) / 2 * (next - t);
        t = next;
    }
    return sum;
}
// The integral of t(N) dN over [a, b], by parts: b t(b) - a t(a) - integral of N dt.
double timeIntegral(const MesoCurve& c, double a, double b) {
    const double ta = c.time(a), tb = c.time(b);
    return b * tb - a * ta - area(c, ta, tb);
}
double steps(const MesoRun& r) { return static_cast<double>(r.loading.up.empty() ? 0 : r.loading.up.front().at.size() - 1); }
}
std::optional<double> mesoTravelTime(const MesoCurve& up, const MesoCurve& down, double a, double b) {
    if (!(b - a > 1e-9) || down.at.empty() || down.at.back() < b - 1e-9) return std::nullopt;
    return (timeIntegral(down, a, b) - timeIntegral(up, a, b)) / (b - a);
}
std::vector<MesoLinkSlice> mesoLinkSlices(const MesoRun& r, std::size_t l) {
    std::vector<MesoLinkSlice> out;
    const auto& L = r.loading;
    const auto& link = r.graph.links[l];
    const auto k = static_cast<std::size_t>(steps(r));
    const auto per = std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(r.settings.slice / L.step)));
    for (std::size_t s = 0; s < k; s += per) {
        const auto e = std::min(k, s + per);
        MesoLinkSlice x;
        x.start = static_cast<double>(s) * L.step; x.end = static_cast<double>(e) * L.step;
        const double hours = (x.end - x.start) / 3600;
        x.inflow = (L.up[l].at[e] - L.up[l].at[s]) / hours;
        x.outflow = (L.down[l].at[e] - L.down[l].at[s]) / hours;
        for (auto i = s + 1; i <= e; ++i) {
            x.density += (L.up[l].at[i] - L.down[l].at[i]) / (link.length / 1000);
            const double lag = link.freeSpeed > 0 ? link.length / link.freeSpeed : 0;
            x.queue += std::max(0.0, L.up[l].value(static_cast<double>(i) * L.step - lag) - L.down[l].at[i]);
        }
        x.density /= static_cast<double>(e - s); x.queue /= static_cast<double>(e - s);
        x.travelTime = mesoTravelTime(L.up[l], L.down[l], L.up[l].at[s], L.up[l].at[e]);
        for (auto i = s; i < e; ++i) x.spillBack = x.spillBack || L.receiving[l][i] <= 1e-9;
        out.push_back(x);
    }
    return out;
}
Json mesoJson(const MesoRun& r) {
    const auto& L = r.loading;
    Json j{{"validated", false}, {"measure", kMeasure}, {"revision", r.revision},
           {"settings", {{"step", r.settings.step}, {"slice", r.settings.slice}}}, {"duration", r.duration},
           {"cancelled", L.cancelled}, {"advisories", Json::array()}, {"classes", Json::array()}, {"totals", Json::object()},
           {"links", Json::array()}, {"turns", Json::array()}, {"inputManifest", r.inputManifest}};
    for (const auto& i : r.advisories) j["advisories"].push_back({{"code", i.code}, {"path", i.path}});
    for (std::size_t c = 0; c < r.classes.size(); ++c) {
        double demand = 0, entered = 0, arrived = 0;
        for (const auto& x : L.commodities)
            if (x.cls == c && !x.demand.empty()) { demand += x.demand.back(); entered += x.entered.back(); arrived += x.arrived.back(); }
        j["classes"].push_back({{"id", r.classes[c].id}, {"pcu", r.classes[c].pcu}, {"paths", r.classes[c].paths.size()},
                                {"demanded", demand}, {"entered", entered}, {"arrived", arrived}, {"waiting", demand - entered}});
    }
    double vkt = 0, vht = 0, free = 0, stored = 0;
    for (std::size_t l = 0; l < r.graph.links.size(); ++l) {
        const auto& link = r.graph.links[l];
        const double done = L.down[l].at.back();
        vkt += done * link.length / 1000;
        if (done > 0) vht += (timeIntegral(L.down[l], 0, done) - timeIntegral(L.up[l], 0, done)) / 3600;
        if (link.freeSpeed > 0) free += done * link.length / link.freeSpeed / 3600;
        stored += L.up[l].at.back() - done;
        Json slices = Json::array();
        for (const auto& x : mesoLinkSlices(r, l))
            slices.push_back({{"start", x.start}, {"end", x.end}, {"inflow", x.inflow}, {"outflow", x.outflow}, {"density", x.density},
                              {"travelTime", x.travelTime ? Json(*x.travelTime) : Json(nullptr)}, {"queue", x.queue}, {"spillBack", x.spillBack}});
        j["links"].push_back({{"source", link.source}, {"fromStation", link.fromStation}, {"toStation", link.toStation},
                              {"length", link.length}, {"lanes", link.lanes}, {"capacity", link.capacityPerLane * link.lanes}, {"slices", slices}});
    }
    // Time inside nodes (D301): each turn's free time, for the PCU that finished it by the end.
    double inNodes = 0;
    const double T = L.up.empty() ? 0 : static_cast<double>(L.up.front().at.size() - 1) * L.step;
    for (std::size_t u = 0; u < r.graph.turns.size() && u < L.turned.size(); ++u) {
        const double tau = r.graph.turns[u].freeTime;
        if (!(tau > 0)) continue;
        const MesoCurve c{L.step, L.turned[u]};
        const double through = c.value(T - tau);
        vht += through * tau / 3600; free += through * tau / 3600;
        inNodes += c.at.back() - through;
    }
    j["totals"] = {{"vkt", vkt}, {"vht", vht}, {"delay", vht - free}, {"storedAtEnd", stored}, {"inNodesAtEnd", inNodes}};
    const auto per = std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(r.settings.slice / L.step)));
    const auto k = static_cast<std::size_t>(steps(r));
    for (std::size_t u = 0; u < r.graph.turns.size(); ++u) {
        const auto& t = r.graph.turns[u];
        Json slices = Json::array();
        for (std::size_t s = 0; s < k; s += per) {
            const auto e = std::min(k, s + per);
            slices.push_back({{"start", static_cast<double>(s) * L.step}, {"end", static_cast<double>(e) * L.step},
                              {"transferred", L.turned[u][e] - L.turned[u][s]}});
        }
        j["turns"].push_back({{"node", r.graph.nodes[t.node].name}, {"name", t.name}, {"from", r.graph.links[t.in].source},
                              {"to", r.graph.links[t.out].source}, {"signal", t.signal}, {"green", t.green}, {"freeTime", t.freeTime}, {"slices", slices}});
    }
    return j;
}
std::string mesoCsv(const MesoRun& r) {
    std::string out = std::string("# MULTRA - not yet validated. ") + kMeasure + ".\n";
    out += "# Step: " + csvNumber(r.settings.step) + " s; slice: " + csvNumber(r.settings.slice) + " s; duration: " + csvNumber(r.duration) + " s\n";
    out += "source,fromStation,toStation,start,end,inflow,outflow,density,travelTime,queue,spillBack\n";
    for (std::size_t l = 0; l < r.graph.links.size(); ++l) {
        const auto& link = r.graph.links[l];
        for (const auto& x : mesoLinkSlices(r, l))
            out += csvQuoted(link.source) + "," + csvNumber(link.fromStation) + "," + csvNumber(link.toStation) + "," + csvNumber(x.start) + ","
                + csvNumber(x.end) + "," + csvNumber(x.inflow) + "," + csvNumber(x.outflow) + "," + csvNumber(x.density) + ","
                + csvNumber(x.travelTime) + "," + csvNumber(x.queue) + "," + (x.spillBack ? "yes" : "no") + "\n";
    }
    return out;
}
}
