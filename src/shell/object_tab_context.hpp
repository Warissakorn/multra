#pragma once
#include "../project/document.hpp"
#include <QString>
#include <cstdint>
#include <functional>
#include <string>

class QAction;
class QWidget;
namespace multra {
// D521: what an object tab's panel needs from the editor window. The window builds one per panel;
// the panel keeps no pointer to the window. Actions are registered with the window, so the command
// palette, automation and translation find them by key as before.
struct ObjectTabContext {
    std::function<const ProjectDocument&()> document;
    std::function<std::uint64_t()> revision; // History::revision(): the rows are rebuilt when it moves
    std::function<QString(const std::string&)> text;
    std::function<QAction*(const std::string& key, const std::function<void()>&)> action;
    // One History step; false when refused (the window shows why). commitChange is the form of it
    // a validated dialog calls, so the refusal shows in the dialog instead.
    std::function<bool(const std::string&, const std::function<void(ProjectDocument&)>&)> execute, commitChange;
    std::function<void()> selectionChanged; // the window's refresh(false)
    std::function<void(QWidget*)> show;     // brings this page forward in the object tabs
};
}
