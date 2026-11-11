#pragma once
class QPainter;
class QColor;
namespace multra {
enum class EditorIcon { document, open, save, undo, redo, fit, finish, rotate, remove,
    select, link, connector, route, input, signal, split, measure, image, run, pause,
    step, reset, inspector, objects, focus, grid, conflict, counter, section, zone, road };
// One glyph on the 20-unit grid in `ink`: the toolbar's icons (shell `editorIcon`) and the
// canvas's tool cursors (D306). Lives in the editor so the canvas never includes the shell.
void paintEditorGlyph(QPainter& painter,EditorIcon icon,const QColor& ink,double stroke);
}
