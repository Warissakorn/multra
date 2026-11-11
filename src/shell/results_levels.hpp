#pragma once
#include <QString>
#include <functional>
#include <map>
#include <set>
#include <string>
class QComboBox;
class QTabWidget;
class QToolBar;
class QWidget;
namespace multra {
// D497: Results shows one level's tabs at a time (Micro, Meso, Macro, Node), chosen in a picker at
// the head of the Results toolbar, as the Objects panel shows one task group (D194). `pages` maps a
// tab's locale key to its page.
using ResultPages = std::map<std::string, QWidget*>;
// Orders the tabs by level, adds the picker (object name editorResultsLevel) to `bar` and keeps the
// two in step: picking a level shows its tabs, and making any tab current picks its level.
QComboBox* addResultsLevels(QTabWidget* tabs, const ResultPages& pages, QToolBar* bar);
// Applies the picked level. A page named in `withoutData` is hidden while it has nothing to show;
// the current tab always stays. A level left with one tab shows no strip (D462).
void applyResultsLevel(QTabWidget* tabs, const ResultPages& pages, const std::set<std::string>& withoutData);
void translateResultsLevels(QComboBox* picker, const std::function<QString(const char*)>& text);
// The level key (editorSeamMicro, ...) a page's key belongs to; empty when it is not a Results tab.
std::string resultsLevelOf(const std::string& pageKey);
}
