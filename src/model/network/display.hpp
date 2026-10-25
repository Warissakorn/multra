#pragma once
#include <map>
#include <string>
#include <vector>
namespace multra {
struct DisplayLevel {int order{};std::map<std::string,std::string> name;};
struct DisplayType {
    std::string id;
    std::map<std::string,std::string> name;
    std::string linkColor,connectorColor,laneColor,vehicleColor;
};
// vehicleColors: vehicle-type id -> #rrggbb, from data/vehicle-appearance (D97). A type with no
// entry is drawn in its segment's DisplayType::vehicleColor.
struct DisplayCatalog {
    std::vector<DisplayLevel> levels;std::vector<DisplayType> types;
    std::map<std::string,std::string> vehicleColors;
    // Optional display-only archetype. Omission retains the legacy length-based choice.
    std::map<std::string,std::string> vehicleShapes;
    // User overrides: one colour fixes a type; several colours mix within that type.
    std::map<std::string,std::vector<std::string>> vehicleColorPalettes;
    std::vector<std::string> availableVehicleColors;
};
}
