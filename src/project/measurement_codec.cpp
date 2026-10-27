#include "network_fields.hpp"
#include <nlohmann/json.hpp>

namespace multra::networkCodec {
std::vector<AuthoredQueueCounter> readQueueCounters(const Json& network,int version) {
    std::vector<AuthoredQueueCounter> counters;
    if(version<16 || !present(network,"queueCounters"))return counters;
    for(const auto& c:value(network,Object::network,"queueCounters")) {
        const auto path="queueCounters["+std::to_string(counters.size())+"]";
        knownFields(c,Object::queueCounter,path,version);
        const auto field=[&](const char* key)->const Json& {return value(c,Object::queueCounter,key);};
        AuthoredQueueCounter counter{field("id").get<std::string>(),present(c,"name")?field("name").get<std::string>():"",{}};
        for(const auto& l:field("lines")) {
            const auto at=path+".lines["+std::to_string(counter.lines.size())+"]";
            knownFields(l,Object::measurementLine,at,version);
            MeasurementLine line;
            if(present(l,"referenceId"))line.referenceId=value(l,Object::measurementLine,"referenceId").get<std::string>();
            if(present(l,"point")) {
                const auto& point=value(l,Object::measurementLine,"point");
                knownFields(point,Object::controlPoint,at+".point",version);
                line.point=ControlPoint{readControlPath(value(point,Object::controlPoint,"path"),at+".point.path",version),
                    value(point,Object::controlPoint,"station").get<double>()};
            }
            counter.lines.push_back(std::move(line));
        }
        counters.push_back(std::move(counter));
    }
    return counters;
}
std::vector<TravelTimeSection> readTravelTimeSections(const Json& network,int version) {
    std::vector<TravelTimeSection> sections;
    if(version<23 || !present(network,"travelTimeSections"))return sections;
    for(const auto& s:value(network,Object::network,"travelTimeSections")) {
        const auto path="travelTimeSections["+std::to_string(sections.size())+"]";
        knownFields(s,Object::travelTimeSection,path,version);
        const auto field=[&](const char* key)->const Json& {return value(s,Object::travelTimeSection,key);};
        const auto line=[&](const char* key) {
            const auto& l=field(key);
            knownFields(l,Object::sectionLine,path+"."+key,version);
            return SectionLine{value(l,Object::sectionLine,"linkId").get<std::string>(),value(l,Object::sectionLine,"station").get<double>()};
        };
        sections.push_back({field("id").get<std::string>(),present(s,"name")?field("name").get<std::string>():"",line("start"),line("end"),{}});
        if(version>=24 && present(s,"controlType"))sections.back().controlType=sectionControlFromName(field("controlType").get<std::string>());
    }
    return sections;
}
}
