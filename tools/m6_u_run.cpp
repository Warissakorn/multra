// M6 kind U (docs/reference/M6_U_RUN.md, D542): the minor left turn's capacity under Stop at five
// major flows, seeds 42-81, counted by the priority-line counter (D528), against Eq. 20-18 at the
// engine's pooled Siegloch t_c and follow-up t_f (U-c) and at HCM 7's 6.2 / 3.3 s (U-a).
// Writes a fresh directory; metadata.json with complete=true is written LAST.
#include "m6_u_network.hpp"
#include "m6_u_summary.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "build_info.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace multra;
namespace {
constexpr const char* kUsage = "usage: multra-m6-u-run <data-dir> <compile_commands.json> <new-output-dir>\n";
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
Json opt(const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); }
Json fitJson(const std::optional<SieglochFit>& f) {
    return f ? Json{{"t0", f->t0}, {"tf", f->tf}, {"tc", f->tc}} : Json(nullptr);
}
Json pointsJson(const std::vector<SieglochPoint>& points) {
    Json out = Json::array();
    for (const auto& p : points) out.push_back({{"n", p.vehicles}, {"gaps", p.gaps}, {"meanGap", p.meanGap}});
    return out;
}
struct Level { double vc{}; std::string name; ProjectDocument document; RunSnapshot snapshot; EvaluationSpec evaluation; double areaLength{}; };
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") { std::cout << kUsage; return 0; }
    if (argc != 4) { std::cerr << kUsage; return 2; }
    try {
        const std::filesystem::path data = argv[1], directory = argv[3];
        if (std::filesystem::exists(directory)) throw std::invalid_argument("Output already exists: " + directory.string());
        InputManifest inputs;
        const auto commands = readInputJson(argv[2], "build/compile_commands.json", &inputs);
        Json command;
        for (const auto& entry : commands)
            if (std::filesystem::path(entry.at("file").get<std::string>()).filename() == "m6_u_run.cpp") command = entry;
        if (command.is_null()) throw std::invalid_argument("Missing m6_u_run.cpp compiler command");
        Json sources = Json::array();
        const auto sourceDirectory = std::filesystem::path(command.at("file").get<std::string>()).parent_path();
        for (const auto* name : {"m6_u_run.cpp", "m6_u_network.hpp", "m6_u_summary.hpp", "m6_s1_summary.hpp"})
            sources.push_back({{"file", name}, {"sha256", inputSha256(bytes(sourceDirectory / name))}});
        std::vector<Level> levels;
        Json documents = Json::array();
        for (const double vc : m6u::kLevels) {
            auto j = m6u::junction(vc);
            putDemandCatalog(j.document, resolveDemandCatalog(*j.document.definition, data, true, &inputs));
            auto snapshot = compileDocument(j.document, data, &inputs);
            auto evaluation = evaluationSpec(j.document, snapshot, data, &inputs);
            char name[32];
            std::snprintf(name, sizeof name, "vc%04.0f", vc);
            const auto text = documentJson(j.document).dump(2) + "\n";
            documents.push_back({{"file", std::string(name) + ".traffic.json"}, {"sha256", inputSha256(text)}});
            levels.push_back({vc, name, std::move(j.document), std::move(snapshot), std::move(evaluation), j.areaLength});
        }
        inputs.validate();
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Cannot create output directory");
        for (const auto& l : levels) write(directory / (l.name + ".traffic.json"), documentJson(l.document).dump(2) + "\n");
        std::string csv = "vc,seed,minorPerHour,majorPerHour,minorCrossed,majorEntered,followUps,followUpMean,sieglochGaps,share,excluded\n";
        std::vector<std::vector<PriorityLineRow>> rows(levels.size());
        std::vector<PriorityLineRow> all;
        for (std::size_t k = 0; k < levels.size(); ++k) {
            const auto& scenario = levels[k].snapshot.scenario;
            if (scenario.conflictZones.size() != 1) throw std::invalid_argument("Expected the junction's one area");
            for (std::uint32_t seed = 42; seed <= 81; ++seed) {
                PriorityLineAccumulator counter(levels[k].evaluation);
                auto state = createSimulation(scenario, seed);
                counter.observe(state);
                for (const auto ticks = totalTicks(scenario); state.tick < ticks;) {
                    state = stepSimulation(std::move(state));
                    counter.observe(state);
                }
                const auto r = counter.report().rows.at(0);
                rows[k].push_back(r); all.push_back(r);
                const auto s = m6u::share(r);
                csv += Json(levels[k].vc).dump() + "," + std::to_string(seed) + "," + Json(r.minorPerHour).dump() + "," +
                    Json(r.majorPerHour).dump() + "," + std::to_string(r.minorCrossed) + "," + std::to_string(r.majorEntered) + "," +
                    std::to_string(r.followUps) + "," + (r.followUpMean ? Json(*r.followUpMean).dump() : std::string()) + "," +
                    std::to_string(r.sieglochGaps) + "," + (s ? Json(*s).dump() : std::string()) + "," + std::to_string(r.excluded) + "\n";
                std::cerr << "vc " << levels[k].vc << " seed " << seed << " minor " << r.minorPerHour << " major " << r.majorPerHour << '\n';
            }
        }
        // §4: U-c's one t_c from the pooled Siegloch points, t_f the pooled follow-up mean.
        const auto pooledFit = sieglochFit(m6u::pool(all));
        const auto tf = m6u::pooledFollowUp(all);
        Json out = Json::array();
        for (std::size_t k = 0; k < levels.size(); ++k) {
            std::vector<double> minor, major, shares;
            for (const auto& r : rows[k]) {
                minor.push_back(r.minorPerHour); major.push_back(r.majorPerHour);
                if (const auto s = m6u::share(r)) shares.push_back(*s);
            }
            const auto capacity = m6s1::figure(minor);
            const double vc = *m6s1::figure(major).mean;
            const double meanShare = shares.empty() ? 0 : *m6s1::figure(shares).mean;
            const bool ok = m6u::available(meanShare);
            const auto uc = pooledFit && tf ? std::optional{m6u::harders(vc, pooledFit->tc, *tf)} : std::nullopt;
            const double ua = m6u::harders(vc, m6u::kTc, m6u::kTf);
            const auto diff = [&](double ref) { return capacity.mean ? Json(*capacity.mean / ref - 1) : Json(nullptr); };
            out.push_back({{"vcNominal", levels[k].vc}, {"vcCounted", m6s1::figureJson(m6s1::figure(major))},
                {"capacity", m6s1::figureJson(capacity)}, {"share", meanShare}, {"available", ok},
                {"uc", {{"reference", opt(uc)}, {"difference", uc ? diff(*uc) : Json(nullptr)},
                        {"verdict", m6s1::verdictName(uc ? m6u::verdict(capacity.mean, *uc, ok) : m6s1::Verdict::unavailable)}}},
                {"ua", {{"reference", ua}, {"difference", diff(ua)}, {"verdict", m6s1::verdictName(m6u::verdict(capacity.mean, ua, ok))},
                        {"atNominal", m6u::harders(levels[k].vc, m6u::kTc, m6u::kTf)}}},
                {"sieglochForm", pooledFit ? Json(m6u::siegloch(vc, pooledFit->t0, pooledFit->tf)) : Json(nullptr)},
                {"levelFit", fitJson(sieglochFit(m6u::pool(rows[k])))}, {"levelFollowUp", opt(m6u::pooledFollowUp(rows[k]))},
                {"followUp", m6s1::figureJson(m6s1::figure([&] { std::vector<double> v; for (const auto& r : rows[k]) if (r.followUpMean) v.push_back(*r.followUpMean); return v; }()))}});
        }
        write(directory / "runs.csv", csv);
        write(directory / "summary.json", Json{{"label", "M6 kind U first comparable run (D542); band ±15 %"},
            {"areaLength", levels[0].areaLength}, {"tau", m6u::tau(levels[0].areaLength)}, {"gapTime", m6u::gapTime(levels[0].areaLength)},
            {"headway", m6u::kHeadway}, {"minorDemand", m6u::kMinorDemand},
            {"pooled", {{"points", pointsJson(m6u::pool(all))}, {"fit", fitJson(pooledFit)}, {"followUpMean", opt(tf)}}},
            {"hcm", {{"tc", m6u::kTc}, {"tf", m6u::kTf}}}, {"levels", out}}.dump(2) + "\n");
        Json outputs = Json::array();
        for (const auto* name : {"runs.csv", "summary.json"}) {
            const auto captured = bytes(directory / name);
            outputs.push_back({{"file", name}, {"bytes", captured.size()}, {"sha256", inputSha256(captured)}});
        }
        write(directory / "metadata.json", Json{{"complete", true}, {"label", "M6 kind U first comparable run (D542)"},
            {"buildCommit", MULTRA_BUILD_COMMIT}, {"compiler", MULTRA_COMPILER}, {"compilerCommand", command},
            {"toolSources", sources}, {"documents", documents}, {"outputs", outputs}, {"inputManifest", inputs.json()},
            {"firstSeed", 42}, {"lastSeed", 81}}.dump(2) + "\n");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
