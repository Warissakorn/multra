#pragma once
#include "../network/network.hpp"
#include "../../meso/demand.hpp"
#include <optional>

namespace multra {
// M11.3a (docs/reference/MESO_NETWORK.md §6, D295): one authored route (Link and Connector ids in
// travel order) as Meso links and turns. Each Link is driven from 0, or from where the Connector
// before it lands, to its end, or to where the Connector after it leaves; its Meso links inside that
// range are taken in order, so the pieces inside a Node drop out. Between consecutive links the turn
// joining them is taken. Without one (a Connector that does not join its Links, a gap no turn
// crosses, no Meso link at all) nullopt.
// The result has no `source` or `flow`; the caller sets them.
std::optional<MesoPath> mesoPath(const Network&, const MesoGraph&, const std::vector<std::string>& objectIds);
// M11.3b (D296): a runtime route's segments (lane sections and connector paths, as expandRouteless
// writes them) as the Link and Connector ids they belong to, repeats collapsed. Empty when a
// segment is in neither table.
std::vector<std::string> mesoObjectChain(const RuntimeSections&, const std::vector<std::string>& segments);
}
