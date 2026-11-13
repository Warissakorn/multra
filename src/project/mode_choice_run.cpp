#include "mode_choice_run.hpp"
#include "input_manifest.hpp"
#include "macro_input.hpp"
#include "mode_codec.hpp"
#include "zone_skim.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace multra {
namespace {
void add(DistributionMatrix& into, const DistributionMatrix& m, double scale = 1) {
    if (into.empty()) into.assign(m.size(), std::vector<double>(m.size(), 0.0));
    for (std::size_t i = 0; i < m.size(); ++i)
        for (std::size_t j = 0; j < m.size(); ++j) into[i][j] += m[i][j] * scale;
}
}
ModeChoiceRun runModeChoice(const ProjectDocument& d, const std::filesystem::path& data, const std::string& rateSetId,
                            const std::string& deterrenceSetId, const std::string& skimMode, const std::string& costMatrixId,
                            const std::string& setId, const std::vector<std::string>& periodIds,
                            InputManifest* manifest, const std::function<void(ModeChoiceStage)>& stage,
                            const std::vector<double>* linkTimes) {
    const auto enter = [&](ModeChoiceStage s) { if (stage) stage(s); };
    ModeChoiceRun run;
    enter(ModeChoiceStage::distribution);
    // D548: balancing refusals are held until the set is known, because a logsum purpose balances again on its
    // composite; without one they are thrown here, in their usual order.
    std::vector<ValidationIssue> skimRefusals;
    run.distribution = runTripDistribution(d, data, rateSetId, deterrenceSetId, skimMode, costMatrixId, manifest, {}, {}, linkTimes, nullptr, &skimRefusals);
    const auto& zoneIds = run.distribution.generation.zoneIds;

    if (skimRefusals.empty()) enter(ModeChoiceStage::set); // else entered once the set is known to balance again
    // E2 (D479): the project's own sets come first; one sharing the chosen id with a catalog set is refused
    // at the project's row, never a silent override (as D474's deterrence sets).
    try {
        std::vector<ResolvedModeChoiceSet> sets;
        for (std::size_t k = 0; k < d.modeChoiceSets.size(); ++k) sets.push_back({d.modeChoiceSets[k], "modeChoiceSets[" + std::to_string(k) + "]"});
        const auto owned = sets.size();
        for (auto& s : resolveModeChoiceSets(data, manifest)) sets.push_back(std::move(s));
        const auto found = std::find_if(sets.begin(), sets.end(), [&](const auto& s) { return s.set.id == setId; });
        if (found == sets.end()) throw ValidationError({{"MODE_CHOICE_SET_UNKNOWN", "modeChoiceSets"}});
        if (static_cast<std::size_t>(found - sets.begin()) < owned &&
            std::any_of(sets.begin() + static_cast<std::ptrdiff_t>(owned), sets.end(), [&](const auto& s) { return s.set.id == setId; }))
            throw ValidationError({{"DUPLICATE_ID", found->path + ".id"}});
        run.set = found->set; run.setPath = found->path;
    } catch (const ValidationError&) { // a refusal of the distribution comes first, as before D548
        if (!skimRefusals.empty()) throw ValidationError(std::move(skimRefusals));
        throw;
    }
    if (!skimRefusals.empty()) {
        if (std::none_of(run.set.purposes.begin(), run.set.purposes.end(), [](const auto& p) { return p.distributionCost.has_value(); }))
            throw ValidationError(std::move(skimRefusals));
        enter(ModeChoiceStage::set);
    }
    std::vector<std::size_t> purposeAt;
    for (const auto& p : run.distribution.purposes) {
        const auto& ps = run.set.purposes;
        const auto match = std::find_if(ps.begin(), ps.end(), [&](const auto& q) { return q.id == p.id; });
        if (match == ps.end()) throw ValidationError({{"MODE_CHOICE_PURPOSE_MISSING", run.setPath + ".purposes"}});
        purposeAt.push_back(static_cast<std::size_t>(match - ps.begin()));
    }

    enter(ModeChoiceStage::costs);
    // The graph was already compiled and its files recorded by the distribution run.
    const auto network = macroNetwork(d, data, nullptr);
    std::vector<std::string> known;
    for (const auto& m : resolveModes(d.definition.value_or(AuthoringDefinition{}), data, nullptr)) known.push_back(m.mode.id);
    std::map<std::string, DistributionMatrix> skims;
    std::vector<ValidationIssue> unknown;
    for (std::size_t p = 0; p < purposeAt.size(); ++p) {
        const auto& alternatives = run.set.purposes[purposeAt[p]].alternatives;
        for (std::size_t k = 0; k < alternatives.size(); ++k) {
            const auto& mode = alternatives[k].modeId;
            if (!mode) continue;
            if (std::find(known.begin(), known.end(), *mode) == known.end())
                unknown.push_back({"UNKNOWN_MODE", run.setPath + ".purposes[" + std::to_string(purposeAt[p]) + "].alternatives[" + std::to_string(k) + "].modeId"});
            else if (!skims.count(*mode)) skims[*mode] = zoneSkim(d, network.graph, zoneIds, *mode, linkTimes);
        }
    }
    if (!unknown.empty()) throw ValidationError(std::move(unknown));

    enter(ModeChoiceStage::split);
    const auto zoneIndex = [&](std::size_t z) {
        const auto& zones = d.zones.zones;
        return std::to_string(std::find_if(zones.begin(), zones.end(), [&](const auto& zone) { return zone.id == zoneIds[z]; }) - zones.begin());
    };
    const auto n = zoneIds.size();
    std::vector<ValidationIssue> issues;
    std::vector<std::vector<ModeAlternativeInput>> inputsOf;
    std::vector<ModeNests> nestsOf;
    for (std::size_t p = 0; p < purposeAt.size(); ++p) {
        ModeChoicePurposeResult purpose{run.distribution.purposes[p].id, {}};
        std::vector<ModeAlternativeInput> inputs;
        for (const auto& a : run.set.purposes[purposeAt[p]].alternatives) {
            auto costs = a.modeId ? skims.at(*a.modeId) : DistributionMatrix(n, std::vector<double>(n, std::numeric_limits<double>::infinity()));
            if (a.costMatrix) overlayCostMatrix(d, zoneIds, *a.costMatrix, costs);
            inputs.push_back({a.constant, a.timeCoefficient, costs});
            purpose.alternatives.push_back({a, std::move(costs), {}});
        }
        // N1 (D481): a nested purpose maps its members to alternative indices; no nests keeps the MNL path.
        const auto& setPurpose = run.set.purposes[purposeAt[p]];
        ModeNests nests;
        if (!setPurpose.nests.empty()) {
            nests.nestOf.assign(setPurpose.alternatives.size(), -1);
            for (std::size_t n = 0; n < setPurpose.nests.size(); ++n) {
                nests.phi.push_back(setPurpose.nests[n].phi);
                for (const auto& member : setPurpose.nests[n].alternatives)
                    for (std::size_t k = 0; k < setPurpose.alternatives.size(); ++k)
                        if (setPurpose.alternatives[k].id == member) nests.nestOf[k] = static_cast<int>(n);
            }
        }
        inputsOf.push_back(std::move(inputs)); nestsOf.push_back(std::move(nests));
        run.purposes.push_back(std::move(purpose));
    }
    // D548 (MODE_CHOICE_LOGSUM §4): a logsum purpose's distribution runs again on its composite cost (Eq. 6.10),
    // after Eq. 6.11 (beta <= lambda) is checked; the other purposes keep the skim.
    std::map<std::string, DistributionMatrix> composites;
    for (std::size_t p = 0; p < purposeAt.size(); ++p) {
        const auto& setPurpose = run.set.purposes[purposeAt[p]];
        if (!setPurpose.distributionCost) continue;
        const double lambda = -setPurpose.alternatives.front().timeCoefficient;
        if (run.distribution.purposes[p].beta > lambda)
            issues.push_back({"MODE_CHOICE_STRUCTURE_INVALID", run.setPath + ".purposes[" + std::to_string(purposeAt[p]) + "]"});
        else composites[setPurpose.id] = compositeCost(inputsOf[p], nestsOf[p], lambda);
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    if (!composites.empty()) { // the files were recorded by the first distribution; keep its manifest
        auto inputs = std::move(run.distribution.generation.inputManifest);
        run.distribution = runTripDistribution(d, data, rateSetId, deterrenceSetId, skimMode, costMatrixId, nullptr, {}, {}, linkTimes, &composites);
        run.distribution.generation.inputManifest = std::move(inputs);
    }
    for (std::size_t p = 0; p < purposeAt.size(); ++p) {
        auto& purpose = run.purposes[p];
        auto split = splitModes(run.distribution.purposes[p].result.trips, inputsOf[p], nestsOf[p]);
        std::set<std::size_t> origins;
        for (const auto& i : split.issues)
            if (origins.insert(i.origin).second) issues.push_back({"MODE_CHOICE_NO_ALTERNATIVE", "zones[" + zoneIndex(i.origin) + "]." + purpose.id});
        if (split.issues.empty())
            for (std::size_t k = 0; k < purpose.alternatives.size(); ++k) purpose.alternatives[k].persons = std::move(split.persons[k]);
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));

    enter(ModeChoiceStage::periods);
    const auto& periods = d.zones.studyPeriods;
    std::vector<std::string> chosen = periodIds;
    for (const auto& id : chosen)
        if (std::none_of(periods.begin(), periods.end(), [&](const auto& s) { return s.id == id; })) throw ValidationError({{"UNKNOWN_PERIOD", "studyPeriods"}});
    if (chosen.empty())
        for (const auto& s : periods)
            if (std::any_of(d.periodFactors.begin(), d.periodFactors.end(), [&](const auto& f) { return f.periodId == s.id; })) chosen.push_back(s.id);
    if (chosen.empty()) throw ValidationError({{"MODE_CHOICE_NO_PERIOD", "periodFactors"}});
    for (const auto& periodId : chosen) {
        ModeChoicePeriod period{periodId, {}, {}};
        for (const auto& purpose : run.purposes) {
            const auto f = std::find_if(d.periodFactors.begin(), d.periodFactors.end(),
                                        [&](const auto& x) { return x.periodId == periodId && x.purposeId == purpose.id; });
            if (f == d.periodFactors.end()) { run.advisories.push_back({"MODE_PERIOD_FACTOR_MISSING", "periodFactors"}); continue; }
            for (const auto& a : purpose.alternatives) {
                const auto od = directionalOd(a.persons, f->productionToAttraction, f->attractionToProduction);
                auto at = std::find_if(period.alternatives.begin(), period.alternatives.end(), [&](const auto& x) { return x.id == a.alternative.id; });
                if (at == period.alternatives.end()) at = period.alternatives.insert(period.alternatives.end(), {a.alternative.id, a.alternative.modeId, {}, {}});
                add(at->persons, od);
                if (!a.alternative.occupancy || !a.alternative.modeId) continue;
                const double perVehicle = 1 / *a.alternative.occupancy;
                if (!at->vehicles) at->vehicles = DistributionMatrix{};
                add(*at->vehicles, od, perVehicle);
                auto mode = std::find_if(period.modes.begin(), period.modes.end(), [&](const auto& m) { return m.modeId == *a.alternative.modeId; });
                if (mode == period.modes.end()) mode = period.modes.insert(period.modes.end(), {*a.alternative.modeId, {}});
                add(mode->vehicles, od, perVehicle);
            }
        }
        run.periods.push_back(std::move(period));
    }
    return run;
}
std::vector<OdMatrix> generatedOdMatrices(const ModeChoiceRun& run) {
    std::vector<OdMatrix> out;
    for (const auto& period : run.periods)
        for (const auto& mode : period.modes) {
            OdMatrix m;
            m.id = "mc-" + period.id + "-" + mode.modeId;
            m.periodId = period.id; m.modeId = mode.modeId; m.generatedBy = "mode-choice";
            m.zoneIds = run.distribution.generation.zoneIds;
            m.cells = mode.vehicles;
            out.push_back(std::move(m));
        }
    return out;
}
}
