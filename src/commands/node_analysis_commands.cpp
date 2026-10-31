#include "node_analysis_commands.hpp"
#include <algorithm>
#include <stdexcept>

namespace multra {
namespace {
bool changedDemand(const NodeAnalysis& a,const NodeAnalysis& b) {
    return a.turns!=b.turns||a.peakHourFactor!=b.peakHourFactor||a.analysisPeriod!=b.analysisPeriod;
}
}
void setNodeAnalysis(ProjectDocument& d, const std::string& id, std::optional<NodeAnalysis> analysis) {
    const auto node = std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const auto& n) { return n.id == id; });
    if (node == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    if (analysis && *analysis == NodeAnalysis{}) analysis.reset();
    if(analysis&&node->analysis&&analysis->modelSource==node->analysis->modelSource&&changedDemand(*analysis,*node->analysis))analysis->modelSource.reset();
    node->analysis = std::move(analysis);
    node->generated = false;
}
void settleNodeAnalysisSources(ProjectDocument& d,const ProjectDocument& before) {
    for(auto& n:d.network.nodes)if(n.analysis&&n.analysis->modelSource) {
        const auto old=std::find_if(before.network.nodes.begin(),before.network.nodes.end(),[&](const auto& x){return x.id==n.id;});
        if(old!=before.network.nodes.end()&&old->analysis&&n.analysis->modelSource==old->analysis->modelSource&&changedDemand(*n.analysis,*old->analysis))n.analysis->modelSource.reset();
    }
}
}
