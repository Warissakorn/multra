#include "meso_run.hpp"
#include "input_manifest.hpp"
#include "meso_settings.hpp"
#include "../core/validate.hpp"
#include <algorithm>

namespace multra {
MesoRun runMeso(const ProjectDocument& d, const std::filesystem::path& data, InputManifest* manifest, const MesoObserver* observer,
                const std::vector<std::string>* watchLinks) {
    const auto enter = [&](MesoStage s) { if (observer && observer->stage) observer->stage(s); };
    MesoRun run;
    run.revision = d.revision; run.settings = mesoSettingsOf(d);
    run.duration = d.definition ? d.definition->duration : 0.0;
    enter(MesoStage::graph);
    auto network = mesoNetwork(d, data, manifest);
    if (!network.issues.empty()) throw ValidationError(std::move(network.issues));
    enter(MesoStage::demand);
    auto demand = mesoDemand(d, data, network.graph, manifest);
    if (!demand.issues.empty()) throw ValidationError(std::move(demand.issues));
    if (manifest) { manifest->validate(); run.inputManifest = manifest->json(); }
    run.graph = std::move(network.graph);
    run.classes = std::move(demand.classes);
    run.advisories = std::move(network.advisories);
    enter(MesoStage::loading);
    std::vector<bool> watch;
    if (watchLinks)
        for (const auto& l : run.graph.links) watch.push_back(std::find(watchLinks->begin(), watchLinks->end(), l.source) != watchLinks->end());
    run.loading = loadMeso(run.graph, run.classes, run.settings.step, run.duration,
                           observer ? observer->progress : std::function<bool(std::size_t, std::size_t)>{}, watchLinks ? &watch : nullptr);
    for (const auto& [c, p] : run.loading.refused)
        run.advisories.push_back({"MESO_PATH_NOT_LOADED", "classes[" + std::to_string(c) + "].paths[" + std::to_string(p) + "]"});
    return run;
}
}
