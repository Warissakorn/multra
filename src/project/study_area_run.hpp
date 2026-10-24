#pragma once
// S5.2a (STUDY_AREA §4-§5, D327): the document a Micro run over a study area compiles. Never saved,
// and no Undo step: compileDocument and runDiagnostics build it when `studyArea` names a Node.
#include "document.hpp"

namespace multra {
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
// MICRO_AREA_INPUT_IGNORED at its path in `advisories`.
ProjectDocument studyAreaDocument(const ProjectDocument&, std::vector<ValidationIssue>* advisories = nullptr);
}
