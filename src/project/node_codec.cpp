#include "node_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

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
            if (key != "id" && key != "name" && key != "polygon" && key != "generated" && (key != "coarse" || schema < 53))
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
    if (!g.is_boolean() || !g.get<bool>()) throw ValidationError({{"EDIT_CONNECTOR_INVALID", where + ".generated"}});
    return true;
}
}
