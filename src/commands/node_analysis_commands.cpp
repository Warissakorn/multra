#include "node_analysis_commands.hpp"
#include <algorithm>
#include <stdexcept>

namespace multra {
void setNodeAnalysis(ProjectDocument& d, const std::string& id, std::optional<NodeAnalysis> analysis) {
    const auto node = std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const auto& n) { return n.id == id; });
    if (node == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    if (analysis && *analysis == NodeAnalysis{}) analysis.reset();
    node->analysis = std::move(analysis);
    node->generated = false;
}
}
