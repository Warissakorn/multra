#include "node_analysis_run.hpp"
#include "csv_format.hpp"

namespace multra {
namespace {
constexpr const char* kMeasure = "Analytical (HCM 7th ed. Ch. 19: pretimed, protected, exclusive lane groups), not simulated: "
    "flow in veh/h, saturation flow in veh/h/ln, delay in s/veh";
const char* movementName(LaneGroupMovement m) {
    return m == LaneGroupMovement::through ? "through" : m == LaneGroupMovement::kerbTurn ? "kerbTurn" : "farTurn";
}
Json optionalNumber(const std::optional<double>& x) { return x ? Json(*x) : Json(nullptr); }
Json optionalLetter(const std::optional<char>& x) { return x ? Json(std::string(1, *x)) : Json(nullptr); }
std::string letter(const std::optional<char>& x) { return x ? std::string(1, *x) : std::string(); }
}
Json nodeAnalysisJson(const NodeAnalysisRun& r) {
    Json j{{"validated", false}, {"measure", kMeasure}, {"revision", r.revision}, {"losPack", r.losPack},
           {"nodes", Json::array()}, {"inputManifest", r.inputManifest}};
    for (const auto& n : r.nodes) {
        Json node{{"id", n.id}, {"name", n.name}, {"peakHourFactor", n.peakHourFactor}, {"analysisPeriod", n.analysisPeriod},
                  {"baseSaturationFlow", {{"value", n.baseSaturationFlow}, {"source", n.baseSaturationSource}}},
                  {"defaults", n.defaults}, {"advisories", Json::array()}, {"laneGroups", Json::array()}, {"approaches", Json::array()},
                  {"flow", n.flow}, {"delay", optionalNumber(n.delay)}, {"los", optionalLetter(n.los)}};
        for (const auto& i : n.advisories) node["advisories"].push_back({{"code", i.code}, {"path", i.path}});
        for (const auto& g : n.groups) {
            Json row{{"name", g.group.name}, {"in", g.group.inLink}, {"out", g.group.outLink}, {"movement", movementName(g.group.movement)},
                     {"lanes", g.group.lanes}, {"laneWidth", g.group.laneWidth}, {"program", g.group.program},
                     {"displayedGreen", g.group.displayedGreen}, {"effectiveGreen", g.group.effectiveGreen}, {"cycle", g.group.cycle},
                     {"volume", g.volume}, {"heavyVehiclePercent", g.heavyPercent}, {"issue", g.issue ? Json(*g.issue) : Json(nullptr)}};
            if (g.result) {
                const auto& x = *g.result;
                const auto& f = x.factors;
                row["factors"] = {{"fw", f.fw}, {"fHVg", f.fHVg}, {"fp", f.fp}, {"fbb", f.fbb}, {"fa", f.fa}, {"fLU", f.fLU}, {"fLT", f.fLT}, {"fRT", f.fRT}};
                row["flowRate"] = g.volume / n.peakHourFactor;
                row["saturationFlow"] = x.saturation; row["capacity"] = x.capacity; row["ratio"] = x.ratio; row["greenRatio"] = x.greenRatio;
                row["progressionFactor"] = x.progression; row["uniformDelay"] = x.uniformDelay; row["incrementalDelay"] = x.incrementalDelay;
                row["initialQueueDelay"] = x.initialQueueDelay; row["delay"] = x.delay; row["los"] = optionalLetter(g.los);
            }
            node["laneGroups"].push_back(row);
        }
        for (const auto& a : n.approaches)
            node["approaches"].push_back({{"in", a.inLink}, {"name", a.name}, {"flow", a.flow}, {"delay", optionalNumber(a.delay)}, {"los", optionalLetter(a.los)}});
        j["nodes"].push_back(node);
    }
    return j;
}
std::string nodeAnalysisCsv(const NodeAnalysisRun& r) {
    std::string out = std::string("# MULTRA - not yet validated. ") + kMeasure + ".\n";
    out += "level,node,name,movement,lanes,flowRate,saturationFlow,capacity,ratio,greenRatio,uniformDelay,incrementalDelay,delay,los,issue\n";
    for (const auto& n : r.nodes) {
        for (const auto& g : n.groups) {
            out += "laneGroup," + csvQuoted(n.name) + "," + csvQuoted(g.group.name) + "," + movementName(g.group.movement) + "," +
                   std::to_string(g.group.lanes) + "," + csvNumber(g.volume / n.peakHourFactor) + ",";
            if (g.result) {
                const auto& x = *g.result;
                out += csvNumber(x.saturation) + "," + csvNumber(x.capacity) + "," + csvNumber(x.ratio) + "," + csvNumber(x.greenRatio) + "," +
                       csvNumber(x.uniformDelay) + "," + csvNumber(x.incrementalDelay) + "," + csvNumber(x.delay) + "," + letter(g.los) + ",\n";
            } else {
                out += ",,,,,,,," + *g.issue + "\n";
            }
        }
        for (const auto& a : n.approaches)
            out += "approach," + csvQuoted(n.name) + "," + csvQuoted(a.name) + ",,," + csvNumber(a.flow) + ",,,,,,," + csvNumber(a.delay) + "," + letter(a.los) + ",\n";
        out += "node," + csvQuoted(n.name) + "," + csvQuoted(n.name) + ",,," + csvNumber(n.flow) + ",,,,,,," + csvNumber(n.delay) + "," + letter(n.los) + ",\n";
    }
    return out;
}
}
