#pragma once
#include "node_analysis_report.hpp"
#include "../runner/batch.hpp"
namespace multra {
struct NodeAnalysisMicroKey {std::string id;std::vector<std::string> inLinks;};
struct NodeAnalysisMicroSource {ProjectDocument document;std::vector<NodeAnalysisMicroKey> keys;};
NodeAnalysisMicroSource nodeAnalysisMicroSource(const ProjectDocument&,const Network& compiled);
struct NodeAnalysisComparisonRow {
    std::string nodeId,inLink,nodeName,approachName;
    std::optional<double> analyticalDelay;std::optional<char> analyticalLos;
    Estimate microDelay,vehicles,unfinished;
};
struct NodeAnalysisComparison {
    std::uint64_t revision{};double start{},end{};
    ExportMarking marking;
    std::string projectSha256;
    std::vector<std::uint32_t> seeds;
    std::vector<NodeAnalysisComparisonRow> rows;
    std::vector<ValidationIssue> issues;
};
// Values copied from the two reports, matched only through captured Node/Link ids and positions.
// Missing source/window or a stale/mismatched snapshot yields named issues, never guessed rows.
NodeAnalysisComparison compareNodeAnalysis(const NodeAnalysisReport&,const NodeAnalysisMicroSource&,
    const BatchReport&,const MovementReport& evaluation);
std::string nodeAnalysisComparisonCsv(const NodeAnalysisComparison&,bool stale=false);
}
