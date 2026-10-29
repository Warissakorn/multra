#include "test.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/junction_commands.hpp"
#include "../src/commands/coarse_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/project/junction_template_codec.hpp"
#include "../src/model/network/approach_layout.hpp"
#include "../src/model/network/coarse_nodes.hpp"
using namespace multra;
namespace {
std::string nameOf(const Network& n, const Node& node, const ApproachDirections& overrides = {}) {
    const auto layout = approachLayout(n,node,overrides);
    CHECK(layout.issues.empty() && layout.movements.size() == 1);
    return layout.movements[0].name;
}
ProjectDocument through() {
    ProjectDocument d;
    d.network.links = {{"in",{{-200,0},{-10,0}},{{"i",3.5}}},{"out",{{10,0},{200,0}},{{"o",3.5}}}};
    d.network.nodes.push_back({"n","Site",{{-30,-30},{30,-30},{30,30},{-30,30}},true});
    addConnectorRange(d,{"in","i"},{"out","o"},1,1);
    return d;
}
}
TEST(junctiontemplates, jt5_choice_carries_major_clears_coarse_and_is_one_undo) {
    ProjectDocument d; d.network.nodes.push_back({"n","Site",{{-20,-20},{20,-20},{20,20},{-20,20}},true,NodeCoarse{}});
    // No detail: coarse and its explicit hierarchy can legally exist before choosing a recipe.
    d.network.streetSegments = {{"r","Arm",{{-200,0},{0,0}}}};
    d.network.nodes[0].coarse->major = {"r"};
    History h; h.reset(d); const auto before = documentJson(d);
    CHECK(d.network.nodes[0].coarse->major == std::vector<std::string>{"r"}); // forcing
    CHECK(h.execute("junction",[&](auto& doc) { setNodeJunction(doc,"n",NodeJunction{"t"}); }));
    const auto& node = h.document().network.nodes[0];
    CHECK(!node.coarse && node.junction && !node.generated);
    CHECK(node.junction->major == std::vector<std::string>{"r"});
    CHECK(h.states().size() == 2);
    const auto after = documentJson(h.document()); CHECK(after.at("schemaVersion") == 61);
    CHECK(documentJson(parseDocument(after)) == after);
    h.undo(); CHECK(documentJson(h.document()) == before);
    h.redo(); CHECK(documentJson(h.document()) == after);
    h.execute("detach",[&](auto& doc) { setNodeJunction(doc,"n",std::nullopt); });
    CHECK(!h.document().network.nodes[0].junction);
    h.undo(); CHECK(documentJson(h.document()) == after);
}
TEST(junctiontemplates, jt5_catalog_capture_is_atomic_and_owned_empty_never_falls_back) {
    const auto installed = resolveJunctionTemplates(AuthoringDefinition{},test::root()/"data");
    CHECK(installed.size() == 12); // forcing
    History h; h.reset(ProjectDocument{}); const auto before = documentJson(h.document());
    h.execute("catalog",[&](auto& d) { putJunctionTemplateCatalog(d,{installed[0].type}); });
    CHECK(h.document().definition && !h.document().definition->externalJunctionTemplates);
    const auto after = documentJson(h.document());
    CHECK(documentJson(parseDocument(after)) == after);
    CHECK(resolveJunctionTemplates(*h.document().definition,test::root()/"data").size() == 1);
    h.undo(); CHECK(documentJson(h.document()) == before); h.redo();
    h.execute("empty",[&](auto& d) { putJunctionTemplateCatalog(d,{}); });
    CHECK(resolveJunctionTemplates(*h.document().definition,test::root()/"data").empty());
    auto bad = installed[0].type; bad.roles.at("major")[0].movements = "FK";
    CHECK(bad.roles.at("major")[0].movements == "FK");
    const auto empty = documentJson(h.document());
    test::throws([&] { h.execute("invalid",[&](auto& d) { putJunctionTemplateCatalog(d,{bad}); }); },"JUNCTION_TEMPLATE_INVALID");
    CHECK(documentJson(h.document()) == empty);
}
TEST(junctiontemplates, jt5_saved_direction_works_without_template_and_follows_split) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) {
        auto d = through(); d.network.drivingSide = side;
        CHECK(nameOf(d.network,d.network.nodes[0]) == "EBT"); // forcing
        History h; h.reset(d); const auto before = documentJson(d);
        h.execute("direction",[&](auto& doc) { setNodeApproachDirections(doc,"n",{{"in",ApproachDirection::north}}); });
        const auto chosen = documentJson(h.document());
        CHECK(chosen.at("schemaVersion") == 61 && !h.document().network.nodes[0].junction);
        auto reopened = parseDocument(chosen);
        CHECK(nameOf(reopened.network,reopened.network.nodes[0]) == "NBT");
        CHECK(nameOf(reopened.network,reopened.network.nodes[0],{{"in",ApproachDirection::south}}) == "SBT");
        std::string down;
        h.execute("split",[&](auto& doc) { down = splitLink(doc,"in",80); });
        const auto& n = h.document().network;
        CHECK(n.nodes[0].approachDirections.contains(down) && !n.nodes[0].approachDirections.contains("in"));
        CHECK(nameOf(n,n.nodes[0]) == "NBT");
        h.undo(); CHECK(documentJson(h.document()) == chosen);
        h.undo(); CHECK(documentJson(h.document()) == before);
        h.execute("direction again",[&](auto& doc) { setNodeApproachDirections(doc,"n",{{"in",ApproachDirection::north}}); });
        const auto retained = documentJson(h.document());
        CHECK(h.document().network.nodes[0].approachDirections.contains("in")); // forcing before deletion
        h.execute("delete approach",[&](auto& doc) { deleteLink(doc,"in"); });
        CHECK(h.document().network.nodes[0].approachDirections.empty());
        h.undo(); CHECK(documentJson(h.document()) == retained);
    }
}
TEST(junctiontemplates, jt5_recipe_survives_detail_removal_and_two_leg_lifecycle) {
    auto d = through();
    d.network.streetSegments = {{"a","In",{{-200,0},{0,0}},std::nullopt,0,{}, {"in"}},
                                {"b","Out",{{200,0},{0,0}},std::nullopt,0,{}, {"out"}}};
    d.network.links[0].streetSegmentId = "a"; d.network.links[1].streetSegmentId = "b";
    d.definition.emplace();
    d.definition->signalPrograms.push_back({"program",0,{{20,SignalColor::green},{10,SignalColor::red}}});
    d.network.signalHeads.push_back({"head",{},0,"program",d.network.connectors[0].id});
    d.network.nodes[0].junction = NodeJunction{"t"};
    CHECK(nodeHasDetail(d.network,d.network.nodes[0]) && deriveCoarse(d,d.network.nodes[0])); // forcing
    History h; h.reset(d); const auto before = documentJson(d);
    const auto connector = d.network.connectors[0].id;
    h.execute("remove detail",[&](auto& doc) { deleteConnector(doc,connector); });
    CHECK(h.document().network.nodes[0].junction && !h.document().network.nodes[0].coarse);
    h.undo(); CHECK(documentJson(h.document()) == before);

}
TEST(junctiontemplates, jt5_recipe_is_not_merged_away_by_the_two_leg_lifecycle) {
    ProjectDocument t; t.definition.emplace();
    t.definition->externalCrossSections = false;
    t.definition->crossSections.push_back({"section","Test",2,1,1,3.5,{},0,std::nullopt,"Test values"});
    t.network.streetSegments = {{"a","A",{{0,0},{100,0}},"section"},
                                {"b","B",{{100,0},{200,0}},"section"},
                                {"stem","Stem",{{100,0},{100,100}},"section"}};
    t.network.nodes.push_back({"n","Site",{{90,-10},{110,-10},{110,10},{90,10}}});
    t.network.nodes[0].junction = NodeJunction{"t"};
    CHECK(nodeStreetSegmentLegs(t.network,t.network.nodes[0]).size() == 3); // forcing
    History h; h.reset(t);
    h.execute("remove stem",[&](auto& doc) { deleteStreetSegment(doc,"stem"); });
    CHECK(h.document().network.nodes.size() == 1 && h.document().network.nodes[0].junction);
}
TEST(junctiontemplates, jt5_recipe_leg_ids_follow_only_the_split_street_segments_moved_end) {
    ProjectDocument d; d.network.streetSegments = {{"r","Arm",{{0,0},{300,0}}}};
    d.network.nodes = {{"near","Near",{{-10,-10},{10,-10},{10,10},{-10,10}}},
                       {"far","Far",{{290,-10},{310,-10},{310,10},{290,10}}}};
    for (auto& node : d.network.nodes) {
        node.junction = NodeJunction{"t",{"r"},{{"r",{{60},{20},{}}}}};
    }
    CHECK(nodeStreetSegmentLegs(d.network,d.network.nodes[1])[0].streetSegmentId == "r"); // forcing
    History h; h.reset(d); const auto before = documentJson(d); std::string next;
    h.execute("split arm",[&](auto& doc) { next = splitStreetSegment(doc,"r",150); });
    const auto& near = *h.document().network.nodes[0].junction;
    const auto& far = *h.document().network.nodes[1].junction;
    CHECK(near.major == std::vector<std::string>{"r"} && near.legs.contains("r"));
    CHECK(far.major == std::vector<std::string>{next} && far.legs.contains(next) && !far.legs.contains("r"));
    CHECK(far.legs.at(next).storage[0] == 60);
    h.undo(); CHECK(documentJson(h.document()) == before);
}
