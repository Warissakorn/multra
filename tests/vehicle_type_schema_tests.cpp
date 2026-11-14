#include "test.hpp"
#include "../src/project/latest_schema.hpp"
#include "vehicle_type_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/json.hpp"

using namespace multra;
namespace vtc=vehicleTypeCases;
namespace {
const Json& resolve(const Json& schema,const Json& ref) {
    return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));
}
const Json& definitionFor(const Json& schema,int version) {
    for(const auto& branch:schema.at("allOf"))
        for(const auto& v:branch.at("if").at("properties").at("schemaVersion").at("enum"))
            if(v==version)return resolve(schema,branch.at("then").at("properties").at("definition"));
    throw std::runtime_error("Missing definition schema version");
}
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(document);}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
void typeRefused(const Json& type,const std::string& code,const std::string& path) {
    try {(void)parseVehicleType(type);}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
Json with(int version,const char* key,Json value) {auto j=vtc::minimal(version);j["definition"]["vehicleTypes"][0][key]=std::move(value);return j;}
}
TEST(project, vehicle_type_reader_keeps_its_open_legacy_rows_codes_and_order) {
    // Below 21 an unknown key is read and not written back; from 21 parseBehaviourLibrary refuses it.
    auto legacy=with(vtc::closedFrom-1,"extra",1);CHECK(legacy.at("definition").at("vehicleTypes")[0].contains("extra"));
    CHECK(!documentJson(parseDocument(legacy)).at("definition").at("vehicleTypes")[0].contains("extra"));
    refusedAt(with(vtc::closedFrom,"extra",1),"EDIT_UNSUPPORTED_FIELD","vehicleTypes[0].extra");
    // D562: desiredSpeed and curve points close with the row from 21; older files stay additive.
    auto speed=vtc::minimal(vtc::closedFrom-1);speed["definition"]["vehicleTypes"][0]["desiredSpeed"]["extra"]=1;
    CHECK(!documentJson(parseDocument(speed)).at("definition").at("vehicleTypes")[0].at("desiredSpeed").contains("extra"));
    speed["schemaVersion"]=vtc::closedFrom;refusedAt(speed,"EDIT_UNSUPPORTED_FIELD","vehicleTypes[0].desiredSpeed.extra");
    auto point=vtc::minimal(vtc::curveFrom);point["definition"]["vehicleTypes"][0]["desiredSpeed"]["curve"]=
        Json::array({Json{{"speed",11.11},{"share",0}},Json{{"speed",16.67},{"share",1},{"extra",1}}});
    refusedAt(point,"EDIT_UNSUPPORTED_FIELD","vehicleTypes[0].desiredSpeed.curve[1].extra");
    CHECK(parseVehicleType(point.at("definition").at("vehicleTypes")[0]).desiredSpeed.curve.size()==2); // the catalog reader stays open
    auto t=vtc::type();t["axles"]["extra"]=1;CHECK(t.at("axles").contains("extra"));
    typeRefused(t,"EDIT_UNSUPPORTED_FIELD","vehicleType.axles.extra");
    t=vtc::type();t.erase("id");t.erase("desiredSpeed");CHECK(!t.contains("id"));
    test::throws([&]{parseVehicleType(t);},"Missing field: desiredSpeed"); // read before id
    t=vtc::type();t.erase("id");test::throws([&]{parseVehicleType(t);},"Missing field: id");
    t=vtc::type();t["length"]="x";test::throws([&]{parseVehicleType(t);},"Expected number: length");
    t=vtc::type();t["axles"]=nullptr;test::throws([&]{parseVehicleType(t);},"Field is null: axles");
    t=vtc::type();t["axles"].erase("rearOverhang");test::throws([&]{parseVehicleType(t);},"Missing field: rearOverhang");
    t=vtc::type();t["desiredSpeed"].erase("min");CHECK(!t.at("desiredSpeed").contains("min"));
    test::throws([&]{parseVehicleType(t);},"Missing field: min"); // D562: MULTRA's sentence
    t=vtc::type();t["desiredSpeed"]["max"]=true;test::throws([&]{parseVehicleType(t);},"Expected number: max");
    t=vtc::type();t["desiredSpeed"]["min"]=nullptr;test::throws([&]{parseVehicleType(t);},"Field is null: min");
    t=vtc::type();t["kerbLanes"]=2.0;CHECK(t.at("kerbLanes").is_number_float());
    typeRefused(t,"INVALID_KERB_LANES","vehicleType.kerbLanes");
    t=vtc::type();t["pcu"]={{"value",1}};typeRefused(t,"INVALID_PCU","vehicleType.pcu");
    t=vtc::type();t["desiredSpeed"]["curve"]=7;typeRefused(t,"INVALID_SPEED_CURVE","vehicleType.desiredSpeed.curve");
    auto named=with(vtc::closedFrom,"name",nullptr);CHECK(named.at("definition").at("vehicleTypes")[0].at("name").is_null());
    test::throws([&]{parseDocument(named);},"EDIT_CATALOG_READ");
    CHECK(parseDefinition(named.at("definition")).vehicleTypes.size()==1); // a bare definition ignores name
    auto curve=vtc::minimal(vtc::curveFrom-1);curve["definition"]["vehicleTypes"][0]["desiredSpeed"]["curve"]=nullptr;
    refusedAt(curve,"EDIT_UNSUPPORTED_FIELD","vehicleTypes[0].desiredSpeed.curve");
    refusedAt(with(vtc::kerbLanesFrom-1,"kerbLanes",nullptr),"EDIT_UNSUPPORTED_FIELD","vehicleTypes[0].kerbLanes");
    refusedAt(with(vtc::pcuFrom-1,"pcu",nullptr),"EDIT_UNSUPPORTED_FIELD","vehicleTypes[0].pcu");
}
TEST(project, vehicle_type_schema_tracks_the_closed_row_floor_and_gated_keys) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& d=definitionFor(schema,v);const auto& list=d.at("properties").at("vehicleTypes");
        for(const auto& r:d.at("required"))CHECK(r!="vehicleTypes");
        CHECK(list.at("type")=="array");
        const auto& t=resolve(schema,list.at("items"));const auto& p=t.at("properties");
        CHECK(t.at("additionalProperties")==(v<vtc::closedFrom));
        Json required=Json::array();for(const auto* k:vtc::requiredKeys)required.push_back(k);
        CHECK(t.at("required")==required);
        CHECK(p.at("name").at("type")=="string");
        const auto& axles=resolve(schema,p.at("axles"));CHECK(axles.at("additionalProperties")==false);
        CHECK(axles.at("required")==Json::array({"wheelbase","frontOverhang","rearOverhang"}));
        const auto& speed=resolve(schema,p.at("desiredSpeed"));CHECK(speed.at("additionalProperties")==(v<vtc::closedFrom));
        CHECK(speed.at("required")==Json::array({"min","max"}));
        CHECK((speed.at("properties").at("curve")==false)==(v<vtc::curveFrom));
        if(v>=vtc::curveFrom)CHECK(resolve(schema,speed.at("properties").at("curve").at("items")).at("additionalProperties")==false);
        CHECK((p.at("kerbLanes")==false)==(v<vtc::kerbLanesFrom));
        if(v>=vtc::kerbLanesFrom) {
            CHECK(p.at("kerbLanes").at("type")=="integer");CHECK(!p.at("kerbLanes").contains("minimum"));
            CHECK(p.at("kerbLanes").at("x-multra-nativeType")=="integer-token");
        }
        CHECK((p.at("pcu")==false)==(v<vtc::pcuFrom));
        if(v>=vtc::pcuFrom) {
            const auto& pcu=resolve(schema,p.at("pcu"));CHECK(pcu.at("additionalProperties")==false);
            CHECK(pcu.at("required")==Json::array({"value","source"}));
        }
    }
}
TEST(project, vehicle_type_structural_refusals_are_also_refused_by_the_codec) {
    for(int v=1;v<=kLatestSchema;++v) {
        const auto d=parseDocument(vtc::minimal(v));
        CHECK(!d.definition->externalVehicleTypes);CHECK(d.definition->vehicleTypes.size()==1);
        CHECK(d.definition->vehicleTypes[0].axles.has_value());
    }
    for(const auto& c:vtc::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted vehicle-type structural refusal: "+c.name);
    }
}
