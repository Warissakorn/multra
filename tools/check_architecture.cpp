#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
// Deliberately restricted include syntax: literal same-directory headers and a
// reviewed set of standard headers. Macro includes and modules fail closed.
bool check(const std::string& source, const std::filesystem::path& directory) {
    static const std::set<std::string> standard{
        "algorithm", "array", "cmath", "cstdint", "functional", "limits", "map", "memory", "numbers",
        "optional", "set", "stdexcept", "string", "utility", "variant", "vector", "type_traits"};
    const std::regex directive(R"(^\s*#\s*(include|include_next)\b.*)");
    const std::regex literal(R"(^\s*#\s*include\s*([<"])([^>"\r\n]+)[>"]\s*(//.*)?$)");
    const std::regex forbidden(R"(\b(import|random_device|rand|srand|system_clock|steady_clock|unordered_map|unordered_set|__TIME__|__DATE__)\b)");
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        // Comments are not executable; full C++ parsing is intentionally not claimed.
        const auto code = line.substr(0, line.find("//"));
        if (std::regex_search(code, forbidden)) return false;
        if (!std::regex_match(line, directive)) continue;
        std::smatch match;
        if (!std::regex_match(line, match, literal)) return false;
        const std::string header = match[2];
        if (match[1] == "<") { if (!standard.contains(header)) return false; }
        else if (header.find_first_of("/\\") != std::string::npos ||
                 std::filesystem::path(header).extension() != ".hpp" ||
                 !std::filesystem::is_regular_file(directory / header)) return false;
    }
    return true;
}
// Project persistence may read model contracts, but must never know command implementations.
bool projectBoundary(const std::string& source) {
    const std::regex directive(R"(^\s*#\s*(include|include_next)\b.*)");
    const std::regex literal(R"(^\s*#\s*include\s*([<"])([^>"\r\n]+)[>"]\s*(//.*)?$)");
    const std::regex forbidden(R"((^|[/\\])(commands|editor|shell|render)([/\\]|$))");
    std::istringstream stream(source); std::string line;
    while (std::getline(stream, line)) {
        if (!std::regex_match(line, directive)) continue;
        std::smatch match;
        if (!std::regex_match(line, match, literal)) return false;
        const std::string header = match[2];
        if (std::regex_search(header, forbidden) || header.starts_with("Q") || header.starts_with("Qt")) return false;
    }
    return true;
}
// D165: the behaviour lab may build documents through commands, but never knows the editor, the
// shell or Qt -- the window draws it, so it stays testable headless.
bool labBoundary(const std::string& source) {
    const std::regex directive(R"(^\s*#\s*(include|include_next)\b.*)");
    const std::regex literal(R"(^\s*#\s*include\s*([<"])([^>"\r\n]+)[>"]\s*(//.*)?$)");
    const std::regex forbidden(R"((^|[/\\])(editor|shell|render)([/\\]|$))");
    std::istringstream stream(source); std::string line;
    while (std::getline(stream, line)) {
        if (!std::regex_match(line, directive)) continue;
        std::smatch match;
        if (!std::regex_match(line, match, literal)) return false;
        const std::string header = match[2];
        if (std::regex_search(header, forbidden) || header.starts_with("Q") || header.starts_with("Qt")) return false;
    }
    return true;
}
// The layers above the engines: each names the module directories it must not include, and
// whether Qt or nlohmann/json may reach it. Every include must be a literal, as above.
struct LayerRule { const char* module; const char* forbidden; bool qt, json; };
constexpr LayerRule layerRules[]{
    {"model", "project|commands|editor|shell|lab|geo|eval|runner|render", false, false},
    {"commands", "editor|shell|lab|geo|render", false, true},
    {"geo", "commands|editor|shell|lab|render", false, true},
    {"editor", "shell|lab|render", true, true}};
bool layerBoundary(const std::string& source, const LayerRule& rule) {
    const std::regex directive(R"(^\s*#\s*(include|include_next)\b.*)");
    const std::regex literal(R"(^\s*#\s*include\s*([<"])([^>"\r\n]+)[>"]\s*(//.*)?$)");
    const std::regex forbidden(std::string(R"((^|[/\\])()") + rule.forbidden + R"()([/\\]|$))");
    std::istringstream stream(source); std::string line;
    while (std::getline(stream, line)) {
        if (!std::regex_match(line, directive)) continue;
        std::smatch match;
        if (!std::regex_match(line, match, literal)) return false;
        const std::string header = match[2];
        if (std::regex_search(header, forbidden)) return false;
        if (!rule.qt && match[1] == "<" && header.starts_with("Q")) return false;
        if (!rule.json && header.starts_with("nlohmann")) return false;
    }
    return true;
}
// The batch runner (M5.2) may read the core and eval contracts, which `check` would refuse as
// cross-directory headers; replace exactly those includes so every other rule still applies.
std::string runnerContract(std::string source) {
    const std::regex allowed(R"(#include "\.\./(core|eval)/[a-z_]+\.hpp")");
    return std::regex_replace(source, allowed, "#include <vector>");
}
// D511: three edits that cost near-full rebuilds once, kept from coming back. A schema step must not
// touch document.hpp (542 objects), a signal head's look must not reach canvas.hpp's includers (181),
// and a UI executable must reuse the PCH GCC accepts in the shared build (ui_pch.cmake), not
// multra_shell's. Comments are stripped first: `//` for sources, `#` for CMake.
struct FanoutRule { const char* path; const char* refused; bool cmake; };
constexpr FanoutRule fanoutRules[]{
    {"src/project/document.hpp", R"(kLatestSchema|latest_schema\.hpp)", false},
    {"src/editor/canvas.hpp", R"(#\s*include\s*"road_crossbar\.hpp")", false},
    {"src/shell/*.cmake", R"(REUSE_FROM\s+multra_shell\s*\))", true}};
bool fanoutGuard(const std::string& source, const FanoutRule& rule) {
    const std::regex refused(rule.refused);
    std::istringstream stream(source); std::string line;
    while (std::getline(stream, line)) {
        // `#include` is code in a source, a comment in CMake.
        const auto cut = rule.cmake ? line.find('#') : line.find("//");
        if (std::regex_search(line.substr(0, cut), refused)) return false;
    }
    return true;
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    if (std::string(argv[1]) == "--self-test") {
        for (const auto* bad : {"#include <QWidget>", "#include <fstream>", "#include <random>",
                "#include \"../model/network.hpp\"", "#include HEADER", "#include_next <vector>",
                "import bad.module;", "std::rand();", "auto x = __TIME__;"})
            if (check(bad, {})) { std::cerr << "Guard accepted: " << bad << '\n'; return 1; }
        for (const auto* bad : {"#include \"../commands/history.hpp\"", "#include <QFile>", "#include HEADER"})
            if (projectBoundary(bad)) return 1;
        if (!projectBoundary("#include \"../model/network/network.hpp\"\n#include <nlohmann/json.hpp>")) return 1;
        // runner may see core and eval, never project/model/Qt or I/O.
        for (const auto* bad : {"#include \"../project/load.hpp\"", "#include <QObject>", "#include <fstream>",
                "#include \"../model/network/network.hpp\""})
            if (check(runnerContract(bad), {})) { std::cerr << "Runner guard accepted: " << bad << '\n'; return 1; }
        if (!check(runnerContract("#include \"../eval/movement.hpp\"\n#include \"../core/simulation.hpp\"\n"), {})) return 1;
        for (const auto* bad : {"#include \"../shell/editor_window.hpp\"", "#include \"../editor/canvas.hpp\"", "#include <QWidget>"})
            if (labBoundary(bad)) { std::cerr << "Lab guard accepted: " << bad << '\n'; return 1; }
        if (!labBoundary("#include \"../commands/demand_commands.hpp\"\n#include <cmath>")) return 1;
        // model: engines only; commands and geo: never the UI; editor: never the shell.
        for (const auto* bad : {"#include \"../../project/json.hpp\"", "#include <QString>", "#include <nlohmann/json.hpp>", "#include HEADER"})
            if (layerBoundary(bad, layerRules[0])) { std::cerr << "Model guard accepted: " << bad << '\n'; return 1; }
        if (!layerBoundary("#include \"../../macro/graph.hpp\"\n#include \"../network/network.hpp\"\n", layerRules[0])) return 1;
        for (const auto* bad : {"#include \"../shell/editor_window.hpp\"", "#include <QWidget>"})
            if (layerBoundary(bad, layerRules[1]) || layerBoundary(bad, layerRules[2])) { std::cerr << "Layer guard accepted: " << bad << '\n'; return 1; }
        if (layerBoundary("#include \"../shell/path.hpp\"", layerRules[3]) || !layerBoundary("#include <QWidget>\n#include \"../commands/history.hpp\"\n", layerRules[3])) return 1;
        // D511: each old form is refused, the current one and a comment naming it are not.
        for (const auto* bad : {"inline constexpr int kLatestSchema = 74;", "#include \"latest_schema.hpp\""})
            if (fanoutGuard(bad, fanoutRules[0])) { std::cerr << "Fan-out guard accepted: " << bad << '\n'; return 1; }
        if (fanoutGuard("#include \"road_crossbar.hpp\"", fanoutRules[1]) ||
            !fanoutGuard("#include \"road_crossbar_geometry.hpp\"\n// not road_crossbar.hpp: D511\n", fanoutRules[1])) return 1;
        if (fanoutGuard("target_precompile_headers(t REUSE_FROM multra_shell)", fanoutRules[2]) ||
            !fanoutGuard("target_precompile_headers(t REUSE_FROM ${MULTRA_UI_PCH})\n# was REUSE_FROM multra_shell)\n", fanoutRules[2])) return 1;
        return check("#include <vector>\n#include <cmath>\n", {}) ? 0 : 1;
    }
    bool valid = true;
    // macro: S2 (MACRO_NETWORK §7), meso: M11 (MESO_NETWORK §1) and analysis: M13 (NODE_ANALYSIS
    // §7) import nothing, like core.
    for (const auto* module : {"core", "eval", "runner", "macro", "meso", "analysis"}) {
        const auto directory = std::filesystem::path(argv[1]) / "src" / module;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.path().extension() != ".hpp" && entry.path().extension() != ".cpp") continue;
            std::ifstream file(entry.path());
            std::string source((std::istreambuf_iterator<char>(file)), {});
            // eval is allowed the core contract, never model/project/Qt.
            if (std::string(module) == "eval") {
                const std::string allowed = "#include \"../core/types.hpp\"";
                for (auto pos = source.find(allowed); pos != std::string::npos; pos = source.find(allowed))
                    source.replace(pos, allowed.size(), "#include <vector>");
            }
            if (std::string(module) == "runner") source = runnerContract(source);
            if (!file || !check(source, directory)) { std::cerr << "Boundary violation: " << entry.path() << '\n'; valid = false; }
        }
    }
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(argv[1]) / "src/project")) {
        if (entry.path().extension() != ".hpp" && entry.path().extension() != ".cpp") continue;
        std::ifstream file(entry.path()); std::string source((std::istreambuf_iterator<char>(file)), {});
        if (!file || !projectBoundary(source)) { std::cerr << "Project boundary violation: " << entry.path() << '\n'; valid = false; }
    }
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(argv[1]) / "src/lab")) {
        if (entry.path().extension() != ".hpp" && entry.path().extension() != ".cpp") continue;
        std::ifstream file(entry.path()); std::string source((std::istreambuf_iterator<char>(file)), {});
        if (!file || !labBoundary(source)) { std::cerr << "Lab boundary violation: " << entry.path() << '\n'; valid = false; }
    }
    for (const auto& rule : layerRules)
        for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::path(argv[1]) / "src" / rule.module)) {
            if (entry.path().extension() != ".hpp" && entry.path().extension() != ".cpp") continue;
            std::ifstream file(entry.path()); std::string source((std::istreambuf_iterator<char>(file)), {});
            if (!file || !layerBoundary(source, rule)) { std::cerr << "Layer boundary violation: " << entry.path() << '\n'; valid = false; }
        }
    for (const auto& rule : fanoutRules) {
        const auto root = std::filesystem::path(argv[1]);
        std::vector<std::filesystem::path> files;
        if (std::string(rule.path) == "src/shell/*.cmake") {
            for (const auto& entry : std::filesystem::directory_iterator(root / "src/shell"))
                if (entry.path().extension() == ".cmake" || entry.path().filename() == "CMakeLists.txt") files.push_back(entry.path());
        } else files.push_back(root / rule.path);
        for (const auto& path : files) {
            std::ifstream file(path); std::string source((std::istreambuf_iterator<char>(file)), {});
            if (!file || !fanoutGuard(source, rule)) { std::cerr << "Rebuild fan-out (D511): " << path << '\n'; valid = false; }
        }
    }
    return valid ? 0 : 1;
}
