#include "test.hpp"
#include "../src/project/latest_schema.hpp"
#include "zone_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include <functional>

using namespace multra;
namespace zc=zoneCases;
namespace {
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(document);}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
Json withMatrix(int version,const std::function<void(Json&)>& edit) {
    auto j=version>=zc::modeFrom?zc::withPeriod(zc::minimal(version)):zc::minimal(version);edit(j["odMatrices"][0]);return j;
}
}
TEST(project, zone_reader_keeps_its_floors_paths_and_codes) {
    for(int v=zc::from;v<=kLatestSchema;++v) {
        const auto d=parseDocument(v>=zc::modeFrom?zc::withPeriod(zc::minimal(v)):zc::minimal(v));
        CHECK(d.zones.zones.size()==2);CHECK(d.zones.zoneConnectors.size()==1);CHECK(d.zones.parkingLots[0].capacity==10);
        CHECK(d.zones.odMatrices[0].compositionId=="mix");
    }
    for(int v=zc::streetFrom;v<=kLatestSchema;++v)CHECK(parseDocument(zc::street(v)).zones.zoneConnectors[0].streetSegmentId=="west");
    for(const auto* key:{"zones","zoneConnectors","odMatrices","parkingLots"}) {
        auto j=zc::minimal(zc::from-1);j[key]=nullptr;CHECK(j.at(key).is_null());
        for(const auto* other:{"zones","zoneConnectors","odMatrices","parkingLots"})if(std::string(other)!=key)j.erase(other);
        test::throws([&]{parseDocument(j);},std::string("UNSUPPORTED_FIELD: ")+key);
        j=zc::minimal();j[key]=nullptr;test::throws([&]{parseDocument(j);},"EDIT_ZONE_INVALID");
        j=zc::minimal();j[key]=Json::array();test::throws([&]{parseDocument(j);},"EDIT_ZONE_INVALID");
        j=zc::minimal();j[key][0]["extra"]=1;test::throws([&]{parseDocument(j);},"EDIT_ZONE_INVALID");
    }
    // The matrix's either-or checks come before the key check, with their own codes.
    const auto both=[](Json& m){m["periodId"]="am";m["extra"]=1;};
    refusedAt(withMatrix(zc::modeFrom,both),"OD_PERIOD_AMBIGUOUS","odMatrices[0]");
    refusedAt(withMatrix(zc::modeFrom,[](Json& m){m["modeId"]="car-mode";m["extra"]=1;}),"OD_TYPES_AMBIGUOUS","odMatrices[0]");
    refusedAt(withMatrix(zc::modeFrom,[](Json& m){m.erase("compositionId");}),"OD_TYPES_AMBIGUOUS","odMatrices[0]");
    refusedAt(withMatrix(zc::modeFrom-1,[](Json& m){m.erase("compositionId");m["modeId"]="car-mode";}),"EDIT_UNSUPPORTED_FIELD","odMatrices[0].modeId");
    refusedAt(withMatrix(zc::modeFrom-1,[](Json& m){m.erase("startTime");m.erase("endTime");m["periodId"]="am";}),"EDIT_UNSUPPORTED_FIELD","odMatrices[0].periodId");
    refusedAt(withMatrix(zc::generatedFrom-1,[](Json& m){m["generatedBy"]="mode-choice";}),"EDIT_UNSUPPORTED_FIELD","odMatrices[0].generatedBy");
    refusedAt(withMatrix(zc::generatedFrom,[](Json& m){m["generatedBy"]=nullptr;}),"OD_GENERATED_INVALID","odMatrices[0].generatedBy");
    test::throws([&]{parseDocument(withMatrix(zc::modeFrom,[](Json& m){m.erase("startTime");}));},"EDIT_ZONE_INVALID");
    const auto mode=parseDocument(withMatrix(zc::modeFrom,[](Json& m){m.erase("compositionId");m["modeId"]="car-mode";}));
    CHECK(mode.zones.odMatrices[0].modeId=="car-mode");
    const auto period=parseDocument(withMatrix(zc::modeFrom,[](Json& m){m.erase("startTime");m.erase("endTime");m["periodId"]="am";}));
    CHECK(period.zones.odMatrices[0].periodId=="am");
    // A connector's street keys: refused by name below 57, and each side of 59 refuses the other's names.
    auto j=zc::minimal(zc::streetFrom-1);j["zoneConnectors"][0]["roadId"]="west";refusedAt(j,"EDIT_UNSUPPORTED_FIELD","zoneConnectors[0].roadId");
    j=zc::minimal(zc::streetFrom-1);j["zoneConnectors"][0]["roadDirection"]="forward";refusedAt(j,"EDIT_UNSUPPORTED_FIELD","zoneConnectors[0].roadDirection");
    j=zc::street(zc::renamed-1);j["zoneConnectors"][0]["streetSegmentDirection"]="forward";
    refusedAt(j,"EDIT_UNSUPPORTED_FIELD","zoneConnectors[0].streetSegmentDirection");
    j=zc::street(zc::renamed);j["zoneConnectors"][0]["roadId"]="west";refusedAt(j,"EDIT_UNSUPPORTED_FIELD","zoneConnectors[0].roadId");
    j=zc::street(zc::renamed);j["zoneConnectors"][0]["streetSegmentId"]="";test::throws([&]{parseDocument(j);},"EDIT_ZONE_INVALID");
    // A lot's counts are unsigned tokens: a C++ int literal is a signed token and is refused.
    j=zc::minimal();j["parkingLots"][0]["capacity"]=10;CHECK(!j.at("parkingLots")[0].at("capacity").is_number_unsigned());
    test::throws([&]{parseDocument(j);},"EDIT_ZONE_INVALID");
}
TEST(project, zone_structural_refusals_are_also_refused_by_the_codec) {
    for(const auto& c:zc::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted zone structural refusal: "+c.name);
    }
}
TEST(project, zone_schema_is_refused_below_31_and_its_rows_are_closed) {
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    const auto rootFor=[&](int version)->const Json& {
        for(const auto& branch:schema.at("allOf"))
            for(const auto& v:branch.at("if").at("properties").at("schemaVersion").at("enum"))
                if(v==version)return branch.at("then").at("properties");
        throw std::runtime_error("Missing root schema version");
    };
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& p=rootFor(v);
        for(const auto* key:{"zones","zoneConnectors","odMatrices","parkingLots"}) {
            CHECK((p.at(key)==false)==(v<zc::from));
            if(v<zc::from)continue;
            CHECK(p.at(key).at("minItems")==1);
        }
        if(v<zc::from)continue;
        const auto& items=p.at("zoneConnectors").at("items");
        CHECK(items.contains("if")==(v>=zc::streetFrom));
        const auto& link=resolve(v>=zc::streetFrom?items.at("else"):items);
        CHECK(link.at("additionalProperties")==false);CHECK(link.at("required")==Json::array({"id","zoneId","linkId","direction","share"}));
        if(v>=zc::streetFrom) {
            const auto& street=resolve(items.at("then"));const auto id=v<zc::renamed?"roadId":"streetSegmentId";
            CHECK(items.at("if").at("required")==Json::array({id}));CHECK(street.at("properties").at(id).at("minLength")==1);
            CHECK(street.at("properties").at(v<zc::renamed?"streetSegmentId":"roadId")==false);
        }
        const auto& m=resolve(p.at("odMatrices").at("items"));
        CHECK(m.at("additionalProperties")==false);
        CHECK((m.at("properties").at("modeId")==false)==(v<zc::modeFrom));
        CHECK((m.at("properties").at("generatedBy")==false)==(v<zc::generatedFrom));
        CHECK(m.at("required")==(v<zc::modeFrom?Json::array({"id","zoneIds","cells","compositionId","startTime","endTime"}):Json::array({"id","zoneIds","cells"})));
        CHECK(m.contains("allOf")==(v>=zc::modeFrom));
        const auto& lot=resolve(p.at("parkingLots").at("items"));
        CHECK(lot.at("properties").at("capacity").at("x-multra-nativeType")=="unsigned-integer-token");
        CHECK(resolve(p.at("zones").at("items")).at("properties").at("polygon").at("items").at("maxItems")==2);
    }
}
