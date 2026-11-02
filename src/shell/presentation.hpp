#pragma once
#include <functional>
#include <QString>
class QWidget;
class QMainWindow;
namespace multra {
// App-local presentation choices; they do not alter project data or canvas world scale.
void setEditorDensity(int density);
int editorDensity();
void installPresentationMenu(QMainWindow&,const std::function<QString(const char*)>&,
                             const std::function<void()>& relayout);
void translatePresentationMenu(QMainWindow&,const std::function<QString(const char*)>&);
}
