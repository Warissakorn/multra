#pragma once
#include "document.hpp"
namespace trafficsim {
std::vector<Point> routeMarkerGeometry(const Network&,const std::string& objectId);
Json routeEndMarkersJson(const ProjectDocument&);
void parseRouteEndMarkers(const Json&,int schema,ProjectDocument&);
void validateRouteEndMarkers(const ProjectDocument&);
}
