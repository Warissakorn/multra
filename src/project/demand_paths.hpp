#pragma once
#include "document.hpp"
#include "../model/network/routeless.hpp"
#include <map>
#include <optional>
#include <tuple>

namespace multra {
// M2.1.1 glue between the authored demand and the model's routeless walk.
// The decisions that are placed on a Link, with each destination resolved to the object chain a
// route would name. Network-dependent problems (unknown Link, a destination no chain reaches or
// two chains reach equally) go to `issues` when given; that decision entry is left out.
// With `at`, each destination's weight is its flow at that time (M2.1.2); without, relativeFlow.
std::vector<PlacedDecision> placedDecisions(const Network&, const AuthoringDefinition&,
                                            std::vector<ValidationIssue>* issues = nullptr,
                                            std::optional<double> at = std::nullopt,
                                            const std::string& vehicleTypeId = {});
struct RoutelessIssues { std::vector<ValidationIssue> blocking, advisory; };
// D551 (DYNAMIC_ASSIGNMENT §15): each routeless route's family names (`<decision>><final object>` or `<decision>>route-k`).
using RouteFamilies = std::map<std::string, std::vector<std::string>>;
// The routeless demand of one document (M2.1.1, D140, D684): it checks the inputs with a linkId (or naming a placed
// decision) and expands them to runtime routes, sharing every walk between the two. It belongs to one call over one
// unchanged Network and definition, which must outlive it, and is never kept between calls (D28).
class RoutelessDemand {
public:
    RoutelessDemand(const Network&, const AuthoringDefinition&);
    RoutelessDemand(const Network&, AuthoringDefinition&&) = delete; // a temporary definition would dangle
    // Blocking problems (Run refuses; an edit still goes through) and advisories of every routeless input.
    RoutelessIssues issues();
    // Every routeless input becomes one input per complete path, each on an already-expanded runtime route
    // `link:<id>/path-k` that buildScenario passes through. An input whose walk has a problem is left out, as `issues`
    // names it. `families`, when given, receives each routeless route's family names; nothing else changes.
    ScenarioDefinition expand(ScenarioDefinition resolved, RouteFamilies* families = nullptr);
    // runtimeSections of the Network, built on first use; `builtSections` is null until then, so a caller that must
    // check the Network first never builds it early (compileScenario).
    const RuntimeSections& sections();
    const RuntimeSections* builtSections() const { return sections_ ? &*sections_ : nullptr; }
private:
    using ChainMemo = std::map<std::pair<std::string, std::string>, std::vector<std::vector<std::string>>>;
    // The decision flow breakpoints and one representative time per slice between them (M2.1.2).
    const std::vector<double>& points() const { return points_; }
    const std::vector<std::optional<double>>& times() const { return times_; }
    // Every slice's placed decisions for one vehicle type ({} when routing is not by type).
    const std::vector<std::vector<PlacedDecision>>& placed(const std::string& vehicleTypeId);
    // The walk from linkId through slice `slice` of placed(vehicleTypeId).
    const RoutelessResult& walk(const std::string& linkId, const std::string& vehicleTypeId, std::size_t slice);
    const Network& network;
    const AuthoringDefinition& definition;
    const ConnectorPathTable table; // lazy itself: free for a project that never walks
    ChainMemo memo; // a destination's reachable chains depend on the Network and two Links only (D140)
    std::vector<double> points_;
    std::vector<std::optional<double>> times_;
    std::optional<RuntimeSections> sections_;
    std::map<std::string, std::vector<std::vector<PlacedDecision>>> placed_;
    std::map<std::tuple<std::string, std::string, std::size_t>, RoutelessResult> walks_;
};
// M2.1.2: an input's periods (inputPeriods) cut at every breakpoint inside them, each piece at
// its period's volume. Pieces of zero length are not produced.
std::vector<VolumeInterval> cutPeriods(const VehicleInput&, const std::vector<double>& breakpoints);
// The effective entry-lane policy used by the input dialog, diagnostics and demand preview.
struct InputLanePolicy { std::size_t lanes{}; bool acceptsShares{true}; };
InputLanePolicy inputLanePolicy(const Network&,const AuthoringDefinition&,const VehicleInput&);
// Advisories for stored weights that the compiler cannot apply.
std::vector<ValidationIssue> demandAdvisories(const Network&,const AuthoringDefinition&);
// The runtime route id for path k of n from a Link.
std::string routelessRouteId(const std::string& linkId, std::size_t k, std::size_t n);
}
