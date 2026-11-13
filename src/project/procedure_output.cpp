#include "procedure_output.hpp"
#include "macro_output.hpp"
#include "mode_choice_output.hpp"
#include "trip_distribution_output.hpp"
#include "trip_generation_output.hpp"
#include <nlohmann/json.hpp>

namespace multra {
namespace {
constexpr const char* kMeasure = "Procedure sequence: the procedure's active steps run in order, each the existing unit "
    "(trip generation, distribution, mode choice, Macro assignment) on its nearest earlier input; an assignment uses the "
    "nearest earlier mode choice's vehicle matrices in memory, the document unchanged; a refused step stops the later ones";
}
Json procedureJson(const ProcedureRun& r) {
    Json j{{"validated", false}, {"measure", kMeasure}, {"procedure", {{"id", r.procedure.id}, {"name", r.procedure.name}}},
           {"revision", r.revision}, {"completed", r.completed()}, {"cancelled", r.cancelled}, {"steps", Json::array()}};
    for (const auto& s : r.steps) {
        Json issues = Json::array();
        for (const auto& i : s.issues) issues.push_back({{"code", i.code}, {"path", i.path}});
        Json result = s.generation ? tripGenerationJson(*s.generation) : s.distribution ? tripDistributionJson(*s.distribution)
                    : s.modeChoice ? modeChoiceJson(*s.modeChoice) : s.macro ? macroJson(*s.macro) : Json(nullptr);
        j["steps"].push_back({{"index", s.index}, {"kind", procedureStepKindName(s.kind)}, {"status", procedureStepStatusName(s.status)},
                              {"issues", issues}, {"result", result}});
    }
    if (r.feedback) { // PROCEDURE_FEEDBACK §4 (D547): absent without feedback, so older outputs keep their bytes
        const auto& f = *r.feedback;
        const auto number = [](const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); };
        Json iterations = Json::array();
        for (const auto& it : f.iterations)
            iterations.push_back({{"n", it.n}, {"weight", number(it.weight)}, {"tmf", number(it.tmf)}, {"rse", number(it.rse)},
                                  {"assignmentIterations", it.assignmentIterations}, {"relativeGap", it.relativeGap}});
        j["feedback"] = {{"method", feedbackMethodName(f.settings.method)}, {"weight", number(f.settings.weight)},
                         {"maxIterations", f.settings.maxIterations}, {"tolerance", f.settings.tolerance},
                         {"converged", f.converged}, {"iterations", iterations}};
    }
    return j;
}
}
