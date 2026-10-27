#pragma once
#include "right_of_way.hpp"
namespace multra {
class ConnectorSurfaceCache;
// Calculation uses the same painted lane rails as display and Connector path derivation.
// Stations are metres on the authored reference, so file values need no parameter adapter.
struct ConflictSurface {
    std::vector<Point> base, left, right;
    std::vector<double> stations; // Authored reference metres
};
std::optional<ConflictSurface> conflictSurface(const Network&, const ControlPathRef&);
std::optional<ConflictSurface> conflictSurface(const Network&, const ControlPathRef&, ConnectorSurfaceCache* cache);
std::vector<Point> conflictRuntimeOutline(const Network&, const ConflictSide&);
}
