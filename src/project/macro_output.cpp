#include "macro_output.hpp"
#include "csv_format.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
namespace {
constexpr const char* kMeasure = "Static user-equilibrium assignment (multi-class Frank-Wolfe): flow in veh/h per class, "
    "load in pcu/h, times in seconds; Connector arcs at free-flow time, no junction delay";
constexpr const char* kGapFormula = "(sum v t - sum d kappa) / sum v t";
const char* kindName(MacroArc k) { return k == MacroArc::link ? "link" : k == MacroArc::connector ? "connector" : "centroid"; }
std::optional<double> vc(const MacroRun& r, std::size_t a) {
    const auto& l = r.graph.links[a];
    return l.capacity > 0 ? std::optional<double>(r.assignment.load[a] / l.capacity) : std::nullopt;
}
std::string number(double v) { std::ostringstream s; s.imbue(std::locale::classic()); s << v; return s.str(); }
}
Json macroJson(const MacroRun& r) {
    const auto& a = r.assignment;
    Json j{{"validated", false}, {"measure", kMeasure}, {"period", r.period}, {"revision", r.revision},
           {"settings", {{"targetGap", r.settings.targetGap}, {"maxIterations", r.settings.maxIterations}}},
           {"gap", {{"formula", kGapFormula}, {"value", a.gap}, {"target", r.settings.targetGap}}},
           {"iterations", a.iterations}, {"converged", a.converged}, {"cancelled", a.cancelled}, {"convergence", Json::array()},
           {"advisories", Json::array()},
           {"classes", Json::array()}, {"links", Json::array()}, {"inputManifest", r.inputManifest}};
    for (const auto& i : a.convergence) j["convergence"].push_back({{"iteration", i.iteration}, {"gap", i.gap}, {"step", i.step}});
    for (const auto& i : r.advisories) j["advisories"].push_back({{"code", i.code}, {"path", i.path}});
    for (const auto& c : r.classes) j["classes"].push_back({{"id", c.id}, {"mode", c.mode}, {"pcu", c.pcu}});
    for (std::size_t k = 0; k < r.graph.links.size(); ++k) {
        const auto& l = r.graph.links[k];
        Json flow = Json::object();
        for (std::size_t m = 0; m < r.classes.size(); ++m) flow[r.classes[m].id] = a.classFlows[m][k];
        const auto ratio = vc(r, k);
        j["links"].push_back({{"kind", kindName(l.kind)}, {"source", l.source}, {"fromStation", l.fromStation}, {"toStation", l.toStation},
            {"linkType", l.linkType}, {"flow", flow}, {"load", a.load[k]}, {"vc", ratio ? Json(*ratio) : Json(nullptr)},
            {"freeTime", l.freeTime}, {"time", a.time[k]}});
    }
    return j;
}
std::string macroCsv(const MacroRun& r) {
    const auto& a = r.assignment;
    std::ostringstream out;
    out << "# TrafficSim - not yet validated. " << kMeasure << ".\n";
    out << "# Period: " << r.period << '\n';
    out << "# Relative gap: " << number(a.gap) << " (target " << number(r.settings.targetGap) << ", " << kGapFormula
        << "); iterations: " << a.iterations << "; converged: " << (a.converged ? "yes" : "no") << '\n';
    out << "kind,source,fromStation,toStation,linkType";
    for (const auto& c : r.classes) out << ",flow:" << c.id;
    out << ",load,vc,freeTime,time\n";
    for (std::size_t k = 0; k < r.graph.links.size(); ++k) {
        const auto& l = r.graph.links[k];
        out << kindName(l.kind) << ',' << csvQuoted(l.source) << ',' << csvNumber(l.fromStation) << ',' << csvNumber(l.toStation) << ','
            << csvQuoted(l.linkType);
        for (std::size_t m = 0; m < r.classes.size(); ++m) out << ',' << csvNumber(a.classFlows[m][k]);
        out << ',' << csvNumber(a.load[k]) << ',' << csvNumber(vc(r, k)) << ',' << csvNumber(l.freeTime) << ',' << csvNumber(a.time[k]) << '\n';
    }
    return out.str();
}
}
