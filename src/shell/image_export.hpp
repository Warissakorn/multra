#pragma once
#include "../editor/canvas_image.hpp"
#include <QStringList>
#include <functional>
class QWidget;
namespace multra {
class EditorCanvas;
void writeNetworkPng(const QString&,const EditorCanvas&,const CanvasImageOptions&,const QStringList& protectedFiles={});
bool exportNetworkImageDialog(QWidget*,const EditorCanvas&,const std::function<QString(const char*)>&,
    const QStringList& protectedFiles={});
}
