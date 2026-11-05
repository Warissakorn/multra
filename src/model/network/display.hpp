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
    double laneDash{3},laneGap{3}; // D430: dashed lane line, metres on the ground at every zoom
};
// vehicleColors: vehicle-type id -> #rrggbb, from data/vehicle-appearance (D97). A type with no
// entry is drawn in its segment's DisplayType::vehicleColor.
struct DisplayCatalog {
    std::vector<DisplayLevel> levels;std::vector<DisplayType> types;
    std::map<std::string,std::string> vehicleColors;
    // Optional display-only archetype. Omission retains the legacy length-based choice.
    std::map<std::string,std::string> vehicleShapes;
    // Per type: one colour fixes it; several colours mix within it. Starts as the shipped palettes
    // (an optional `palette` in data/vehicle-appearance, D465); a user's choices replace those types.
    std::map<std::string,std::vector<std::string>> vehicleColorPalettes;
    std::map<std::string,std::vector<std::string>> shippedVehiclePalettes;
    std::vector<std::string> availableVehicleColors;
};
}
