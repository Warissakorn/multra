#include "random_streams_codec.hpp"
#include "document.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace multra {
bool ownsRandomStreams(const ProjectDocument& d) {
    return d.definition && d.definition->randomStreams == RandomStreams::perInput;
}
void rejectRandomStreamsBefore74(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("randomStreams"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.randomStreams"}});
}
RandomStreams parseRandomStreams(const Json& definition) {
    if (!definition.contains("randomStreams")) return RandomStreams::shared;
    const auto& j = definition.at("randomStreams");
    if (j == "perInput") return RandomStreams::perInput;
    if (j == "shared") return RandomStreams::shared; // accepted, never written
    throw ValidationError({{"INVALID_RANDOM_STREAMS", "definition.randomStreams"}});
}
void addRandomStreamsJson(const AuthoringDefinition& d, Json& definition) {
    if (d.randomStreams == RandomStreams::perInput) definition["randomStreams"] = randomStreamsName(d.randomStreams);
}
const char* randomStreamsName(RandomStreams streams) {
    return streams == RandomStreams::perInput ? "perInput" : "shared";
}
}
