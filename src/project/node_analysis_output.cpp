#include "node_analysis_run.hpp"
#include "csv_format.hpp"

namespace multra {
namespace {
constexpr const char* kMeasure = "Analytical (HCM 7th ed. Ch. 19 with Ch. 31: pretimed; protected, permitted and protected-permitted lane groups), not simulated: "
    "flow in veh/h, saturation flow in veh/h/ln, delay in s/veh";
const char* movementName(LaneGroupMovement m) {
    return m == LaneGroupMovement::through ? "through" : m == LaneGroupMovement::kerbTurn ? "kerbTurn" : "farTurn";
}
const char* kindName(LaneGroupKind k) {
    static constexpr const char* names[] = {"left", "sharedLeft", "through", "sharedRight", "right", "leftRight", "all"};
    return names[static_cast<int>(k)];
}
double share(double part, double whole) { return whole > 0 ? part / whole : 0; }
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
                  {"flow", n.flow}, {"delay", optionalNumber(n.delay)}, {"los", optionalLetter(n.los)}, {"critical", nullptr}};
        if (n.critical) {
            Json names = Json::array();
            for (const auto i : n.critical->chain) names.push_back(n.groups[i].group.name);
            node["critical"] = {{"ratio", n.critical->ratio}, {"flowRatio", n.critical->flowRatio}, {"lostTime", n.critical->lostTime},
                                {"laneGroups", names}};
        }
        for (const auto& i : n.advisories) node["advisories"].push_back({{"code", i.code}, {"path", i.path}});
        for (const auto& g : n.groups) {
            Json turns = Json::array();
            for (const auto t : g.group.turns) turns.push_back(t < n.turnNames.size() ? n.turnNames[t] : std::string());
            Json row{{"name", g.group.name}, {"in", g.group.inLink}, {"turns", turns}, {"kind", kindName(g.group.kind)},
                     {"movement", movementName(g.group.movement)}, {"lanes", g.group.lanes}, {"laneWidth", g.group.laneWidth}, {"program", g.group.program},
                     {"displayedGreen", g.group.displayedGreen}, {"effectiveGreen", g.group.effectiveGreen}, {"cycle", g.group.cycle},
                     {"flowRate", g.flowRate}, {"farTurns", g.farTurns}, {"kerbTurns", g.kerbTurns}, {"heavyVehiclePercent", g.heavyPercent}, {"issue", g.issue ? Json(*g.issue) : Json(nullptr)}};
            if (g.result) {
                const auto& x = *g.result;
                const auto& f = x.factors;
                row["factors"] = {{"fw", f.fw}, {"fHVg", f.fHVg}, {"fp", f.fp}, {"fbb", f.fbb}, {"fa", f.fa}, {"fLU", f.fLU}, {"fLT", f.fLT}, {"fRT", f.fRT}, {"fShared", f.fShared}};
                row["saturationFlow"] = x.saturation; row["capacity"] = x.capacity; row["ratio"] = x.ratio; row["greenRatio"] = x.greenRatio;
                row["progressionFactor"] = x.progression; row["uniformDelay"] = x.uniformDelay; row["incrementalDelay"] = x.incrementalDelay;
                row["initialQueueDelay"] = x.initialQueueDelay; row["delay"] = x.delay; row["los"] = optionalLetter(g.los);
            }
            row["permitted"] = nullptr;
            if (g.permitted) {
                const auto& p = *g.permitted;
                row["permitted"] = {{"operation", p.operation}, {"opposingFlow", p.opposingFlow}, {"opposingQueue", p.opposingQueue},
                                    {"permittedGreen", p.green.displayed}, {"unblockedGreen", p.green.unblockedDisplayed},
                                    {"startUpLostTime", p.green.startupLost}, {"extension", p.green.extension},
                                    {"effectivePermittedGreen", p.green.effective}, {"effectiveUnblockedGreen", p.green.unblocked},
                                    {"protectedGreen", p.protectedGreen}, {"permittedSaturationFlow", p.permittedSaturation},
                                    {"farShare", p.farShare}, {"firstBlock", p.firstBlock}};
            }
            row["queue"] = nullptr;
            if (g.queue) {
                const auto& q = *g.queue;
                row["queue"] = {{"fullStopTime", q.fullStopTime}, {"first", q.first}, {"second", q.second}, {"third", q.third}, {"mean", q.mean},
                                {"percentile95", q.percentileQueue}, {"storage", g.group.storage}, {"storageRatio", *g.storageRatio},
                                {"storageRatio95", *g.storageRatio95}};
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
    out += "level,node,name,kind,lanes,flowRate,farShare,kerbShare,saturationFlow,capacity,ratio,greenRatio,uniformDelay,incrementalDelay,delay,los,issue,criticalRatio,queue,queue95,storageRatio,storageRatio95,operation,unblockedGreen\n";
    for (const auto& n : r.nodes) {
        for (const auto& g : n.groups) {
            out += "laneGroup," + csvQuoted(n.name) + "," + csvQuoted(g.group.name) + "," + kindName(g.group.kind) + "," +
                   std::to_string(g.group.lanes) + "," + csvNumber(g.flowRate) + "," + csvNumber(share(g.farTurns, g.flowRate)) + "," +
                   csvNumber(share(g.kerbTurns, g.flowRate)) + ",";
            if (g.result) {
                const auto& x = *g.result;
                out += csvNumber(x.saturation) + "," + csvNumber(x.capacity) + "," + csvNumber(x.ratio) + "," + csvNumber(x.greenRatio) + "," +
                       csvNumber(x.uniformDelay) + "," + csvNumber(x.incrementalDelay) + "," + csvNumber(x.delay) + "," + letter(g.los) + ",,";
                if (g.queue) out += "," + csvNumber(g.queue->mean) + "," + csvNumber(g.queue->percentileQueue) + "," + csvNumber(*g.storageRatio) + "," +
                                    csvNumber(*g.storageRatio95);
                else out += ",,,,";
                out += "," + std::string(g.permitted ? g.permitted->operation : "protected") + "," +
                       (g.permitted ? csvNumber(g.permitted->green.unblockedDisplayed) : std::string()) + "\n";
            } else {
                out += ",,,,,,,," + *g.issue + ",,,,,,,\n";
            }
        }
        for (const auto& a : n.approaches)
            out += "approach," + csvQuoted(n.name) + "," + csvQuoted(a.name) + ",,," + csvNumber(a.flow) + ",,,,,,,,," + csvNumber(a.delay) + "," + letter(a.los) + ",,,,,,,,\n";
        out += "node," + csvQuoted(n.name) + "," + csvQuoted(n.name) + ",,," + csvNumber(n.flow) + ",,,,,,,,," + csvNumber(n.delay) + "," + letter(n.los) + ",," + (n.critical ? csvNumber(n.critical->ratio) : std::string()) + ",,,,,,\n";
    }
    return out;
}
}
