#pragma once
// M8.3 (D150, D151): small drawn networks for the OD expansion rows ZO6-ZO12.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/od_expansion.hpp"
#include "../src/project/run.hpp"

namespace od_fixture {
using namespace trafficsim;
inline std::vector<Point> square(double x) { return {{x, -300}, {x + 40, -300}, {x + 40, -270}, {x, -270}}; }
inline LaneReference lane(const ProjectDocument& d, const std::string& link, std::size_t k = 0) {
    for (const auto& l : d.network.links) if (l.id == link) return {l.id, l.lanes.at(k).id};
    throw std::invalid_argument("no link " + link);
}
// A blank project (180 s at 0.1 s, the shipped catalogs) with zones A and B.
inline ProjectDocument blank() {
    ProjectDocument d; d.definition = AuthoringDefinition{};
    putZone(d, {"zone-a", "A", square(0)});
    putZone(d, {"zone-b", "B", square(100)});
    return d;
}
inline std::string origin(ProjectDocument& d, const std::string& id, const std::string& link, double share = 1) {
    return putZoneConnector(d, {id, "zone-a", link, "origin", share});
}
inline std::string destination(ProjectDocument& d, const std::string& id, const std::string& link, double share = 1) {
    return putZoneConnector(d, {id, "zone-b", link, "destination", share});
}
// One matrix A -> B of `trips` over [start, end), vehicle type car.
inline OdMatrix matrix(double trips, double start = 0, double end = 180, std::string id = "od-1") {
    return {std::move(id), "", start, end, "car", "", {"zone-a", "zone-b"}, {{0, trips}, {0, 0}}};
}
// o -> k through one Connector: the simplest runnable pair.
struct Line { ProjectDocument d; std::string o, k, joint; };
inline Line line(int lanes = 1) {
    Line l{blank(), {}, {}, {}};
    l.o = addLink(l.d, {{0, 0}, {100, 0}}, lanes, 3.5);
    l.k = addLink(l.d, {{130, 0}, {230, 0}}, lanes, 3.5);
    l.joint = addConnector(l.d, lane(l.d, l.o), lane(l.d, l.k));
    return l;
}
// `count` one-lane Links in a row, each joined to the next by one Connector: 2(count-1) steps.
inline std::vector<std::string> chainOfLinks(ProjectDocument& d, int count, double y = 0) {
    std::vector<std::string> links;
    for (int i = 0; i < count; ++i) links.push_back(addLink(d, {{i * 130.0, y}, {i * 130.0 + 100, y}}, 1, 3.5));
    for (int i = 0; i + 1 < count; ++i) addConnector(d, lane(d, links[i]), lane(d, links[i + 1]));
    return links;
}
// `count` two-lane Links in a row, each joint carrying four one-lane Connectors (0-0, 0-1, 1-0,
// 1-1): the search's level doubles twice per joint, 4^h partial chains at step 2h-1.
inline std::vector<std::string> ladder(ProjectDocument& d, int count) {
    std::vector<std::string> links;
    for (int i = 0; i < count; ++i) links.push_back(addLink(d, {{i * 130.0, 0}, {i * 130.0 + 100, 0}}, 2, 3.5));
    for (int i = 0; i + 1 < count; ++i)
        for (std::size_t from = 0; from < 2; ++from)
            for (std::size_t to = 0; to < 2; ++to) addConnector(d, lane(d, links[i], from), lane(d, links[i + 1], to));
    return links;
}
inline std::filesystem::path data() { return test::root() / "data"; }
inline bool has(const std::vector<Diagnostic>& rows, const std::string& code, const std::string& path, const std::string& object) {
    return std::any_of(rows.begin(), rows.end(), [&](const auto& r) {
        return r.code == code && r.path == path && r.objectId == object && r.selectId.empty(); });
}
// A row names a generated object by its id, or by a path past the authored routes or inputs.
inline bool namesGenerated(const std::vector<Diagnostic>& rows, const AuthoringDefinition& authored) {
    return std::any_of(rows.begin(), rows.end(), [&](const auto& r) {
        const auto ref = generatedRef(r.path);
        const auto count = ref && ref->route ? authored.routes.size() : authored.inputs.size();
        return r.objectId.starts_with("od:") || (ref && ref->index >= count); });
}
}
