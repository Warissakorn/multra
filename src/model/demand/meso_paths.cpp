#include "meso_paths.hpp"
#include <algorithm>
#include <map>

namespace multra {
std::optional<MesoPath> mesoPath(const Network& network, const MesoGraph& g, const std::vector<std::string>& objectIds) {
    std::map<std::string, double> lengths;
    for (const auto& l : network.links) lengths[l.id] = polylineLength(l.geometry);
    std::map<std::string, const Connector*> connectors;
    for (const auto& c : network.connectors) connectors[c.id] = &c;
    const auto clamp = [&](const std::string& link, double s) { return std::clamp(s, 0.0, lengths[link]); };
    // The route's Links with the range driven on each, checking every Connector joins its neighbours.
    struct Span { std::string link; double entry, exit; };
    std::vector<Span> spans;
    const Connector* arriving = nullptr;
    for (std::size_t k = 0; k < objectIds.size(); ++k) {
        const auto& id = objectIds[k];
        if (lengths.contains(id)) {
            if (arriving && arriving->to.linkId != id) return std::nullopt;
            if (!arriving && !spans.empty()) return std::nullopt; // two Links with no Connector between
            spans.push_back({id, arriving ? clamp(id, arriving->to.station.value_or(0.0)) : 0.0, lengths[id]});
            arriving = nullptr;
        } else if (const auto c = connectors.find(id); c != connectors.end()) {
            if (spans.empty() || arriving || c->second->from.linkId != spans.back().link) return std::nullopt;
            spans.back().exit = clamp(spans.back().link, c->second->from.station.value_or(lengths[spans.back().link]));
            arriving = c->second;
        } else return std::nullopt;
    }
    if (arriving) return std::nullopt; // ends on a Connector
    constexpr double eps = 1e-9;
    MesoPath path;
    for (const auto& s : spans)
        for (std::size_t i = 0; i < g.links.size(); ++i)
            if (g.links[i].source == s.link && g.links[i].fromStation >= s.entry - eps && g.links[i].toStation <= s.exit + eps) path.links.push_back(i);
    if (path.links.empty()) return std::nullopt;
    // compileMesoNetwork keeps one turn per (in, out) pair, at a Node and at a simple node alike.
    for (std::size_t k = 0; k + 1 < path.links.size(); ++k) {
        const auto t = std::find_if(g.turns.begin(), g.turns.end(), [&](const auto& x) { return x.in == path.links[k] && x.out == path.links[k + 1]; });
        if (t == g.turns.end()) return std::nullopt;
        path.turns.push_back(static_cast<std::size_t>(t - g.turns.begin()));
    }
    return path;
}
}
