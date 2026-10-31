#pragma once
#include "node_model_demand.hpp"
namespace multra {
// Stage a whole-set replacement: retain site adjustments/saturation, use the explicit window
// for T and PHF=1, and retain the exact captured source/hash/HV choice. No mutation or I/O.
NodeAnalysis analysisFromModel(const NodeAnalysis&,const NodeModelDemand&,const NodeModelDemandRequest&);
}
