#include "junction_template_codec.hpp"
#include "input_manifest.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <set>

namespace multra {
namespace {
[[noreturn]] void invalid(const std::string& path) { throw ValidationError({{"JUNCTION_TEMPLATE_INVALID",path}}); }
void keys(const Json& j, const std::vector<std::string>& allowed, const std::string& path) {
    if (!j.is_object()) invalid(path);
    for (const auto& [key,value] : j.items()) {
        (void)value;
        if (std::find(allowed.begin(),allowed.end(),key) == allowed.end()) invalid(path+"."+key);
    }
}
std::string text(const Json& j, const char* key, const std::string& path) {
    if (!j.contains(key) || !j.at(key).is_string()) invalid(path+"."+key);
    return j.at(key).get<std::string>();
}
void duplicates(const std::vector<ResolvedJunctionTemplate>& entries) {
    std::set<std::string> ids;
    for (const auto& [t,path] : entries) if (!ids.insert(t.id).second) invalid(path+".id");
}
}
std::vector<JunctionLane> parseJunctionLanes(const Json& j, const std::string& path) {
    if (!j.is_array()) invalid(path);
    std::vector<JunctionLane> rows;
    for (std::size_t i = 0; i < j.size(); ++i) {
        const auto& row = j[i]; const auto at = path+"["+std::to_string(i)+"]";
        keys(row,{"movements","pocket","side"},at);
        JunctionLane lane{text(row,"movements",at)};
        if (row.contains("pocket")) {
            if (!row.at("pocket").is_boolean()) invalid(at+".pocket");
            lane.pocket = row.at("pocket").get<bool>();
        }
        if (row.contains("side")) {
            const auto side = text(row,"side",at);
            if (side == "far") lane.side = PocketSide::far;
            else if (side == "kerb") lane.side = PocketSide::kerb;
            else invalid(at+".side");
        }
        rows.push_back(std::move(lane));
    }
    if (auto issues = junctionLaneIssues(rows,path); !issues.empty()) throw ValidationError(std::move(issues));
    return rows;
}
Json junctionLanesJson(const std::vector<JunctionLane>& lanes) {
    Json j = Json::array();
    for (const auto& lane : lanes) {
        Json row{{"movements",lane.movements}};
        if (lane.pocket) row["pocket"] = true;
        if (lane.side) row["side"] = *lane.side == PocketSide::far ? "far" : "kerb";
        j.push_back(std::move(row));
    }
    return j;
}
JunctionTemplate parseJunctionTemplate(const Json& j, const std::string& path) {
    keys(j,{"id","name","kind","legs","control","phasing","roles"},path);
    JunctionTemplate t;
    t.id = text(j,"id",path); t.name = text(j,"name",path); t.kind = text(j,"kind",path);
    if (!j.contains("legs") || !j.at("legs").is_number_integer() || (j.at("legs") != 3 && j.at("legs") != 4)) invalid(path+".legs");
    t.legs = j.at("legs").get<int>(); t.control = text(j,"control",path);
    if (j.contains("phasing")) {
        t.phasing = text(j,"phasing",path);
        if (t.phasing.empty()) invalid(path+".phasing");
    }
    if (!j.contains("roles") || !j.at("roles").is_object()) invalid(path+".roles");
    for (const auto& [role,rows] : j.at("roles").items()) t.roles[role] = parseJunctionLanes(rows,path+".roles."+role);
    if (auto issues = junctionTemplateIssues(t,path); !issues.empty()) throw ValidationError(std::move(issues));
    return t;
}
Json junctionTemplateJson(const JunctionTemplate& t) {
    Json j{{"id",t.id},{"name",t.name},{"kind",t.kind},{"legs",t.legs},{"control",t.control},{"roles",Json::object()}};
    if (!t.phasing.empty()) j["phasing"] = t.phasing;
    for (const auto& [role,rows] : t.roles) j["roles"][role] = junctionLanesJson(rows);
    return j;
}
std::vector<ResolvedJunctionTemplate> resolveJunctionTemplates(const AuthoringDefinition& d,
    const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<ResolvedJunctionTemplate> result;
    if (!d.externalJunctionTemplates) {
        for (std::size_t i = 0; i < d.junctionTemplates.size(); ++i) {
            const auto path = "definition.junctionTemplates["+std::to_string(i)+"]";
            if (auto issues = junctionTemplateIssues(d.junctionTemplates[i],path); !issues.empty()) throw ValidationError(std::move(issues));
            result.push_back({d.junctionTemplates[i],path});
        }
    } else {
        std::vector<std::filesystem::path> files;
        const auto directory = data/"junction-templates";
        if (std::filesystem::is_directory(directory))
            for (const auto& entry : std::filesystem::directory_iterator(directory))
                if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
        std::sort(files.begin(),files.end());
        for (const auto& file : files) {
            const auto path = "catalog/junction-templates/"+file.filename().generic_string();
            try { result.push_back({parseJunctionTemplate(readInputJson(file,path,manifest),path),path}); }
            catch (const ValidationError&) { throw; }
            catch (const std::exception&) { invalid(path); }
        }
    }
    duplicates(result);
    return result;
}
bool usesJunctionFields(const ProjectDocument& d) {
    return (d.definition && !d.definition->externalJunctionTemplates) ||
        std::any_of(d.network.nodes.begin(),d.network.nodes.end(),[](const auto& n) { return n.junction || !n.approachDirections.empty(); });
}
void rejectJunctionFieldsBefore61(const Json& j) {
    const auto refuse = [](const std::string& path) { throw ValidationError({{"EDIT_UNSUPPORTED_FIELD",path}}); };
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("junctionTemplates"))
        refuse("definition.junctionTemplates");
    if (!j.contains("network") || !j.at("network").is_object() || !j.at("network").contains("nodes") || !j.at("network").at("nodes").is_array()) return;
    const auto& nodes = j.at("network").at("nodes");
    for (std::size_t i = 0; i < nodes.size(); ++i) if (nodes[i].is_object())
        for (const auto* key : {"junction","approachDirections"}) if (nodes[i].contains(key)) refuse("network.nodes["+std::to_string(i)+"]."+key);
}
std::vector<ValidationIssue> junctionDocumentIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < d.network.nodes.size(); ++i) {
        const auto& node = d.network.nodes[i]; const auto path = "network.nodes["+std::to_string(i)+"]";
        if (node.junction) {
            auto rows = junctionRecipeIssues(*node.junction,path+".junction");
            issues.insert(issues.end(),rows.begin(),rows.end());
        }
        for (const auto& [id,direction] : node.approachDirections)
            if (id.find_first_not_of(" \t\r\n") == std::string::npos ||
                std::none_of(d.network.links.begin(),d.network.links.end(),[&](const auto& l) { return l.id == id; }) ||
                (direction != ApproachDirection::north && direction != ApproachDirection::east && direction != ApproachDirection::south && direction != ApproachDirection::west))
                issues.push_back({"EDIT_NODE_INVALID",path+".approachDirections."+id});
    }
    if (d.definition && !d.definition->externalJunctionTemplates) {
        try { resolveJunctionTemplates(*d.definition,{}); }
        catch (const ValidationError& e) { issues.insert(issues.end(),e.issues.begin(),e.issues.end()); }
    }
    return issues;
}
}
