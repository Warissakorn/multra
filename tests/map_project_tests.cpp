#include "test.hpp"
#include "../src/commands/map_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/project/run.hpp"
#include <fstream>
#include <limits>
using namespace trafficsim;

TEST(mapproject, legacy_roundtrip_and_schema) {
    ProjectDocument legacy;const auto before=documentJson(legacy).dump();
    CHECK(parseDocument(documentJson(legacy))==legacy);
    setMapSettings(legacy,MapSettings{});
    const auto encoded=documentJson(legacy);CHECK(encoded.at("schemaVersion")==29);
    CHECK(parseDocument(encoded)==legacy);
    setMapSettings(legacy,{});CHECK(documentJson(legacy).dump()==before);
    auto wrong=encoded;wrong["schemaVersion"]=28;
    test::throws([&]{parseDocument(wrong);},"UNSUPPORTED_FIELD: map");
    wrong=encoded;wrong["schemaVersion"]=35;test::throws([&]{parseDocument(wrong);},"EDIT_VERSION");
}
TEST(mapproject, invalid_settings_are_atomic) {
    History history;const auto before=history.document();MapSettings settings;
    const auto rejects=[&]{
        test::throws([&]{history.execute("map",[&](auto& d){setMapSettings(d,settings);});},"EDIT_MAP_INVALID");
        CHECK(history.document()==before);CHECK(!history.canUndo());CHECK(!history.dirty());
    };
    settings.reference.anchorLatitude=91;rejects();
    settings=MapSettings{};settings.reference.projectedCrs="EPSG:3857";rejects();
    settings=MapSettings{};settings.opacity=std::numeric_limits<double>::quiet_NaN();rejects();
    settings=MapSettings{};settings.reference.anchorWorldX=std::numeric_limits<double>::infinity();rejects();
}
TEST(mapproject, malformed_and_unknown_fields) {
    ProjectDocument d;d.map=MapSettings{};const auto good=documentJson(d);
    for (const auto* field:{"opacity","providerId","reference"}) {
        auto bad=good;bad["map"][field]=nullptr;test::throws([&]{parseDocument(bad);},"EDIT_MAP_INVALID");
    }
    auto bad=good;bad["map"]["reference"]["extra"]=1;test::throws([&]{parseDocument(bad);},"EDIT_MAP_INVALID");
    bad=good;bad["map"]["reference"]["anchorWorldX"]="123";test::throws([&]{parseDocument(bad);},"EDIT_MAP_INVALID");
    bad=good;bad["map"]["reference"]["projectedCrs"]="EPSG:32648";test::throws([&]{parseDocument(bad);},"EDIT_MAP_INVALID");
    bad=good;bad["map"]["providerId"]="../tile";test::throws([&]{parseDocument(bad);},"EDIT_MAP_INVALID");
}
TEST(mapproject, history_and_utm) {
    History history;history.execute("map",[](auto& d){setMapSettings(d,MapSettings{});});
    CHECK(history.document().map);history.undo();CHECK(!history.document().map);
    history.redo();CHECK(history.document().map==MapSettings{});
    const auto network=history.document().network;
    history.execute("map",[](auto& d){auto s=*d.map;s.reference.anchorWorldX=34;s.reference.rotationDegrees=90;setMapSettings(d,s);});
    CHECK(history.document().network==network);
    CHECK(utmCrs(100.5,13.7)=="EPSG:32647");CHECK(utmCrs(180,0)=="EPSG:32660");
    CHECK(utmCrs(18,-34)=="EPSG:32734");CHECK(utmCrs(6,60)=="EPSG:32632");
}
TEST(mapproject, simulation_ignores_map) {
    std::ifstream source(test::root()/"data/scenarios/crossing.json");CHECK(source.good());
    auto document=parseDocument(Json::parse(source));
    const auto base=compileDocument(document,test::root()/"data").scenario;
    document.map=MapSettings{};const auto mapped=compileDocument(document,test::root()/"data").scenario;
    auto a=createSimulation(base,42),b=createSimulation(mapped,42);
    while (a.tick<totalTicks(base)) {
        a=stepSimulation(std::move(a));b=stepSimulation(std::move(b));
        CHECK(checkpointJson(a)==checkpointJson(b));CHECK(a.events==b.events);
        CHECK(a.vehicles==b.vehicles);CHECK(a.inputs==b.inputs);
        CHECK(a.randomState==b.randomState);CHECK(a.stopService==b.stopService);
    }
}
