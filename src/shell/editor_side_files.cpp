#include "editor_macro.hpp"
#include "editor_meso.hpp"
#include "editor_storage.hpp"
#include "editor_window.hpp"
#include "../project/input_manifest.hpp"
#include "../project/side_files.hpp"
#include <QFile>
#include <algorithm>

// S5.5a (STUDY_AREA §8-§9, D331): the side files beside the saved project. Run Macro and Run Meso
// write them; opening a project reads them; every Micro run, batch, comparison and readiness check
// hands them to the study area's cut. An unsaved project has none, and its runs stay in memory.
namespace multra {
namespace {
QString sidePath(const QString& project, const char* kind) {
    return QString::fromStdU16String(sideFilePath(std::filesystem::path(project.toStdU16String()), kind).u16string());
}
// The file's JSON (a JSON null when the bytes are not JSON: the cut refuses it) and its bytes' SHA-256;
// no file, or one too large to be a side file, reads as none.
std::pair<std::shared_ptr<const Json>, std::string> readSide(const QString& file) {
    QFile input(file);
    if (file.isEmpty() || !input.exists() || !input.open(QIODevice::ReadOnly) || input.size() > 256 * 1024 * 1024) return {};
    const auto bytes = input.readAll().toStdString();
    auto json = Json::parse(bytes, nullptr, false);
    if (json.is_discarded()) json = nullptr;
    return {std::make_shared<const Json>(std::move(json)), inputSha256(bytes)};
}
std::string writeSide(const QString& file, const Json& json) {
    const auto bytes = json.dump() + "\n";
    writeEditorBytes(file, bytes, "EDIT_SIDE_FILE_WRITE");
    return inputSha256(bytes);
}
}
void EditorWindow::loadSideFiles() {
    std::tie(macro_->side, macro_->sideSha256) = readSide(file_.isEmpty() ? QString() : sidePath(file_, "macro"));
    std::tie(meso_->side, meso_->sideSha256) = readSide(file_.isEmpty() ? QString() : sidePath(file_, "meso"));
    ++macro_->serial; ++meso_->serial;
}
StudyAreaSources EditorWindow::studyAreaSources() const {
    return {macro_->side, meso_->side, macro_->sideSha256, meso_->sideSha256};
}
const std::string& EditorWindow::projectSha() const {
    if (macro_->hashedRevision != history_.revision() || macro_->hashed.empty()) {
        macro_->hashed = projectSha256(history_.document()); macro_->hashedRevision = history_.revision();
    }
    return macro_->hashed;
}
void EditorWindow::writeMacroSide() {
    if (file_.isEmpty() || !macro_->result) return;
    try {
        const auto json = macroSideJson(*macro_->result, macro_->resultSha256, macro_->side ? *macro_->side : Json());
        macro_->sideSha256 = writeSide(sidePath(file_, "macro"), json);
        macro_->side = std::make_shared<const Json>(json);
    } catch (const std::exception& e) { showError(e); }
}
void EditorWindow::writeMesoSide(const std::vector<std::string>& watched) {
    if (file_.isEmpty() || !meso_->result) return;
    try {
        const auto json = mesoSideJson(*meso_->result, meso_->resultSha256, watched);
        meso_->sideSha256 = writeSide(sidePath(file_, "meso"), json);
        meso_->side = std::make_shared<const Json>(json);
    } catch (const std::exception& e) { showError(e); }
}
// Parsed once per (side file, document) pair: refreshRun asks on every edit.
bool EditorWindow::macroSideStale() const {
    if (!macro_->side) return false;
    if (auto key = macro_->sideSha256 + projectSha(); key != macro_->staleKey) {
        macro_->staleKey = std::move(key);
        try {
            const auto periods = parseMacroSide(*macro_->side).periods;
            macro_->sideStale = std::any_of(periods.begin(), periods.end(), [&](const auto& p) { return p.second.projectSha256 != projectSha(); });
        } catch (const std::exception&) { macro_->sideStale = true; }
    }
    return macro_->sideStale;
}
bool EditorWindow::mesoSideStale() const {
    if (!meso_->side) return false;
    if (auto key = meso_->sideSha256 + projectSha(); key != meso_->staleKey) {
        meso_->staleKey = std::move(key);
        try { meso_->sideStale = parseMesoSide(*meso_->side).projectSha256 != projectSha(); } catch (const std::exception&) { meso_->sideStale = true; }
    }
    return meso_->sideStale;
}
}
