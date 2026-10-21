#include "graph.hpp"
#include <algorithm>

namespace multra {
bool allows(const MacroLink& link, const std::string& mode) {
    return mode.empty() || link.modes.empty() || std::find(link.modes.begin(), link.modes.end(), mode) != link.modes.end();
}
std::vector<bool> reachable(const MacroGraph& graph, std::size_t from, const std::string& mode) {
    std::vector<bool> seen(graph.nodes.size(), false);
    if (from >= seen.size()) return seen;
    std::vector<std::vector<std::size_t>> out(seen.size()); // arcs leaving each node, in link order
    for (std::size_t k = 0; k < graph.links.size(); ++k)
        if (allows(graph.links[k], mode)) out[graph.links[k].from].push_back(k);
    std::vector<std::size_t> stack{from};
    seen[from] = true;
    while (!stack.empty()) {
        const auto node = stack.back(); stack.pop_back();
        for (const auto k : out[node])
            if (!seen[graph.links[k].to]) { seen[graph.links[k].to] = true; stack.push_back(graph.links[k].to); }
    }
    return seen;
}
}
