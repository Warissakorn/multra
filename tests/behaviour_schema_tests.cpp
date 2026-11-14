#include "test.hpp"
#include "../src/project/latest_schema.hpp"
#include "behaviour_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/network_fields.hpp"
#include "../src/core/w74.hpp"
#include "../src/core/perception.hpp"
#include "../src/core/lateral.hpp"

using namespace multra;
namespace bc=behaviourCases;
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
std::vector<std::string> coreKeys() {
    std::vector<std::string> keys;
    for(const auto& k:w74ParameterKeys())keys.push_back(k.name);
    for(const auto& k:perceptionParameterKeys())keys.push_back(k.name);
    for(const auto& k:lateralParameterKeys())keys.push_back(k.name);
    return keys;
}
}
TEST(project, behaviour_schema_tags_the_model_and_splits_the_rows) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& items=definitionFor(schema,v).at("properties").at("behaviours").at("items");
        const bool branched=items.contains("if");CHECK(branched==(v>=bc::w74From));
        const auto& prototype=resolve(schema,branched?items.at("else"):items);const auto& p=prototype.at("properties");
        CHECK(prototype.at("additionalProperties")==true);
        Json required=Json::array({"id"});if(v>=bc::libraryFrom)required.push_back("model");
        for(const auto* k:bc::prototypeRequired)required.push_back(k);
        CHECK(prototype.at("required")==required);
        CHECK((p.at("model")==false)==(v<bc::libraryFrom));CHECK((p.at("name")==false)==(v<bc::libraryFrom));
        if(v>=bc::libraryFrom)CHECK(p.at("model").at("enum").size()==(v<bc::w74From?1u:2u));
        for(const auto& k:w74ParameterKeys())CHECK(p.at(k.name)==false);
        CHECK(p.at("observedVehiclesAhead").at("type")=="integer");
        if(!branched)continue;
        CHECK(items.at("if").at("properties").at("model").at("const")=="w74");
        const auto& w=resolve(schema,items.at("then"));
        for(const auto& k:w74ParameterKeys()) {
            CHECK(w.at("properties").at(k.name).at("type")=="number");
            CHECK(std::find(w.at("required").begin(),w.at("required").end(),k.name)!=w.at("required").end());
        }
        for(const auto* k:bc::prototypeRequired)CHECK(w.at("properties").at(k)==false);
        CHECK(w.at("properties").at("discretionaryLaneChangeHoldTime")==false);
        CHECK(w.at("properties").at("model").at("const")=="w74");
    }
}
TEST(project, behaviour_schema_lists_every_core_and_library_key) {
    // A new core parameter cannot fall out of the schema or the from-21 closure.
    const auto schema=projectDocumentSchema();
    const auto& items=definitionFor(schema,kLatestSchema).at("properties").at("behaviours").at("items");
    const auto& prototype=resolve(schema,items.at("else"));const auto& w=resolve(schema,items.at("then"));
    for(const auto& key:coreKeys()) {
        CHECK(prototype.at("properties").contains(key));CHECK(w.at("properties").contains(key));
        CHECK(networkCodec::behaviourKey(key));
    }
    CHECK(prototype.at("properties").size()==w.at("properties").size());
    for(const auto& [key,rule]:prototype.at("properties").items())CHECK(networkCodec::behaviourKey(key));
    CHECK(!networkCodec::behaviourKey("cc0"));
}
TEST(project, behaviour_structural_refusals_are_also_refused_by_the_codec) {
    for(int v=1;v<=kLatestSchema;++v) {
        const auto d=parseDocument(bc::minimal(v));CHECK(!d.definition->externalBehaviours);
        CHECK(!d.definition->behaviours[0].w74);
        if(v>=bc::w74From)CHECK(parseDocument(bc::minimal(v,true)).definition->behaviours[0].w74.has_value());
    }
    for(const auto& c:bc::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted behaviour structural refusal: "+c.name);
    }
}
