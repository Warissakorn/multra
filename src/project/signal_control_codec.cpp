#include "network_fields.hpp"
#include "node_codec.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <algorithm>

namespace multra {
std::vector<SignalController> parseSignalControllers(const Json& definition) {
    using namespace networkCodec;
    std::vector<SignalController> result;
    // The standalone reader has always accepted generated flags; parseDocument applies
    // its pre-54 guard before reading the definition. Controllers/groups are always strict.
    const auto& rows=value(definition,Object::definition,"signalControllers");
    for(const auto& x:rows) {
        const auto path="signalControllers["+std::to_string(result.size())+"]";
        knownFields(x,Object::signalController,path,generatedVersion);
        const auto cfield=[&](const char* key)->const Json& {return value(x,Object::signalController,key);};
        SignalController c{cfield("id").get<std::string>(),present(x,"name")?cfield("name").get<std::string>():"",
            cfield("cycle").get<double>(),cfield("offset").get<double>(),{}};
        for(const auto& g:cfield("groups")) {
            knownFields(g,Object::signalGroup,path+".groups",generatedVersion);
            const auto gfield=[&](const char* key)->const Json& {return value(g,Object::signalGroup,key);};
            c.groups.push_back({static_cast<int>(gfield("number").get<double>()),
                present(g,"name")?gfield("name").get<std::string>():"",gfield("greenStart").get<double>(),
                gfield("greenEnd").get<double>(),gfield("amber").get<double>()});
        }
        c.generated=generatedFlag(x,path,generatedVersion);result.push_back(std::move(c));
    }
    return result;
}
namespace networkCodec {
std::vector<SignalProgram> readSignalPrograms(const Json& definition) {
    const auto& rows=value(definition,Object::definition,"signalPrograms");
    std::vector<SignalProgram> result;
    for(const auto& p:rows) {
        SignalProgram program{value(p,Object::signalProgram,"id").get<std::string>(),
            value(p,Object::signalProgram,"offset").get<double>(),{}};
        for(const auto& phase:value(p,Object::signalProgram,"phases")) {
            const double duration=value(phase,Object::signalPhase,"duration").get<double>();
            const auto name=value(phase,Object::signalPhase,"color").get<std::string>();
            const auto it=std::find_if(std::begin(signalColors),std::end(signalColors),
                [&](const auto& entry){return name==entry.first;});
            if(it==std::end(signalColors))throw std::invalid_argument("INVALID_SIGNAL_COLOR");
            program.phases.push_back({duration,it->second});
        }
        result.push_back(std::move(program));
    }
    return result;
}
}
}
