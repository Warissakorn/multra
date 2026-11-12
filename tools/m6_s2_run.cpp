// M6 kind S2 (docs/reference/M6_S2_RUN.md): mode `c` is S2-c (D538, x = 0.5, 0.7, 0.85 against FCTL);
// mode `ab` is S2-b and S2-a (D539, §6, x = 0.5 ... 1.05 against HCM 7 d1 + d2). For each x the signal
// and no-signal projects run seeds 42-81; the figure is the paired section travel-time difference.
// Writes a fresh directory; metadata.json with complete=true is written LAST.
#include "m6_s2_summary.hpp"
#include "../src/runner/batch.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "build_info.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace multra;
namespace {
constexpr const char* kUsage = "usage: multra-m6-s2-run c|ab <data-dir> <compile_commands.json> <new-output-dir>\n";
std::string bytes(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot read " + path.string());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
void write(const std::filesystem::path& path, const std::string& value) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot create " + path.string());
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    stream << value;
}
struct Arm { std::string name; ProjectDocument document; RunSnapshot snapshot; EvaluationSpec evaluation; };
Json estimate(const Estimate& e) {
    const auto v = [](const std::optional<double>& x) { return x ? Json(*x) : Json(nullptr); };
    return {{"n", e.n}, {"mean", v(e.mean)}, {"sd", v(e.sd)}, {"halfWidth95", v(e.halfWidth95)}};
}
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") { std::cout << kUsage; return 0; }
    if (argc != 5 || (std::string(argv[1]) != "c" && std::string(argv[1]) != "ab")) { std::cerr << kUsage; return 2; }
    try {
        const bool hcm = std::string(argv[1]) == "ab";
        const std::filesystem::path data = argv[2], directory = argv[4];
        if (std::filesystem::exists(directory)) throw std::invalid_argument("Output already exists: " + directory.string());
        InputManifest inputs;
        const auto commands = readInputJson(argv[3], "build/compile_commands.json", &inputs);
        Json command;
        for (const auto& entry : commands)
            if (std::filesystem::path(entry.at("file").get<std::string>()).filename() == "m6_s2_run.cpp") command = entry;
        if (command.is_null()) throw std::invalid_argument("Missing m6_s2_run.cpp compiler command");
        Json sources = Json::array();
        const auto sourceDirectory = std::filesystem::path(command.at("file").get<std::string>()).parent_path();
        for (const auto* name : {"m6_s2_run.cpp", "m6_s2_summary.hpp", "m6_s2_network.hpp", "m6_s1_summary.hpp"})
            sources.push_back({{"file", name}, {"sha256", inputSha256(bytes(sourceDirectory / name))}});
        const auto xs = hcm ? std::vector<double>{.5, .7, .85, .95, 1, 1.05} : std::vector<double>{.5, .7, .85};
        const auto layout = hcm ? m6s2::hcmLayout() : m6s2::Layout{};
        const auto levelOf = [&](double x) { return hcm ? m6s2::hcmLevel(x) : m6s2::level(x); };
        std::vector<Arm> arms;  // per x: signal, then none
        Json documents = Json::array();
        for (const double x : xs)
            for (const bool signal : {true, false}) {
                auto document = m6s2::approach(levelOf(x).flow, signal, layout);
                putDemandCatalog(document, resolveDemandCatalog(*document.definition, data, true, &inputs));
                auto snapshot = compileDocument(document, data, &inputs);
                auto evaluation = evaluationSpec(document, snapshot, data, &inputs);
                char label[32];
                std::snprintf(label, sizeof label, "x%.2f-%s", x, signal ? "signal" : "none");
                const auto text = documentJson(document).dump(2) + "\n";
                documents.push_back({{"file", std::string(label) + ".traffic.json"}, {"sha256", inputSha256(text)}});
                arms.push_back({label, std::move(document), std::move(snapshot), std::move(evaluation)});
            }
        inputs.validate();
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Cannot create output directory");
        for (const auto& arm : arms) write(directory / (arm.name + ".traffic.json"), documentJson(arm.document).dump(2) + "\n");
        std::vector<std::uint32_t> seeds;
        for (std::uint32_t seed = 42; seed <= 81; ++seed) seeds.push_back(seed);
        std::string csv = "x,seed,ttSignal,ttNone,delta,overloadedSignal,unfinishedSignal,pendingSignal\n";
        Json levels = Json::array();
        for (std::size_t k = 0; k < xs.size(); ++k) {
            std::vector<BatchReport> reports;
            for (const auto* arm : {&arms[2 * k], &arms[2 * k + 1]}) {
                reports.push_back(aggregate(runSeeds(arm->snapshot.scenario, arm->evaluation, seeds)));
                std::cerr << arm->name << " done\n";
            }
            const auto& sig = reports[0].sections.at(0);
            const auto& none = reports[1].sections.at(0);
            const auto pair = m6s2::paired(reports[0].seeds, sig.meanTravelTime.perSeed, none.meanTravelTime.perSeed);
            const auto l = levelOf(xs[k]);
            const bool available = !hcm || m6s2::available(sig.unfinished.perSeed, reports[0].pending.perSeed);
            for (std::size_t i = 0; i < seeds.size(); ++i) {
                const auto cell = [](const std::optional<double>& v) { return v ? Json(*v).dump() : std::string(); };
                const auto& over = reports[0].overloadedSeeds;
                csv += Json(xs[k]).dump() + "," + std::to_string(seeds[i]) + "," + cell(sig.meanTravelTime.perSeed[i]) + "," +
                    cell(none.meanTravelTime.perSeed[i]) + "," +
                    (pair.bySeed.count(seeds[i]) ? Json(pair.bySeed.at(seeds[i])).dump() : std::string()) + "," +
                    (std::find(over.begin(), over.end(), seeds[i]) != over.end() ? "true" : "false") + "," +
                    cell(sig.unfinished.perSeed.at(i)) + "," + cell(reports[0].pending.perSeed.at(i)) + "\n";
            }
            const auto f = pair.delta;
            levels.push_back({{"x", l.x}, {"mu", l.mu}, {"flow", l.flow}, {"slotSeconds", l.slot},
                {"reference", l.reference}, {"delta", m6s1::figureJson(f)},
                {"verdict", m6s1::verdictName(m6s2::verdict(available ? f.mean : std::nullopt, l.reference))},
                {"available", available}, {"rows", hcm ? (xs[k] < .8 ? Json{"S2-b"} : xs[k] < .99 ? Json{"S2-b", "S2-a"} : Json{"S2-a"}) : Json{"S2-c"}},
                {"pendingSignal", estimate(reports[0].pending)},
                {"differencePercent", f.mean ? Json(100 * (*f.mean / l.reference - 1)) : Json(nullptr)},
                {"seedsMissing", pair.missing},
                {"travelTimeSignal", estimate(sig.meanTravelTime)}, {"travelTimeNone", estimate(none.meanTravelTime)},
                {"vehiclesSignal", estimate(sig.vehicles)}, {"unfinishedSignal", estimate(sig.unfinished)},
                {"overloadedSeedsSignal", reports[0].overloadedSeeds}, {"overloadedSeedsNone", reports[1].overloadedSeeds},
                {"inSlotBoundSeconds", l.slot}});
        }
        write(directory / "runs.csv", csv);
        const Json summary{{"label", hcm ? "M6 S2-b and S2-a (D539); a comparison, not a calibration"
                                         : "M6 S2-c (D538); a comparison, not a calibration"}, {"band", m6s2::kBand},
            {"reference", hcm ? "HCM 7 d1 + d2" : "FCTL h * E D"}, {"cycle", layout.cycle}, {"green", layout.green},
            {"amber", layout.amber}, {"approachLength", layout.length}, {"cooldown", layout.cooldown},
            {"declaredSaturation", m6s2::kSaturation},
            {"measuredSaturationS1", 1944.0}, {"seeds", seeds}, {"levels", levels}};
        write(directory / "summary.json", summary.dump(2) + "\n");
        Json outputs = Json::array();
        for (const auto* name : {"runs.csv", "summary.json"}) {
            const auto captured = bytes(directory / name);
            outputs.push_back({{"file", name}, {"bytes", captured.size()}, {"sha256", inputSha256(captured)}});
        }
        const Json metadata{{"complete", true}, {"label", hcm ? "M6 S2-b and S2-a first comparable run (D539)" : "M6 S2-c first comparable run (D538)"},
            {"buildCommit", MULTRA_BUILD_COMMIT}, {"compiler", MULTRA_COMPILER}, {"compilerCommand", command},
            {"toolSources", sources}, {"documents", documents}, {"outputs", outputs},
            {"inputManifest", inputs.json()}, {"firstSeed", 42}, {"lastSeed", 81}};
        write(directory / "metadata.json", metadata.dump(2) + "\n");
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
