#include "network_fields.hpp"
#include "node_codec.hpp"
#include <nlohmann/json.hpp>

namespace multra::networkCodec {
std::vector<NetworkSignalHead> readSignalHeads(const Json& network,int version) {
    std::vector<NetworkSignalHead> result;
    for(const auto& h:value(network,Object::network,"signalHeads")) {
        const auto path="signalHeads["+std::to_string(result.size())+"]";
        knownFields(h,Object::signalHead,path,version);
        NetworkSignalHead head{value(h,Object::signalHead,"id").get<std::string>(),
            readReference(value(h,Object::signalHead,"lane"),version),
            value(h,Object::signalHead,"position").get<double>(),value(h,Object::signalHead,"programId").get<std::string>(),
            present(h,"connectorId")?value(h,Object::signalHead,"connectorId").get<std::string>():std::string{}};
        if(present(h,"name"))head.name=value(h,Object::signalHead,"name").get<std::string>();
        head.generated=generatedFlag(h,path,version);
        // Empty controller strings also activate this read; null and absence do not.
        if(readCondition(h,Object::signalHead,"groupNumber")) {
            head.controllerId=value(h,Object::signalHead,"controllerId").get<std::string>();
            head.groupNumber=static_cast<int>(value(h,Object::signalHead,"groupNumber").get<double>());
        }
        result.push_back(std::move(head));
    }
    return result;
}
}
