#include "junction_recipe_codec.hpp"
#include "junction_template_codec.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace multra {
namespace {
[[noreturn]] void invalid(const std::string& path) { throw ValidationError({{"EDIT_NODE_INVALID",path}}); }
void keys(const Json& j, const std::vector<std::string>& allowed, const std::string& path) {
    if (!j.is_object()) invalid(path);
    for (const auto& [key,value] : j.items()) {
        (void)value;
        if (std::find(allowed.begin(),allowed.end(),key) == allowed.end()) invalid(path+"."+key);
    }
}
std::vector<std::optional<double>> lengths(const Json& j, const std::string& path) {
    if (!j.is_array()) invalid(path);
    std::vector<std::optional<double>> out;
    for (std::size_t i = 0; i < j.size(); ++i) {
        if (j[i].is_null()) out.push_back({});
        else {
            if (!j[i].is_number()) invalid(path+"["+std::to_string(i)+"]");
            out.push_back(j[i].get<double>());
        }
    }
    return out;
}
void optionalNumber(const Json& j, const char* key, std::optional<double>& to, const std::string& path) {
    if (!j.contains(key)) return;
    if (!j.at(key).is_number()) invalid(path+"."+key);
    to = j.at(key).get<double>();
}
Json lengthsJson(const std::vector<std::optional<double>>& values) {
    Json j = Json::array();
    for (const auto value : values) j.push_back(value ? Json(*value) : Json(nullptr));
    return j;
}
}
NodeJunction parseJunctionRecipe(const Json& j, const std::string& path) {
    keys(j,{"templateId","major","legs","signal"},path);
    NodeJunction out;
    if (!j.contains("templateId") || !j.at("templateId").is_string()) invalid(path+".templateId");
    out.templateId = j.at("templateId").get<std::string>();
    if (j.contains("major")) {
        const auto& major = j.at("major");
        if (!major.is_array()) invalid(path+".major");
        for (std::size_t i = 0; i < major.size(); ++i) {
            if (!major[i].is_string()) invalid(path+".major["+std::to_string(i)+"]");
            out.major.push_back(major[i].get<std::string>());
        }
    }
    if (j.contains("legs")) {
        if (!j.at("legs").is_object()) invalid(path+".legs");
        for (const auto& [id,leg] : j.at("legs").items()) {
            const auto at = path+".legs."+id;
            keys(leg,{"storage","taper","lanes"},at);
            JunctionLeg row;
            if (leg.contains("storage")) row.storage = lengths(leg.at("storage"),at+".storage");
            if (leg.contains("taper")) row.taper = lengths(leg.at("taper"),at+".taper");
            if (leg.contains("lanes")) {
                try { row.lanes = parseJunctionLanes(leg.at("lanes"),at+".lanes"); }
                catch (const ValidationError& e) {
                    auto issues = e.issues;
                    for (auto& issue : issues) issue.code = "EDIT_NODE_INVALID";
                    throw ValidationError(std::move(issues));
                }
            }
            out.legs[id] = std::move(row);
        }
    }
    if (j.contains("signal")) {
        const auto& s = j.at("signal"); const auto at = path+".signal";
        keys(s,{"cycle","phases"},at);
        JunctionSignal signal;
        optionalNumber(s,"cycle",signal.cycle,at);
        if (s.contains("phases")) {
            if (!s.at("phases").is_array()) invalid(at+".phases");
            const auto& phases = s.at("phases");
            for (std::size_t i = 0; i < phases.size(); ++i) {
                const auto& p = phases[i]; const auto phase = at+".phases["+std::to_string(i)+"]";
                keys(p,{"number","green","yellow","allRed"},phase);
                if (!p.contains("number") || !p.at("number").is_number_integer() || p.at("number") < 1 || p.at("number") > 8) invalid(phase+".number");
                JunctionPhase row; row.number = p.at("number").get<int>();
                optionalNumber(p,"green",row.green,phase); optionalNumber(p,"yellow",row.yellow,phase); optionalNumber(p,"allRed",row.allRed,phase);
                signal.phases.push_back(std::move(row));
            }
        }
        out.signal = std::move(signal);
    }
    if (auto issues = junctionRecipeIssues(out,path); !issues.empty()) throw ValidationError(std::move(issues));
    return out;
}
Json junctionRecipeJson(const NodeJunction& r) {
    Json j{{"templateId",r.templateId}};
    if (!r.major.empty()) j["major"] = r.major;
    if (!r.legs.empty()) {
        j["legs"] = Json::object();
        for (const auto& [id,leg] : r.legs) {
            Json row = Json::object();
            if (!leg.storage.empty()) row["storage"] = lengthsJson(leg.storage);
            if (!leg.taper.empty()) row["taper"] = lengthsJson(leg.taper);
            if (leg.lanes) row["lanes"] = junctionLanesJson(*leg.lanes);
            j["legs"][id] = std::move(row);
        }
    }
    if (r.signal) {
        Json signal = Json::object();
        if (r.signal->cycle) signal["cycle"] = *r.signal->cycle;
        if (!r.signal->phases.empty()) {
            signal["phases"] = Json::array();
            for (const auto& p : r.signal->phases) {
                Json row{{"number",p.number}};
                if (p.green) row["green"] = *p.green;
                if (p.yellow) row["yellow"] = *p.yellow;
                if (p.allRed) row["allRed"] = *p.allRed;
                signal["phases"].push_back(std::move(row));
            }
        }
        j["signal"] = std::move(signal);
    }
    return j;
}
ApproachDirections parseApproachDirections(const Json& j, const std::string& path) {
    if (!j.is_object()) invalid(path);
    ApproachDirections out;
    for (const auto& [id,direction] : j.items()) {
        const auto at = path+"."+id;
        if (id.find_first_not_of(" \t\r\n") == std::string::npos || !direction.is_string()) invalid(at);
        const auto s = direction.get<std::string>();
        if (s == "N") out[id] = ApproachDirection::north;
        else if (s == "E") out[id] = ApproachDirection::east;
        else if (s == "S") out[id] = ApproachDirection::south;
        else if (s == "W") out[id] = ApproachDirection::west;
        else invalid(at);
    }
    return out;
}
Json approachDirectionsJson(const ApproachDirections& directions) {
    Json j = Json::object();
    for (const auto& [id,d] : directions)
        j[id] = d == ApproachDirection::north ? "N" : d == ApproachDirection::east ? "E" : d == ApproachDirection::south ? "S" : "W";
    return j;
}
}
