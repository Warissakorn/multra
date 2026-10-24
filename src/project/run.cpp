#include "run.hpp"
#include "speed_decisions.hpp"
#include "speed_areas.hpp"
#include "input_manifest.hpp"
#include "diagnostics.hpp"
#include "behaviour_library.hpp"
#include "demand_paths.hpp"
#include "od_demand.hpp"
#include "parking_compile.hpp"
#include "study_area_run.hpp"
#include "../core/validate.hpp"
#include <algorithm>
namespace multra {
namespace {
// M8.4b: a lot parked full at t = 0 is demand of its own, with or without an input.
bool holdsVehicles(const ProjectDocument& d) {
    return std::any_of(d.zones.parkingLots.begin(), d.zones.parkingLots.end(), [](const auto& l) { return l.initialOccupancy > 0; });
}
}
std::vector<Diagnostic> runDiagnostics(const ProjectDocument& d, const std::filesystem::path& data, const StudyAreaSources* sources) {
    // S5.2a (STUDY_AREA §4, D327): a study area runs its run-time copy; its refusals come first.
    if (!d.zones.studyArea.nodes.empty()) {
        std::vector<ValidationIssue> ignored;
        std::vector<Diagnostic> rows;
        try {
            const auto copy = studyAreaDocument(d, &ignored, data, sources);
            rows = runDiagnostics(copy, data);
        } catch (const ValidationError& e) {
            for (const auto& issue : e.issues) rows.push_back({issue.code, issue.path, {}, {}, DiagnosticSeverity::runtime});
            return rows;
        } catch (const std::exception& e) { // a catalog the Macro cut reads (S5.3)
            return {{e.what(), "catalogs", {}, {}, DiagnosticSeverity::runtime}};
        }
        for (const auto& issue : ignored) rows.push_back({issue.code, issue.path, {}, {}, DiagnosticSeverity::advisory});
        return rows;
    }
    // A new project carries run settings (M5.3) but no demand yet: it is still a drawing, and the
    // drawing's own problems are the useful list, exactly as with no definition at all.
    if (!d.definition || (d.zones.empty() && onlyRunSettings(*d.definition))) return documentDiagnostics(d);
    try {
        // M8.3 (D155): the OD matrices are demand like any input; their own Run checks come first.
        std::vector<Diagnostic> od;
        for(const auto& issue:odDemandIssues(d,data))od.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::runtime});
        if (std::any_of(od.begin(), od.end(), [](const auto& row) { return row.code == "OD_ROUTE_SEARCH_LIMIT"; }))
            return od; // expansion is deliberately refused; retain the structured cell diagnostic
        for(const auto& gap:odDemandGaps(d))od.push_back({"OD_SKIPPED_DEMAND",gap.path,{},{},DiagnosticSeverity::advisory});
        const auto expanded=withOdDemand(d,matrixModes(d,data));const auto& def=expanded;
        const auto routeless=routelessIssues(d.network,def);
        if (std::any_of(routeless.blocking.begin(), routeless.blocking.end(),
                        [](const auto& issue) { return issue.code == "ROUTE_SEARCH_LIMIT"; })) {
            for (const auto& issue : routeless.blocking) od.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::runtime});
            for (const auto& issue : routeless.advisory) od.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::advisory});
            return od;
        }
        auto rows=runtimeDiagnostics(d.network,expandRouteless(d.network,def,resolveCatalogs(def,data)));
        rows.insert(rows.begin(),od.begin(),od.end());
        for(const auto& issue:demandAdvisories(d.network,def))
            rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::advisory});
        // M2.1.1: a routeless walk that failed blocks Run; a lane a decision cannot serve advises.
        for(const auto& issue:routeless.blocking)rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::runtime});
        for(const auto& issue:routeless.advisory)rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::advisory});
        // A route is not a network object, and neither is an input: the row names its path.
        for(const auto& issue:compositionIssues(def,data))
            rows.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::runtime});
        if (def.inputs.empty() && !holdsVehicles(d))
            rows.push_back({"EDIT_NO_INPUTS","inputs",{},{},DiagnosticSeverity::runtime});
        return rows;
    } catch (const std::exception& e) {
        return {{e.what(),"catalogs",{},{},DiagnosticSeverity::runtime}};
    }
}
RunSnapshot compileDocument(const ProjectDocument& d, const std::filesystem::path& data,InputManifest* manifest,const StudyAreaSources* sources) {
    validateDocument(d);
    if (!d.zones.studyArea.nodes.empty()) { // S5.2a (D327), S5.5a (D331)
        StudyAreaProvenance provenance;
        auto snapshot = compileDocument(studyAreaDocument(d, nullptr, data, sources, &provenance), data, manifest);
        snapshot.studyArea = std::move(provenance);
        return snapshot;
    }
    if (!d.definition) throw std::invalid_argument("EDIT_NO_DEFINITION");
    // M8.3 (D155): OD matrices become ordinary routes and inputs here, after their Run checks; a
    // document without a zone layer uses its definition as it stands.
    if(auto issues=odDemandIssues(d,data);!issues.empty())throw ValidationError(std::move(issues));
    const auto expanded=d.zones.empty()?std::optional<AuthoringDefinition>{}:std::optional{withOdDemand(d,matrixModes(d,data,manifest))};
    const auto& authored=expanded?*expanded:*d.definition;
    if (authored.inputs.empty() && !holdsVehicles(d)) throw std::invalid_argument("EDIT_NO_INPUTS");
    // An input left out for an unknown or broken composition must stop Run, not vanish from it.
    if(auto issues=compositionIssues(authored,data,manifest);!issues.empty())throw ValidationError(std::move(issues));
    // Resolve once: every part of this snapshot uses the same catalog values.
    // The check and the expansion walk the same routeless paths; walk them once.
    RoutelessWalks walks(d.network,authored);
    if(auto issues=routelessIssues(walks).blocking;!issues.empty())throw ValidationError(std::move(issues));
    auto definition=expandRouteless(walks,resolveCatalogs(authored,data,manifest));
    // M3.3.2b (D127): road assignments become per-segment selections; the library is authoring
    // data, sliced away by resolveCatalogs, so it is read from the document itself.
    definition.segmentBehaviours=compileBehaviourAssignments(d.network,*d.definition);
    if(manifest)manifest->validate();
    CompiledRouteOrigins routeOrigins;
    auto scenario=compileScenario(d.network,definition,walks.builtSections(),&routeOrigins);
    scenario.speedDecisions=compileSpeedDecisions(d.network,authored); // B-6 (D223)
    scenario.reducedSpeedAreas=compileSpeedAreas(d.network,authored); // B-6 §7 (D225)
    // M8.4b (D157): the authored lots become the engine's, on the routes just compiled.
    if(!d.zones.parkingLots.empty()) {
        compileParkingLots(d,authored,scenario,routeOrigins,data);
        if(auto issues=validateScenario(scenario);!issues.empty())throw ValidationError(std::move(issues));
    }
    // M5.9 (D146): the run goes on for the cool-down after the authored duration. Inputs still end
    // by that duration, so the engine releases no new demand in it; extended only here, after every
    // expansion that reads the authored duration, and for every caller (CLI, batch, editor).
    if(d.definition->evaluation)scenario.duration+=d.definition->evaluation->cooldown;
    return {d.revision,d.network,std::move(scenario),std::move(routeOrigins)};
}
}
