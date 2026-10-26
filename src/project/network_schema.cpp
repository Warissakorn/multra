#include "network_schema.hpp"
#include "network_fields.hpp"
#include "document.hpp"
#include "document_envelope.hpp"
#include <nlohmann/json.hpp>
#include <map>

namespace multra {
namespace {
using namespace networkCodec;
class NetworkSchema {
    Json definitions_=Json::object();
    std::map<std::string,std::string> names_;
    Json fieldSchema(const Field& field,int version) {
        Json rule=Json::object();
        switch(field.kind) {
        case Kind::text:rule={{"type",field.nullable?Json::array({"string","null"}):Json("string")}};break;
        case Kind::side:
            rule={{"type",field.nullable?Json::array({"string","null"}):Json("string")},
                {"enum",field.nullable?Json::array({"left","right",nullptr}):Json::array({"left","right"})}};
            break;
        case Kind::number:rule={{"type","number"}};break;
        case Kind::level:rule={{"type","integer"},{"minimum",-levelLimit},{"maximum",levelLimit},{"x-multra-nativeType","integer-token"}};break;
        case Kind::array:rule={{"type","array"},{"items",Json::object()}};break;
        case Kind::links:rule={{"type","array"},{"items",reference(Object::link,version)}};break;
        case Kind::connectors:rule={{"type","array"},{"items",reference(Object::connector,version)}};break;
        case Kind::reference:rule=reference(Object::reference,version);break;
        case Kind::numbers:rule={{"type","array"},{"items",{{"type","number"}}}};break;
        case Kind::generated:rule={{"type","boolean"},{"const",true}};break;
        case Kind::lanes:rule={{"type","array"},{"items",reference(Object::lane,version)}};break;
        case Kind::geometry:rule={{"type","array"},{"items",reference(Object::point,version)}};break;
        case Kind::markings: {
            Json names=Json::array();
            for(const auto m:{MarkingType::solid,MarkingType::dashed,MarkingType::none,MarkingType::doubleLine})names.push_back(markingName(m));
            rule={{"type","array"},{"items",{{"type","string"},{"enum",names}}}};break;
        }
        case Kind::unchecked:break;
        }
        return rule;
    }
    Json objectSchema(Object object,int version) {
        Json properties=Json::object(),required=Json::array();
        for(const auto& field:fields(object)) {
            if(version<field.rejectBefore || (field.rejectFrom && version>=field.rejectFrom)) {properties[field.name]=false;continue;}
            if(version<field.knownFrom)continue;
            properties[field.name]=fieldSchema(field,version);
            if(field.required)required.push_back(field.name);
        }
        if(object==Object::link) {
            const Link defaults;
            properties["level"]["default"]=defaults.level;
            properties["laneOffset"]["default"]=defaults.laneOffset;
            properties["displayType"]["default"]=defaults.displayType;
            properties["name"]["default"]=defaults.name;
            properties["boundaryMarkings"]["default"]=Json::array();
        }
        if(object==Object::connector) {
            const Connector defaults;
            properties["fromLaneCount"]["default"]=defaults.fromLaneCount;
            properties["toLaneCount"]["default"]=defaults.toLaneCount;
            properties["level"]["default"]=defaults.level;
            properties["displayType"]["default"]=defaults.displayType;
            properties["name"]["default"]=defaults.name;
            for(const auto* key:{"laneBlend","laneWidths","laneMarkings"})properties[key]["default"]=Json::array();
        }
        return {{"type","object"},{"properties",properties},{"required",required},{"additionalProperties",version<strictVersion}};
    }
public:
    Json reference(Object object,int version) {
        const auto schema=objectSchema(object,version);
        const auto signature=schema.dump();
        auto it=names_.find(signature);
        if(it==names_.end()) {
            const char* prefix=object==Object::network?"network":object==Object::link?"link":object==Object::lane?"lane":
                object==Object::connector?"connector":object==Object::reference?"reference":"point";
            const auto name=std::string(prefix)+std::to_string(version);
            definitions_[name]=schema;
            it=names_.emplace(signature,name).first;
        }
        return {{"$ref","#/$defs/"+it->second}};
    }
    Json compose() {
        auto result=projectEnvelopeSchema();
        result["x-multra-coverage"]="envelope-background-network-link-lane-connector-reference";
        result["$comment"]="Partial envelope/background and versioned Network/Link/Lane/Connector/geometry/attachment-reference structure. Model invariants and nested signal/control/demand/optional features require the codec. Native integer tokens and UTF-8 byte limits also require the codec.";
        Json groups=Json::array();std::map<std::string,std::size_t> indices;
        for(int version=1;version<=kLatestSchema;++version) {
            const auto ref=reference(Object::network,version);
            const auto key=ref.at("$ref").get<std::string>();
            auto [it,inserted]=indices.emplace(key,groups.size());
            if(inserted)groups.push_back({{"if",{{"properties",{{"schemaVersion",{{"enum",Json::array()}}}}}}},
                {"then",{{"properties",{{"network",ref}}}}}});
            groups[it->second]["if"]["properties"]["schemaVersion"]["enum"].push_back(version);
        }
        result["$defs"]=std::move(definitions_);result["allOf"]=std::move(groups);
        return result;
    }
};
}
Json projectDocumentSchema() {return NetworkSchema{}.compose();}
}
