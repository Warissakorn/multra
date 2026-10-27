#pragma once
#include "document.hpp"
#include "input_manifest.hpp"
#include "../model/demand/node_analysis_compile.hpp"
#include "../analysis/critical.hpp"
#include "../analysis/queue.hpp"
#include <array>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace multra {
// M13 A1 (NODE_ANALYSIS §3-§5, D371): HCM 7 Ch. 19 on every Node that carries `analysis`. Analytical,
// not simulated, not validated. Shared by the CLI and, later, the editor.
// The LOS pack: data/los/hcm7-signalised.json, HCM 7 Exhibit 19-8 (PDF p. 1422) — five increasing
// upper bounds of A-E in s/veh, F above or when a lane group's X > 1. EDIT_CATALOG_READ otherwise.
// data/los/hcm.json (6th edition, Micro) is a different pack and stays untouched (D371, 7a).
struct SignalisedLosPack { std::string id; std::array<double, 5> bounds{}; };
SignalisedLosPack loadSignalisedLosPack(const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
struct AnalysedLaneGroup {
    AnalysisLaneGroup group;
    // v in veh/h (hourly volume / PHF; on a coupled approach, from approachLaneFlows), its far- and
    // kerb-turn parts, and its flow-weighted %HV.
    double flowRate{}, farTurns{}, kerbTurns{}, heavyPercent{};
    std::optional<std::string> issue; // signalisedRangeIssue; then there is no result
    std::optional<SignalisedResult> result;
    std::optional<char> los;
    // Back of queue (Ch. 31 §4) with its storage ratios, when the approach states a speed limit.
    std::optional<BackOfQueue> queue;
    std::optional<double> storageRatio, storageRatio95;
};
struct AnalysedApproach { std::string inLink, name; double flow{}; std::optional<double> delay; std::optional<char> los; };
struct AnalysedNode {
    std::string id, name;
    double peakHourFactor{}, analysisPeriod{}, baseSaturationFlow{}; std::string baseSaturationSource;
    std::vector<std::string> defaults; // the Exhibit 19-11 defaults this Node used
    std::vector<std::string> turnNames; // the membership's turn names, which lane groups index
    std::vector<AnalysedLaneGroup> groups;
    std::vector<AnalysedApproach> approaches;
    double flow{}; std::optional<double> delay; std::optional<char> los;
    // Critical v/c (Eqs. 19-30, 19-31 over a non-overlapping chain, §11); none, with NODE_ANALYSIS_PARTIAL,
    // unless every turn of the Node is in a lane group with a result. Its chain indexes `groups`.
    std::optional<CriticalResult> critical;
    // The compile's, then NODE_ANALYSIS_NO_VOLUME per turn without a row (a coupled approach then has no
    // lane group) and NODE_ANALYSIS_NO_CONVERGENCE per approach whose lane flows did not converge.
    std::vector<ValidationIssue> advisories;
};
struct NodeAnalysisRun {
    std::uint64_t revision{};
    std::string losPack;
    std::vector<AnalysedNode> nodes;
    Json inputManifest; // the files read; null without a manifest
};
NodeAnalysisRun runNodeAnalysis(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
Json nodeAnalysisJson(const NodeAnalysisRun&);
std::string nodeAnalysisCsv(const NodeAnalysisRun&);
}
