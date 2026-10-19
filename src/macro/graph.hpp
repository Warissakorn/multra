#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// S2.1 (docs/reference/MACRO_NETWORK.md §3, D273): the Macro graph, plain values with no I/O. The
// compiler (src/model/network/macro_compile.*) fills it; the assignment (S2.2) only reads it.
namespace trafficsim {
enum class MacroArc { link, connector, centroid };
// One directed arc. A `link` is a piece of a drawn Link between two Macro nodes; a `connector` or a
// `centroid` arc is uncapacitated (capacity 0) with a fixed time.
struct MacroLink {
    MacroArc kind{MacroArc::link};
    std::string source;              // the Link, Connector or zone connector id
    double fromStation{}, toStation{}; // on the Link for a `link`; 0 otherwise
    std::size_t from{}, to{};        // node indices
    double length{}, lanes{}, capacity{}, freeTime{};
    double alpha{}, beta{};          // the BPR parameters (§5); unused while capacity is 0
    std::vector<std::string> modes;  // empty: every mode may use it
    bool operator==(const MacroLink&) const = default;
};
struct MacroGraph {
    std::vector<std::string> nodes;  // a name per node: `<link>@<station>` or `zone:<id>:in|out`
    std::vector<MacroLink> links;    // links in (Link id, station) order, then Connectors, then centroids
    std::map<std::string, std::size_t> origins, destinations; // zone id -> its node
    bool operator==(const MacroGraph&) const = default;
};
// May a class of `mode` (empty: a composition class, every link) use this arc?
bool allows(const MacroLink&, const std::string& mode);
// The nodes reachable from `from` over the arcs `mode` may use.
std::vector<bool> reachable(const MacroGraph&, std::size_t from, const std::string& mode);
}
