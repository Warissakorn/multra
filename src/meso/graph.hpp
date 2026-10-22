#pragma once
#include <string>
#include <vector>

// M11.2 (docs/reference/MESO_NETWORK.md §3, §4, D293): the Meso graph, plain values with no I/O.
// The compiler (src/model/demand/meso_compile.*) fills it; the LTM engine (M11.3) only reads it.
namespace multra {
enum class MesoPriority { firstYields, secondYields, undetermined };
// One directed link: a piece of a drawn Link between two stations, outside every Node.
struct MesoLink {
    std::string source;                 // the Link id
    double fromStation{}, toStation{};  // metres on the Link's reference polyline
    std::size_t from{}, to{};           // node indices
    double length{}, lanes{};
    double capacityPerLane{}, freeSpeed{}, jamDensity{}; // veh/h/lane, m/s, veh/km/lane; 0 without a type
    std::vector<std::string> modes;     // empty: every mode may use it
    bool operator==(const MesoLink&) const = default;
};
// A movement through a node from one link to another, with its fixed free-flow time.
struct MesoTurn {
    std::size_t node{}, in{}, out{};    // node and link indices
    std::string name;
    double freeTime{};                  // s
    std::vector<std::string> via;       // the Connectors (and Links inside a Node) it crosses
    bool operator==(const MesoTurn&) const = default;
};
struct MesoConflict {
    std::size_t first{}, second{};      // turn indices
    MesoPriority priority{MesoPriority::undetermined};
    bool operator==(const MesoConflict&) const = default;
};
// A drawn Node (`nodeId` set) or a simple junction of Connectors and Link stations.
struct MesoNode {
    std::string name, nodeId;
    std::string control;                // "signalised", "unsignalised" or empty
    std::vector<std::size_t> turns;
    std::vector<MesoConflict> conflicts;
    bool operator==(const MesoNode&) const = default;
};
// The run's step and reporting slice, s (D292 choice 7a); absent in the file = kMesoDefaults.
struct MesoSettings { double step{}, slice{}; bool operator==(const MesoSettings&) const = default; };
inline constexpr MesoSettings kMesoDefaults{1, 300};
struct MesoGraph {
    std::vector<MesoNode> nodes;
    std::vector<MesoLink> links;        // in (Link id, station) order
    std::vector<MesoTurn> turns;        // grouped by node, in node order
    std::vector<std::size_t> entries, exits; // links with no turn into / out of them
    bool operator==(const MesoGraph&) const = default;
};
// The backward wave speed of a link's triangular diagram, m/s and positive: q_max / (k_jam - k_c)
// (§4); 0 without a free speed or a jam density above the critical one.
double mesoWaveSpeed(const MesoLink&);
// The longest step one link allows: min(L/v, L/w). Yperman (2007) Eq. 3.4 prints only L/v; L/w is
// derived here, because the receiving volume (Eq. 4.35) reads the downstream count at t + dt - L/w,
// which must already be known (D298). A link without a wave speed gives L/v.
double mesoLinkTimeLimit(const MesoLink&);
// The longest step the link model allows: the least mesoLinkTimeLimit over links, s (§4). Links
// without a speed are skipped; no link gives 0.
double mesoStepLimit(const MesoGraph&);
// Its shortest link, or links.size() when there is none.
std::size_t mesoShortestLink(const MesoGraph&);
}
