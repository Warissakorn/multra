#include "meso_input.hpp"
#include "link_type_codec.hpp"
#include "meso_settings.hpp"
#include "mode_codec.hpp"

namespace multra {
MesoCompile mesoNetwork(const ProjectDocument& d, const std::filesystem::path& data, InputManifest* manifest) {
    const auto def = d.definition.value_or(AuthoringDefinition{});
    const auto resolved = resolveLinkTypes(def, data, manifest);
    std::vector<LinkType> types; std::vector<std::string> paths;
    for (const auto& r : resolved) { types.push_back(r.type); paths.push_back(r.path); }
    auto out = compileMesoNetwork(d.network, types, paths, mesoSettingsOf(d));
    std::vector<std::string> modeIds;
    for (const auto& m : resolveModes(def, data, manifest)) modeIds.push_back(m.mode.id);
    const auto issues = linkTypeIssues(resolved, &modeIds);
    out.issues.insert(out.issues.end(), issues.begin(), issues.end());
    return out;
}
}
