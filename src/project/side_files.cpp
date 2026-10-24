#include "side_files.hpp"
#include "input_manifest.hpp"
#include "macro_run.hpp"
#include "meso_run.hpp"
#include "../core/validate.hpp"
#include <cmath>
#include <fstream>
#include <iterator>

namespace multra {
namespace {
constexpr int kSchema = 1;
constexpr const char* kMacroFormat = "multra-macro-side";
constexpr const char* kMesoFormat = "multra-meso-side";
const char* arcKind(MacroArc k) { return k == MacroArc::link ? "link" : k == MacroArc::connector ? "connector" : "centroid"; }
Json arcsJson(const std::vector<SideArc>& arcs) {
    Json out = Json::array();
    for (const auto& a : arcs) out.push_back({a.kind, a.source, a.fromStation});
    return out;
}
// Reads one side file, failing at the first bad value with its JSON path.
struct Reader {
    std::string where;
    [[noreturn]] void fail(const std::string& at) const { throw ValidationError({{"MICRO_AREA_SOURCE_INVALID", where + at}}); }
    const Json& key(const Json& o, const std::string& at, const char* name) const {
        if (!o.is_object() || !o.contains(name)) fail(at + "." + name);
        return o.at(name);
    }
    const Json& array(const Json& o, const std::string& at, const char* name) const {
        const auto& v = key(o, at, name);
        if (!v.is_array()) fail(at + "." + name);
        return v;
    }
    double number(const Json& v, const std::string& at, double low = -HUGE_VAL) const {
        if (!v.is_number() || !std::isfinite(v.get<double>()) || v.get<double>() < low) fail(at);
        return v.get<double>();
    }
    std::size_t index(const Json& v, const std::string& at, std::size_t size) const {
        if (!v.is_number_unsigned() || v.get<std::size_t>() >= size) fail(at);
        return v.get<std::size_t>();
    }
    std::string text(const Json& v, const std::string& at) const {
        if (!v.is_string()) fail(at);
        return v.get<std::string>();
    }
    std::string sha(const Json& o, const std::string& at) const {
        const auto s = text(key(o, at, "projectSha256"), at + ".projectSha256");
        if (s.size() != 64 || s.find_first_not_of("0123456789abcdef") != std::string::npos) fail(at + ".projectSha256");
        return s;
    }
    void header(const Json& j, const char* format) const {
        if (!j.is_object()) fail("");
        if (text(key(j, "", "format"), ".format") != format) fail(".format");
        if (!key(j, "", "schema").is_number_integer() || j.at("schema").get<int>() != kSchema) fail(".schema");
    }
    std::vector<SideArc> arcs(const Json& o, const std::string& at, const char* name) const {
        std::vector<SideArc> out;
        const auto& list = array(o, at, name);
        for (std::size_t i = 0; i < list.size(); ++i) {
            const auto p = at + "." + name + "[" + std::to_string(i) + "]";
            if (!list[i].is_array() || list[i].size() != 3) fail(p);
            out.push_back({text(list[i][0], p + "[0]"), text(list[i][1], p + "[1]"), number(list[i][2], p + "[2]")});
        }
        return out;
    }
};
}
std::string documentBytes(const ProjectDocument& d) { return documentJson(d).dump(2) + "\n"; }
std::string projectSha256(const ProjectDocument& d) { return inputSha256(documentBytes(d)); }
std::filesystem::path sideFilePath(const std::filesystem::path& project, const std::string& kind) {
    auto p = project;
    return p.replace_extension("." + kind + ".json");
}
std::vector<SideArc> sideArcs(const MacroGraph& g) {
    std::vector<SideArc> out;
    for (const auto& l : g.links) out.push_back({arcKind(l.kind), l.source, l.fromStation});
    return out;
}
std::vector<SideArc> sideArcs(const MesoGraph& g) {
    std::vector<SideArc> out;
    for (const auto& l : g.links) out.push_back({"link", l.source, l.fromStation});
    return out;
}
Json macroSideJson(const MacroRun& r, const std::string& sha, const Json& previous) {
    Json j{{"format", kMacroFormat}, {"schema", kSchema}, {"periods", Json::object()}};
    try { parseMacroSide(previous); j["periods"] = previous.at("periods"); } catch (const ValidationError&) {} // invalid: replaced whole
    const auto& a = r.assignment;
    Json p{{"projectSha256", sha}, {"settings", {{"targetGap", r.settings.targetGap}, {"maxIterations", r.settings.maxIterations}}},
           {"gap", a.gap}, {"iterations", a.iterations}, {"converged", a.converged}, {"classes", Json::array()},
           {"arcs", arcsJson(sideArcs(r.graph))}, {"inputManifest", r.inputManifest}};
    for (const auto& c : r.classes) p["classes"].push_back({{"id", c.id}, {"mode", c.mode}, {"pcu", c.pcu}});
    if (a.paths.size() == r.classes.size() && !r.classes.empty()) {
        p["paths"] = Json::array();
        for (const auto& cls : a.paths) {
            Json list = Json::array();
            for (const auto& f : cls) list.push_back({{"cell", f.cell}, {"links", f.links}, {"vehiclesPerHour", f.vehiclesPerHour}});
            p["paths"].push_back(std::move(list));
        }
    }
    j["periods"][r.period] = std::move(p);
    return j;
}
Json mesoSideJson(const MesoRun& r, const std::string& sha, const std::vector<std::string>& watchedLinks) {
    Json j{{"format", kMesoFormat}, {"schema", kSchema}, {"projectSha256", sha},
           {"settings", {{"step", r.settings.step}, {"slice", r.settings.slice}}}, {"duration", r.duration},
           {"links", arcsJson(sideArcs(r.graph))}, {"classes", Json::array()}, {"watchedLinks", watchedLinks}, {"watched", Json::array()}, {"inputManifest", r.inputManifest}};
    for (const auto& c : r.classes) {
        Json paths = Json::array();
        for (const auto& p : c.paths) paths.push_back({{"source", p.source}, {"links", p.links}});
        j["classes"].push_back({{"id", c.id}, {"pcu", c.pcu}, {"paths", std::move(paths)}});
    }
    for (const auto& w : r.loading.watched)
        j["watched"].push_back({{"class", w.cls}, {"path", w.path}, {"position", w.position}, {"step", w.entrance.step}, {"at", w.entrance.at}});
    return j;
}
MacroSide parseMacroSide(const Json& j, const std::string& where) {
    const Reader in{where};
    in.header(j, kMacroFormat);
    const auto& periods = in.key(j, "", "periods");
    if (!periods.is_object()) in.fail(".periods");
    MacroSide side;
    for (const auto& [id, p] : periods.items()) {
        const auto at = ".periods." + id;
        MacroSidePeriod out;
        out.projectSha256 = in.sha(p, at);
        const auto& s = in.key(p, at, "settings");
        out.settings.targetGap = in.number(in.key(s, at + ".settings", "targetGap"), at + ".settings.targetGap", 0);
        const auto& cap = in.key(s, at + ".settings", "maxIterations");
        if (!cap.is_number_integer()) in.fail(at + ".settings.maxIterations");
        out.settings.maxIterations = cap.get<int>();
        out.gap = in.number(in.key(p, at, "gap"), at + ".gap");
        const auto& n = in.key(p, at, "iterations");
        if (!n.is_number_integer() || n.get<int>() < 0) in.fail(at + ".iterations");
        out.iterations = n.get<int>();
        if (!in.key(p, at, "converged").is_boolean()) in.fail(at + ".converged");
        out.converged = p.at("converged").get<bool>();
        const auto& classes = in.array(p, at, "classes");
        for (std::size_t c = 0; c < classes.size(); ++c) {
            const auto cp = at + ".classes[" + std::to_string(c) + "]";
            MacroClass cls;
            cls.id = in.text(in.key(classes[c], cp, "id"), cp + ".id");
            cls.mode = in.text(in.key(classes[c], cp, "mode"), cp + ".mode");
            cls.pcu = in.number(in.key(classes[c], cp, "pcu"), cp + ".pcu", 0);
            out.classes.push_back(std::move(cls));
        }
        out.arcs = in.arcs(p, at, "arcs");
        out.inputManifest = in.key(p, at, "inputManifest");
        if ((out.hasPaths = p.contains("paths"))) {
            const auto& paths = in.array(p, at, "paths");
            if (paths.size() != out.classes.size()) in.fail(at + ".paths");
            for (std::size_t c = 0; c < paths.size(); ++c) {
                const auto cp = at + ".paths[" + std::to_string(c) + "]";
                if (!paths[c].is_array()) in.fail(cp);
                auto& list = out.paths.emplace_back();
                for (std::size_t k = 0; k < paths[c].size(); ++k) {
                    const auto fp = cp + "[" + std::to_string(k) + "]";
                    MacroPathFlow f;
                    const auto& cell = in.key(paths[c][k], fp, "cell");
                    if (!cell.is_number_unsigned()) in.fail(fp + ".cell");
                    f.cell = cell.get<std::size_t>();
                    const auto& links = in.array(paths[c][k], fp, "links");
                    for (std::size_t i = 0; i < links.size(); ++i) f.links.push_back(in.index(links[i], fp + ".links[" + std::to_string(i) + "]", out.arcs.size()));
                    f.vehiclesPerHour = in.number(in.key(paths[c][k], fp, "vehiclesPerHour"), fp + ".vehiclesPerHour", 0);
                    list.push_back(std::move(f));
                }
            }
        }
        side.periods.emplace(id, std::move(out));
    }
    return side;
}
MesoSide parseMesoSide(const Json& j, const std::string& where) {
    const Reader in{where};
    in.header(j, kMesoFormat);
    MesoSide side;
    side.projectSha256 = in.sha(j, "");
    const auto& s = in.key(j, "", "settings");
    side.settings.step = in.number(in.key(s, ".settings", "step"), ".settings.step", 0);
    side.settings.slice = in.number(in.key(s, ".settings", "slice"), ".settings.slice", 0);
    if (side.settings.step <= 0) in.fail(".settings.step");
    side.duration = in.number(in.key(j, "", "duration"), ".duration", 0);
    side.links = in.arcs(j, "", "links");
    const auto& classes = in.array(j, "", "classes");
    for (std::size_t c = 0; c < classes.size(); ++c) {
        const auto cp = ".classes[" + std::to_string(c) + "]";
        MesoClass cls;
        cls.id = in.text(in.key(classes[c], cp, "id"), cp + ".id");
        cls.pcu = in.number(in.key(classes[c], cp, "pcu"), cp + ".pcu", 0);
        if (cls.pcu <= 0) in.fail(cp + ".pcu");
        const auto& paths = in.array(classes[c], cp, "paths");
        for (std::size_t p = 0; p < paths.size(); ++p) {
            const auto pp = cp + ".paths[" + std::to_string(p) + "]";
            MesoPath path;
            path.source = in.text(in.key(paths[p], pp, "source"), pp + ".source");
            const auto& links = in.array(paths[p], pp, "links");
            for (std::size_t i = 0; i < links.size(); ++i) path.links.push_back(in.index(links[i], pp + ".links[" + std::to_string(i) + "]", side.links.size()));
            cls.paths.push_back(std::move(path));
        }
        side.classes.push_back(std::move(cls));
    }
    const auto& watchedLinks = in.array(j, "", "watchedLinks");
    for (std::size_t i = 0; i < watchedLinks.size(); ++i) side.watchedLinks.push_back(in.text(watchedLinks[i], ".watchedLinks[" + std::to_string(i) + "]"));
    const auto& watched = in.array(j, "", "watched");
    for (std::size_t w = 0; w < watched.size(); ++w) {
        const auto wp = ".watched[" + std::to_string(w) + "]";
        MesoLoading::Watched out;
        out.cls = in.index(in.key(watched[w], wp, "class"), wp + ".class", side.classes.size());
        out.path = in.index(in.key(watched[w], wp, "path"), wp + ".path", side.classes[out.cls].paths.size());
        out.position = in.index(in.key(watched[w], wp, "position"), wp + ".position", side.classes[out.cls].paths[out.path].links.size());
        out.entrance.step = in.number(in.key(watched[w], wp, "step"), wp + ".step", 0);
        if (out.entrance.step <= 0) in.fail(wp + ".step");
        const auto& at = in.array(watched[w], wp, "at");
        if (at.empty()) in.fail(wp + ".at");
        for (std::size_t k = 0; k < at.size(); ++k) out.entrance.at.push_back(in.number(at[k], wp + ".at[" + std::to_string(k) + "]", 0));
        side.watched.push_back(std::move(out));
    }
    side.inputManifest = in.key(j, "", "inputManifest");
    return side;
}
StudyAreaSources readStudyAreaSources(const std::filesystem::path& project) {
    StudyAreaSources s;
    const auto read = [&](const char* kind, std::shared_ptr<const Json>& json, std::string& sha) {
        std::ifstream in(sideFilePath(project, kind), std::ios::binary);
        if (!in) return;
        const std::string bytes(std::istreambuf_iterator<char>(in), {});
        auto parsed = Json::parse(bytes, nullptr, false);
        json = std::make_shared<const Json>(parsed.is_discarded() ? Json(nullptr) : std::move(parsed));
        sha = inputSha256(bytes);
    };
    read("macro", s.macro, s.macroSha256);
    read("meso", s.meso, s.mesoSha256);
    return s;
}
}
