#include "node_commands.hpp"
#include "../model/network/node_suggestions.hpp"
#include <algorithm>
#include <stdexcept>
namespace trafficsim {
namespace {
Node& find(ProjectDocument& d, const std::string& id) {
    const auto found = std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const Node& n) { return n.id == id; });
    if (found == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    return *found;
}
}
std::string putNode(ProjectDocument& d, Node value) {
    if (value.id.empty()) {
        value.id = allocateId(d, "node");
        d.network.nodes.push_back(value);
        return value.id;
    }
    auto& node = find(d, value.id);
    value.generated = false;
    node = std::move(value);
    return node.id;
}
void renameNode(ProjectDocument& d, const std::string& id, const std::string& name) {
    auto& node = find(d, id);
    node.name = name; node.generated = false;
}
void deleteNode(ProjectDocument& d, const std::string& id) {
    find(d, id);
    std::erase_if(d.network.nodes, [&](const Node& n) { return n.id == id; });
}
std::vector<std::string> suggestNodes(ProjectDocument& d) {
    std::vector<std::string> ids;
    for (auto& node : nodeSuggestions(d.network)) ids.push_back(putNode(d, std::move(node)));
    return ids;
}
}
