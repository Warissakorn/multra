#pragma once
#include <QString>
#include <filesystem>
#include <functional>
#include <vector>
class QMenu;
namespace multra {
// One shipped example project, as data/examples.json lists it (D498), beside data/projects/. Names and purposes
// are data-borne text, stored in both languages with the data and picked at render time.
struct ExampleProject { std::filesystem::path file; QString nameEn, nameTh, purposeEn, purposeTh; };
// The index in file order. Throws std::runtime_error("EDIT_EXAMPLES_INDEX") when it is missing or
// malformed, or names a file that is not in data/projects/.
std::vector<ExampleProject> loadExampleProjects(const std::filesystem::path& data);
// Replaces the menu's entries with one action per example in `language` ("en" or "th"); the
// purpose is the action's tool tip and status tip. `open` receives the chosen file.
void fillExampleMenu(QMenu& menu, const std::vector<ExampleProject>& examples, const QString& language,
                     const std::function<void(const std::filesystem::path&)>& open);
}
