#pragma once
// S5.2a (STUDY_AREA §4-§5, D327): the document a Micro run over a study area compiles. Never saved,
// and no Undo step: compileDocument and runDiagnostics build it when `studyArea` names a Node.
#include "document.hpp"
#include <filesystem>
#include <memory>

namespace multra {
// S5.5a (STUDY_AREA §8-§9, D331): the side files beside the project, as the caller read them: the
// parsed JSON (null when the bytes are not JSON, which the cut refuses) and the bytes' SHA-256. A
// null pointer is a missing file: the cut runs Macro or Meso at compile, as in D329/D330.
struct StudyAreaSources {
    std::shared_ptr<const Json> macro, meso;
    std::string macroSha256, mesoSha256;
};
// What a run from a study area was cut from: `flowSource`, the side file's SHA-256 when one was used
// (empty when the source ran at compile, or for typed flows), the periods taken from it (Macro), the
// document's projectSha256, and whether MICRO_AREA_SOURCE_STALE was advised.
struct StudyAreaProvenance {
    std::string flowSource, sourceSha256, projectSha256;
    std::vector<std::string> periods;
    bool stale{};
    bool operator==(const StudyAreaProvenance&) const = default;
};
// The gate zones' names: `gate:<link>:in` at an entry gate's start, `gate:<link>:out` at an exit's end.
std::string entryGateName(const std::string& linkId);
std::string exitGateName(const std::string& linkId);
// S5.2b (D328): what gate flows may name, as the run reads them: the entry gates (cut order), then the
// zones with an origin connector on a kept Link; the exit gates, then the zones with a destination
// connector on one. Throws studyAreaCut's issues.
struct StudyAreaEnds { std::vector<std::string> origins, destinations; };
StudyAreaEnds studyAreaEnds(const ProjectDocument&);
// Throws ValidationError with studyAreaCut's issues, then STUDY_AREA_GATE_UNKNOWN at
// studyArea.gateFlows[i].from / .to for a name that is neither such a gate nor a zone kept with a
// connector on a kept Link. Otherwise the copy holds cutNetwork's network; the definition less its
// inputs, routes, routing decisions and route destinations, and less the speed decisions and areas
// on dropped Links; the kept zones with their kept connectors and lots; one zone per gate (a 1 m
// triangle at the Link's end, presentation only); and one OD matrix per (period, composition or
// mode) of the gate flows, in trips (veh/h x period / 3600). `studyArea` is gone from it. Every
// input, route, routing decision, route destination and OD matrix of the document not run is
// MICRO_AREA_INPUT_IGNORED at its path in `advisories`. S5.3 (D329): under `flowSource` "macro" the gate
// flows are macroGateFlows' (study_area_macro.hpp), read from `dataDirectory`, and the typed ones stay
// stored and unused; its advisories follow. S5.4 (D330): "meso" the same with mesoGateFlows. S5.5a
// (D331): `sources` lets the cut read a side file in place of the run at compile; `provenance`, when
// given, is filled.
ProjectDocument studyAreaDocument(const ProjectDocument&, std::vector<ValidationIssue>* advisories = nullptr,
                                  const std::filesystem::path& dataDirectory = {}, const StudyAreaSources* sources = nullptr,
                                  StudyAreaProvenance* provenance = nullptr);
}
