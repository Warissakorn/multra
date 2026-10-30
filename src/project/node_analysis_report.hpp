#pragma once
#include "node_analysis_run.hpp"
#include "export_marking.hpp"
#include <string_view>
namespace multra {
// J7/D418: immutable input/result for a single analysis. Record and parse identical captured
// project bytes, including a UI Save snapshot. No project mutation or persisted result.
struct NodeAnalysisReport { ProjectDocument source; NodeAnalysisRun run; ExportMarking marking; };
NodeAnalysisReport analyseNodeAnalysisInput(std::string_view,const std::filesystem::path& data);
NodeAnalysisReport analyseNodeAnalysisDocument(ProjectDocument,const std::filesystem::path& data,InputManifest&);
Json nodeAnalysisReportJson(const NodeAnalysisReport&,bool stale=false);
std::string nodeAnalysisReportCsv(const NodeAnalysisReport&,bool stale=false);
}
