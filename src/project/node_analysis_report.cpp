#include "node_analysis_report.hpp"
#include "build_metadata.hpp"
#include <algorithm>
namespace multra {
namespace {
ExportMarking marking(const NodeAnalysisReport& r,bool stale) {
    auto m=r.marking;
    if(stale&&std::find(m.stale.begin(),m.stale.end(),"project")==m.stale.end())m.stale.push_back("project");
    return m;
}
}
NodeAnalysisReport analyseNodeAnalysisDocument(ProjectDocument source,const std::filesystem::path& data,InputManifest& inputs) {
    auto run=runNodeAnalysis(source,data,&inputs);auto marks=exportMarking(source,{});
    return {std::move(source),std::move(run),std::move(marks)};
}
NodeAnalysisReport analyseNodeAnalysisInput(std::string_view bytes,const std::filesystem::path& data) {
    InputManifest inputs;inputs.record("project",bytes);
    return analyseNodeAnalysisDocument(parseDocument(Json::parse(bytes.begin(),bytes.end())),data,inputs);
}
Json nodeAnalysisReportJson(const NodeAnalysisReport& r,bool stale) {
    auto j=nodeAnalysisJson(r.run);addMarkingJson(j,marking(r,stale));addBuildMetadata(j);return j;
}
std::string nodeAnalysisReportCsv(const NodeAnalysisReport& r,bool stale) {return markedCsv(nodeAnalysisCsv(r.run),marking(r,stale));}
}
