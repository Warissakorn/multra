#pragma once
// Descriptive BA38 summaries: each seed contributes ONE mean per lane (not one per cycle).
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <map>
#include <tuple>
namespace multra::reactiondischarge {
struct Moments {
    std::size_t n{}; double sum{}, squares{};
    void add(double v) { ++n; sum += v; squares += v * v; }
    Json json() const {
        return {{"n", n}, {"mean", n ? Json(sum / n) : Json(nullptr)},
            {"sampleSd", n > 1 ? Json(std::sqrt(std::max(0., (squares - sum * sum / n) / (n - 1)))) : Json(nullptr)}};
    }
};
class Summary {
    struct Lane {
        Moments headway, startup; std::size_t cycles{}, validHeadways{}, validStartups{}, seeds{};
        std::map<std::string, std::size_t> headReasons, startupReasons;
    };
    using Key = std::tuple<std::string, std::string, std::string>;
    std::map<Key, Lane> lanes_;
    struct Run { Moments completed, active, pending, clamps; };
    std::map<std::string, Run> runs_;
    Json seeds_ = Json::array();
public:
    void add(const Json& run) {
        const auto variant = run.at("variant").get<std::string>();
        auto& whole = runs_[variant];
        whole.completed.add(run.at("completed")); whole.active.add(run.at("active"));
        whole.pending.add(run.at("pending")); whole.clamps.add(run.at("safetyClamps"));
        struct Local { Moments headway, startup; };
        std::map<Key, Local> local;
        for (const auto& cycle : run.at("discharge").at("cycles")) {
            const Key key{variant, cycle.at("headId"), cycle.at("laneId")};
            auto& lane = lanes_[key]; auto& sample = local[key]; ++lane.cycles;
            if (!cycle.at("meanHeadwaySeconds").is_null()) {
                sample.headway.add(cycle.at("meanHeadwaySeconds")); ++lane.validHeadways;
            } else ++lane.headReasons[cycle.at("unavailableReason")];
            if (!cycle.at("startupLostTimeSeconds").is_null()) {
                sample.startup.add(cycle.at("startupLostTimeSeconds")); ++lane.validStartups;
            } else ++lane.startupReasons[cycle.at("startupUnavailableReason")];
        }
        for (const auto& [key, sample] : local) {
            auto& lane = lanes_[key]; ++lane.seeds;
            const auto headway = sample.headway.json(), startup = sample.startup.json();
            if (sample.headway.n) lane.headway.add(headway["mean"]);
            if (sample.startup.n) lane.startup.add(startup["mean"]);
            seeds_.push_back({{"variant", variant}, {"seed", run.at("seed")},
                {"headId", std::get<1>(key)}, {"laneId", std::get<2>(key)},
                {"headway", headway}, {"startupLostTime", startup}});
        }
    }
    Json json() const {
        Json lanes = Json::array(), runs = Json::array();
        for (const auto& [key, lane] : lanes_)
            lanes.push_back({{"variant", std::get<0>(key)}, {"headId", std::get<1>(key)}, {"laneId", std::get<2>(key)},
                {"observedSeeds", lane.seeds}, {"cycles", lane.cycles},
                {"validHeadwayCycles", lane.validHeadways}, {"validStartupCycles", lane.validStartups},
                {"headway", lane.headway.json()}, {"startupLostTime", lane.startup.json()},
                {"headwayUnavailableReasons", lane.headReasons}, {"startupUnavailableReasons", lane.startupReasons}});
        for (const auto& [variant, run] : runs_)
            runs.push_back({{"variant", variant}, {"completed", run.completed.json()}, {"active", run.active.json()},
                {"pending", run.pending.json()}, {"safetyClamps", run.clamps.json()}});
        return {{"label", "not yet validated; descriptive BA38 sensitivity"},
            {"aggregation", "mean of available per-seed cycle means, independently per head/lane; null means unavailable"},
            {"runs", runs}, {"lanes", lanes}, {"seedMeans", seeds_}};
    }
};
}
