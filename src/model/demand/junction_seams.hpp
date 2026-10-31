#pragma once
#include "definition.hpp"
#include "../network/network.hpp"
namespace multra {
// J6/D410: pure advisory comparison of a recipe and its existing drawing. No detail means no seam.
// A refined object, incompatible lane/movement/pocket layout or control/timing mismatch differs.
std::vector<ValidationIssue> junctionSeamIssues(const Network&,const std::vector<JunctionTemplate>&,
    const std::vector<LinkType>&,const std::vector<SignalController>& controllers,double timeStep);
}
