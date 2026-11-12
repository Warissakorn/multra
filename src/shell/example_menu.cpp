#include "example_menu.hpp"
#include "../editor/path.hpp"
#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <stdexcept>

namespace multra {
std::vector<ExampleProject> loadExampleProjects(const std::filesystem::path& data) {
    const auto directory = data / "projects";
    QFile file(displayPath(data / "examples.json"));
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("EDIT_EXAMPLES_INDEX");
    const auto rows = QJsonDocument::fromJson(file.readAll()).object().value("examples").toArray();
    std::vector<ExampleProject> examples;
    for (const auto& value : rows) {
        const auto row = value.toObject();
        const auto name = row.value("name").toObject(), purpose = row.value("purpose").toObject();
        ExampleProject e{directory / nativePath(row.value("file").toString()), name.value("en").toString(),
                         name.value("th").toString(), purpose.value("en").toString(), purpose.value("th").toString()};
        if (row.value("file").toString().isEmpty() || e.nameEn.isEmpty() || e.nameTh.isEmpty() || e.purposeEn.isEmpty() ||
            e.purposeTh.isEmpty() || !std::filesystem::is_regular_file(e.file))
            throw std::runtime_error("EDIT_EXAMPLES_INDEX");
        examples.push_back(std::move(e));
    }
    if (examples.empty()) throw std::runtime_error("EDIT_EXAMPLES_INDEX");
    return examples;
}
void fillExampleMenu(QMenu& menu, const std::vector<ExampleProject>& examples, const QString& language,
                     const std::function<void(const std::filesystem::path&)>& open) {
    menu.clear();
    const bool thai = language == "th";
    for (const auto& e : examples) {
        auto* a = menu.addAction(thai ? e.nameTh : e.nameEn);
        a->setObjectName("editorExample:" + displayPath(e.file.filename()));
        a->setToolTip(thai ? e.purposeTh : e.purposeEn); a->setStatusTip(a->toolTip());
        QObject::connect(a, &QAction::triggered, &menu, [open, file = e.file] { open(file); });
    }
    menu.setToolTipsVisible(true);
}
}
