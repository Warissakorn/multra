#include "procedure_codec.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <set>
#include <cmath>
#include <cstdint>

namespace multra {
namespace {
using Issues = std::vector<ValidationIssue>;
bool blank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }
bool onlyKeys(const Json& j, std::initializer_list<const char*> allowed) {
    for (const auto& [key, value] : j.items())
        if (std::none_of(allowed.begin(), allowed.end(), [&](const char* a) { return key == a; })) return false;
    return true;
}
void bad(Issues& issues, const std::string& path) { issues.push_back({"INVALID_PROCEDURE", path}); }
std::optional<std::string> id(const Json& j, const char* key, const std::string& at, Issues& issues, bool required) {
    if (!j.contains(key)) { if (required) bad(issues, at + "." + key); return std::nullopt; }
    if (!j.at(key).is_string() || blank(j.at(key).get<std::string>())) { bad(issues, at + "." + key); return std::nullopt; }
    return j.at(key).get<std::string>();
}
const std::vector<std::pair<ProcedureStepKind, const char*>> kKinds{
    {ProcedureStepKind::tripGeneration, "tripGeneration"}, {ProcedureStepKind::tripDistribution, "tripDistribution"},
    {ProcedureStepKind::modeChoice, "modeChoice"}, {ProcedureStepKind::assignment, "assignment"}};
ProcedureStep step(const Json& j, const std::string& at, Issues& issues) {
    ProcedureStep s;
    if (!j.is_object() || !j.contains("kind") || !j.at("kind").is_string()) { bad(issues, at); return s; }
    const auto kind = std::find_if(kKinds.begin(), kKinds.end(), [&](const auto& k) { return j.at("kind").get<std::string>() == k.second; });
    if (kind == kKinds.end()) { bad(issues, at + ".kind"); return s; }
    s.kind = kind->first;
    if (j.contains("active")) {
        if (!j.at("active").is_boolean()) bad(issues, at + ".active");
        else s.active = j.at("active").get<bool>();
    }
    switch (s.kind) {
    case ProcedureStepKind::tripGeneration:
        if (!onlyKeys(j, {"kind", "active", "rateSet"})) bad(issues, at);
        s.set = id(j, "rateSet", at, issues, true).value_or("");
        break;
    case ProcedureStepKind::tripDistribution:
        if (!onlyKeys(j, {"kind", "active", "deterrenceSet", "skimMode", "costMatrix"})) bad(issues, at);
        s.set = id(j, "deterrenceSet", at, issues, true).value_or("");
        s.skimMode = id(j, "skimMode", at, issues, false);
        s.costMatrix = id(j, "costMatrix", at, issues, false);
        break;
    case ProcedureStepKind::modeChoice:
        if (!onlyKeys(j, {"kind", "active", "modeChoiceSet", "periods"})) bad(issues, at);
        s.set = id(j, "modeChoiceSet", at, issues, true).value_or("");
        if (j.contains("periods")) {
            const auto& list = j.at("periods");
            if (!list.is_array() || list.empty()) bad(issues, at + ".periods");
            else for (std::size_t k = 0; k < list.size(); ++k) {
                if (!list[k].is_string() || blank(list[k].get<std::string>())) bad(issues, at + ".periods[" + std::to_string(k) + "]");
                else s.periods.push_back(list[k].get<std::string>());
            }
        }
        break;
    case ProcedureStepKind::assignment:
        if (!onlyKeys(j, {"kind", "active", "period", "macroRun"})) bad(issues, at);
        s.period = id(j, "period", at, issues, false);
        s.macroRun = id(j, "macroRun", at, issues, false);
        if (j.contains("period") == j.contains("macroRun")) bad(issues, at);
        break;
    }
    return s;
}
ProcedureFeedback feedback(const Json& j, const std::string& at, Issues& issues) {
    ProcedureFeedback f;
    if (!j.is_object()) { bad(issues, at); return f; }
    if (!onlyKeys(j, {"method", "weight", "maxIterations", "tolerance"})) bad(issues, at);
    const auto method = j.contains("method") && j.at("method").is_string() ? j.at("method").get<std::string>() : "";
    if (method == "msa") f.method = FeedbackMethod::msa;
    else if (method != "constantWeight") bad(issues, at + ".method");
    if (j.contains("weight")) {
        if (!j.at("weight").is_number()) bad(issues, at + ".weight");
        else f.weight = j.at("weight").get<double>();
    }
    if (!j.contains("maxIterations") || !j.at("maxIterations").is_number_integer()) bad(issues, at + ".maxIterations");
    else f.maxIterations = static_cast<int>(std::clamp<std::int64_t>(j.at("maxIterations").get<std::int64_t>(), -1, 1001));
    if (!j.contains("tolerance") || !j.at("tolerance").is_number()) bad(issues, at + ".tolerance");
    else f.tolerance = j.at("tolerance").get<double>();
    return f;
}
std::vector<ValidationIssue> feedbackIssues(const ProcedureFeedback& f, const std::string& at) {
    Issues issues;
    if (f.method == FeedbackMethod::constantWeight && (!f.weight || !std::isfinite(*f.weight) || *f.weight < 0 || *f.weight >= 1))
        bad(issues, at + ".weight");
    if (f.method == FeedbackMethod::msa && f.weight) bad(issues, at + ".weight");
    if (f.maxIterations < 2 || f.maxIterations > 1000) bad(issues, at + ".maxIterations");
    if (!std::isfinite(f.tolerance) || f.tolerance < 0) bad(issues, at + ".tolerance");
    return issues;
}
}
const char* feedbackMethodName(FeedbackMethod m) { return m == FeedbackMethod::msa ? "msa" : "constantWeight"; }
const char* procedureStepKindName(ProcedureStepKind k) {
    return std::find_if(kKinds.begin(), kKinds.end(), [&](const auto& x) { return x.first == k; })->second;
}
std::vector<Procedure> parseProcedures(const Json& file) {
    if (!file.contains("procedures")) return {};
    const auto& list = file.at("procedures");
    if (!list.is_array() || list.empty()) throw ValidationError({{"INVALID_PROCEDURE", "procedures"}});
    Issues issues;
    std::vector<Procedure> out;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto at = "procedures[" + std::to_string(i) + "]";
        const auto& j = list[i];
        Procedure p;
        if (!j.is_object() || !onlyKeys(j, {"id", "name", "steps", "feedback"})) { bad(issues, at); continue; }
        if (!j.contains("id") || !j.at("id").is_string()) issues.push_back({"INVALID_ID", at + ".id"});
        else p.id = j.at("id").get<std::string>();
        if (!j.contains("name") || !j.at("name").is_string()) bad(issues, at + ".name");
        else p.name = j.at("name").get<std::string>();
        if (!j.contains("steps") || !j.at("steps").is_array() || j.at("steps").empty()) bad(issues, at + ".steps");
        else for (std::size_t k = 0; k < j.at("steps").size(); ++k) p.steps.push_back(step(j.at("steps")[k], at + ".steps[" + std::to_string(k) + "]", issues));
        if (j.contains("feedback")) p.feedback = feedback(j.at("feedback"), at + ".feedback", issues); // D547
        out.push_back(std::move(p));
    }
    if (issues.empty()) issues = procedureIssues(out);
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return out;
}
void addProceduresJson(const std::vector<Procedure>& procedures, Json& file) {
    if (procedures.empty()) return;
    Json list = Json::array();
    for (const auto& p : procedures) {
        Json steps = Json::array();
        for (const auto& s : p.steps) {
            Json j{{"kind", procedureStepKindName(s.kind)}};
            if (!s.active) j["active"] = false;
            switch (s.kind) {
            case ProcedureStepKind::tripGeneration: j["rateSet"] = s.set; break;
            case ProcedureStepKind::tripDistribution:
                j["deterrenceSet"] = s.set;
                if (s.skimMode) j["skimMode"] = *s.skimMode;
                if (s.costMatrix) j["costMatrix"] = *s.costMatrix;
                break;
            case ProcedureStepKind::modeChoice:
                j["modeChoiceSet"] = s.set;
                if (!s.periods.empty()) j["periods"] = s.periods;
                break;
            case ProcedureStepKind::assignment:
                if (s.period) j["period"] = *s.period;
                if (s.macroRun) j["macroRun"] = *s.macroRun;
                break;
            }
            steps.push_back(std::move(j));
        }
        Json entry{{"id", p.id}, {"name", p.name}, {"steps", steps}};
        if (p.feedback) {
            Json f{{"method", feedbackMethodName(p.feedback->method)}};
            if (p.feedback->weight) f["weight"] = *p.feedback->weight;
            f["maxIterations"] = p.feedback->maxIterations;
            f["tolerance"] = p.feedback->tolerance;
            entry["feedback"] = std::move(f);
        }
        list.push_back(std::move(entry));
    }
    file["procedures"] = list;
}
std::vector<ValidationIssue> procedureIssues(const std::vector<Procedure>& procedures) {
    Issues issues;
    std::set<std::string> ids;
    for (std::size_t i = 0; i < procedures.size(); ++i) {
        const auto at = "procedures[" + std::to_string(i) + "]";
        if (blank(procedures[i].id)) issues.push_back({"INVALID_ID", at + ".id"});
        else if (!ids.insert(procedures[i].id).second) issues.push_back({"DUPLICATE_ID", at + ".id"});
        if (procedures[i].steps.empty()) bad(issues, at + ".steps");
        if (procedures[i].feedback)
            for (auto& issue : feedbackIssues(*procedures[i].feedback, at + ".feedback")) issues.push_back(std::move(issue));
        for (std::size_t k = 0; k < procedures[i].steps.size(); ++k) {
            const auto& s = procedures[i].steps[k];
            const auto where = at + ".steps[" + std::to_string(k) + "]";
            if (s.kind != ProcedureStepKind::assignment && blank(s.set)) bad(issues, where);
            if (s.kind == ProcedureStepKind::assignment && s.period.has_value() == s.macroRun.has_value()) bad(issues, where);
            // The reader's blank-value rules, so a step built outside JSON (the dialog) is refused before it is saved.
            for (const auto& [value, key] : {std::pair{&s.period, ".period"}, std::pair{&s.macroRun, ".macroRun"},
                                             std::pair{&s.skimMode, ".skimMode"}, std::pair{&s.costMatrix, ".costMatrix"}})
                if (value->has_value() && blank(**value)) bad(issues, where + key);
            for (std::size_t m = 0; m < s.periods.size(); ++m)
                if (blank(s.periods[m])) bad(issues, where + ".periods[" + std::to_string(m) + "]");
        }
    }
    return issues;
}
}
