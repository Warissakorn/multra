#pragma once
#include "history.hpp"
#include <optional>
namespace multra {
// M13 A1 (NODE_ANALYSIS §4, D371): sets or clears a Node's `analysis` (an all-default value is stored as
// none) and, as any edit of a Node does, clears `generated`. EDIT_UNKNOWN_OBJECT when absent. The
// method's ranges are checked when the analysis runs, not here.
void setNodeAnalysis(ProjectDocument&, const std::string& id, std::optional<NodeAnalysis>);
}
