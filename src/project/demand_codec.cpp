#include "network_fields.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace multra::networkCodec {
std::vector<Route> readRoutes(const Json& definition) {
    std::vector<Route> routes;
    for(const auto& r:value(definition,Object::definition,"routes")) {
        Route route{value(r,Object::route,"id").get<std::string>(),{}};
        for(const auto& id:value(r,Object::route,"segmentIds")) {
            if(!id.is_string())throw std::invalid_argument("Expected strings: segmentIds");
            route.segmentIds.push_back(id.get<std::string>());
        }
        routes.push_back(std::move(route));
    }
    return routes;
}
std::vector<VehicleInput> readVehicleInputs(const Json& definition) {
    std::vector<VehicleInput> inputs;
    for(const auto& i:value(definition,Object::definition,"inputs")) {
        const auto field=[&](const char* key)->const Json& {return value(i,Object::vehicleInput,key);};
        VehicleInput input{field("id").get<std::string>(),field("routeId").get<std::string>(),
            field("vehicleTypeId").get<std::string>(),field("vehiclesPerHour").get<double>(),
            field("startTime").get<double>(),field("endTime").get<double>()};
        // contains, rather than present: explicit null has always been an error here.
        if(i.contains("laneShares"))for(const auto& share:field("laneShares")) {
            if(!share.is_number())throw std::invalid_argument("Expected numbers: laneShares");
            input.laneShares.push_back(share.get<double>());
        }
        if(i.contains("compositionId"))input.compositionId=field("compositionId").get<std::string>();
        if(i.contains("routingDecisionId"))input.routingDecisionId=field("routingDecisionId").get<std::string>();
        if(i.contains("linkId"))input.linkId=field("linkId").get<std::string>();
        if(i.contains("volumeFromCounts"))input.volumeFromCounts=field("volumeFromCounts").get<bool>();
        if(i.contains("intervals")) {
            for(const auto& p:field("intervals")) {
                const auto period=[&](const char* key)->const Json& {return value(p,Object::volumeInterval,key);};
                input.intervals.push_back({period("startTime").get<double>(),period("endTime").get<double>(),
                    period("vehiclesPerHour").get<double>()});
            }
            deriveInputTotals(input); // Keep interval-derived scalars at the original read stage.
        }
        inputs.push_back(std::move(input));
    }
    return inputs;
}
}
