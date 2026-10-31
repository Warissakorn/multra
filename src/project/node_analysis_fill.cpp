#include "node_analysis_fill.hpp"
#include "node_analysis_source_codec.hpp"
#include <algorithm>
namespace multra {
NodeAnalysis analysisFromModel(const NodeAnalysis& old,const NodeModelDemand& demand,const NodeModelDemandRequest& request) {
    NodeAnalysis a=old;a.turns=demand.turns;a.analysisPeriod=(request.end-request.start)/3600;a.peakHourFactor=1;
    NodeAnalysisModelSource source;source.revision=demand.revision;source.start=request.start;source.end=request.end;source.heavyVehicleTypes=request.heavyVehicleTypes;
    std::sort(source.heavyVehicleTypes.begin(),source.heavyVehicleTypes.end());
    for(const auto& f:demand.inputManifest.at("files")) {
        const auto path=f.at("logicalPath").get<std::string>(),sha=f.at("sha256").get<std::string>();
        if(path=="project")source.projectSha256=sha;else source.files.push_back({path,sha});
    }
    source.fallbacks=demand.inputManifest.at("fallbacks").get<std::vector<std::string>>();
    (void)parseNodeAnalysisSource(nodeAnalysisSourceJson(source),"analysis.modelSource",64);
    a.modelSource=std::move(source);return a;
}
}
