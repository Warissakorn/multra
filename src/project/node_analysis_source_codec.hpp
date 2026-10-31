#pragma once
#include "document.hpp"
namespace multra {
Json nodeAnalysisSourceJson(const NodeAnalysisModelSource&);
NodeAnalysisModelSource parseNodeAnalysisSource(const Json&,const std::string& path,int schema);
bool usesNodeAnalysisSource(const ProjectDocument&);
void validateNodeAnalysisSources(const ProjectDocument&);
}
