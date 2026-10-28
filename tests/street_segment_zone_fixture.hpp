#pragma once
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace test {
// Test-only parameters and the owned catalogs from the existing Macro fixture. Two two-way
// StreetSegments meet end-to-end; neither generates Links. All counts are vehicle trips in one hour.
inline multra::ProjectDocument streetSegmentZoneFixture(const std::filesystem::path& macroFixture) {
    using namespace multra;
    std::ifstream file(macroFixture); Json j; file >> j;
    auto d = parseDocument(j);
    d.network = {}; d.network.id = "road-network";
    d.definition->linkTypes[0].jamDensity = JamDensity{120, "Test values"}; // reach Meso's demand check
    putCrossSection(d, {"road-section", "Test section", 2, 1, 1, 3.5, {}, 0, std::string("arterial"), "Test values"}, {});
    putStreetSegment(d, {"west", "West", {{-200, 0}, {0, 0}}, std::string("road-section"), 0, {}, {}});
    putStreetSegment(d, {"east", "East", {{0, 0}, {200, 0}}, std::string("road-section"), 0, {}, {}});
    d.zones.zoneConnectors = {
        {"west-in", "site", "", ZoneDirection::origin, 1, "west", true},
        {"east-out", "town", "", ZoneDirection::destination, 1, "east", true},
        {"east-in", "town", "", ZoneDirection::origin, 1, "east", false},
        {"west-out", "site", "", ZoneDirection::destination, 1, "west", false}};
    d.zones.odMatrices[0].cells = {{0, 1200}, {300, 0}};
    return d;
}
}
