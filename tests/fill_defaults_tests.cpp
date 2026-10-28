// S4.3 rows RS5-RS8 (docs/reference/READINESS.md §4, §9, §11, D316): Fill defaults F1-F3, each from a
// value the project already cites, applied only when chosen. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/fill_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/model/demand/macro_compile.hpp"
#include "../src/model/demand/meso_compile.hpp"
#include "../src/model/demand/seam_checks.hpp"
#include "../src/project/link_type_codec.hpp"
#include "../src/project/json.hpp"
#include <algorithm>
#include <fstream>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
// Test values only: no template or owned type ships (D304).
CrossSection section(const char* id, std::optional<std::string> type = "arterial") { return {id, id, 2, 1, 1, 3.5, {}, 0, type, "Test template"}; }
LinkType arterial(std::optional<JamDensity> jam = JamDensity{150, "Test value"}) { return {"arterial", "Arterial", 1000, 10, "bpr", {}, "Test values", jam}; }
ProjectDocument owned() {
    ProjectDocument d;
    d.definition = AuthoringDefinition{}; d.definition->externalLinkTypes = false;
    d.definition->linkTypes = {arterial()};
    putCrossSection(d, section("ns"), {});
    return d;
}
const std::vector<CrossSection>& templates(const ProjectDocument& d) { return d.definition->crossSections; }
std::size_t offered(const std::vector<FillOffer>& offers, FillKind kind, const std::string& id) {
    return static_cast<std::size_t>(std::count_if(offers.begin(), offers.end(), [&](const auto& o) { return o.kind == kind && o.objectId == id; }));
}
bool issue(const std::vector<ValidationIssue>& issues, const std::string& code) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code; });
}
}
TEST(filldefaults, rs5_link_type_from_its_road) { // RS5
    auto d = owned();
    const auto road = putStreetSegment(d, {"", "", {{0, 0}, {100, 0}}, std::string("ns"), 0, {}, {}});
    const auto links = applyTemplate(d, road, {});
    setLinkType(d, links[0], std::nullopt);
    const std::vector<Vdf> vdfs{{"bpr", "bpr", 0.15, 4, "Test values", ""}};
    CHECK(issue(compileMacroNetwork(d.network, {}, d.definition->linkTypes, {""}, vdfs).issues, "MACRO_LINK_TYPE_MISSING")); // forcing
    const auto offers = fillOffers(d, templates(d));
    CHECK(offered(offers, FillKind::linkType, links[0]) == 1 && offers.front().value == "arterial" && offers.front().source == "template ns: Test template");
    applyFills(d, {offers.front()}, templates(d));
    CHECK(d.network.links[0].linkTypeId == "arterial" && d.network.streetSegments[0].links == links); // attached still
    CHECK(!issue(compileMacroNetwork(d.network, {}, d.definition->linkTypes, {""}, vdfs).issues, "MACRO_LINK_TYPE_MISSING"));
    // A template without a type has nothing to give.
    auto untyped = owned(); putCrossSection(untyped, section("bare", std::nullopt), {});
    const auto r = putStreetSegment(untyped, {"", "", {{0, 0}, {100, 0}}, std::string("bare"), 0, {}, {}});
    applyTemplate(untyped, r, {});
    CHECK(fillOffers(untyped, templates(untyped)).empty());
}
TEST(filldefaults, rs6_hcm_jam_density) { // RS6
    auto d = owned();
    d.definition->linkTypes = {arterial(std::nullopt)};
    const auto road = putStreetSegment(d, {"", "", {{0, 0}, {100, 0}}, std::string("ns"), 0, {}, {}});
    applyTemplate(d, road, {});
    CHECK(issue(compileMesoNetwork(d.network, d.definition->linkTypes, {"definition.linkTypes[0]"}, {1, 60}).issues, "MESO_JAM_DENSITY_MISSING")); // forcing
    const auto offers = fillOffers(d, templates(d));
    CHECK(offered(offers, FillKind::jamDensity, "arterial") == 1);
    // The value and source are the shipped catalog's, word for word.
    std::ifstream file(data() / "link-types" / "freeway-55.json"); Json shipped; file >> shipped;
    CHECK(shipped["jamDensity"]["value"].get<double>() == hcmJamDensity && shipped["jamDensity"]["source"].get<std::string>() == hcmJamDensitySource);
    applyFills(d, {offers.front()}, templates(d));
    CHECK(d.definition->linkTypes[0].jamDensity == (JamDensity{hcmJamDensity, hcmJamDensitySource}));
    CHECK(!issue(compileMesoNetwork(d.network, d.definition->linkTypes, {"definition.linkTypes[0]"}, {1, 60}).issues, "MESO_JAM_DENSITY_MISSING"));
    CHECK(linkTypeIssues({{d.definition->linkTypes[0], "definition.linkTypes[0]"}}).empty());
    // Not offered: an installed catalog, or a type the HCM value would make invalid.
    auto installed = d; installed.definition->externalLinkTypes = true; installed.definition->linkTypes[0].jamDensity.reset();
    CHECK(offered(fillOffers(installed, templates(installed)), FillKind::jamDensity, "arterial") == 0);
    auto fast = d; fast.definition->linkTypes = {arterial(std::nullopt)}; fast.definition->linkTypes[0].capacityPerLane = 5000;
    CHECK(offered(fillOffers(fast, templates(fast)), FillKind::jamDensity, "arterial") == 0);
}
TEST(filldefaults, rs7_road_links) { // RS7
    auto d = owned();
    const auto road = putStreetSegment(d, {"", "", {{0, 0}, {100, 0}}, std::string("ns"), 0, {}, {}});
    const auto bare = putStreetSegment(d, {"", "", {{0, 50}, {100, 50}}, std::nullopt, 0, {}, {}});
    const auto seams = [&] { return seamChecks(d.network, {}, templates(d), SeamLevel::micro); };
    CHECK(seams().size() == 2); // forcing: both StreetSegments lack Links
    const auto offers = fillOffers(d, templates(d));
    CHECK(offered(offers, FillKind::streetSegmentLinks, road) == 1 && offered(offers, FillKind::streetSegmentLinks, bare) == 0);
    applyFills(d, offers, templates(d));
    CHECK(d.network.streetSegments[0].links.size() == 2 && seams().size() == 1);
}
TEST(filldefaults, rs8_only_what_is_chosen) { // RS8
    auto d = owned();
    d.definition->linkTypes = {arterial(std::nullopt)};
    const auto a = putStreetSegment(d, {"", "", {{0, 0}, {100, 0}}, std::string("ns"), 0, {}, {}});
    const auto b = putStreetSegment(d, {"", "", {{0, 50}, {100, 50}}, std::string("ns"), 0, {}, {}});
    const auto bytes = documentJson(d).dump();
    const auto offers = fillOffers(d, templates(d));
    CHECK(offers.size() == 3 && documentJson(d).dump() == bytes); // forcing; listing changes nothing
    History h; h.reset(d);
    const auto steps = h.states().size();
    CHECK(h.execute("editorFillDefaults", [&](auto& x) { applyFills(x, {offers[0], offers[1]}, templates(x)); }));
    const auto& n = h.document().network;
    CHECK(h.states().size() == steps + 1 && h.document().definition->linkTypes[0].jamDensity);
    CHECK(n.streetSegments[0].links.size() == 2 && n.streetSegments[1].links.empty()); // the third offer was not chosen
    h.undo();
    CHECK(documentJson(h.document()).dump() == bytes);
    // An offer that no longer applies is refused.
    auto done = d; applyFills(done, {offers[1]}, templates(done));
    try { applyFills(done, {offers[1]}, templates(done)); CHECK(false); } catch (const std::invalid_argument& e) { CHECK(std::string(e.what()) == "EDIT_FILL_STALE"); }
    (void)a; (void)b;
}
TEST(filldefaults, rs8b_stale_offers_and_one_list) { // RS8 (D384): what applyFills checks, offer by offer
    auto d = owned();
    d.definition->linkTypes = {arterial(std::nullopt)};
    for (int i = 0; i < 3; ++i) putStreetSegment(d, {"", "", {{0, 50.0 * i}, {100, 50.0 * i}}, std::string("ns"), 0, {}, {}});
    const auto offers = fillOffers(d, templates(d));
    CHECK(offers.size() == 4); // forcing: one jam density, three StreetSegments
    const auto stale = [&](std::vector<FillOffer> chosen) {
        auto x = d;
        try { applyFills(x, chosen, templates(x)); } catch (const std::invalid_argument& e) { return std::string(e.what()) == "EDIT_FILL_STALE"; }
        return false;
    };
    auto changed = offers[1]; changed.value = "other";
    CHECK(stale({changed}));                    // a field differs from the current offer
    CHECK(stale({offers[1], offers[1]}));       // applied once already in the same list
    auto unknown = offers[1]; unknown.objectId = "road-nope";
    CHECK(stale({unknown}));                    // names no object
    auto kind = offers[0]; kind.kind = FillKind::streetSegmentLinks;
    CHECK(stale({kind}));                       // the right object under another kind
    // One list gives the same document as one offer at a time.
    auto all = d; applyFills(all, offers, templates(all));
    auto each = d; for (const auto& o : offers) applyFills(each, {o}, templates(each));
    CHECK(documentJson(all) == documentJson(each));
}
