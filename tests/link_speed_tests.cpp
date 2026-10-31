// JT2 (JUNCTION_TEMPLATES §9, §13, D380, D390): the posted speed is the Link's `speedLimit` (km/h),
// schema 60. Schemas 58-59 held it on a Node analysis approach row; reading one moves it to the Link
// the row names, and no authored speed is dropped.
#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/project/link_speed_codec.hpp"
#include "../src/project/node_analysis_run.hpp"
#include <cmath>
#include <fstream>
using namespace multra;
namespace {
Json file(const std::filesystem::path& path) { std::ifstream in(test::root() / path); Json j; in >> j; return j; }
// The A1/A2 fixture, schema 58: 50 km/h on the approach rows of link-4, -16, -28 and -40.
Json fixture58() { return file("tests/node-analysis-four-leg.traffic.json"); }
const Link& link(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; });
}
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(Json::parse(j.dump())); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
std::vector<double> queues(const ProjectDocument& d) {
    std::vector<double> out;
    // Keep the run alive: ranging over runNodeAnalysis(...).nodes.front().groups binds through
    // front(), so the temporary dies before the loop (read garbage on MSVC Release).
    const auto run = runNodeAnalysis(d, test::root() / "data");
    for (const auto& g : run.nodes.front().groups) out.push_back(g.queue ? g.queue->mean : -1);
    return out;
}
}
TEST(linkspeed, jt2_link_speed_round_trips_and_absent_keeps_the_bytes) {
    for (const auto* name : {"four-leg-signalised", "t-junction-priority", "m2.6-study-template", "lane-change-lab"}) {
        const auto j = file(std::filesystem::path("data/projects") / (std::string(name) + ".traffic.json"));
        CHECK(documentJson(parseDocument(j)) == j); // no speed: the shipped bytes and version stay
    }
    auto d = parseDocument(file("data/projects/four-leg-signalised.traffic.json"));
    CHECK(documentJson(d)["schemaVersion"] < 60); // forcing: the speed is what raises it
    setLinkSpeedLimit(d, "link-4", 42.5);
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 60);
    CHECK(parseDocument(saved) == d && documentJson(parseDocument(saved)) == saved);
    CHECK(link(parseDocument(saved), "link-4").speedLimit == 42.5);
    setLinkSpeedLimit(d, "link-4", std::nullopt);
    CHECK(!usesLinkSpeedLimits(d));
    test::throws([&] { setLinkSpeedLimit(d, "link-4", 0); }, "INVALID_LINK_SPEED");
    test::throws([&] { setLinkSpeedLimit(d, "link-4", std::nan("")); }, "INVALID_LINK_SPEED");
    // Refusals: a malformed speed, a 59 file with a Link speed, a 60 file with an approach speed.
    auto bad = saved;
    for (auto& l : bad["network"]["links"]) if (l["id"] == "link-4") l["speedLimit"] = -5;
    CHECK(!refusedAt(saved, "INVALID_LINK_SPEED", "network.links[0].speedLimit")); // forcing
    std::size_t at = 0;
    while (saved["network"]["links"][at]["id"] != "link-4") ++at;
    CHECK(refusedAt(bad, "INVALID_LINK_SPEED", "network.links[" + std::to_string(at) + "].speedLimit"));
    auto old = saved; old["schemaVersion"] = 59;
    CHECK(refusedAt(old, "EDIT_UNSUPPORTED_FIELD", "network.links[" + std::to_string(at) + "].speedLimit"));
    auto rowAt60 = documentJson(parseDocument(fixture58()));
    rowAt60["network"]["nodes"][0]["analysis"]["approaches"][0]["speedLimit"] = 50;
    CHECK(refusedAt(rowAt60, "EDIT_UNSUPPORTED_FIELD", "network.nodes[0].analysis.approaches[0].speedLimit"));
}
TEST(linkspeed, jt2_approach_speeds_move_to_their_links) {
    const auto j = fixture58();
    CHECK(j["schemaVersion"] == 58 && j["network"]["nodes"][0]["analysis"]["approaches"][0].contains("speedLimit")); // forcing
    const auto d = parseDocument(j);
    for (const auto* in : {"link-4", "link-16", "link-28", "link-40"}) CHECK(link(d, in).speedLimit == 50.0);
    CHECK(std::count_if(d.network.links.begin(), d.network.links.end(), [](const auto& l) { return l.speedLimit; }) == 4);
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 60);
    for (const auto& row : saved["network"]["nodes"][0]["analysis"]["approaches"]) CHECK(!row.contains("speedLimit"));
    CHECK(parseDocument(saved) == d);
    // Node analysis reads the in-leg's speed: every lane group keeps a queue, and the 60 file gives
    // the same queues as the 58 one.
    const auto before = queues(d);
    CHECK(before.size() == 12 && std::all_of(before.begin(), before.end(), [](double q) { return q >= 0; }));
    CHECK(queues(parseDocument(saved)) == before);
    auto slower = d; setLinkSpeedLimit(slower, "link-4", 30);
    CHECK(queues(slower) != before); // the Link's value is the one read
    // Nothing authored is dropped: a row naming no Link, or one Link given two speeds, is refused.
    auto stray = j; stray["network"]["nodes"][0]["analysis"]["approaches"][0]["link"] = "nowhere";
    CHECK(refusedAt(stray, "EDIT_NODE_INVALID", "network.nodes[0].analysis.approaches[0].speedLimit"));
    auto twice = j;
    auto& rows = twice["network"]["nodes"][0]["analysis"]["approaches"];
    auto copy = rows[0]; copy["speedLimit"] = 60; rows.push_back(copy);
    CHECK(refusedAt(twice, "EDIT_NODE_INVALID", "network.nodes[0].analysis.approaches[" + std::to_string(rows.size() - 1) + "].speedLimit"));
    auto same = j; same["network"]["nodes"][0]["analysis"]["approaches"].push_back(j["network"]["nodes"][0]["analysis"]["approaches"][0]);
    CHECK(link(parseDocument(same), "link-4").speedLimit == 50.0); // an equal repeat is one speed
}
TEST(linkspeed, jt2_template_speed_reaches_generated_links) {
    ProjectDocument d;
    putCrossSection(d, {"divided", "2+2 divided", 2, 2, 2, 3.25, {}, 2.0, {}, "Test values", 60.0}, {});
    const auto id = putStreetSegment(d, {"", "Main", {{0, 0}, {200, 0}}, std::string("divided"), 0, {}, {}});
    applyTemplate(d, id, {});
    CHECK(d.network.links.size() == 2);
    for (const auto& l : d.network.links) CHECK(l.speedLimit == 60.0);
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 60 && saved["definition"]["crossSections"][0]["speedLimit"] == 60.0);
    CHECK(parseDocument(saved) == d);
    auto old = saved; old["schemaVersion"] = 59;
    for (auto& l : old["network"]["links"]) l.erase("speedLimit");
    CHECK(refusedAt(old, "EDIT_UNSUPPORTED_FIELD", "definition.crossSections[0].speedLimit"));
    // A template without a speed leaves a Link's own; a split keeps it on both pieces.
    setLinkSpeedLimit(d, d.network.links[0].id, 45);
    d.definition->crossSections[0].speedLimit.reset();
    applyTemplate(d, id, {});
    CHECK(d.network.links[0].speedLimit == 45.0);
    const auto first = d.network.links[0].id;
    splitLink(d, first, 100);
    CHECK(d.network.links.size() == 3 && d.network.links.back().speedLimit == 45.0 && link(d, first).speedLimit == 45.0);
}
