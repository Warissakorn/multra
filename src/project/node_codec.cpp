#include "node_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace multra {
void addNodesJson(const Network& n, Json& network) {
    if (n.nodes.empty()) return;
    Json nodes = Json::array();
    for (const auto& node : n.nodes) {
        Json polygon = Json::array();
        for (const auto& p : node.polygon) polygon.push_back({{"x", p.x}, {"y", p.y}});
        nodes.push_back({{"id", node.id}, {"name", node.name}, {"polygon", polygon}});
        if (node.generated) nodes.back()["generated"] = true;
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
            if (key != "id" && key != "name" && key != "polygon" && key != "generated") invalid(path + "." + key);
        }
        Node node;
        if (!j.contains("id") || !j.at("id").is_string()) invalid(path + ".id");
        if (!j.contains("name") || !j.at("name").is_string()) invalid(path + ".name");
        if (!j.contains("polygon") || !j.at("polygon").is_array()) invalid(path + ".polygon");
        if (j.contains("generated") && !(j.at("generated").is_boolean() && j.at("generated").get<bool>())) invalid(path + ".generated");
        node.id = j.at("id").get<std::string>(); node.name = j.at("name").get<std::string>();
        node.generated = j.contains("generated");
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
}
