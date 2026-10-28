// JT1 (JUNCTION_TEMPLATES §13, D389): schema 59 renamed the Road keys. A file below 59 keeps
// `network.roads`, a Link's `roadId` and a zone connector's `roadId`/`roadDirection`; from 59 they are
// `streetSegments`, `streetSegmentId` and `streetSegmentDirection`. Each version refuses the other's.
#include "test.hpp"
#include "street_segment_zone_fixture.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/project/street_segment_codec.hpp"
using namespace multra;
namespace {
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(Json::parse(j.dump())); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
Json as58(const Json& j) { return test::atSchema(j, 58); }
// A two-way street segment with generated Links, so both Links carry `streetSegmentId`.
ProjectDocument withLinks() {
    ProjectDocument d;
    putCrossSection(d, {"divided", "2+2 divided", 2, 2, 2, 3.25, {}, 2.0, {}, "Test values"}, {});
    putStreetSegment(d, {"", "Main", {{0, 0}, {200, 0}}, std::string("divided"), 0, {}, {}});
    applyTemplate(d, d.network.streetSegments[0].id, {});
    return d;
}
}
TEST(streetsegmentschema, jt1_keys_renamed_at_59_and_older_files_read) {
    const auto zones = test::streetSegmentZoneFixture(test::root() / "tests/macro-two-route.traffic.json");
    const auto links = withLinks();
    CHECK(!zones.zones.zoneConnectors.empty() && links.network.links.size() == 2); // forcing: every renamed key is in use
    for (const auto* d : {&zones, &links}) {
        const auto j = documentJson(*d);
        const auto text = j.dump();
        CHECK(j["schemaVersion"] == 59 && j["network"].contains("streetSegments"));
        CHECK(text.find("\"roads\"") == std::string::npos && text.find("\"roadId\"") == std::string::npos &&
              text.find("\"roadDirection\"") == std::string::npos);
        CHECK(parseDocument(Json::parse(text)) == *d);
        const auto old = as58(j);
        CHECK(old["network"].contains("roads"));
        CHECK(parseDocument(old) == *d); // an older file reads to the same document
        CHECK(documentJson(parseDocument(old)) == j); // and saves at 59
    }
    // Each version refuses the other's names, at the key the file holds.
    auto newAt58 = as58(documentJson(links)); newAt58["network"]["streetSegments"] = newAt58["network"]["roads"];
    CHECK(refusedAt(newAt58, "EDIT_UNSUPPORTED_FIELD", "network.streetSegments"));
    auto idAt58 = as58(documentJson(links)); idAt58["network"]["links"][0]["streetSegmentId"] = "x";
    CHECK(refusedAt(idAt58, "EDIT_UNSUPPORTED_FIELD", "network.links[0].streetSegmentId"));
    auto oldAt59 = documentJson(links); oldAt59["network"]["roads"] = oldAt59["network"]["streetSegments"];
    CHECK(refusedAt(oldAt59, "EDIT_UNSUPPORTED_FIELD", "network.roads"));
    auto oldIdAt59 = documentJson(links); oldIdAt59["network"]["links"][0]["roadId"] = "x";
    CHECK(refusedAt(oldIdAt59, "EDIT_UNSUPPORTED_FIELD", "network.links[0].roadId"));
    auto zoneNewAt58 = as58(documentJson(zones)); zoneNewAt58["zoneConnectors"][0]["streetSegmentId"] = "west";
    CHECK(refusedAt(zoneNewAt58, "EDIT_UNSUPPORTED_FIELD", "zoneConnectors[0].streetSegmentId"));
    auto zoneOldAt59 = documentJson(zones); zoneOldAt59["zoneConnectors"][0]["roadDirection"] = "forward";
    CHECK(refusedAt(zoneOldAt59, "EDIT_UNSUPPORTED_FIELD", "zoneConnectors[0].roadDirection"));
    // A malformed entry in an older file is named by that file's key.
    auto bad = as58(documentJson(links)); bad["network"]["roads"][0]["name"] = 1;
    CHECK(refusedAt(bad, "EDIT_STREET_SEGMENT_INVALID", "network.roads[0].name"));
    // A document using none of the renamed keys keeps its version: owned templates alone stay at 53.
    ProjectDocument owned;
    putCrossSection(owned, {"divided", "2+2 divided", 2, 2, 2, 3.25, {}, 2.0, {}, "Test values"}, {});
    CHECK(!usesStreetSegmentKeys(owned) && documentJson(owned)["schemaVersion"] == 53);
}
