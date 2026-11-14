// M6 kind S1's first comparable run (D535, docs/reference/M6_S1_RUN.md §2): seeds 42-81 on the
// one-lane approach, discharge per cycle, reduced by m6_s1_summary.hpp. Writes a fresh directory;
// metadata.json with complete=true is written LAST, so its absence marks an incomplete run.
#include "m6_candidates.hpp"
#include "m6_s1_diagnostic.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "build_info.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace multra;
namespace {
constexpr const char* kUsage =
    "usage: multra-m6-s1-run <project> <data-dir> <compile_commands.json> <new-output-dir> [--candidate base|B|C|D|E|DE]\n";
std::string bytes(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot read " + path.string());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
std::ofstream output(const std::filesystem::path& path) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot create " + path.string());
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    return stream;
}
void write(const std::filesystem::path& path, const std::string& value) {
    auto stream = output(path); stream << value; stream.close();
}
DischargeSpec spec(double duration) {
    DischargeSpec s;
    s.windowStart = 0; s.windowEnd = duration; s.warmup = 900;
    s.steadyFirst = 5; s.steadyLast = m6s1::kRanks; s.startupLast = 4;
    s.vehicleTypeIds = {"car"};
    validateDischargeSpec(s);
    return s;
}
Json run(const Scenario& scenario, const QueueDefinition& queue, const DischargeSpec& s, std::uint32_t seed,
         m6s1::DiagnosticObserver& observer) {
    DischargeAccumulator discharge(s, queue);
    auto state = createSimulation(scenario, seed);
    discharge.observe(state); observer.observe(state);
    const auto ticks = totalTicks(scenario);
    while (state.tick < ticks) {
        state = stepSimulation(std::move(state));
        discharge.observe(state); observer.observe(state);
    }
    const auto pending = pendingCount(state);
    if (state.nextVehicleId - 1 != state.completed + state.vehicles.size() + pending)
        throw std::runtime_error("Vehicle accounting mismatch");
    return {{"seed", seed}, {"generated", state.nextVehicleId - 1}, {"completed", state.completed},
        {"active", state.vehicles.size()}, {"pending", pending}, {"time", state.time},
        {"discharge", dischargeJson(discharge.report(), s)}};
}
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") { std::cout << kUsage; return 0; }
    try {
        const auto candidate = m6sweep::take(argc, argv);  // D545
        if (argc != 5) { std::cerr << kUsage; return 2; }
        const std::filesystem::path data = argv[2], directory = argv[4];
        if (std::filesystem::exists(directory)) throw std::invalid_argument("Output already exists: " + directory.string());
        InputManifest inputs;
        auto document = parseDocument(readInputJson(argv[1], "project", &inputs));
        putDemandCatalog(document, resolveDemandCatalog(*document.definition, data, true, &inputs));
        auto snapshot = compileDocument(document, data, &inputs);
        if (candidate) m6sweep::apply(snapshot.scenario, *candidate);
        const auto evaluation = evaluationSpec(document, snapshot, data, &inputs);
        const auto commands = readInputJson(argv[3], "build/compile_commands.json", &inputs);
        Json command;
        for (const auto& entry : commands)
            if (std::filesystem::path(entry.at("file").get<std::string>()).filename() == "m6_s1_run.cpp") command = entry;
        if (command.is_null()) throw std::invalid_argument("Missing m6_s1_run.cpp compiler command");
        Json sources = Json::array();
        const auto sourceDirectory = std::filesystem::path(command.at("file").get<std::string>()).parent_path();
        for (const auto* name : {"m6_s1_run.cpp", "m6_s1_summary.hpp", "m6_s1_network.hpp", "m6_s1_diagnostic.hpp", "m6_candidates.hpp"})
            sources.push_back({{"file", name}, {"sha256", inputSha256(bytes(sourceDirectory / name))}});
        inputs.validate();
        const auto& scenario = snapshot.scenario;
        const auto s = spec(scenario.duration);
        // §7: the line's route distance; only the one-route, one-head layout of §1 is supported.
        if (scenario.routes.size() != 1 || scenario.signalHeads.size() != 1 || scenario.signalPrograms.size() != 1 ||
            scenario.routes[0].segmentIds.empty() || scenario.signalHeads[0].segmentId != scenario.routes[0].segmentIds[0])
            throw std::invalid_argument("The S1 diagnostic needs one route, one signal program and one head on the route's first segment");
        const double line = scenario.signalHeads[0].position;
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Cannot create output directory");
        auto runs = output(directory / "runs.jsonl"), csv = output(directory / "cycles.csv"),
             diag = output(directory / "diagnostic.csv");
        diag << "seed,go,rank,vehicleId,goTime,goDistance,goSpeed,moveTime,crossingTime,lineSpeed\n";
        std::vector<m6s1::DiagnosticCycle> diagnostics;
        csv << "seed,go,end,complete,reason,meanHeadway";
        for (std::size_t r = 1; r <= m6s1::kRanks; ++r) csv << ",t" << r;
        csv << '\n';
        std::vector<m6s1::Cycle> cycles;
        std::vector<std::uint32_t> seeds;
        for (std::uint32_t seed = 42; seed <= 81; ++seed) {
            m6s1::DiagnosticObserver observer(line, scenario.signalPrograms[0], evaluation.queue.beginSpeed);
            const auto result = run(scenario, evaluation.queue, s, seed, observer);
            runs << result.dump() << '\n';
            seeds.push_back(seed);
            for (const auto& row : result["discharge"]["cycles"]) {
                const auto c = m6s1::cycleOf(seed, row);
                csv << seed << ',' << row["go"].dump() << ',' << row["end"].dump() << ','
                    << row["complete"].dump() << ",\"" << c.reason << "\","
                    << (c.meanHeadway ? Json(*c.meanHeadway).dump() : "");
                for (std::size_t r = 0; r < m6s1::kRanks; ++r)
                    csv << ',' << (r < c.times.size() ? Json(c.times[r]).dump() : "");
                csv << '\n';
                cycles.push_back(c);
                if (!m6s1::usable(c)) continue;
                const auto d = m6s1::diagnosticCycle(c, observer.records());
                const auto cell = [](const std::optional<double>& v) { return v ? Json(*v).dump() : std::string(); };
                for (std::size_t r = 0; r < m6s1::kRanks; ++r) {
                    const auto record = r < d.ranks.size() ? d.ranks[r] : m6s1::Record{};
                    diag << seed << ',' << row["go"].dump() << ',' << r + 1 << ','
                         << (r < c.vehicleIds.size() ? std::to_string(c.vehicleIds[r]) : "") << ','
                         << cell(record.goTime) << ',' << cell(record.goDistance) << ',' << cell(record.goSpeed) << ','
                         << cell(record.moveTime) << ','
                         << cell(record.crossingTime) << ',' << cell(record.lineSpeed) << '\n';
                }
                diagnostics.push_back(d);
            }
            std::cerr << "seed " << seed << " generated " << result["generated"] << " completed "
                      << result["completed"] << '\n';
        }
        runs.close(); csv.close(); diag.close();
        auto summary = m6s1::summaryJson(cycles, seeds);
        summary["diagnostic"] = m6s1::diagnosticJson(m6s1::diagnostic(
            diagnostics, seeds, m6s1::seedHeadways(m6s1::saturation(cycles, seeds))));
        if (candidate) summary["candidate"] = candidate->name;
        write(directory / "summary.json", summary.dump(2) + "\n");
        Json outputs = Json::array();
        for (const auto* name : {"runs.jsonl", "cycles.csv", "diagnostic.csv", "summary.json"}) {
            const auto captured = bytes(directory / name);
            outputs.push_back({{"file", name}, {"bytes", captured.size()}, {"sha256", inputSha256(captured)}});
        }
        Json metadata{{"complete", true}, {"label", "M6 kind S1 comparable run (D535) with the S1 diagnostic (D536)"},
            {"buildCommit", MULTRA_BUILD_COMMIT}, {"compiler", MULTRA_COMPILER},
            {"compilerCommand", command}, {"toolSources", sources}, {"outputs", outputs},
            {"inputManifest", inputs.json()}, {"timeStep", scenario.timeStep}, {"duration", scenario.duration},
            {"firstSeed", 42}, {"lastSeed", 81},
            {"discharge", {{"warmup", s.warmup}, {"steadyFirst", s.steadyFirst}, {"steadyLast", s.steadyLast},
                           {"startupLast", s.startupLast}, {"vehicleTypeIds", s.vehicleTypeIds}}}};
        if (candidate) metadata["candidate"] = candidate->name;
        write(directory / "metadata.json", metadata.dump(2) + "\n");
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
