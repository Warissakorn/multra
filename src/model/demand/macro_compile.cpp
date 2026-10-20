#include "macro_compile.hpp"
#include <algorithm>
#include <map>

namespace trafficsim {
namespace {
template<class T> std::vector<const T*> byId(const std::vector<T>& values) {
    std::vector<const T*> sorted;
    for (const auto& v : values) sorted.push_back(&v);
    std::sort(sorted.begin(), sorted.end(), [](const T* a, const T* b) { return a->id < b->id; });
    return sorted;
}
double clampStation(double s, double length) { return std::clamp(s, 0.0, length); }
}
MacroCompile compileMacroNetwork(const Network& network, const std::vector<ZoneConnector>& zoneConnectors,
                                 const std::vector<LinkType>& types, const std::vector<std::string>& typePaths,
                                 const std::vector<Vdf>& vdfs) {
    MacroCompile out;
    auto& g = out.graph;
    std::map<std::string, std::size_t> index; // Link id -> its index in the document, for paths
    for (std::size_t i = 0; i < network.links.size(); ++i) index[network.links[i].id] = i;
    std::map<std::string, double> lengths;
    for (const auto& l : network.links) lengths[l.id] = polylineLength(l.geometry);
    // Every station a node stands on, per Link.
    std::map<std::string, std::vector<double>> stations;
    for (const auto& l : network.links) stations[l.id] = {0.0, lengths[l.id]};
    const auto fromStation = [&](const Connector& c) { return clampStation(c.from.station.value_or(lengths[c.from.linkId]), lengths[c.from.linkId]); };
    const auto toStation = [&](const Connector& c) { return clampStation(c.to.station.value_or(0.0), lengths[c.to.linkId]); };
    for (const auto& c : network.connectors) {
        if (stations.contains(c.from.linkId)) stations[c.from.linkId].push_back(fromStation(c));
        if (stations.contains(c.to.linkId)) stations[c.to.linkId].push_back(toStation(c));
    }
    std::map<std::pair<std::string, double>, std::size_t> node;
    for (auto& [id, list] : stations) {
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
        for (const auto s : list) { node[{id, s}] = g.nodes.size(); g.nodes.push_back(id + "@" + std::to_string(s)); }
    }
    // Link pieces, in (Link id, station) order.
    for (const auto* l : byId(network.links)) {
        const auto path = "network.links[" + std::to_string(index[l->id]) + "].linkType";
        const LinkType* type = nullptr;
        if (!l->linkTypeId) out.issues.push_back({"MACRO_LINK_TYPE_MISSING", path});
        else if (const auto t = std::find_if(types.begin(), types.end(), [&](const auto& x) { return x.id == *l->linkTypeId; }); t == types.end())
            out.issues.push_back({"MACRO_LINK_TYPE_UNKNOWN", path});
        else type = &*t;
        const Vdf* vdf = nullptr;
        if (type) {
            const auto v = std::find_if(vdfs.begin(), vdfs.end(), [&](const auto& x) { return x.id == type->vdfId; });
            if (v != vdfs.end()) vdf = &*v;
        }
        const auto& list = stations[l->id];
        for (std::size_t k = 0; k + 1 < list.size(); ++k) {
            MacroLink m;
            m.kind = MacroArc::link; m.source = l->id; m.linkType = l->linkTypeId.value_or(""); m.fromStation = list[k]; m.toStation = list[k + 1];
            m.from = node[{l->id, list[k]}]; m.to = node[{l->id, list[k + 1]}];
            m.length = list[k + 1] - list[k]; m.lanes = static_cast<double>(l->lanes.size());
            if (type) {
                m.capacity = m.lanes * type->capacityPerLane; m.freeTime = m.length / type->freeSpeed; m.modes = type->modes;
                if (vdf) { m.alpha = vdf->alpha; m.beta = vdf->beta; }
            }
            g.links.push_back(std::move(m));
        }
    }
    for (std::size_t k = 0; k < types.size(); ++k)
        if (std::none_of(vdfs.begin(), vdfs.end(), [&](const auto& v) { return v.id == types[k].vdfId; }) &&
            std::any_of(network.links.begin(), network.links.end(), [&](const auto& l) { return l.linkTypeId == types[k].id; }))
            out.issues.push_back({"MACRO_VDF_UNKNOWN", (k < typePaths.size() ? typePaths[k] : types[k].id) + ".vdfId"});
    // Connector arcs, by Connector id; a Connector to an unknown Link is the document check's.
    for (const auto* c : byId(network.connectors)) {
        if (!lengths.contains(c->from.linkId) || !lengths.contains(c->to.linkId)) continue;
        MacroLink m;
        m.kind = MacroArc::connector; m.source = c->id;
        m.from = node[{c->from.linkId, fromStation(*c)}]; m.to = node[{c->to.linkId, toStation(*c)}];
        m.length = polylineLength(c->geometry);
        const auto& from = network.links[index[c->from.linkId]];
        const auto t = std::find_if(types.begin(), types.end(), [&](const auto& x) { return from.linkTypeId == x.id; });
        if (t != types.end()) m.freeTime = m.length / t->freeSpeed;
        g.links.push_back(std::move(m));
    }
    // Centroid arcs, by zone connector id.
    for (const auto* z : byId(zoneConnectors)) {
        if (!lengths.contains(z->linkId)) continue;
        const auto centroid = g.nodes.size();
        g.nodes.push_back("zone-connector:" + z->id);
        MacroLink m;
        m.kind = MacroArc::centroid; m.source = z->id;
        if (z->direction == ZoneDirection::origin) { m.from = centroid; m.to = node[{z->linkId, 0.0}]; g.origins[z->id] = centroid; }
        else { m.from = node[{z->linkId, lengths[z->linkId]}]; m.to = centroid; g.destinations[z->id] = centroid; }
        g.links.push_back(std::move(m));
    }
    return out;
}
}
