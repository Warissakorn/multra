// M8.2 rows Z1-Z6 (docs/reference/ZONES_AND_PARKING.md §7): the schema-31 zone layer's codec,
// its two tiers of checks and its undoable commands.
#include "test.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/history.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
using namespace multra;
namespace {
Json readJson(const std::filesystem::path& path) {
    std::ifstream source(path);CHECK(source.good());return Json::parse(source);
}
// Two one-lane roads, an entry and a terminal, each with one zone at its free end, and one matrix.
struct Fixture { ProjectDocument d; std::string in, out; };
Fixture fixture() {
    Fixture f;f.in=addLink(f.d,{{0,0},{100,0}},1,3.5);f.out=addLink(f.d,{{0,20},{100,20}},1,3.5);
    putZone(f.d,{"site","Site",{{-10,-10},{-10,10},{-30,0}},{{"jobs",120}}});
    putZone(f.d,{"town","",{{110,10},{130,30},{130,10}},{}});
    putZoneConnector(f.d,{"site-out","site",f.in,ZoneDirection::origin,2});
    putZoneConnector(f.d,{"town-in","town",f.in,ZoneDirection::destination,1});
    putOdMatrix(f.d,{"am",0,3600,"car-only",{"site","town"},{{0,40},{10,0}}});
    return f;
}
bool hasIssue(const ValidationError& e,const std::string& code,const std::string& path) {
    for (const auto& i:e.issues) if (i.code==code && i.path==path) return true;
    return false;
}
}
TEST(zonescodec, z1_shipped_files_keep_their_bytes) {
    const auto root=test::root()/"data";
    std::vector<std::filesystem::path> files{root/"scenarios/crossing.json"};
    for (const auto& entry:std::filesystem::directory_iterator(root/"projects"))
        if (entry.path().string().ends_with(".traffic.json")) files.push_back(entry.path());
    CHECK(files.size()>=5);
    for (const auto& file:files) {
        const auto json=readJson(file);
        CHECK(!json.contains("zones") && !json.contains("zoneConnectors") && !json.contains("odMatrices"));
        const auto d=parseDocument(json);CHECK(d.zones.empty());
        if (!json.contains("schemaVersion")) continue; // an M0 scenario is saved as a project, as before
        CHECK(documentJson(d)==json);
    }
    const ProjectDocument empty;const auto saved=documentJson(empty);
    CHECK(!saved.contains("zones"));CHECK(saved.at("schemaVersion")==17);
}
TEST(zonescodec, z2_round_trip_at_31) {
    auto f=fixture();const auto saved=documentJson(f.d);
    CHECK(saved.at("schemaVersion")==31);
    CHECK(saved.at("zones").size()==2);CHECK(saved.at("zoneConnectors").size()==2);CHECK(saved.at("odMatrices").size()==1);
    CHECK(saved.at("zoneConnectors")[0].at("direction")=="origin");
    CHECK(parseDocument(saved)==f.d);
    CHECK(documentJson(parseDocument(saved))==saved);
    auto plain=f.d;plain.zones={};
    auto expected=saved;expected.erase("zones");expected.erase("zoneConnectors");expected.erase("odMatrices");
    expected["schemaVersion"]=17;CHECK(documentJson(plain)==expected);
}
TEST(zonescodec, z3_refused_below_31_and_malformed) {
    const auto saved=documentJson(fixture().d);
    CHECK(parseDocument(saved).zones==fixture().d.zones); // forcing: the same keys load at 31
    for (const auto* key:{"zones","zoneConnectors","odMatrices"}) {
        auto old=saved;old["schemaVersion"]=30;
        for (const auto* other:{"zones","zoneConnectors","odMatrices"}) if (std::string(other)!=key) old.erase(other);
        test::throws([&]{parseDocument(old);},std::string("UNSUPPORTED_FIELD: ")+key);
    }
    auto future=saved;future["schemaVersion"]=52;test::throws([&]{parseDocument(future);},"EDIT_VERSION");
    const std::vector<std::pair<const char*,const char*>> fields{{"zones","name"},{"zones","polygon"},{"zones","attributes"},
        {"zoneConnectors","share"},{"zoneConnectors","direction"},{"odMatrices","cells"},{"odMatrices","startTime"}};
    for (const auto& [array,field]:fields) {
        auto bad=saved;bad[array][0][field]=nullptr;test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID");
        bad=saved;bad[array][0].erase(field);test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID");
    }
    for (const auto* array:{"zones","zoneConnectors","odMatrices"}) {
        auto bad=saved;bad[array][0]["extra"]=1;test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID");
        bad=saved;bad[array]=Json::object();test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID");
        bad=saved;bad[array]=Json::array();test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID"); // never written empty
    }
    auto bad=saved;bad["zoneConnectors"][0]["direction"]="both";test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID");
    bad=saved;bad["zones"][0]["attributes"]["jobs"]="120";test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID");
    bad=saved;bad["odMatrices"][0]["cells"][0][1]="40";test::throws([&]{parseDocument(bad);},"EDIT_ZONE_INVALID");
}
TEST(zonescodec, z4_structural_issues_are_atomic) {
    const auto base=fixture();
    const auto rejects=[&](const char* code,const char* path,const std::function<void(ProjectDocument&)>& change) {
        History history;history.reset(base.d);
        try { history.execute("zone",change); }
        catch (const ValidationError& e) {
            if (!hasIssue(e,code,path)) throw std::runtime_error(std::string("missing ")+code+" at "+path+": "+e.what());
            CHECK(history.document()==base.d);CHECK(!history.canUndo());CHECK(!history.dirty());return;
        }
        throw std::runtime_error(std::string("accepted ")+code);
    };
    const auto nan=std::numeric_limits<double>::quiet_NaN();
    rejects("ZONE_ID_INVALID","zones[1].id",[](auto& d){d.zones.zones[1].id="";});
    rejects("ZONE_ID_INVALID","zones[1].id",[](auto& d){d.zones.zones[1].id="site";});
    rejects("ZONE_ID_INVALID","odMatrices[0].id",[&](auto& d){d.zones.odMatrices[0].id=base.in;});
    rejects("ZONE_POLYGON_INVALID","zones[0].polygon",[](auto& d){d.zones.zones[0].polygon.pop_back();});
    rejects("ZONE_POLYGON_INVALID","zones[0].polygon",[&](auto& d){d.zones.zones[0].polygon[0].x=nan;});
    rejects("ZONE_POLYGON_INVALID","zones[0].polygon",[](auto& d){d.zones.zones[0].polygon={{0,0},{10,10},{10,0},{0,10}};});
    rejects("ZONE_ATTRIBUTE_INVALID","zones[0].attributes.jobs",[&](auto& d){d.zones.zones[0].attributes["jobs"]=nan;});
    rejects("ZONE_CONNECTOR_UNKNOWN_REF","zoneConnectors[0].zoneId",[](auto& d){d.zones.zoneConnectors[0].zoneId="nowhere";});
    rejects("ZONE_CONNECTOR_UNKNOWN_REF","zoneConnectors[0].linkId",[](auto& d){d.zones.zoneConnectors[0].linkId="nowhere";});
    rejects("ZONE_CONNECTOR_SHARE_INVALID","zoneConnectors[0].share",[](auto& d){d.zones.zoneConnectors[0].share=0;});
    rejects("ZONE_CONNECTOR_SHARE_INVALID","zoneConnectors[0].share",[](auto& d){d.zones.zoneConnectors[0].share=std::numeric_limits<double>::infinity();});
    rejects("OD_ZONE_UNKNOWN","odMatrices[0].zoneIds[1]",[](auto& d){d.zones.odMatrices[0].zoneIds[1]="nowhere";});
    rejects("OD_ZONE_REPEATED","odMatrices[0].zoneIds[1]",[](auto& d){d.zones.odMatrices[0].zoneIds[1]="site";});
    rejects("OD_CELLS_SHAPE","odMatrices[0].cells",[](auto& d){d.zones.odMatrices[0].cells[1].pop_back();});
    rejects("OD_CELLS_SHAPE","odMatrices[0].cells",[](auto& d){d.zones.odMatrices[0].cells.pop_back();});
    rejects("OD_CELL_INVALID","odMatrices[0].cells[0][1]",[](auto& d){d.zones.odMatrices[0].cells[0][1]=-1;});
    rejects("OD_CELL_INVALID","odMatrices[0].cells[1][0]",[&](auto& d){d.zones.odMatrices[0].cells[1][0]=nan;});
    rejects("OD_PERIOD_INVALID","odMatrices[0]",[](auto& d){d.zones.odMatrices[0].startTime=-1;});
    rejects("OD_PERIOD_INVALID","odMatrices[0]",[](auto& d){d.zones.odMatrices[0].endTime=0;});
    rejects("OD_COMPOSITION_MISSING","odMatrices[0].compositionId",[](auto& d){d.zones.odMatrices[0].compositionId="";});
    // The same checks guard a hand-edited file.
    auto file=documentJson(base.d);file["zoneConnectors"][0]["linkId"]="nowhere";
    test::throws([&]{parseDocument(file);},"ZONE_CONNECTOR_UNKNOWN_REF");
}
TEST(zonescommands, z5_history_and_cascades) {
    auto f=fixture();History history;history.reset(f.d);
    CHECK(history.execute("add zone",[](auto& d){const auto id=putZone(d,{"",{},{{0,50},{10,50},{10,60}},{}});CHECK(id=="zone-"+std::to_string(d.nextId-1));}));
    CHECK(history.document().zones.zones.size()==3);history.undo();CHECK(history.document()==f.d);
    history.redo();CHECK(history.document().zones.zones.size()==3);history.undo();
    // A Zone a matrix names is refused, and nothing changes.
    test::throws([&]{history.execute("delete zone",[](auto& d){deleteZone(d,"town");});},"EDIT_ZONE_IN_USE");
    CHECK(history.document()==f.d);CHECK(!history.canUndo());
    // Without the matrix the zone goes, and its connectors with it, in one step.
    CHECK(history.execute("delete matrix",[](auto& d){deleteOdMatrix(d,"am");}));
    CHECK(history.execute("delete zone",[](auto& d){deleteZone(d,"town");}));
    CHECK(history.document().zones.zones.size()==1);CHECK(history.document().zones.zoneConnectors.size()==1);
    history.undo();history.undo();CHECK(history.document()==f.d);
    // Deleting the Link deletes the connectors on it; Undo restores them.
    CHECK(history.document().zones.zoneConnectors.size()==2); // forcing: both name f.in
    CHECK(history.execute("delete road",[&](auto& d){deleteLink(d,f.in);}));
    CHECK(history.document().zones.zoneConnectors.empty());CHECK(history.document().zones.zones.size()==2);
    history.undo();CHECK(history.document()==f.d);
    CHECK(history.execute("delete selection",[&](auto& d){deleteObjects(d,{f.in});}));
    CHECK(history.document().zones.zoneConnectors.empty());history.undo();
    // Split: the origin stays on the upstream piece (it keeps the id), the destination moves on.
    std::string downstream;
    CHECK(history.execute("split",[&](auto& d){downstream=splitLink(d,f.in,50);}));
    const auto& c=history.document().zones.zoneConnectors;
    CHECK(c[0].linkId==f.in);CHECK(c[1].linkId==downstream);CHECK(downstream!=f.in);
    history.undo();
    // Replace by id; unknown ids are refused.
    CHECK(history.execute("share",[](auto& d){putZoneConnector(d,{"site-out","site",d.zones.zoneConnectors[0].linkId,ZoneDirection::origin,5});}));
    CHECK(history.document().zones.zoneConnectors[0].share==5);CHECK(history.document().zones.zoneConnectors.size()==2);
    test::throws([&]{history.execute("x",[](auto& d){deleteZoneConnector(d,"nowhere");});},"EDIT_UNKNOWN_OBJECT");
    test::throws([&]{history.execute("x",[](auto& d){deleteOdMatrix(d,"nowhere");});},"EDIT_UNKNOWN_OBJECT");
    // A new id never collides with a zone-layer id.
    auto d=f.d;d.zones.zones[1].id="link-"+std::to_string(d.nextId);d.zones.odMatrices[0].zoneIds[1]=d.zones.zones[1].id;
    d.zones.zoneConnectors[1].zoneId=d.zones.zones[1].id;validateDocument(d);
    CHECK(allocateId(d,"link")!=d.zones.zones[1].id);
}
TEST(zonescommands, z6_mid_network_connector_is_kept_by_authoring) {
    auto f=fixture();
    const auto link=addLink(f.d,{{120,0},{200,0}},1,3.5);
    addConnector(f.d,{f.in,f.d.network.links[0].lanes[0].id,100},{link,f.d.network.links.back().lanes[0].id,0});
    // forcing: f.in is no longer a terminal Link, so a destination on it is not at a Link end.
    CHECK(std::any_of(f.d.network.connectors.begin(),f.d.network.connectors.end(),[&](const auto& c){return c.from.linkId==f.in;}));
    putZoneConnector(f.d,{"mid","town",f.in,ZoneDirection::destination,1});
    validateDocument(f.d);CHECK(parseDocument(documentJson(f.d))==f.d); // Run refuses it (M8.3)
}
