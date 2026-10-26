#pragma once
// S5.5a (STUDY_AREA §8-§9, D331): the Macro and Meso side files, `<project>.macro.json` and
// `<project>.meso.json`, beside the project file. A side file is data only: it is parsed and checked,
// never run. It holds what a study area's cut needs (owner, D331): Macro path flows per period, Meso
// watched path counts, each with the run's settings, its input manifest and `projectSha256`, the
// SHA-256 of the project bytes as saved (an unsaved edit counts), so staleness survives a reopen.
#include "document.hpp"
#include "study_area_run.hpp"
#include "../macro/assign.hpp"
#include "../meso/loading.hpp"
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>

namespace multra {
struct MacroRun; struct MesoRun;
// The bytes saving writes for `d` (documentJson, indented by 2, then a newline) and their SHA-256.
std::string documentBytes(const ProjectDocument&);
std::string projectSha256(const ProjectDocument&);
// `<project>` less its extension, then `.macro.json` or `.meso.json` (`kind`: "macro" or "meso").
std::filesystem::path sideFilePath(const std::filesystem::path& project, const std::string& kind);

// The graph a side file was cut on: each arc's kind, source and from-station, compared with the graph
// compiled again from the document before any stored index is used.
struct SideArc { std::string kind, source; double fromStation{}; bool operator==(const SideArc&) const = default; };
std::vector<SideArc> sideArcs(const MacroGraph&);
std::vector<SideArc> sideArcs(const MesoGraph&);

struct MacroSidePeriod {
    std::string projectSha256;
    MacroSettings settings;
    double gap{}; int iterations{}; bool converged{};
    std::string definitionId, definitionName; // D342: the named Macro run, blank for project settings
    std::vector<MacroClass> classes;         // id, mode and pcu; no cells
    std::vector<SideArc> arcs;
    bool hasPaths{};                         // kept only when the study area cuts Macro (owner, D331)
    std::vector<std::vector<MacroPathFlow>> paths;
    Json inputManifest;
};
struct MacroSide { std::map<std::string, MacroSidePeriod> periods; };
struct MesoSide {
    std::string projectSha256;
    MesoSettings settings;
    double duration{};
    std::vector<MesoClass> classes;          // id, pcu and each path's source and links
    std::vector<SideArc> links;
    std::vector<std::string> watchedLinks;   // the Links the run watched (the kept set when the area cut Meso)
    std::vector<MesoLoading::Watched> watched;
    Json inputManifest;
};
// The side file with `run`'s period replacing that period's entry in `previous` (null or an earlier
// file; an invalid one is replaced whole).
Json macroSideJson(const MacroRun&, const std::string& projectSha256, const Json& previous);
Json mesoSideJson(const MesoRun&, const std::string& projectSha256, const std::vector<std::string>& watchedLinks);
// Refused by ValidationError: MICRO_AREA_SOURCE_INVALID at `where` plus the JSON path of the first
// bad value (a wrong format or schema, a missing or mistyped key, a number out of range, an index past
// its own array).
MacroSide parseMacroSide(const Json&, const std::string& where = "macroSide");
MesoSide parseMesoSide(const Json&, const std::string& where = "mesoSide");
// S5.5b (D332): the side files beside `project` as the CLI reads them, the same way the editor does:
// a missing file is none, bytes that are not JSON are a JSON null (refused by the cut).
StudyAreaSources readStudyAreaSources(const std::filesystem::path& project);
}
