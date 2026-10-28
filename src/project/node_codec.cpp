#include "node_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
const char* controlName(CoarseControl c) {
    return c == CoarseControl::signalised ? "signalised" : c == CoarseControl::allWayStop ? "allWayStop" : "priority";
}
Json turnsJson(const std::vector<std::pair<std::string, std::string>>& turns) {
    Json list = Json::array();
    for (const auto& [in, out] : turns) list.push_back(Json::array({in, out}));
    return list;
}
// S3.6a (ROADS §6, D310): fields at their defaults are not written.
Json coarseJson(const NodeCoarse& c) {
    Json j = Json::object();
    if (c.control) j["control"] = controlName(*c.control);
    if (!c.major.empty()) j["major"] = c.major;
    if (!c.bannedTurns.empty()) j["bannedTurns"] = turnsJson(c.bannedTurns);
    if (c.signal) {
        Json stages = Json::array();
        for (const auto& s : c.signal->stages) stages.push_back({{"green", s.green}, {"turns", turnsJson(s.turns)}});
        j["signal"] = {{"cycle", c.signal->cycle}, {"stages", stages}};
    }
    return j;
}
NodeCoarse parseCoarse(const Json& j, const std::string& path) {
    const auto invalid = [](const std::string& at) { throw ValidationError({{"EDIT_NODE_INVALID", at}}); };
    if (!j.is_object()) invalid(path);
    const auto turns = [&](const Json& list, const std::string& at) {
        std::vector<std::pair<std::string, std::string>> result;
        if (!list.is_array()) invalid(at);
        for (std::size_t k = 0; k < list.size(); ++k) {
            const auto& t = list[k];
            if (!t.is_array() || t.size() != 2 || !t[0].is_string() || !t[1].is_string()) invalid(at + "[" + std::to_string(k) + "]");
            result.emplace_back(t[0].get<std::string>(), t[1].get<std::string>());
        }
        return result;
    };
    NodeCoarse c;
    for (const auto& [key, value] : j.items()) {
        const auto at = path + "." + key;
        if (key == "control") {
            const auto name = value.is_string() ? value.get<std::string>() : std::string{};
            if (name == "priority") c.control = CoarseControl::priority;
            else if (name == "signalised") c.control = CoarseControl::signalised;
            else if (name == "allWayStop") c.control = CoarseControl::allWayStop;
            else invalid(at);
        } else if (key == "major") {
            if (!value.is_array()) invalid(at);
            for (std::size_t k = 0; k < value.size(); ++k) {
                if (!value[k].is_string()) invalid(at + "[" + std::to_string(k) + "]");
                c.major.push_back(value[k].get<std::string>());
            }
        } else if (key == "bannedTurns") {
            c.bannedTurns = turns(value, at);
        } else if (key == "signal") {
            if (!value.is_object() || value.size() != 2 || !value.contains("cycle") || !value.at("cycle").is_number() ||
                !value.contains("stages") || !value.at("stages").is_array()) invalid(at);
            CoarseSignal signal{value.at("cycle").get<double>(), {}};
            const auto& stages = value.at("stages");
            for (std::size_t s = 0; s < stages.size(); ++s) {
                const auto& stage = stages[s];
                const auto stageAt = at + ".stages[" + std::to_string(s) + "]";
                if (!stage.is_object() || stage.size() != 2 || !stage.contains("green") || !stage.at("green").is_number() || !stage.contains("turns"))
                    invalid(stageAt);
                signal.stages.push_back({stage.at("green").get<double>(), turns(stage.at("turns"), stageAt + ".turns")});
            }
            c.signal = std::move(signal);
        } else invalid(at);
    }
    return c;
}
// M13 A1 (NODE_ANALYSIS §4, D371): defaults are absent and not written.
Json analysisJson(const NodeAnalysis& a) {
    Json j = Json::object();
    if (a.cbd) j["cbd"] = true;
    if (a.baseSaturationFlow) j["baseSaturationFlow"] = {{"value", a.baseSaturationFlow->value}, {"source", a.baseSaturationFlow->source}};
    if (a.peakHourFactor) j["peakHourFactor"] = *a.peakHourFactor;
    if (a.analysisPeriod) j["analysisPeriod"] = *a.analysisPeriod;
    if (!a.approaches.empty()) {
        Json list = Json::array();
        for (const auto& r : a.approaches) {
            Json row = {{"link", r.link}};
            if (r.grade != 0) row["grade"] = r.grade;
            if (r.parkingManeuvers) row["parkingManeuvers"] = *r.parkingManeuvers;
            if (r.busesStopping != 0) row["busesStopping"] = r.busesStopping;
            if (r.speedLimit) row["speedLimit"] = *r.speedLimit;
            list.push_back(row);
        }
        j["approaches"] = list;
    }
    if (!a.turns.empty()) {
        Json list = Json::array();
        for (const auto& t : a.turns) {
            list.push_back({{"in", t.in}, {"out", t.out}, {"vehiclesPerHour", t.vehiclesPerHour}});
            if (t.heavyVehiclePercent) list.back()["heavyVehiclePercent"] = *t.heavyVehiclePercent;
        }
        j["turns"] = list;
    }
    return j;
}
// Shape and sign only; the method's ranges are checked when it runs (signalisedRangeIssue).
NodeAnalysis parseAnalysis(const Json& j, const std::string& path) {
    const auto invalid = [](const std::string& at) { throw ValidationError({{"EDIT_NODE_INVALID", at}}); };
    const auto number = [&](const Json& v, const std::string& at, bool positive) {
        if (!v.is_number() || !std::isfinite(v.get<double>()) || (positive ? !(v.get<double>() > 0) : v.get<double>() < 0)) invalid(at);
        return v.get<double>();
    };
    const auto rows = [&](const Json& list, const std::string& at, const auto& keys, const auto& read) {
        if (!list.is_array() || list.empty()) invalid(at);
        for (std::size_t k = 0; k < list.size(); ++k) {
            const auto rowAt = at + "[" + std::to_string(k) + "]";
            if (!list[k].is_object()) invalid(rowAt);
            for (const auto& [key, v] : list[k].items()) {
                (void)v;
                if (std::find(keys.begin(), keys.end(), key) == keys.end()) invalid(rowAt + "." + key);
            }
            read(list[k], rowAt);
        }
    };
    if (!j.is_object()) invalid(path);
    NodeAnalysis a;
    for (const auto& [key, value] : j.items()) {
        const auto at = path + "." + key;
        if (key == "cbd") {
            if (!value.is_boolean() || !value.get<bool>()) invalid(at);
            a.cbd = true;
        } else if (key == "baseSaturationFlow") {
            if (!value.is_object() || value.size() != 2 || !value.contains("value") || !value.contains("source") || !value.at("source").is_string())
                invalid(at);
            a.baseSaturationFlow = SaturationFlowSource{number(value.at("value"), at + ".value", true), value.at("source").get<std::string>()};
        } else if (key == "peakHourFactor") {
            a.peakHourFactor = number(value, at, true);
            if (*a.peakHourFactor > 1) invalid(at);
        } else if (key == "analysisPeriod") {
            a.analysisPeriod = number(value, at, true);
        } else if (key == "approaches") {
            const std::vector<std::string> keys{"link", "grade", "parkingManeuvers", "busesStopping", "speedLimit"};
            rows(value, at, keys, [&](const Json& r, const std::string& rowAt) {
                if (!r.contains("link") || !r.at("link").is_string()) invalid(rowAt + ".link");
                NodeAnalysisApproach row{r.at("link").get<std::string>()};
                if (r.contains("grade")) {
                    if (!r.at("grade").is_number() || !std::isfinite(r.at("grade").get<double>())) invalid(rowAt + ".grade");
                    row.grade = r.at("grade").get<double>();
                }
                if (r.contains("parkingManeuvers")) row.parkingManeuvers = number(r.at("parkingManeuvers"), rowAt + ".parkingManeuvers", false);
                if (r.contains("busesStopping")) row.busesStopping = number(r.at("busesStopping"), rowAt + ".busesStopping", false);
                if (r.contains("speedLimit")) row.speedLimit = number(r.at("speedLimit"), rowAt + ".speedLimit", true);
                a.approaches.push_back(std::move(row));
            });
        } else if (key == "turns") {
            const std::vector<std::string> keys{"in", "out", "vehiclesPerHour", "heavyVehiclePercent"};
            rows(value, at, keys, [&](const Json& t, const std::string& rowAt) {
                if (!t.contains("in") || !t.at("in").is_string()) invalid(rowAt + ".in");
                if (!t.contains("out") || !t.at("out").is_string()) invalid(rowAt + ".out");
                if (!t.contains("vehiclesPerHour")) invalid(rowAt + ".vehiclesPerHour");
                NodeAnalysisTurn turn{t.at("in").get<std::string>(), t.at("out").get<std::string>(),
                                      number(t.at("vehiclesPerHour"), rowAt + ".vehiclesPerHour", false)};
                if (t.contains("heavyVehiclePercent")) turn.heavyVehiclePercent = number(t.at("heavyVehiclePercent"), rowAt + ".heavyVehiclePercent", false);
                a.turns.push_back(std::move(turn));
            });
        } else invalid(at);
    }
    return a;
}
}
void addNodesJson(const Network& n, Json& network) {
    if (n.nodes.empty()) return;
    Json nodes = Json::array();
    for (const auto& node : n.nodes) {
        Json polygon = Json::array();
        for (const auto& p : node.polygon) polygon.push_back({{"x", p.x}, {"y", p.y}});
        nodes.push_back({{"id", node.id}, {"name", node.name}, {"polygon", polygon}});
        if (node.generated) nodes.back()["generated"] = true;
        if (node.coarse) nodes.back()["coarse"] = coarseJson(*node.coarse);
        if (node.analysis) nodes.back()["analysis"] = analysisJson(*node.analysis);
    }
    network["nodes"] = nodes;
}
std::vector<Node> parseNodes(const Json& network, int schema) {
    std::vector<Node> nodes;
    if (schema < 48 || !network.is_object() || !network.contains("nodes")) return nodes;
    const auto& values = network.at("nodes");
    const auto invalid = [](const std::string& path) { throw ValidationError({{"EDIT_NODE_INVALID", path}}); };
    if (!values.is_array() || values.empty()) invalid("network.nodes"); // never written empty
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto& j = values[i];
        const auto path = "network.nodes[" + std::to_string(i) + "]";
        if (!j.is_object()) invalid(path);
        for (const auto& [key, value] : j.items()) {
            (void)value;
            if (key != "id" && key != "name" && key != "polygon" && key != "generated" && (key != "coarse" || schema < 53) &&
                (key != "analysis" || schema < 58))
                invalid(path + "." + key);
        }
        Node node;
        if (!j.contains("id") || !j.at("id").is_string()) invalid(path + ".id");
        if (!j.contains("name") || !j.at("name").is_string()) invalid(path + ".name");
        if (!j.contains("polygon") || !j.at("polygon").is_array()) invalid(path + ".polygon");
        if (j.contains("generated") && !(j.at("generated").is_boolean() && j.at("generated").get<bool>())) invalid(path + ".generated");
        node.id = j.at("id").get<std::string>(); node.name = j.at("name").get<std::string>();
        node.generated = j.contains("generated");
        if (j.contains("coarse")) node.coarse = parseCoarse(j.at("coarse"), path + ".coarse");
        if (j.contains("analysis")) node.analysis = parseAnalysis(j.at("analysis"), path + ".analysis");
        const auto& polygon = j.at("polygon");
        for (std::size_t k = 0; k < polygon.size(); ++k) {
            const auto& p = polygon[k];
            if (!p.is_object() || p.size() != 2 || !p.contains("x") || !p.contains("y") || !p.at("x").is_number() || !p.at("y").is_number())
                invalid(path + ".polygon[" + std::to_string(k) + "]");
            node.polygon.push_back({p.at("x").get<double>(), p.at("y").get<double>()});
        }
        nodes.push_back(std::move(node));
    }
    return nodes;
}
Json withoutGenerated(const Json& object, int schema) {
    if (schema < 54 || !object.is_object() || !object.contains("generated")) return object;
    auto copy = object;
    copy.erase("generated");
    return copy;
}
bool generatedFlag(const Json& object, const std::string& where, int schema) {
    if (schema < 54 || !object.is_object() || !object.contains("generated")) return false;
    const auto& g = object.at("generated");
    if (!g.is_boolean() || !g.get<bool>()) throw ValidationError({{"EDIT_GENERATED_INVALID", where + ".generated"}});
    return true;
}
bool usesGeneratedFlags(const ProjectDocument& d) {
    const auto any = [](const auto& list) { return std::any_of(list.begin(), list.end(), [](const auto& x) { return x.generated; }); };
    const auto& n = d.network;
    const auto& r = n.rightOfWay;
    return any(n.connectors) || any(n.signalHeads) || any(r.waitingLines) || any(r.conflictAreas) || any(r.priorityRules) ||
           any(r.stopControls) || (d.definition && any(d.definition->signalControllers));
}
bool usesNodeAnalysis(const ProjectDocument& d) {
    return std::any_of(d.network.nodes.begin(), d.network.nodes.end(), [](const auto& n) { return n.analysis.has_value(); });
}
void rejectGeneratedControllersBefore54(const Json& file) {
    if (!file.is_object() || !file.contains("definition") || !file.at("definition").is_object()) return;
    const auto& def = file.at("definition");
    if (!def.contains("signalControllers") || !def.at("signalControllers").is_array()) return;
    const auto& list = def.at("signalControllers");
    for (std::size_t i = 0; i < list.size(); ++i)
        if (list[i].is_object() && list[i].contains("generated"))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.signalControllers[" + std::to_string(i) + "].generated"}});
}
}
