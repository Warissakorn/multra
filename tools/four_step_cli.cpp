#include "four_step_cli.hpp"
#include "../src/project/build_metadata.hpp"
#include "../src/project/export_marking.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/json.hpp"
#include "../src/project/load.hpp"
#include "../src/project/mode_choice_output.hpp"
#include "../src/project/procedure_output.hpp"
#include "../src/project/trip_distribution_output.hpp"
#include "../src/project/trip_generation_output.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace multra {
namespace {
void writeCsv(const std::filesystem::path& csvFile, const std::string& text) {
    if (csvFile.empty()) return;
    std::ofstream csv(csvFile);
    csv << text;
    if (!csv) throw std::runtime_error("Failed writing CSV output");
}
void stamp(Json& result) { addBuildMetadata(result); }
}
// --trip-generation (M10.2 G1, TRIP_GENERATION §8, D470): one rate set over the project's zones.
int runTripGenerationProject(const std::filesystem::path& file, const std::filesystem::path& csvFile, const std::filesystem::path& data,
                             const std::string& rateSet) {
    if (!std::filesystem::exists(file)) throw std::runtime_error("Cannot read project: " + file.string());
    InputManifest inputs;
    const auto document = parseDocument(readInputJson(file, "project", &inputs));
    if (!csvFile.empty() && std::filesystem::exists(csvFile))
        throw std::invalid_argument("CSV output already exists: " + csvFile.string());
    const auto run = runTripGeneration(document, data, rateSet, &inputs);
    const auto marking = exportMarking(document, {});
    writeCsv(csvFile, markedCsv(tripGenerationCsv(run), marking));
    auto result = tripGenerationJson(run);
    addMarkingJson(result, marking);
    stamp(result);
    std::cout << result.dump(2) << '\n';
    return std::cout ? 0 : 1;
}
// --trip-distribution (M10.3 D1, TRIP_DISTRIBUTION §8, D473): generation, the free-flow skim and the gravity model.
int runTripDistributionProject(const std::filesystem::path& file, const std::filesystem::path& csvFile, const std::filesystem::path& data,
                               const std::string& rateSet, const std::string& deterrence, const std::string& mode,
                               const std::string& matrix) {
    if (!std::filesystem::exists(file)) throw std::runtime_error("Cannot read project: " + file.string());
    InputManifest inputs;
    const auto document = parseDocument(readInputJson(file, "project", &inputs));
    if (!csvFile.empty() && std::filesystem::exists(csvFile)) throw std::invalid_argument("CSV output already exists: " + csvFile.string());
    const auto run = runTripDistribution(document, data, rateSet, deterrence, mode, matrix, &inputs);
    const auto marking = exportMarking(document, {});
    writeCsv(csvFile, markedCsv(tripDistributionCsv(run), marking));
    auto result = tripDistributionJson(run);
    addMarkingJson(result, marking);
    stamp(result);
    std::cout << result.dump(2) << '\n';
    if (!run.converged()) std::cerr << "MULTRA: warning: DISTRIBUTION_NOT_CONVERGED at the pass cap\n";
    return std::cout ? 0 : 1;
}
// --mode-choice (M10.4 E1, MODE_CHOICE §7, D478): distribution, the logit split and each period's OD.
int runModeChoiceProject(const std::filesystem::path& file, const std::filesystem::path& csvFile, const std::filesystem::path& data,
                         const std::string& rateSet, const std::string& deterrence, const std::string& mode, const std::string& matrix,
                         const std::string& modeChoiceSet, const std::vector<std::string>& periods) {
    if (!std::filesystem::exists(file)) throw std::runtime_error("Cannot read project: " + file.string());
    InputManifest inputs;
    const auto document = parseDocument(readInputJson(file, "project", &inputs));
    if (!csvFile.empty() && std::filesystem::exists(csvFile)) throw std::invalid_argument("CSV output already exists: " + csvFile.string());
    const auto run = runModeChoice(document, data, rateSet, deterrence, mode, matrix, modeChoiceSet, periods, &inputs);
    const auto marking = exportMarking(document, {});
    writeCsv(csvFile, markedCsv(modeChoiceCsv(run), marking));
    auto result = modeChoiceJson(run);
    addMarkingJson(result, marking);
    stamp(result);
    std::cout << result.dump(2) << '\n';
    if (!run.distribution.converged()) std::cerr << "MULTRA: warning: DISTRIBUTION_NOT_CONVERGED at the pass cap\n";
    return std::cout ? 0 : 1;
}
// --procedure (PROCEDURE_SEQUENCE §4, D484): a saved procedure's steps in order; JSON only.
int runProcedureProject(const std::filesystem::path& file, const std::filesystem::path& data, const std::string& id) {
    if (!std::filesystem::exists(file)) throw std::runtime_error("Cannot read project: " + file.string());
    InputManifest inputs;
    const auto document = parseDocument(readInputJson(file, "project", &inputs));
    const auto run = runProcedure(document, data, id, &inputs);
    auto result = procedureJson(run);
    addMarkingJson(result, exportMarking(document, {}));
    stamp(result);
    std::cout << result.dump(2) << '\n';
    for (const auto& s : run.steps)
        for (const auto& i : s.issues) std::cerr << "MULTRA: step " << s.index << " refused: " << i.code << " at " << i.path << '\n';
    return std::cout && run.completed() ? 0 : 1;
}
}
