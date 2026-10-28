// S3.7 rows RD20 (Micro half), RD21, RD22 and RD29 (docs/reference/STREET_SEGMENTS.md §7, §13,
// STREET_SEGMENTS_IMPLEMENTED §21, D312): seam checks, on demand, by level. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/model/demand/macro_compile.hpp"
#include "../src/model/demand/seam_checks.hpp"
#include <algorithm>
using namespace multra;
namespace {
bool row(const std::vector<SeamRow>& rows, const std::string& code, const std::string& path, SeamSeverity severity) {
    return std::find(rows.begin(), rows.end(), SeamRow{code, path, severity}) != rows.end();
}
std::size_t count(const std::vector<SeamRow>& rows, const std::string& code) {
    return static_cast<std::size_t>(std::count_if(rows.begin(), rows.end(), [&](const auto& r) { return r.code == code; }));
}
// Test values only: no template or type ships for these (D304).
CrossSection section(const char* id, int lanes) { return {id, id, 2, lanes, lanes, 3.5, {}, 0, std::string("arterial"), "Test values"}; }
LinkType type(const char* id, double kmh) { return {id, id, 1000, kmh / 3.6, "bpr", {}, "Test values", JamDensity{150, "Test value"}}; }
const std::vector<CrossSection>& owned(const ProjectDocument& d) { return d.definition->crossSections; }
}
TEST(streetsegmentseams, rd20_micro_reports_roads_without_links) { // RD20, the Micro half
    ProjectDocument d;
    putCrossSection(d, section("ns", 1), {});
    putStreetSegment(d, {"", "", {{0, 0}, {100, 0}}, std::string("ns"), 0, {}, {}});
    const auto rows = seamChecks(d.network, {type("arterial", 50)}, owned(d), SeamLevel::micro);
    CHECK(rows == (std::vector<SeamRow>{{"SEAM_STREET_SEGMENT_NOT_GENERATED", "network.streetSegments[0]", SeamSeverity::advisory}})); // forcing
    // Meso and Macro compile it, so they report nothing.
    CHECK(seamChecks(d.network, {type("arterial", 50)}, owned(d), SeamLevel::meso).empty());
    CHECK(seamChecks(d.network, {type("arterial", 50)}, owned(d), SeamLevel::macro).empty());
    applyTemplate(d, d.network.streetSegments[0].id, {});
    CHECK(seamChecks(d.network, {type("arterial", 50)}, owned(d), SeamLevel::micro).empty());
}
TEST(streetsegmentseams, rd21_lane_count) { // RD21
    ProjectDocument d;
    putCrossSection(d, section("wide", 3), {});
    const auto road = putStreetSegment(d, {"", "", {{0, 0}, {200, 0}}, std::string("wide"), 0, {}, {}});
    const auto links = applyTemplate(d, road, {});
    CHECK(seamChecks(d.network, {}, owned(d), SeamLevel::micro).empty());
    // The template narrows to two lanes and is not applied again: 3-lane Links on a 2-lane StreetSegment.
    putCrossSection(d, section("wide", 2), {});
    const auto rows = seamChecks(d.network, {}, owned(d), SeamLevel::micro);
    CHECK(count(rows, "SEAM_LANE_COUNT") == 2); // forcing: both directions
    CHECK(row(rows, "SEAM_LANE_COUNT", "network.links[0]", SeamSeverity::warning));
    CHECK(row(seamChecks(d.network, {}, owned(d), SeamLevel::meso), "SEAM_LANE_COUNT", "network.links[0]", SeamSeverity::advisory));
    // Use Link on the forward Link: the StreetSegment's override follows Micro, in one Command.
    History h; h.reset(d);
    CHECK(h.execute("editorSeamUseLink", [&](auto& x) { useLinkLanes(x, links[0]); }));
    CHECK(h.document().network.streetSegments[0].overrides.lanesForward == 3 && !h.document().network.streetSegments[0].overrides.lanesBackward);
    CHECK(count(seamChecks(h.document().network, {}, owned(h.document()), SeamLevel::micro), "SEAM_LANE_COUNT") == 1);
    // Use Node on the backward Link: the Links follow the StreetSegment, keeping their ids.
    CHECK(h.execute("editorSeamUseNode", [&](auto& x) { useStreetSegmentLanes(x, links[1], {}); }));
    const auto& n = h.document().network;
    CHECK(seamChecks(n, {}, owned(h.document()), SeamLevel::micro).empty());
    CHECK(n.links[0].id == links[0] && n.links[0].lanes.size() == 3 && n.links[1].id == links[1] && n.links[1].lanes.size() == 2);
    h.undo(); h.undo();
    CHECK(count(seamChecks(h.document().network, {}, owned(h.document()), SeamLevel::micro), "SEAM_LANE_COUNT") == 2);
    // A Link without a StreetSegment has nothing to follow.
    auto plain = d; const auto loose = addLink(plain, {{0, 50}, {100, 50}}, 1, 3.5);
    try { useLinkLanes(plain, loose); CHECK(false); } catch (const std::invalid_argument& e) { CHECK(std::string(e.what()) == "EDIT_SEAM_NOT_GENERATED"); }
}
TEST(streetsegmentseams, rd22_speed_jump) { // RD22
    ProjectDocument d;
    const auto fast = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), slow = addLink(d, {{110, 0}, {200, 0}}, 1, 3.5);
    setLinkType(d, fast, std::string("fast")); setLinkType(d, slow, std::string("slow"));
    const auto& lanes = [&](const std::string& id) { return std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; })->lanes; };
    addConnector(d, {fast, lanes(fast)[0].id}, {slow, lanes(slow)[0].id});
    const std::vector<LinkType> types{type("fast", 100), type("slow", 60)};
    for (const auto level : {SeamLevel::micro, SeamLevel::meso, SeamLevel::macro})
        CHECK(seamChecks(d.network, types, {}, level) == (std::vector<SeamRow>{{"SEAM_SPEED_JUMP", "network.connectors[0]", SeamSeverity::warning}}));
    // A display choice: 40 km/h is not above a 40 km/h threshold, and 20 km/h is not a jump.
    CHECK(seamChecks(d.network, types, {}, SeamLevel::micro, 40).empty());
    CHECK(seamChecks(d.network, {type("fast", 80), type("slow", 60)}, {}, SeamLevel::micro).empty());
    // It refuses nothing: Macro compiles the same network without a seam issue.
    const auto macro = compileMacroNetwork(d.network, {}, types, {"", ""}, {{"bpr", "bpr", 0.15, 4, "Test values", ""}});
    CHECK(std::none_of(macro.issues.begin(), macro.issues.end(), [](const auto& i) { return i.code.rfind("SEAM_", 0) == 0; }));
    // A turn inside a junction is not a corridor.
    d.network.nodes.push_back({"node", "", {{95, -10}, {115, -10}, {115, 10}, {95, 10}}, false});
    CHECK(seamChecks(d.network, types, {}, SeamLevel::micro).empty());
}
TEST(streetsegmentseams, rd29_detached_link_and_link_less_continuation) { // RD29
    ProjectDocument d;
    putCrossSection(d, section("ns", 1), {});
    const auto road = putStreetSegment(d, {"", "", {{0, 0}, {200, 0}}, std::string("ns"), 0, {}, {}});
    const auto links = applyTemplate(d, road, {});
    changeLanes(d, links[1], {3.5, 3.5}); // editing lanes detaches the Link (D305)
    CHECK(d.network.streetSegments[0].links.size() == 1); // forcing
    for (const auto level : {SeamLevel::micro, SeamLevel::meso, SeamLevel::macro})
        CHECK(row(seamChecks(d.network, {}, owned(d), level), "SEAM_LINK_DETACHED", "network.streetSegments[0]", SeamSeverity::advisory));
    // Two Link-less StreetSegments continuing end to start with types 100 and 60 km/h.
    ProjectDocument e;
    auto a = section("fast", 1); a.linkType = "fast"; putCrossSection(e, a, {});
    auto b = section("slow", 1); b.linkType = "slow"; putCrossSection(e, b, {});
    putStreetSegment(e, {"", "", {{0, 0}, {100, 0}}, std::string("fast"), 0, {}, {}});
    putStreetSegment(e, {"", "", {{101, 0}, {200, 0}}, std::string("slow"), 0, {}, {}});
    const std::vector<LinkType> types{type("fast", 100), type("slow", 60)};
    CHECK(row(seamChecks(e.network, types, owned(e), SeamLevel::macro), "SEAM_SPEED_JUMP", "network.streetSegments[1]", SeamSeverity::warning));
}
