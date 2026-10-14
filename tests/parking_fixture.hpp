#pragma once
// The M8.4b lot study shared by parking_project_tests and parking_output_tests (M8.5).
#include "test.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/run.hpp"

namespace lotfix {
using namespace trafficsim;
inline std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
inline void join(ProjectDocument& d, const std::string& from, double station, const std::string& to) {
    addConnector(d, {from, laneOf(d, from), station}, {to, laneOf(d, to), 0});
}
// Town traffic drives A -> D, the site's driveway, which ends at the lot's gate. The site's own
// traffic leaves by X -> T back to town.
struct Fixture { ProjectDocument d; std::string a, drive, x, t; };
inline Fixture fixture(std::uint32_t capacity = 3, double toSite = 40, double toTown = 1) {
    Fixture f;
    f.a = addLink(f.d, {{0, 0}, {100, 0}}, 1, 3.5);
    f.drive = addLink(f.d, {{120, 0}, {150, 0}}, 1, 3.5);
    f.x = addLink(f.d, {{0, 40}, {30, 40}}, 1, 3.5);
    f.t = addLink(f.d, {{50, 40}, {250, 40}}, 1, 3.5);
    join(f.d, f.a, 100, f.drive);join(f.d, f.x, 30, f.t);
    auto& def = demand(f.d);def.duration = 3600;def.timeStep = 0.5;
    putZone(f.d, {"town", "", {{-20, 0}, {-30, 10}, {-30, -10}}, {}});
    putZone(f.d, {"site", "", {{160, 0}, {170, 10}, {170, -10}}, {}});
    putZoneConnector(f.d, {"town-out", "town", f.a, ZoneDirection::origin, 1});
    putZoneConnector(f.d, {"town-in", "town", f.t, ZoneDirection::destination, 1});
    putZoneConnector(f.d, {"site-in", "site", f.drive, ZoneDirection::destination, 1});
    putZoneConnector(f.d, {"site-out", "site", f.x, ZoneDirection::origin, 1});
    putOdMatrix(f.d, {"am", 0, 1800, "car-only", {"town", "site"}, {{0, toSite}, {toTown, 0}}});
    putParkingLot(f.d, {"lot", "site", capacity, 0, "", 0, {{100, 0}, {200, 1}}});
    return f;
}
}
