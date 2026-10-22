#pragma once
#include "graph.hpp"

// M11.3a (docs/reference/MESO_NETWORK.md §6, D295): the paths Meso loads until M9, as plain values.
// `mesoDemand` (src/project/meso_input.*) fills them from Micro's authored routes; the LTM engine
// (M11.3) reads the turn shares and entry flows.
namespace multra {
struct MesoInterval { double start{}, end{}, vehiclesPerHour{}; bool operator==(const MesoInterval&) const = default; }; // run s, [start, end)
// One authored route as Meso links and the turns between them: turns[k] joins links[k] and links[k + 1].
struct MesoPath {
    std::string source;                 // the route id
    std::vector<std::size_t> links, turns;
    std::vector<MesoInterval> flow;     // veh/h entering at links.front()
    bool operator==(const MesoPath&) const = default;
};
// One vehicle class: "composition:<id>" or "type:<id>", with its PCU and its paths by source.
struct MesoClass {
    std::string id; double pcu{}; std::vector<MesoPath> paths;
    bool operator==(const MesoClass&) const = default;
};
// A path's veh/h at time t: the sum of the intervals holding t.
double mesoPathFlow(const MesoPath&, double t);
// Per turn: the class's share of its in-link flow that takes the turn at time t, counted by the path
// flows in force at t (not shifted by travel time). 0 where the in-link carries none of the class;
// the rest of an in-link's flow ends there, at its paths' last link.
std::vector<double> mesoTurnShares(const MesoGraph&, const MesoClass&, double t);
// Per link: the class's veh/h entering the network at the link's upstream end at time t.
std::vector<double> mesoEntryFlows(const MesoGraph&, const MesoClass&, double t);
}
