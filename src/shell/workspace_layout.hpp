#pragma once
class QMainWindow;
namespace multra {
// Transfer palette width to the canvas; presets supply their intended Properties width.
void resizePalette(QMainWindow& window,int requested,int inspectorWidth=-1);
}
