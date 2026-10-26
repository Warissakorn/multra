#pragma once
#include "right_of_way.hpp"

namespace multra {
class ConnectorSurfaceCache;
inline constexpr double kConflictRailOffset=0.5; // metres normal to each painted rail
// Mouth station spans belong to the attached Link, never an extension of Connector stations.
struct ConflictMouthBand { ConflictSide lane;ConflictPolygons clips; };
// Measured geometry and its separate station spans along each participant's lane.
// Merge spans describe physical mouths; they do not replace the runtime admission extents.
struct ConflictAreaGeometry {
    ConflictPolygons polygons;
    std::vector<ConflictSide> first,second;
    std::vector<ConflictMouthBand> firstMouth,secondMouth;
};
std::vector<Point> conflictBandOutline(const Network&,const ConflictSide&);
std::vector<ConflictMouthBand> conflictMouthBands(const Network&,const ControlPathRef&,bool source,bool target);
std::vector<ConflictMouthBand> conflictMouthBands(const Network&,const ControlPathRef&,bool source,bool target, ConnectorSurfaceCache* cache);
ConflictAreaGeometry conflictAreaGeometry(const Network&,ConflictKind,const ConflictSide&,const ConflictSide&);
ConflictAreaGeometry conflictAreaGeometry(const Network&,ConflictKind,const ConflictSide&,const ConflictSide&, ConnectorSurfaceCache* cache);
}
