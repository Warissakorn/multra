#pragma once
#include "meso_input.hpp"
#include "../meso/loading.hpp"
#include <functional>
#include <nlohmann/json.hpp>

namespace multra {
// M11.4 (docs/reference/MESO_NETWORK.md §7, D300): one Meso run over the whole Micro run, [0,
// definition.duration], with the demand Micro loads (owner, 2026-10-22). Shared by the CLI and the
// editor. runMeso reports graph, demand and loading as it enters each, so on a refusal the last one
// reported is the refusing stage; the caller owns inputs and outputs.
enum class MesoStage { inputs, graph, demand, loading, outputs };
struct MesoObserver {
    std::function<void(MesoStage)> stage;
    std::function<bool(std::size_t step, std::size_t steps)> progress; // false cancels (MesoLoading::cancelled)
};
struct MesoRun {
    std::uint64_t revision{};
    MesoSettings settings;
    double duration{};
    MesoGraph graph;
    std::vector<MesoClass> classes;
    MesoLoading loading;
    std::vector<ValidationIssue> advisories; // the compile's, then MESO_PATH_NOT_LOADED per refused path
    Json inputManifest;                      // the files read; null without a manifest
    std::string odPathRule;                  // Road OD only (D341); empty preserves legacy outputs
};
// Refused by ValidationError: the graph's issues (stage graph), then the demand's (stage demand).
// S5.4 (D330): `watchLinks` names the Links whose Meso links keep per-path entrance counts.
MesoRun runMeso(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr,
                const MesoObserver* = nullptr, const std::vector<std::string>* watchLinks = nullptr);
}
