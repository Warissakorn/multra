#pragma once
#include "document.hpp"
namespace multra {
std::vector<Point> routeDestinationGeometry(const Network&, const std::string& objectId);
void validateRouteDestinations(const ProjectDocument&);
void parseRouteDestinations(const Json& definition, int schema, AuthoringDefinition&);
void addRouteDestinationsJson(const AuthoringDefinition&, Json& definition);
}
