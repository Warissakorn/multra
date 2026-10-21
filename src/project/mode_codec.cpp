#include "mode_codec.hpp"
#include "input_manifest.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <stdexcept>

namespace multra {
bool usesModes(const ProjectDocument& d) {
    if (std::any_of(d.zones.odMatrices.begin(), d.zones.odMatrices.end(), [](const auto& m) { return !m.modeId.empty(); })) return true;
    if (!d.definition) return false;
    if (!d.definition->externalModes) return true;
    return !d.definition->externalVehicleTypes &&
           std::any_of(d.definition->vehicleTypes.begin(), d.definition->vehicleTypes.end(), [](const auto& t) { return t.pcu.has_value(); });
}
void rejectModesBefore49(const Json& j) {
    const auto refuse = [](const std::string& path) { throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path}}); };
    if (j.contains("odMatrices") && j.at("odMatrices").is_array())
        for (std::size_t i = 0; i < j.at("odMatrices").size(); ++i)
            if (j.at("odMatrices")[i].is_object() && j.at("odMatrices")[i].contains("modeId")) refuse("odMatrices[" + std::to_string(i) + "].modeId");
    if (!j.contains("definition") || !j.at("definition").is_object()) return;
    const auto& definition = j.at("definition");
    if (definition.contains("modes")) refuse("definition.modes");
    if (!definition.contains("vehicleTypes") || !definition.at("vehicleTypes").is_array()) return;
    const auto& types = definition.at("vehicleTypes");
    for (std::size_t i = 0; i < types.size(); ++i)
        if (types[i].is_object() && types[i].contains("pcu")) refuse("vehicleTypes[" + std::to_string(i) + "].pcu");
}
Mode parseMode(const Json& j) {
    if (!j.is_object() || j.size() != 3) throw std::invalid_argument("EDIT_CATALOG_READ");
    for (const auto* key : {"id", "name", "compositionId"})
        if (!j.contains(key) || !j.at(key).is_string()) throw std::invalid_argument("EDIT_CATALOG_READ");
    return {j.at("id").get<std::string>(), j.at("name").get<std::string>(), j.at("compositionId").get<std::string>()};
}
Json modeJson(const Mode& m) { return {{"id", m.id}, {"name", m.name}, {"compositionId", m.compositionId}}; }
std::optional<Pcu> parsePcu(const Json& t) {
    if (!t.contains("pcu")) return std::nullopt;
    const auto& p = t.at("pcu");
    if (!p.is_object() || p.size() != 2 || !p.contains("value") || !p.contains("source") || !p.at("value").is_number() ||
        !p.at("source").is_string())
        throw ValidationError({{"INVALID_PCU", "vehicleType.pcu"}});
    return Pcu{p.at("value").get<double>(), p.at("source").get<std::string>()};
}
void addPcuJson(const VehicleType& t, Json& item) {
    if (t.pcu) item["pcu"] = {{"value", t.pcu->value}, {"source", t.pcu->source}};
}
std::vector<ResolvedMode> resolveModes(const AuthoringDefinition& d, const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<ResolvedMode> result;
    if (!d.externalModes) {
        for (std::size_t i = 0; i < d.modes.size(); ++i) result.push_back({d.modes[i], "definition.modes[" + std::to_string(i) + "].compositionId"});
        return result;
    }
    const auto directory = data / "modes";
    if (!std::filesystem::is_directory(directory)) return result;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    try {
        for (const auto& file : files) {
            const auto logical = "catalog/modes/" + file.filename().generic_string();
            result.push_back({parseMode(readInputJson(file, logical, manifest)), logical});
        }
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    return result;
}
std::vector<Mode> modesOf(const std::vector<ResolvedMode>& resolved) {
    std::vector<Mode> modes;
    for (const auto& r : resolved) modes.push_back(r.mode);
    return modes;
}
std::optional<double> modePcu(const Mode& mode, const std::vector<Composition>& compositions, const std::vector<VehicleType>& types) {
    const auto c = std::find_if(compositions.begin(), compositions.end(), [&](const auto& x) { return x.id == mode.compositionId; });
    if (c == compositions.end() || !c->intervals.empty() || c->types.empty()) return std::nullopt;
    double weighted = 0, total = 0;
    for (const auto& share : c->types) {
        const auto t = std::find_if(types.begin(), types.end(), [&](const auto& x) { return x.id == share.vehicleTypeId; });
        if (t == types.end() || !t->pcu) return std::nullopt;
        weighted += share.share * t->pcu->value; total += share.share;
    }
    return total > 0 ? std::optional(weighted / total) : std::nullopt;
}
}
