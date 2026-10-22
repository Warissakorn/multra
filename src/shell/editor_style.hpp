#pragma once
#include <QIcon>
#include <QFont>
#include <QString>
#include <filesystem>
class QWidget;
class QPainter;
class QColor;
class QTableView;
namespace multra {
enum class EditorIcon { document, open, save, undo, redo, fit, finish, rotate, remove,
    select, link, connector, route, input, signal, split, measure, image, run, pause,
    step, reset, inspector, objects, focus, grid, conflict, counter, section, zone };
QIcon editorIcon(EditorIcon icon);
// One glyph on the 20-unit grid in `ink`: the toolbar's icons and the canvas's tool cursors (D306).
void paintEditorGlyph(QPainter& painter,EditorIcon icon,const QColor& ink,double stroke);
// The complete editor QSS: palette(role) references only, no colour literal.
QString editorStyleSheet();
// Registers the bundled face (400 and 600) and makes it the application font at the body size.
// Throws when the files are missing. Idempotent (editor_appearance.cpp).
QFont loadEditorFont(const std::filesystem::path& data);
// Fusion as the application style; idempotent (editor_appearance.cpp).
void installEditorStyle();
// Where the dropdown/spin chevrons drawn from the palette live, for the style sheet's `image`.
QString editorGlyphDirectory();
void applyEditorStyle(QWidget* window);
// The table recipe (D84, D196): 24 px rows, alternating rows, no grid, control-height minimum
// columns. Applied to every table view when it is polished, dialogs and the Lab included; a table
// whose cells hold editors sets the property editorCustomRows and keeps its own row height.
void applyTableRecipe(QTableView* table);
}
