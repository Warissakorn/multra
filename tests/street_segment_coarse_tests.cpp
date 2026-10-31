// S3.6a rows RD4, RD16, RD19 and RD20's Macro and Meso half (docs/reference/STREET_SEGMENTS.md §6, §9, §13,
// STREET_SEGMENTS_IMPLEMENTED §19, D310): the link-type rank, the coarse codec and its checks, and Macro and
// Meso compiling coarse Nodes and Link-less StreetSegments. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/model/network/coarse_nodes.hpp"
#include "../src/model/network/street_segment_crossings.hpp"
#include "../src/model/demand/coarse_compile.hpp"
#include "../src/model/demand/macro_compile.hpp"
#include "../src/model/demand/meso_compile.hpp"
#include "../src/project/link_type_codec.hpp"
#include "../src/project/street_segment_codec.hpp"
#include <algorithm>
#include <cmath>
using namespace multra;
namespace {
using Turn = std::pair<std::string, std::string>;
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
std::vector<ValidationIssue> refused(const ProjectDocument& d) {
    try { validateDocument(d); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
// Test values only: no template or ranked type ships (D304, 2a; the owner ranks types).
LinkType type(const char* id, std::optional<int> rank) {
    LinkType t{id, id, 1000, 10, "bpr", {}, "Test values", JamDensity{150, "Test value"}};
    t.rank = rank; return t;
}
CrossSection section(const char* id, const char* linkType, int directions = 2) {
    return {id, id, directions, 1, directions == 2 ? 1 : 0, 3.5, {}, 0, std::string(linkType), "Test values"};
}
const std::vector<Vdf> vdfs{{"bpr", "bpr", 0.15, 4, "Test values", ""}};
std::string road(ProjectDocument& d, std::vector<Point> centreline, std::optional<std::string> section) {
    return putStreetSegment(d, {"", "", std::move(centreline), std::move(section), 0, {}, {}});
}
std::vector<Point> square(Point c, double h) { return {{c.x - h, c.y - h}, {c.x + h, c.y - h}, {c.x + h, c.y + h}, {c.x - h, c.y + h}}; }
// Four Link-less two-way StreetSegments ending in one Node at the origin; legs n, e, s, w.
struct Four { ProjectDocument d; std::string n, e, s, w; };
Four four(const char* ns = "ns", const char* ew = "ns") {
    Four f;
    f.n = road(f.d, {{0, 100}, {0, 0}}, ns); f.e = road(f.d, {{100, 0}, {0, 0}}, ew);
    f.s = road(f.d, {{0, -100}, {0, 0}}, ns); f.w = road(f.d, {{-100, 0}, {0, 0}}, ew);
    putNode(f.d, {"", "", square({0, 0}, 10), false});
    return f;
}
MesoCompile meso(const Network& n, const std::vector<LinkType>& types, const std::vector<CrossSection>& sections) {
    return compileMesoNetwork(n, types, std::vector<std::string>(types.size()), {1, 60}, {}, sections);
}
MacroCompile macro(const Network& n, const std::vector<LinkType>& types, const std::vector<CrossSection>& sections) {
    return compileMacroNetwork(n, {}, types, std::vector<std::string>(types.size()), vdfs, sections);
}
std::size_t connectorArcs(const MacroGraph& g) {
    return static_cast<std::size_t>(std::count_if(g.links.begin(), g.links.end(), [](const auto& l) { return l.kind == MacroArc::connector; }));
}
// The Meso turn from the piece of `in` entering the Node to the piece of `out` leaving it.
const MesoTurn* mesoTurn(const MesoGraph& g, const std::string& in, const std::string& out) {
    for (const auto& t : g.turns)
        if (g.links[t.in].source == in && g.links[t.out].source == out) return &t;
    return nullptr;
}
}
TEST(streetsegmentcoarse, rd4_rank) { // RD4
    using Ranks = std::vector<std::pair<std::string, std::optional<int>>>;
    const auto major = majorByRank(Ranks{{"a", 2}, {"b", 1}, {"c", 2}, {"d", 1}});
    CHECK(major && *major == (std::vector<std::string>{"a", "c"})); // forcing: two ranks
    CHECK(!majorByRank(Ranks{{"a", 2}, {"b", 2}, {"c", 2}}));      // a tie
    CHECK(!majorByRank(Ranks{{"a", 2}, {"b", std::nullopt}, {"c", 1}})); // an unranked leg
    // Compiled: a priority Node advises only while the ranks decide nothing.
    auto ranked = four("major", "minor");
    const std::vector<CrossSection> sections{section("major", "arterial"), section("minor", "local")};
    const std::vector<LinkType> types{type("arterial", 2), type("local", 1)};
    CHECK(meso(ranked.d.network, types, sections).advisories.empty());
    CHECK(macro(ranked.d.network, types, sections).advisories.empty());
    const std::vector<LinkType> tied{type("arterial", 1), type("local", 1)};
    CHECK(issueAt(meso(ranked.d.network, tied, sections).advisories, "NODE_PRIORITY_UNDETERMINED", "network.nodes[0]"));
    CHECK(issueAt(macro(ranked.d.network, tied, sections).advisories, "NODE_PRIORITY_UNDETERMINED", "network.nodes[0]"));
    // An authored `major` decides, and so does a signal.
    ranked.d.network.nodes[0].coarse = NodeCoarse{std::nullopt, {ranked.n, ranked.s}, {}, std::nullopt};
    CHECK(meso(ranked.d.network, tied, sections).advisories.empty());
    // The rank round-trips on an owned type at 53 and is unsupported below.
    ProjectDocument owned;
    owned.definition = AuthoringDefinition{}; owned.definition->externalLinkTypes = false;
    owned.definition->linkTypes = {type("arterial", 3)};
    auto j = documentJson(owned);
    CHECK(j["schemaVersion"] == 53 && j["definition"]["linkTypes"][0]["rank"] == 3);
    CHECK(parseDocument(Json::parse(j.dump())).definition->linkTypes[0].rank == 3);
    owned.definition->linkTypes[0].rank.reset();
    CHECK(documentJson(owned)["schemaVersion"] == 52 && !documentJson(owned)["definition"]["linkTypes"][0].contains("rank"));
    j["schemaVersion"] = 52;
    try { parseDocument(j); CHECK(false); } catch (const ValidationError& e) { CHECK(issueAt(e.issues, "EDIT_UNSUPPORTED_FIELD", "definition.linkTypes[0].rank")); }
}
TEST(streetsegmentcoarse, rd16_coarse_codec_and_checks) { // RD16
    auto f = four();
    f.d.network.nodes[0].coarse = NodeCoarse{CoarseControl::signalised, {f.n, f.s}, {{f.e, f.n}},
                                             CoarseSignal{90, {{40, {{f.n, f.s}, {f.s, f.n}}}, {30, {{f.e, f.w}}}}}};
    CHECK(refused(f.d).empty()); // forcing: a valid coarse Node
    auto j = documentJson(f.d);
    CHECK(j["schemaVersion"] == 59 && j["network"]["nodes"][0]["coarse"]["control"] == "signalised");
    CHECK(parseDocument(Json::parse(j.dump())).network.nodes == f.d.network.nodes);
    // Defaults are not written: an empty coarse is an empty object.
    auto plain = f.d; plain.network.nodes[0].coarse = NodeCoarse{};
    CHECK(documentJson(plain)["network"]["nodes"][0]["coarse"] == Json::object());
    auto old = j; old["schemaVersion"] = 52; old["network"].erase("streetSegments");
    try { parseDocument(old); CHECK(false); } catch (const ValidationError& e) { CHECK(issueAt(e.issues, "EDIT_UNSUPPORTED_FIELD", "network.nodes[0].coarse")); }
    auto bad = j; bad["network"]["nodes"][0]["coarse"]["control"] = "roundabout";
    try { parseDocument(bad); CHECK(false); } catch (const ValidationError& e) { CHECK(issueAt(e.issues, "EDIT_NODE_INVALID", "network.nodes[0].coarse.control")); }
    // Detail inside refuses it (D311): a Link wholly inside; a Link only reaching in is a leg.
    auto reaching = f.d; addLink(reaching, {{-5, -50}, {-5, 0}}, 1, 3.5);
    CHECK(!issueAt(refused(reaching), "NODE_COARSE_WITH_DETAIL", "nodes[0].coarse"));
    auto detailed = f.d; addLink(detailed, {{-5, -5}, {5, 5}}, 1, 3.5);
    CHECK(issueAt(refused(detailed), "NODE_COARSE_WITH_DETAIL", "nodes[0].coarse"));
    // Unknown legs, wherever they are named.
    auto unknown = f.d; auto& c = *unknown.network.nodes[0].coarse;
    c.major.push_back("nope"); c.bannedTurns.push_back({f.n, "nope"}); c.signal->stages[1].turns.push_back({"nope", f.w});
    const auto issues = refused(unknown);
    CHECK(issueAt(issues, "NODE_COARSE_UNKNOWN_LEG", "nodes[0].coarse.major[2]"));
    CHECK(issueAt(issues, "NODE_COARSE_UNKNOWN_LEG", "nodes[0].coarse.bannedTurns[1]"));
    CHECK(issueAt(issues, "NODE_COARSE_UNKNOWN_LEG", "nodes[0].coarse.signal.stages[1].turns[1]"));
    // Bad signals.
    const auto signalRefused = [&](auto edit) { auto x = f.d; edit(*x.network.nodes[0].coarse); return issueAt(refused(x), "NODE_COARSE_SIGNAL_INVALID", "nodes[0].coarse.signal"); };
    CHECK(signalRefused([](NodeCoarse& x) { x.signal->stages.clear(); }));
    CHECK(signalRefused([](NodeCoarse& x) { x.signal->stages[0].green = 61; }));
    CHECK(signalRefused([](NodeCoarse& x) { x.signal.reset(); }));
    CHECK(signalRefused([](NodeCoarse& x) { x.control = CoarseControl::priority; }));
    CHECK(!signalRefused([](NodeCoarse& x) { x.signal->stages[0].green = 60; }));
}
TEST(streetsegmentcoarse, rd19_coarse_compiles_in_meso_and_macro) { // RD19
    auto f = four();
    const std::vector<CrossSection> sections{section("ns", "arterial")};
    const std::vector<LinkType> types{type("arterial", std::nullopt)};
    f.d.network.nodes[0].coarse = NodeCoarse{CoarseControl::signalised, {}, {},
        CoarseSignal{90, {{30, {{f.n, f.s}, {f.s, f.n}, {f.n, f.e}}}, {30, {{f.e, f.w}, {f.w, f.e}, {f.n, f.e}}}}}};
    const auto m = meso(f.d.network, types, sections);
    CHECK(m.issues.empty() && m.graph.links.size() == 8); // forcing: four two-way legs
    CHECK(m.graph.turns.size() == 12);
    const auto node = std::find_if(m.graph.nodes.begin(), m.graph.nodes.end(), [&](const auto& x) { return !x.nodeId.empty(); });
    CHECK(node != m.graph.nodes.end() && node->control == "signalised" && node->turns.size() == 12);
    const auto* ns = mesoTurn(m.graph, f.n, f.s); const auto* ne = mesoTurn(m.graph, f.n, f.e); const auto* ew = mesoTurn(m.graph, f.e, f.n);
    CHECK(ns && ne && ew && ns->signal == "node:" + f.d.network.nodes[0].id);
    test::near(ns->green, 30.0 / 90, 1e-12); test::near(ne->green, 60.0 / 90, 1e-12); test::near(ew->green, 0, 1e-12);
    CHECK(ns->freeTime == 0 && !mesoTurn(m.graph, f.n, f.n)); // no geometry, and no U-turn
    const auto a = macro(f.d.network, types, sections);
    CHECK(a.issues.empty() && connectorArcs(a.graph) == 12);
    // A banned turn is absent from both.
    f.d.network.nodes[0].coarse->bannedTurns = {{f.e, f.n}};
    const auto banned = meso(f.d.network, types, sections);
    CHECK(banned.graph.turns.size() == 11 && !mesoTurn(banned.graph, f.e, f.n) && mesoTurn(banned.graph, f.n, f.e));
    CHECK(connectorArcs(macro(f.d.network, types, sections).graph) == 11);
}
TEST(streetsegmentcoarse, rd20_link_less_roads_compile) { // RD20 (Macro and Meso; the Micro half is S3.7)
    ProjectDocument d;
    // A T at a Node, then a continuation: the stem's far end meets a one-way StreetSegment end to end.
    const auto a = road(d, {{-100, 0}, {0, 0}}, "two"), b = road(d, {{0, 0}, {100, 0}}, "two");
    const auto stem = road(d, {{0, 0}, {0, -100}}, "two"), tail = road(d, {{0, -101}, {0, -200}}, "one");
    putNode(d, {"", "", square({0, 0}, 10), false});
    const std::vector<CrossSection> sections{section("two", "arterial"), section("one", "arterial", 1)};
    const std::vector<LinkType> types{type("arterial", std::nullopt)};
    const auto m = meso(d.network, types, sections);
    CHECK(m.issues.empty() && m.graph.links.size() == 7); // forcing: three two-way StreetSegments and a one-way
    for (const auto& l : m.graph.links)
        if (l.source != tail) test::near(l.length, 90, 1e-9); // split at the Node's boundary
    CHECK(m.graph.turns.size() == 6 + 1); // the T's 3 x 2, and the stem continuing into the tail
    const auto* through = mesoTurn(m.graph, stem, tail);
    CHECK(through && m.graph.links[through->in].fromStation < m.graph.links[through->in].toStation);
    CHECK(!mesoTurn(m.graph, tail, stem)); // the tail is one-way
    const auto backward = std::count_if(m.graph.links.begin(), m.graph.links.end(), [](const auto& l) { return l.toStation < l.fromStation; });
    CHECK(backward == 3);
    const auto g = macro(d.network, types, sections);
    CHECK(g.issues.empty() && connectorArcs(g.graph) == 7);
    CHECK(std::count_if(g.graph.links.begin(), g.graph.links.end(), [](const auto& l) { return l.kind == MacroArc::link; }) == 7);
    // A Link-less StreetSegment needs a template, and its template a known type.
    auto bare = d; road(bare, {{500, 0}, {600, 0}}, std::nullopt);
    CHECK(issueAt(meso(bare.network, types, sections).issues, "MACRO_STREET_SEGMENT_TEMPLATE_MISSING", "network.streetSegments[4].crossSectionId"));
    CHECK(issueAt(macro(bare.network, types, sections).issues, "MACRO_STREET_SEGMENT_TEMPLATE_MISSING", "network.streetSegments[4].crossSectionId"));
    CHECK(issueAt(meso(d.network, {type("other", 1)}, sections).issues, "MACRO_LINK_TYPE_UNKNOWN", "network.streetSegments[0].crossSectionId"));
    // A StreetSegment with generated Links compiles from its Links, as before.
    auto linked = d;
    putCrossSection(linked, section("two", "arterial"), {});
    applyTemplate(linked, a, {});
    const auto fromLinks = meso(linked.network, types, sections).graph;
    CHECK(std::any_of(fromLinks.links.begin(), fromLinks.links.end(), [&](const auto& l) { return l.source == linked.network.links[0].id; }) && std::none_of(fromLinks.links.begin(), fromLinks.links.end(), [&](const auto& l) { return l.source == a; }));
    (void)b;
}
TEST(streetsegmentcoarse, rd19_coarse_over_generated_links) { // RD19 with D311: a junction over generated Links, its Connectors gone
    ProjectDocument d;
    putCrossSection(d, section("ns", "arterial"), {});
    const auto a = road(d, {{0, 0}, {200, 0}}, "ns"), b = road(d, {{100, -100}, {100, 100}}, "ns");
    applyTemplate(d, a, {}); applyTemplate(d, b, {});
    const auto found = findCrossings(d.network, 2);
    applyCrossings(d, found, {CrossingChoice::junction}, 2, {});
    CHECK(d.network.nodes.size() == 1 && d.network.links.size() == 8); // forcing: a split junction
    const std::vector<LinkType> types{type("arterial", std::nullopt)};
    // The split's through Connectors are detail, so Micro's membership compiles it (4 through turns).
    CHECK(nodeHasDetail(d.network, d.network.nodes[0]) && meso(d.network, types, {}).graph.turns.size() == 4);
    std::vector<std::string> connectors;
    for (const auto& c : d.network.connectors) connectors.push_back(c.id);
    deleteObjects(d, connectors);
    CHECK(!nodeHasDetail(d.network, d.network.nodes[0])); // only the Links' legs reach in
    const auto m = meso(d.network, types, {});
    CHECK(m.issues.empty() && m.graph.turns.size() == 12);
    for (const auto& l : m.graph.links) CHECK(l.length < 100); // the Links' parts inside the Node are not compiled
    CHECK(connectorArcs(macro(d.network, types, {}).graph) == 12);
    // Coarse values apply: a banned pair, by StreetSegment id.
    const auto pieceOf = [&](const std::string& link) { for (const auto& l : d.network.links) if (l.id == link) return *l.streetSegmentId; return std::string{}; };
    const auto& first = m.graph.turns.front();
    const Turn pair{pieceOf(m.graph.links[first.in].source), pieceOf(m.graph.links[first.out].source)};
    d.network.nodes[0].coarse = NodeCoarse{std::nullopt, {}, {pair}, std::nullopt};
    CHECK(refused(d).empty());
    CHECK(meso(d.network, types, {}).graph.turns.size() == 11 && connectorArcs(macro(d.network, types, {}).graph) == 11);
}
