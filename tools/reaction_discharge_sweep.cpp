// BA38: fixed experiment matrix, not in check. See tools/README.md and PERCEPTION §5/§9.
#include "reaction_discharge.hpp"
#include "reaction_discharge_summary.hpp"
#include "../src/project/input_manifest.hpp"
#include "build_info.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace multra;
namespace {
std::string bytes(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot read " + path.string());
    const std::string value{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (stream.bad()) throw std::runtime_error("Failed reading " + path.string());
    return value;
}
std::ofstream output(const std::filesystem::path& path) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot create " + path.string());
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    return stream;
}
void write(const std::filesystem::path& path, const std::string& bytes) {
    auto stream = output(path); stream << bytes; stream.close();
}
std::string cell(const Json& value) {
    if (value.is_null()) return "";
    if (!value.is_string()) return value.dump();
    // RFC 4180, including commas/quotes/newlines in authored head/lane ids and reasons.
    std::string escaped = "\"";
    for (const char c : value.get<std::string>()) { escaped += c; if (c == '"') escaped += '"'; }
    return escaped + '"';
}
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "usage: multra-reaction-discharge-sweep <project> <w74.json> <data-dir> <compile_commands.json> <new-output-dir>\n";
        return 0;
    }
    if (argc != 6) { std::cerr << "Use --help for the BA38 sweep arguments\n"; return 2; }
    try {
        const std::filesystem::path data = argv[3], directory = argv[5];
        if (std::filesystem::exists(directory)) throw std::invalid_argument("Output already exists: " + directory.string());
        InputManifest inputs;
        auto base = parseDocument(readInputJson(argv[1], "project", &inputs));
        putDemandCatalog(base, resolveDemandCatalog(*base.definition, data, true, &inputs));
        const auto w74 = parseBehaviour(readInputJson(argv[2], "w74-preset", &inputs));
        if (!w74.w74) throw std::invalid_argument("Expected a w74 preset");
        const auto commands = readInputJson(argv[4], "build/compile_commands.json", &inputs);
        Json command;
        for (const auto& entry : commands)
            if (std::filesystem::path(entry.at("file").get<std::string>()).filename() == "reaction_discharge_sweep.cpp") {
                if (!command.is_null()) throw std::invalid_argument("Ambiguous sweep compiler command");
                command = entry;
            }
        if (command.is_null()) throw std::invalid_argument("Missing sweep compiler command");
        Json sources = Json::array();
        const auto sourceDirectory = std::filesystem::path(command.at("file").get<std::string>()).parent_path();
        for (const auto* name : {"reaction_discharge_sweep.cpp", "reaction_discharge.hpp", "reaction_discharge_summary.hpp"})
            sources.push_back({{"file", name}, {"sha256", inputSha256(bytes(sourceDirectory / name))}});
        struct Arm { std::string name; ProjectDocument document; RunSnapshot snapshot; EvaluationSpec evaluation; };
        std::vector<Arm> arms;
        Json variants = Json::array();
        // Prepare/validate every arm before creating output. No unsupported road assignment may
        // silently keep prototype traffic in the W74 arm; this tool targets the unassigned fixture.
        for (const std::string model : {"prototype", "w74"})
            for (const auto reaction : {std::optional<double>{}, std::optional<double>{0},
                                       std::optional<double>{.5}, std::optional<double>{1}}) {
                const auto name = model + (reaction ? "-" + std::to_string(static_cast<int>(*reaction * 10)) : "-absent");
                auto document = reactiondischarge::variant(base, model == "w74" ? &w74 : nullptr, reaction);
                auto snapshot = compileDocument(document, data, &inputs);
                if (!snapshot.scenario.segmentBehaviours.empty() || !snapshot.scenario.parkingLots.empty())
                    throw std::invalid_argument("BA38 requires unassigned roads and no parking lots");
                for (const auto& type : snapshot.scenario.vehicleTypes) {
                    const auto behaviour = std::find_if(snapshot.scenario.behaviours.begin(), snapshot.scenario.behaviours.end(),
                        [&](const auto& b) { return b.id == type.behaviourId; });
                    if (behaviour == snapshot.scenario.behaviours.end() || behaviour->w74.has_value() != (model == "w74"))
                        throw std::invalid_argument("Unexpected model in BA38 arm");
                }
                auto evaluation = evaluationSpec(document, snapshot, data, &inputs);
                const auto bytes = documentJson(document).dump(2) + "\n";
                variants.push_back({{"name", name}, {"model", model},
                    {"reactionTime", reaction ? Json(*reaction) : Json(nullptr)},
                    {"snapshotFile", name + ".traffic.json"}, {"snapshotSha256", inputSha256(bytes)}});
                arms.push_back({name, std::move(document), std::move(snapshot), std::move(evaluation)});
            }
        inputs.validate();
        // create_directory fails atomically if another writer created the directory meanwhile.
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Cannot create output directory");
        for (const auto& arm : arms) write(directory / (arm.name + ".traffic.json"), documentJson(arm.document).dump(2) + "\n");
        auto runs = output(directory / "runs.jsonl"), csv = output(directory / "cycles.csv");
        csv << "model,reactionTime,seed,head,lane,go,end,complete,reason,samples,meanHeadway,startupLostTime,startupReason\n";
        reactiondischarge::Summary summary;
        for (std::size_t group = 0; group < arms.size(); group += 4) {
            const auto& absent = arms[group].snapshot.scenario;
            for (std::size_t offset = 1; offset < 4; ++offset) {
                const auto& arm = arms[group + offset];
                const auto& variant = variants[group + offset];
                for (std::uint32_t seed = 42; seed <= 81; ++seed) {
                    auto run = reactiondischarge::measure(arm.snapshot.scenario, arm.evaluation.queue, seed,
                                                          offset == 1 ? &absent : nullptr);
                    run["variant"] = arm.name; run["model"] = variant["model"];
                    run["reactionTime"] = variant["reactionTime"]; summary.add(run); runs << run.dump() << '\n';
                    for (const auto& cycle : run["discharge"]["cycles"]) {
                        csv << cell(run["model"]) << ',' << cell(run["reactionTime"]) << ',' << seed;
                        for (const char* key : {"headId", "laneId", "go", "end", "complete", "unavailableReason", "samples",
                                                "meanHeadwaySeconds", "startupLostTimeSeconds", "startupUnavailableReason"})
                            csv << ',' << cell(cycle[key]);
                        csv << '\n';
                    }
                    std::cerr << arm.name << " seed " << seed << " completed " << run["completed"]
                              << " clamps " << run["safetyClamps"] << '\n';
                }
            }
        }
        runs.close(); csv.close();
        write(directory / "summary.json", summary.json().dump(2) + "\n");
        Json outputs = Json::array();
        for (const auto* name : {"runs.jsonl", "cycles.csv", "summary.json"}) {
            const auto captured = bytes(directory / name);
            outputs.push_back({{"file", name}, {"bytes", captured.size()}, {"sha256", inputSha256(captured)}});
        }
        // Written LAST: absence of complete=true means an interrupted/failed experiment.
        Json metadata{{"complete", true}, {"label", "not yet validated; BA38 sensitivity, not calibration"},
            {"buildCommit", MULTRA_BUILD_COMMIT}, {"compiler", MULTRA_COMPILER},
            {"compilerCommand", command}, {"toolSources", sources}, {"outputs", outputs},
            {"inputManifest", inputs.json()}, {"variants", variants},
            {"timeStep", .1}, {"firstSeed", 42}, {"lastSeed", 81}, {"experimentalRuns", 240},
            {"zeroAbsentControlRuns", 80}, {"copiedReplayTicksPerRun", 32}};
        write(directory / "metadata.json", metadata.dump(2) + "\n");
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
