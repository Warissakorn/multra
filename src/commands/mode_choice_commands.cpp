#include "mode_choice_commands.hpp"
#include "catalog_commands.hpp"
#include <algorithm>

namespace multra {
void setModeChoiceSets(ProjectDocument& d, std::vector<ModeChoiceSet> sets) { d.modeChoiceSets = std::move(sets); }
void setPeriodFactors(ProjectDocument& d, std::vector<PeriodFactor> factors) { d.periodFactors = std::move(factors); }
void writeGeneratedOdMatrices(ProjectDocument& d, std::vector<OdMatrix> matrices, std::vector<Mode> installedModes) {
    auto& list = d.zones.odMatrices;
    std::erase_if(list, [](const auto& m) { return m.generatedBy == "mode-choice"; });
    for (auto& m : matrices) list.push_back(std::move(m));
    ownModesWhenNamed(d, std::move(installedModes));
}
}
