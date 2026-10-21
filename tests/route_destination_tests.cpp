#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/project/route_destination.hpp"
#include "../src/project/run.hpp"
using namespace multra;
TEST(stationrouting, destination_codec_keeps_legacy_and_runtime_routes) {
    ProjectDocument d;const auto link=addLink(d,{{0,0},{100,0}},2,3.5);
    const auto route=putRoute(d,{{},{link}});putInput(d,{"input",route,"car",100,0,60});const auto legacy=documentJson(d);
    const auto compiled=compileDocument(d,test::root()/"data").scenario;
    CHECK(legacy["schemaVersion"]==17);
    d.definition->routeDestinations[route]={link,60};validateDocument(d);
    const auto saved=documentJson(d);CHECK(saved["schemaVersion"]==30);
    CHECK(parseDocument(saved)==d);
    const auto after=compileDocument(d,test::root()/"data").scenario;
    CHECK(compiled.routes==after.routes);CHECK(compiled.segments.size()==after.segments.size());
    for(std::size_t i=0;i<compiled.segments.size();++i) {
        CHECK(compiled.segments[i].id==after.segments[i].id);
        CHECK(compiled.segments[i].length==after.segments[i].length);
        CHECK(compiled.segments[i].next==after.segments[i].next);
    }
    auto old=saved;old["schemaVersion"]=29;test::throws([&]{parseDocument(old);},"UNSUPPORTED_FIELD");
    auto bad=saved;bad["definition"]["routeDestinations"][route]["station"]=-1;
    test::throws([&]{parseDocument(bad);},"EDIT_ROUTE_DESTINATION_INVALID");
    bad=saved;bad["definition"]["routeDestinations"][route]["objectId"]="missing";
    test::throws([&]{parseDocument(bad);},"EDIT_ROUTE_DESTINATION_INVALID");
    bad=saved;bad["definition"]["routeDestinations"][route]["extra"]=1;
    test::throws([&]{parseDocument(bad);},"EDIT_ROUTE_DESTINATION_INVALID");
    d.definition->routeDestinations.clear();CHECK(documentJson(d)==legacy);
}
TEST(stationrouting, destination_history_deletion_and_shortening) {
    ProjectDocument d;const auto link=addLink(d,{{0,0},{100,0}},1,3.5);
    const auto route=putRoute(d,{{},{link}});d.definition->routeDestinations[route]={link,60};
    History history;history.reset(d);
    test::throws([&]{history.execute("shorten",[&](auto& value){changeGeometry(value,link,{{0,0},{40,0}});});},"EDIT_ROUTE_DESTINATION_INVALID");
    CHECK(history.document()==d);
    CHECK(history.execute("delete",[&](auto& value){deleteRoute(value,route);}));
    CHECK(history.document().definition->routeDestinations.empty());history.undo();CHECK(history.document().definition->routeDestinations==d.definition->routeDestinations);
    CHECK(history.execute("delete road",[&](auto& value){deleteLink(value,link);}));
    CHECK(history.document().definition->routeDestinations.empty());history.undo();
    reverseLink(d,link);test::near(d.definition->routeDestinations.at(route).station,40);
    const auto other=addLink(d,{{0,50},{100,50}},1,3.5);
    putRoute(d,{route,{link,other}});CHECK(d.definition->routeDestinations.empty());
    putRoute(d,{route,{link}});d.definition->routeDestinations[route]={link,40};
    putRoute(d,{route,{other}});CHECK(d.definition->routeDestinations.empty());
}
TEST(stationrouting, destination_split_remaps_station_or_rejects_gap) {
    ProjectDocument d;const auto link=addLink(d,{{0,0},{100,0}},1,3.5);
    const auto route=putRoute(d,{{},{link}});d.definition->routeDestinations[route]={link,60};
    const auto down=splitLink(d,link,40);validateDocument(d);
    const auto& end=d.definition->routeDestinations.at(route);CHECK(end.objectId==down);test::near(end.station,19.9);
    CHECK(parseDocument(documentJson(d))==d);
    History history;history.reset(d);
    test::throws([&]{history.execute("split at marker",[&](auto& value){splitLink(value,down,19.9);});},"EDIT_ROUTE_DESTINATION_INVALID");
    CHECK(history.document()==d);
}
