#pragma once
#include "macro_run.hpp"
#include "meso_run.hpp"
#include <optional>

namespace multra {
// S4.4 (READINESS §5, §12, D317): the Nodes a run points at, step 6 of the workflow loop. The
// program only lists them; it never refines one. Pure, never throws, changes nothing.
// - A hot arc: a Macro `link` arc with capacity and load / capacity >= 1.0 (macroJson's V/C), or a
//   Meso link with spill-back in at least one slice (mesoLinkSlices).
// - A hot arc is a leg of a Node when its source names one of the Node's nodeLinkLegs or
//   nodeStreetSegmentLegs and its station range reaches that leg's crossing, within streetSegmentEndTolerance, from
//   the outside: Macro keeps a detailed Node's Links whole, Meso cuts them at the boundary.
// - A hot arc that is a leg of no Node gives a row per end that is a junction (a Macro node with a
//   Connector arc, a Meso node with turns and no Node), named `<source>@<station>`; boundary ends none.
// Node rows come in Node order, then no-Node rows by end; legs in graph order. A run of another
// revision than the document's proposes nothing.
enum class ProposalLevel { macro, meso };
struct ProposalLeg {
    std::string source; double fromStation{}, toStation{};
    std::optional<double> vc;   // Macro
    std::size_t spillSlices{};  // Meso
    bool operator==(const ProposalLeg&) const = default;
};
struct NodeProposal {
    ProposalLevel level{}; std::uint64_t revision{};
    std::string nodeId, end;    // exactly one is set
    std::vector<ProposalLeg> legs;
    bool operator==(const NodeProposal&) const = default;
};
std::vector<NodeProposal> macroProposals(const ProjectDocument&, const MacroRun&);
std::vector<NodeProposal> mesoProposals(const ProjectDocument&, const MesoRun&);
}
