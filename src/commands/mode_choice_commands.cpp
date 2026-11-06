#include "mode_choice_commands.hpp"
#include "catalog_commands.hpp"
#include <algorithm>

namespace multra {
void setModeChoiceSets(ProjectDocument& d, std::vector<ModeChoiceSet> sets) { d.modeChoiceSets = std::move(sets); }
void setPeriodFactors(ProjectDocument& d, std::vector<PeriodFactor> factors) { d.periodFactors = std::move(factors); }
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
void writeGeneratedOdMatrices(ProjectDocument& d, std::vector<OdMatrix> matrices, std::vector<Mode> installedModes) {
    auto& list = d.zones.odMatrices;
    std::erase_if(list, [](const auto& m) { return m.generatedBy == "mode-choice"; });
    for (auto& m : matrices) list.push_back(std::move(m));
    ownModesWhenNamed(d, std::move(installedModes));
}
}
