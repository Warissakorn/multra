#include "network_fields.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace multra {
namespace {
using namespace networkCodec;
std::vector<double> numbers(const Json& j,Object object,const char* key,const char* error) {
    std::vector<double> result;
    for(const auto& n:value(j,object,key)) {
        if(!n.is_number())throw std::invalid_argument(error);
        result.push_back(n.get<double>());
    }
    return result;
}
}
std::vector<RoutingDecision> parseRoutingDecisions(const Json& definition) {
    using namespace networkCodec;
    std::vector<RoutingDecision> result;
    for(const auto& x:value(definition,Object::definition,"routingDecisions")) {
        // These rows have always been strict independently of the file version.
        // The versioned document position guard remains before definition parsing.
        knownFields(x,Object::routingDecision,"routingDecision",routingPositionVersion);
        const auto field=[&](const char* key)->const Json& {return value(x,Object::routingDecision,key);};
        RoutingDecision decision{field("id").get<std::string>(),present(x,"name")?field("name").get<std::string>():"",{}};
        if(present(x,"linkId"))decision.linkId=field("linkId").get<std::string>();
        if(present(x,"position"))decision.position=field("position").get<double>();
        for(const auto& r:field("routes")) {
            knownFields(r,Object::routingRoute,"routingDecision.routes",routingPositionVersion);
            const auto share=[&](const char* key)->const Json& {return value(r,Object::routingRoute,key);};
            decision.routes.push_back({share("routeId").get<std::string>(),share("relativeFlow").get<double>()});
            if(present(r,"destinationLinkId"))decision.routes.back().destinationLinkId=share("destinationLinkId").get<std::string>();
            if(present(r,"intervalFlows"))decision.routes.back().intervalFlows=numbers(r,Object::routingRoute,"intervalFlows","INVALID_SHARE");
        }
        if(present(x,"intervals"))for(const auto& i:field("intervals"))
            decision.intervals.push_back({value(i,Object::routingInterval,"startTime").get<double>(),
                value(i,Object::routingInterval,"endTime").get<double>()});
        if(x.contains("typeRules"))for(const auto& rule:field("typeRules")) {
            knownFields(rule,Object::routingTypeRule,"routingDecision.typeRules",routingPositionVersion);
            RoutingTypeRule parsed{value(rule,Object::routingTypeRule,"vehicleTypeId").get<std::string>(),
                numbers(rule,Object::routingTypeRule,"relativeFlows","Expected numbers: relativeFlows"),{}};
            for(const auto& row:value(rule,Object::routingTypeRule,"intervalFlows")) {
                if(!row.is_array())throw std::invalid_argument("ROUTING_DECISION_INTERVALS");
                std::vector<double> flows;
                for(const auto& f:row) {
                    if(!f.is_number())throw std::invalid_argument("INVALID_SHARE");
                    flows.push_back(f.get<double>());
                }
                parsed.intervalFlows.push_back(std::move(flows));
            }
            decision.typeRules.push_back(std::move(parsed));
        }
        result.push_back(std::move(decision));
    }
    return result;
}
}
