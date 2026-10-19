#pragma once
#include "document.hpp"
#include <filesystem>
#include <map>
namespace trafficsim {
class InputManifest;
struct DemandCatalog {
    std::vector<VehicleType> vehicleTypes;
    std::vector<DriverBehaviour> behaviours;
    std::vector<Composition> compositions;
    std::map<std::string,std::string> vehicleTypeNames;
    std::vector<Mode> modes; // S1.2 (D269): owned by putDemandCatalog only when a matrix names a mode
};
// Resolve authored ownership without expanding routes, lanes or compositions.
DemandCatalog resolveDemandCatalog(const AuthoringDefinition&,const std::filesystem::path&,
                                   bool includeCompositions=true, InputManifest* =nullptr);
// Validate every owned composition and input reference, including unused entries.
std::vector<ValidationIssue> ownedCatalogIssues(const AuthoringDefinition&);
}
