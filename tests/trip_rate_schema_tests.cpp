#include "test.hpp"
#include "trip_rate_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace tr=tripRateCases;
namespace {
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(Json::parse(document.dump()));}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
}
TEST(project, trip_rate_reader_keeps_its_floor_paths_and_codes) { // TR1
    for(int v=tr::from;v<=kLatestSchema;++v) {
        const auto d=parseDocument(tr::minimal(v));CHECK(d.tripRateSets.size()==1u);
        CHECK(d.tripRateSets[0].purposes[0].attractions.terms[0].attribute=="jobs");
    }
    auto j=tr::minimal(tr::from-1);j["tripRateSets"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","tripRateSets");
    j=tr::minimal();j["tripRateSets"]=nullptr;refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets");
    j=tr::minimal();j["tripRateSets"]=Json::array();refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets");
    j=tr::minimal();j["tripRateSets"][0]=7;refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0]");
    j=tr::minimal();j["tripRateSets"][0]["extra"]=1;refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0]");
    j=tr::minimal();j["tripRateSets"][0]["source"]["extra"]=1;refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0].source");
    j=tr::minimal();j["tripRateSets"][0]["purposes"][0]["extra"]=1;refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0].purposes[0]");
    j=tr::minimal();j["tripRateSets"][0]["purposes"][0]["productions"]["constant"]=0;
    refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0].purposes[0].productions");
    j=tr::minimal();j["tripRateSets"][0]["purposes"][0]["attractions"]["terms"][0]["extra"]=1;
    refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0].purposes[0].attractions.terms[0]");
    j=tr::minimal();j["tripRateSets"][0]["purposes"][0]["productions"]["categories"][0]["rate"]=-1;
    refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0].purposes[0].productions.categories[0].rate");
    j=tr::minimal();j["tripRateSets"][0]["purposes"][0]["productions"]["method"]="gravity";
    refusedAt(j,"INVALID_TRIP_RATE_SET","tripRateSets[0].purposes[0].productions.method");
    j=tr::minimal();j["tripRateSets"].push_back(tr::set());refusedAt(j,"DUPLICATE_ID","tripRateSets[1].id");
}
TEST(project, trip_rate_structural_refusals_are_also_refused_by_the_codec) { // TR3 (the codec half)
    for(const auto& c:tr::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted trip-rate structural refusal: "+c.name);
    }
}
TEST(project, trip_rate_schema_floor_and_closed_rows) { // TR2
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);
        CHECK((root->at("tripRateSets")==false)==(v<tr::from));
        if(v<tr::from)continue;
        const auto& list=root->at("tripRateSets");CHECK(list.at("minItems")==1);
        const auto& s=resolve(list.at("items"));
        CHECK(s.at("additionalProperties")==false);CHECK(s.at("required")==Json::array({"id","name","source","unit","purposes"}));
        const auto& source=resolve(s.at("properties").at("source"));
        CHECK(source.at("additionalProperties")==false);CHECK(!source.contains("required") || source.at("required").empty());
        const auto& purposes=s.at("properties").at("purposes");CHECK(purposes.at("minItems")==1);
        const auto& p=resolve(purposes.at("items"));
        CHECK(p.at("additionalProperties")==false);CHECK(p.at("required")==Json::array({"id","productions","attractions"}));
        const auto& side=p.at("properties").at("productions");CHECK(side==p.at("properties").at("attractions"));
        const auto& cross=resolve(side.at("else"));const auto& regression=resolve(side.at("then"));
        CHECK(cross.at("additionalProperties")==false);CHECK(cross.at("required")==Json::array({"method","categories"}));
        CHECK(cross.at("properties").at("categories").at("minItems")==1);
        const auto& category=resolve(cross.at("properties").at("categories").at("items"));
        CHECK(category.at("additionalProperties")==false);CHECK(category.at("properties").at("rate").at("minimum")==0);
        CHECK(regression.at("additionalProperties")==false);CHECK(regression.at("required")==Json::array({"method","constant","terms"}));
        CHECK(!regression.at("properties").at("terms").contains("minItems"));
        const auto& term=resolve(regression.at("properties").at("terms").at("items"));
        CHECK(term.at("additionalProperties")==false);CHECK(term.at("required")==Json::array({"attribute","coefficient"}));
    }
}
