#include "node_model_demand_fixture.hpp"
#include "../src/project/node_analysis_fill.hpp"
#include "../src/project/document_schema.hpp"
using namespace multra;
namespace {
Json sourceJson() {
    return {{"kind","micro-demand"},{"revision",0},{"projectSha256",std::string(64,'a')},
        {"start",0},{"end",900},{"heavyVehicleTypes",Json::array({"rigid-truck"})},
        {"files",Json::array({{{"logicalPath","catalog/type"},{"sha256",std::string(64,'b')}}})}};
}
void codec(DrivingSide side) {
    const auto d=nodeModelTest::fixture(side);const auto old=documentJson(d);
    CHECK(old["schemaVersion"]<64&&!old["network"]["nodes"][0]["analysis"].contains("modelSource"));
    auto j=old;j["schemaVersion"]=64;j["network"]["nodes"][0]["analysis"]["modelSource"]=sourceJson();
    CHECK(j["network"]["nodes"][0]["analysis"]["modelSource"]["projectSha256"]==std::string(64,'a'));
    const auto loaded=parseDocument(j);CHECK(documentJson(loaded)==j&&documentSchemaVersion(loaded)==64);
    auto earlier=j;earlier["schemaVersion"]=63;
    for(const auto& v:{sourceJson(),Json(nullptr)}) {
        earlier["network"]["nodes"][0]["analysis"]["modelSource"]=v;bool threw=false;
        try{(void)parseDocument(earlier);}catch(const ValidationError& e){threw=true;CHECK(junctionTest::issue(e.issues,"EDIT_UNSUPPORTED_FIELD","network.nodes[0].analysis.modelSource"));}
        CHECK(threw);
    }
    for(const auto* field:{"kind","revision","projectSha256","start","end","heavyVehicleTypes","files"}) {
        auto bad=j;bad["network"]["nodes"][0]["analysis"]["modelSource"].erase(field);
        CHECK(!bad["network"]["nodes"][0]["analysis"]["modelSource"].contains(field));
        test::throws([&]{(void)parseDocument(bad);},"modelSource");
    }
    for(const auto& [field,value]:std::vector<std::pair<std::string,Json>>{{"kind","macro"},{"projectSha256","bad"},{"start",-1},{"end",0},{"heavyVehicleTypes",{"car","car"}},{"unexpected",true}}) {
        auto bad=j;bad["network"]["nodes"][0]["analysis"]["modelSource"][field]=value;
        CHECK(bad!=j);test::throws([&]{(void)parseDocument(bad);},"modelSource");
    }
    CHECK(documentJson(parseDocument(old))==old); // feature-selected, no older-file migration
}
void commands(DrivingSide side) {
    auto d=nodeModelTest::fixture(side);const auto id=d.network.nodes[0].id;const auto before=documentJson(d);
    const NodeModelDemandRequest request{30,90,{"rigid-truck"}};
    const auto p=previewNodeModelDemand(" \r\n"+before.dump()+"\r\n",id,request,test::root()/"data");
    CHECK(p.turns.size()==12&&p.turns[0].vehiclesPerHour>0);
    const auto filled=analysisFromModel(*d.network.nodes[0].analysis,p,request);
    CHECK(filled.peakHourFactor==1&&filled.analysisPeriod==(60./3600));
    CHECK(filled.cbd&&filled.baseSaturationFlow==d.network.nodes[0].analysis->baseSaturationFlow&&filled.approaches==d.network.nodes[0].analysis->approaches);
    History h;h.reset(d);CHECK(h.execute("editorNodeAnalysis",[&](auto& doc){setNodeAnalysis(doc,id,filled);}));
    const auto after=documentJson(h.document());CHECK(after["schemaVersion"]==64&&h.states().size()==2);
    const auto& source=after["network"]["nodes"][0]["analysis"]["modelSource"];
    CHECK(source["start"]==30&&source["end"]==90&&source["heavyVehicleTypes"]==Json::array({"rigid-truck"}));
    CHECK(source["projectSha256"]==inputSha256(" \r\n"+before.dump()+"\r\n"));
    CHECK(parseDocument(after)==h.document());h.undo();CHECK(documentJson(h.document())==before);h.redo();CHECK(documentJson(h.document())==after);
    for(int which=0;which<4;++which) {
        CHECK(h.execute("manual",[&](auto& doc){auto& a=*doc.network.nodes[0].analysis;
            if(which==0)a.turns[0].vehiclesPerHour+=1;else if(which==1)a.turns[0].heavyVehiclePercent=17;else if(which==2)a.peakHourFactor=.92;else a.analysisPeriod=.25;}));
        CHECK(!documentJson(h.document())["network"]["nodes"][0]["analysis"].contains("modelSource"));
        h.undo();CHECK(documentJson(h.document())==after);
    }
    CHECK(h.execute("site",[&](auto& doc){auto a=*doc.network.nodes[0].analysis;a.cbd=!a.cbd;setNodeAnalysis(doc,id,a);}));
    CHECK(documentJson(h.document())["network"]["nodes"][0]["analysis"]["modelSource"]==source);
}
}
TEST(nodeanalysis,na26_source_codec_left){codec(DrivingSide::left);}
TEST(nodeanalysis,na26_source_codec_right){codec(DrivingSide::right);}
TEST(nodeanalysis,na26_source_command_left){commands(DrivingSide::left);}
TEST(nodeanalysis,na26_source_command_right){commands(DrivingSide::right);}
