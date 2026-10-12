#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/route_markers.hpp"
#include "../src/project/run.hpp"
#include <limits>

using namespace trafficsim;
namespace {
ProjectDocument markedFixture() {
    ProjectDocument d;const auto id=addLink(d,{{0,0},{100,0}},1,3.5);
    d.definition.emplace();putRoute(d,{"r",{id}});validateDocument(d);return d;
}
}
TEST(project, gui_route_marker_opt_in_strict_codec_and_legacy_bytes) {
    auto d=markedFixture();const auto original=documentJson(d).dump();
    CHECK(!documentJson(d).contains("routeEndMarkers"));
    putRouteEndMarker(d,"r",RouteEndMarker{d.network.links.front().id,60});
    auto json=documentJson(d);CHECK(json["schemaVersion"]==30);
    CHECK(documentJson(parseDocument(json))==json);
    auto bad=json;bad["schemaVersion"]=29;test::throws([&]{parseDocument(bad);},"UNSUPPORTED_FIELD");
    for(const auto& value:std::vector<Json>{nullptr,true,Json::array(),Json{{"objectId","missing"},{"station",1}},
        Json{{"objectId",d.network.links.front().id},{"station","60"}},Json{{"objectId",d.network.links.front().id},{"station",60},{"extra",true}}}) {
        bad=json;bad["routeEndMarkers"]["r"]=value;test::throws([&]{parseDocument(bad);},"EDIT_ROUTE_MARKER");
    }
    putRouteEndMarker(d,"r",std::nullopt);CHECK(documentJson(d).dump()==original);
}
TEST(project, gui_route_marker_history_validation_and_lifecycle) {
    auto d=markedFixture();const auto id=d.network.links.front().id;History h;h.reset(d);
    CHECK(h.execute("mark",[&](auto& candidate){putRouteEndMarker(candidate,"r",RouteEndMarker{id,60});}));
    h.markSaved();const auto revision=h.revision();
    CHECK(!h.execute("same",[&](auto& candidate){putRouteEndMarker(candidate,"r",RouteEndMarker{id,60});}));
    CHECK(h.revision()==revision && !h.dirty());
    for(double station:{-1.,101.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
        test::throws([&]{h.execute("invalid",[&](auto& candidate){putRouteEndMarker(candidate,"r",RouteEndMarker{id,station});});},"EDIT_ROUTE_MARKER");
        CHECK(h.revision()==revision && h.document().routeEndMarkers.at("r").station==60);
    }
    test::throws([&]{h.execute("short",[&](auto& candidate){changeGeometry(candidate,id,{{0,0},{50,0}});});},"EDIT_ROUTE_MARKER");
    CHECK(h.document().network.links.front().geometry.back().x==100);
    h.undo();CHECK(h.document().routeEndMarkers.empty());h.redo();CHECK(h.document().routeEndMarkers.at("r").station==60);
    h.execute("delete",[&](auto& candidate){deleteLink(candidate,id);});
    CHECK(h.document().definition->routes.empty() && h.document().routeEndMarkers.empty());
    h.undo();CHECK(h.document().routeEndMarkers.contains("r"));
    h.execute("delete route",[](auto& candidate){deleteRoute(candidate,"r");});CHECK(h.document().routeEndMarkers.empty());
}
TEST(project, gui_route_marker_split_reverse_and_reference_validation) {
    auto d=markedFixture();const auto id=d.network.links.front().id;putRouteEndMarker(d,"r",RouteEndMarker{id,60});
    auto reverse=d;reverseLink(reverse,id);test::near(reverse.routeEndMarkers.at("r").station,40);validateDocument(reverse);
    const auto downstream=splitLink(d,id,40,false);CHECK(d.routeEndMarkers.at("r").objectId==downstream);
    test::near(d.routeEndMarkers.at("r").station,19.9);validateDocument(d);
    auto gap=markedFixture();const auto link=gap.network.links.front().id;putRouteEndMarker(gap,"r",RouteEndMarker{link,40});
    const auto before=gap;test::throws([&]{splitLink(gap,link,40,false);},"EDIT_ROUTE_MARKER");CHECK(gap==before);
    auto stale=markedFixture();const auto road=stale.network.links.front().id;
    putRouteEndMarker(stale,"r",RouteEndMarker{road,0});validateDocument(stale);
    stale.definition->routingDecisions.push_back({});auto& decision=stale.definition->routingDecisions.back();
    decision.id="decision";decision.linkId=road;decision.position=50;decision.routes.push_back({"r",1});
    test::throws([&]{validateRouteEndMarkers(stale);},"EDIT_ROUTE_MARKER");
    Connector invalid;invalid.id="bad";invalid.from={"missing","missing"};invalid.to=invalid.from;
    invalid.geometry={{0,0},{10,0}};stale.network.connectors.push_back(invalid);
    test::throws([&]{connectorCentreline(stale.network,invalid);}); // Prove the forcing first.
    CHECK(routeMarkerGeometry(stale.network,"bad").empty());
}
TEST(project, gui_route_marker_never_changes_the_simulation) {
    auto d=markedFixture();const auto id=d.network.links.front().id;
    d.definition->duration=20;d.definition->inputs.push_back({"input","r","car",900,0,20});
    const auto plain=compileDocument(d,test::root()/"data").scenario;
    putRouteEndMarker(d,"r",RouteEndMarker{id,60});const auto marked=compileDocument(d,test::root()/"data").scenario;
    auto a=createSimulation(plain,42),b=createSimulation(marked,42);
    CHECK(checkpointJson(a)==checkpointJson(b));
    while(a.time<plain.duration) {
        a=stepSimulation(a);b=stepSimulation(b);
        CHECK(a.events.size()==b.events.size());
        for(std::size_t i=0;i<a.events.size();++i)CHECK(eventJson(a.events[i])==eventJson(b.events[i]));
        CHECK(checkpointJson(a)==checkpointJson(b));
        CHECK(a.randomState==b.randomState);
    }
}
