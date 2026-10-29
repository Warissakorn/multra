#pragma once
#include "history.hpp"
#include <map>
namespace multra {
// Shared F4/J3b HCM ranks; a minor yielding turn Stops, a signal turn does not.
struct GeneratedTurn { std::string inArm; int rank{}; bool minor{}; std::vector<std::size_t> stages; bool signal{}; std::size_t order{}; };
void authorNodePriority(ProjectDocument&, const std::map<std::string,GeneratedTurn>&, const PriorityDefaults&);
}
