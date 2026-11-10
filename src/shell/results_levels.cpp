#include "results_levels.hpp"
#include "table_layout.hpp"
#include <QComboBox>
#include <QSignalBlocker>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVariant>
#include <utility>
#include <vector>

namespace multra {
namespace {
// The study chain's order within Macro: generation, distribution, mode choice, assignment, procedure.
const std::vector<std::pair<const char*, std::vector<const char*>>> levels{
    {"editorSeamMicro", {"editorResultsTabMovements", "editorResultsTabDischarge", "editorResultsTabClamps",
                         "editorResultsTabLots", "editorResultsTabNodes"}},
    {"editorSeamMeso", {"editorResultsTabMeso"}},
    {"editorSeamMacro", {"editorResultsTabTripGeneration", "editorResultsTabTripDistribution", "editorResultsTabModeChoice",
                         "editorResultsTabMacro", "editorResultsTabProcedure"}},
    {"editorResultsLevelNode", {"editorResultsTabNodeAnalysis"}}};
std::string keyOf(const ResultPages& pages, const QWidget* page) {
    for (const auto& [key, widget] : pages) if (widget == page) return key;
    return {};
}
QComboBox* pickerOf(const QTabWidget* tabs) {
    return tabs->parentWidget() ? tabs->parentWidget()->findChild<QComboBox*>("editorResultsLevel") : nullptr;
}
std::set<std::string> withoutDataOf(const QTabWidget* tabs) {
    std::set<std::string> keys;
    for (const auto& key : tabs->property("resultsWithoutData").toStringList()) keys.insert(key.toStdString());
    return keys;
}
}
std::string resultsLevelOf(const std::string& pageKey) {
    for (const auto& [level, keys] : levels) for (const auto* key : keys) if (pageKey == key) return level;
    return {};
}
void applyResultsLevel(QTabWidget* tabs, const ResultPages& pages, const std::set<std::string>& withoutData) {
    auto* picker = pickerOf(tabs);
    if (!picker) return;
    QStringList stored;
    for (const auto& key : withoutData) stored.push_back(QString::fromStdString(key));
    if (tabs->property("resultsWithoutData").toStringList() != stored) tabs->setProperty("resultsWithoutData", stored);
    const auto level = picker->currentData().toString().toStdString();
    int shown = 0;
    const auto* current = tabs->currentWidget();
    const auto set = [&](const std::string& key, QWidget* page) {
        const int index = tabs->indexOf(page);
        if (index < 0) return;
        const bool visible = resultsLevelOf(key) == level && (!withoutData.contains(key) || current == page);
        if (tabs->isTabVisible(index) != visible) tabs->setTabVisible(index, visible); // unchanged: no relayout (D487)
        shown += visible;
    };
    // The current tab first: while it is hidden, Qt makes the next tab shown the current one.
    for (const auto& [key, page] : pages) if (page == current) set(key, page);
    for (const auto& [key, page] : pages) if (page != current) set(key, page);
    if (tabs->tabBar()->isHidden() != (shown <= 1)) tabs->tabBar()->setVisible(shown > 1);
}
QComboBox* addResultsLevels(QTabWidget* tabs, const ResultPages& pages, QToolBar* bar) {
    int position = 0;
    for (const auto& [level, keys] : levels)
        for (const auto* key : keys)
            if (const auto found = pages.find(key); found != pages.end()) tabs->tabBar()->moveTab(tabs->indexOf(found->second), position++);
    auto* picker = new QComboBox(bar); picker->setObjectName("editorResultsLevel");
    for (const auto& [level, keys] : levels) picker->addItem(QString(), QString::fromLatin1(level));
    const auto before = bar->actions().isEmpty() ? nullptr : bar->actions().front();
    bar->insertWidget(before, picker); bar->insertSeparator(before);
    tabs->setCurrentIndex(0);
    // Picking a level shows its tabs and opens its first one.
    QObject::connect(picker, &QComboBox::currentIndexChanged, tabs, [tabs, picker, pages] {
        const auto level = picker->currentData().toString().toStdString();
        const auto hidden = withoutDataOf(tabs);
        if (resultsLevelOf(keyOf(pages, tabs->currentWidget())) != level)
            for (int i = 0; i < tabs->count(); ++i) {
                const auto key = keyOf(pages, tabs->widget(i));
                if (resultsLevelOf(key) == level && !hidden.contains(key)) { const QSignalBlocker block(picker); tabs->setCurrentIndex(i); break; }
            }
        if (resultsLevelOf(keyOf(pages, tabs->currentWidget())) != level) // every tab of the level is without data
            for (int i = 0; i < tabs->count(); ++i)
                if (resultsLevelOf(keyOf(pages, tabs->widget(i))) == level) { const QSignalBlocker block(picker); tabs->setCurrentIndex(i); break; }
        applyResultsLevel(tabs, pages, hidden);
    });
    // Making a tab current, from a command or a jump, picks its level.
    QObject::connect(tabs, &QTabWidget::currentChanged, picker, [tabs, picker, pages] {
        const auto level = resultsLevelOf(keyOf(pages, tabs->currentWidget()));
        const int index = picker->findData(QString::fromStdString(level));
        if (index >= 0 && index != picker->currentIndex()) { const QSignalBlocker block(picker); picker->setCurrentIndex(index); }
        applyResultsLevel(tabs, pages, withoutDataOf(tabs));
    });
    applyResultsLevel(tabs, pages, {});
    return picker;
}
void translateResultsLevels(QComboBox* picker, const std::function<QString(const char*)>& text) {
    if (!picker) return;
    for (int i = 0; i < picker->count(); ++i) picker->setItemText(i, text(picker->itemData(i).toString().toLatin1().constData()));
    picker->setAccessibleName(text("editorResultsLevel")); picker->setToolTip(text("editorResultsLevel"));
}
}
