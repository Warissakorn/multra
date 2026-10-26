#include "test.hpp"
#include "signal_head_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace {
const Json& resolve(const Json& schema,const Json& ref) {
    return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));
}
const Json& headFor(const Json& schema,int version) {
    for(const auto& branch:schema.at("allOf"))
        for(const auto& v:branch.at("if").at("properties").at("schemaVersion").at("enum"))
            if(v==version) {
                const auto& n=resolve(schema,branch.at("then").at("properties").at("network"));
                return resolve(schema,n.at("properties").at("signalHeads").at("items"));
            }
    throw std::runtime_error("Missing SignalHead schema version");
}
}
TEST(project, signal_head_schema_defaults_and_conditional_group_shapes_match_native_reads) {
    const auto schema=projectDocumentSchema();const NetworkSignalHead defaults;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto j=signalHeadCases::minimal(v);const auto d=parseDocument(j);
        CHECK(d.network.signalHeads.size()==1);const auto& h=d.network.signalHeads.front();
        CHECK(h.name==defaults.name);CHECK(h.connectorId==defaults.connectorId);
        CHECK(h.controllerId==defaults.controllerId);CHECK(h.groupNumber==defaults.groupNumber);
        const auto& shape=headFor(schema,v);const auto& p=shape.at("properties");
        CHECK(shape.at("additionalProperties")==Json(v<7));
        CHECK(p.at("controllerId").at("type")==Json::array({"string","null"}));
        CHECK(p.at("groupNumber")==Json::object());
        CHECK(p.contains("generated")== (v>=54));
        const auto& active=shape.at("allOf").at(0).at("then");
        CHECK(active.at("required")==Json::array({"groupNumber"}));
        CHECK(active.at("properties").at("groupNumber").at("maximum")==std::numeric_limits<int>::max());
        CHECK(!active.at("properties").at("groupNumber").contains("x-multra-nativeType"));
        auto ignored=j;ignored["network"]["signalHeads"][0]["groupNumber"]="ignored";
        CHECK(ignored.at("network").at("signalHeads")[0].at("groupNumber").is_string());
        CHECK(parseDocument(ignored)==d);
        ignored["network"]["signalHeads"][0]["controllerId"]=nullptr;
        CHECK(parseDocument(ignored)==d);
    }
}
TEST(project, signal_head_empty_controller_activates_group_and_accepts_integer_valued_floats) {
    auto j=signalHeadCases::minimal();auto& h=j["network"]["signalHeads"][0];
    h["controllerId"]="";CHECK(h.at("controllerId")=="");
    test::throws([&]{parseDocument(j);},"Missing field: groupNumber");
    h["groupNumber"]=1.0;CHECK(h.at("groupNumber").is_number_float());
    CHECK(parseDocument(j).network.signalHeads[0].groupNumber==1);
    h["groupNumber"]=1.5;CHECK(h.at("groupNumber")==1.5);
    test::throws([&]{parseDocument(j);},"DUPLICATE_SIGNAL_GROUP");
    h["groupNumber"]=static_cast<std::int64_t>(std::numeric_limits<int>::max())+1;
    CHECK(h.at("groupNumber")>std::numeric_limits<int>::max());
    test::throws([&]{parseDocument(j);},"DUPLICATE_SIGNAL_GROUP");
    h["groupNumber"]=true;CHECK(h.at("groupNumber").is_boolean());
    test::throws([&]{parseDocument(j);},"Expected number: groupNumber");
}
TEST(project, signal_head_group_read_is_not_artificially_gated_at_schema_13) {
    for(int v=1;v<=kLatestSchema;++v) {
        auto j=signalHeadCases::minimal(v);auto& h=j["network"]["signalHeads"][0];
        h["controllerId"]="controller";h["programId"]="";h["groupNumber"]=2;
        CHECK(h.at("controllerId")=="controller");
        const auto n=parseNetwork(j.at("network"),v);
        CHECK(n.signalHeads[0].controllerId=="controller");CHECK(n.signalHeads[0].groupNumber==2);
    }
}
TEST(project, signal_head_legacy_program_migration_stays_after_document_decoding) {
    auto j=signalHeadCases::minimal();auto d=parseDocument(j);d.definition.emplace();
    d.definition->signalPrograms.push_back({"legacy",0,{{10,SignalColor::green},{10,SignalColor::red}}});
    j=documentJson(d);j["schemaVersion"]=12;
    CHECK(j.at("network").at("signalHeads")[0].at("programId")=="legacy");
    const auto migrated=parseDocument(j);
    CHECK(migrated.definition->signalControllers.size()==1);
    CHECK(migrated.definition->signalPrograms.empty());
    CHECK(!migrated.network.signalHeads[0].controllerId.empty());
    CHECK(migrated.network.signalHeads[0].programId.empty());
    j["schemaVersion"]=13;
    const auto unmigrated=parseDocument(j);
    CHECK(unmigrated.network.signalHeads[0].programId=="legacy");
    CHECK(unmigrated.network.signalHeads[0].controllerId.empty());
}
TEST(project, signal_head_structural_refusals_are_also_refused_by_the_codec) {
    for(const auto& c:signalHeadCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted SignalHead structural refusal: "+c.name);
    }
}
