#include "editor_style.hpp"
#include "table_grid.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QApplication>
#include <QHeaderView>
#include <QTableView>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QString>
#include <QWidget>
#include <algorithm>
#include <map>
#include <utility>
namespace multra {
QIcon editorIcon(EditorIcon icon) {
    static std::map<EditorIcon,QIcon> cache;
    if(const auto found=cache.find(icon);found!=cache.end())return found->second;
    QIcon result;
    // 16 px logical, rastered at 1x/2x/3x so QIcon picks the size matching the display. Glyphs
    // are drawn on a 20-unit grid, so a 1.5 px stroke is 1.5*20/16 grid units at every raster.
    constexpr double stroke=editorDesign::iconStroke*20./editorDesign::iconSize;
    for(int ratio:{1,2,3}) {
        const int size=editorDesign::iconSize*ratio;
        QPixmap pixmap(size,size);pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);p.scale(size/20.,size/20.);
        paintEditorGlyph(p,icon,editorDesign::role(QPalette::WindowText),stroke);
        p.end();result.addPixmap(pixmap);
    }
    cache.emplace(icon,result);return result;
}
// Every colour is a palette(role) reference, so the QSS holds no hex literal and follows the one
// editorPalette(). Box model: a control is 1 px border + 2 px padding + 18 px content = 24 px, and
// the 2 px focus / invalid border takes 1 px of padding back so a control never changes height.
// Fields and buttons are PINNED at 24 px (min-height = max-height, no vertical padding): the
// bundled Thai face has a 20 px line at 13 px, so padding-based heights came out 26-28 px, and
// Qt adds its own few pixels to spin boxes differently per version. A pinned height is the same
// for every font, language and Qt release; the text is centred in it by the widget.
// Qt's stylesheet style adds 3 px to a QToolButton's content before the box model, so a toolbar
// button is 16 icon + 3 + 3 padding + 2 border = 24 px. The odd pixel of padding sits top/left
// because Qt centres the icon in the 19 px content box with the extra pixel below/right.
// Qt Style Sheets have no letter-spacing, transition or shadow property; tracking is set on the
// label QFont (styleGroupLabel), and nothing here animates.
// State language, one for every surface: hover is a Midlight fill only, pressed a Mid fill,
// checked/selected a Mid fill plus an accent marker on one edge (bottom for buttons and tabs,
// left for list rows; a toolbar button's 2 px underline takes its pixel from the bottom
// padding), and only keyboard :focus draws an accent frame (a focused tab turns accent). No
// hover, pressed or checked state changes a control's height, so nothing shifts. Weight: 600 for group and table
// headers, dock and section titles, the command title and the Run action; everything else 400,
// and a selected tab or menu item is never bolded (its width would jump).
QString editorStyleSheet() {
    QString stylesheet=QStringLiteral(R"(
        QWidget { color: palette(text); font-size: @fontBodypx; }
        QMainWindow::separator { background: palette(midlight); width: @space1px; height: @space1px; border-left: 1px solid palette(mid); border-right: 1px solid palette(mid); }
        QMainWindow::separator:hover { background: palette(mid); }
        QMenuBar { background: palette(base); padding: 0 @space1px; border-bottom: 1px solid palette(mid); min-height: @controlHeightpx; }
        QMenuBar::item { padding: 2px @space2px; margin: 2px 0; border-radius: @radiuspx; background: transparent; }
        QMenuBar::item:selected, QMenu::item:selected { background: palette(midlight); color: palette(text); }
        QMenuBar::item:pressed { background: palette(mid); }
        QMenu { background: palette(base); border: 1px solid palette(mid); padding: @space1px; }
        QMenu::item { min-height: @controlContentpx; padding: @padYpx @space6px; border-radius: @radiuspx; }
        QMenu::separator { height: 1px; background: palette(mid); margin: @space1px @space2px; }
        QToolBar { background: palette(base); border: 0; spacing: @space1px; padding: @space1px; }
        QToolBar::separator { background: palette(mid); width: 1px; margin: @space1px; }
        /* Under the toolbar row above the panels: the panels' own separator band (4 px + its two 1 px edges).
           Qt's toolbar layout ignores a bottom-only border, so layoutToolbars adds its height (toolbarBand). */
        QToolBar[editorLastRow="true"] { border-bottom: @panelGappx solid palette(midlight); }
        QToolButton, QPushButton { min-height: @fieldContentpx; max-height: @fieldContentpx; padding: 0 @space2px; border: 1px solid palette(dark); border-radius: @radiuspx; background: palette(button); }
        QToolBar QToolButton { min-width: @toolbarIconpx; min-height: @iconpx; border-color: transparent; background: transparent; padding: 2px 1px 1px 2px; }
        QToolButton:hover, QPushButton:hover { background: palette(midlight); }
        QToolButton:pressed, QPushButton:pressed { background: palette(mid); }
        QToolButton:checked, QPushButton:checked { background: palette(mid); border-bottom-color: palette(highlight); }
        QToolBar QToolButton:checked { border-color: transparent; border-bottom: 2px solid palette(highlight); padding-bottom: 0; }
        QToolButton:focus, QPushButton:focus { border: 2px solid palette(highlight); padding: 0 7px; min-height: @fieldFocuspx; max-height: @fieldFocuspx; }
        QToolBar QToolButton:focus { padding: 1px 0 0 1px; }
        QToolButton:disabled, QPushButton:disabled { color: palette(window-text); background: palette(midlight); border-color: palette(mid); }
        QToolButton#editorRunButton { background: palette(midlight); border-color: palette(highlight); color: palette(text); font-weight: 600; }
        QToolButton#editorRunButton:hover, QToolButton#editorRunButton:pressed { background: palette(mid); }
        QToolBar QToolButton#editorRunButton, QToolBar QToolButton#editorRunButton:focus { padding: 0 @space2px; }
        QDockWidget { border: 1px solid palette(mid); }
        QDockWidget::title { background: palette(window); min-height: @controlContentpx; padding: 1px @space2px; color: palette(text); font-size: @fontBodypx; font-weight: 600; }
        QGroupBox { font-size: @fontSectionpx; font-weight: 600; }
        QGroupBox QWidget { font-weight: 400; }
        QScrollArea, QTabWidget::pane { border: 0; background: palette(base); }
        QGraphicsView { background: palette(base); border: 1px solid palette(mid); }
        QGraphicsView:focus { border: 2px solid palette(highlight); }
        QTabBar::tab { min-height: @controlContentpx; padding: @padYpx @space2px; border-bottom: 1px solid transparent; color: palette(window-text); }
        QTabBar::tab:selected { color: palette(text); border-bottom: 2px solid palette(highlight); background: palette(base); }
        QTabBar::tab:hover { background: palette(midlight); }
        QTabBar::tab:focus { color: palette(highlight); }
        QLineEdit, QComboBox, QAbstractSpinBox { min-height: @fieldContentpx; max-height: @fieldContentpx; background: palette(base); border: 1px solid palette(dark); border-radius: @radiuspx; padding: 0 @space2px; selection-background-color: palette(highlight); selection-color: palette(highlighted-text); }
        QAbstractSpinBox, QLineEdit[numeric="true"], QComboBox[numeric="true"] { font-size: @fontNumericpx; }
        QAbstractSpinBox, QLineEdit[numeric="true"] { qproperty-alignment: 'AlignRight | AlignVCenter'; }
        QLineEdit:hover, QComboBox:hover, QAbstractSpinBox:hover { border-color: palette(window-text); }
        QLineEdit:focus, QComboBox:focus, QAbstractSpinBox:focus { border: 2px solid palette(highlight); padding: 0 7px; min-height: @fieldFocuspx; max-height: @fieldFocuspx; }
        QLineEdit:read-only { background: palette(midlight); color: palette(window-text); }
        QLineEdit:disabled, QComboBox:disabled, QAbstractSpinBox:disabled { background: palette(midlight); color: palette(window-text); border-color: palette(mid); }
        QAbstractSpinBox[validationState="invalid"], QLineEdit[validationState="invalid"] { border: 2px solid palette(bright-text); padding: 0 7px; min-height: @fieldFocuspx; max-height: @fieldFocuspx; }
        /* An editable combo's own line edit is not a second framed field: no border, height or padding of its own,
           so its text and clear button centre inside the combo's frame. */
        QComboBox QLineEdit, QComboBox QLineEdit:focus, QComboBox QLineEdit:hover { border: 0; padding: 0; min-height: 0; max-height: 16777215; background: transparent; }
        /* D460: a line edit's clear button is a QToolButton too; the field-height rule above made it 24 px
           tall from where Qt centred an 18 px one, so its X sat 3 px low. It keeps Qt's own size. */
        QLineEdit QToolButton, QLineEdit QToolButton:focus, QLineEdit QToolButton:hover { min-height: 0; max-height: 16777215; min-width: 0; padding: 0; border: none; background: transparent; }
        QComboBox::drop-down { width: @space5px; border: 0; border-left: 1px solid palette(mid); }
        QComboBox::down-arrow { image: url(@glyphs/down.png); width: 8px; height: 5px; }
        QComboBox::down-arrow:disabled { image: url(@glyphs/down-off.png); }
        QAbstractSpinBox::up-button, QAbstractSpinBox::down-button { width: @space4px; border: 0; border-left: 1px solid palette(mid); background: transparent; }
        QAbstractSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; }
        QAbstractSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; }
        QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background: palette(midlight); }
        QAbstractSpinBox::up-button:pressed, QAbstractSpinBox::down-button:pressed { background: palette(mid); }
        QAbstractSpinBox::up-arrow { image: url(@glyphs/up.png); width: 8px; height: 5px; }
        QAbstractSpinBox::down-arrow { image: url(@glyphs/down.png); width: 8px; height: 5px; }
        QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::up-arrow:off { image: url(@glyphs/up-off.png); }
        QAbstractSpinBox::down-arrow:disabled, QAbstractSpinBox::down-arrow:off { image: url(@glyphs/down-off.png); }
        QTreeWidget, QListWidget { border: 0; background: palette(base); outline: 0; }
        QTreeWidget::item, QListWidget::item { min-height: @controlContentpx; padding: @padYpx @space1px; border: 1px solid transparent; }
        QTreeWidget::item:hover, QListWidget::item:hover { background: palette(midlight); }
        QTreeWidget::item:selected, QListWidget::item:selected { background: palette(mid); color: palette(text); }
        /* One accent bar per row, on its first cell: not one per column, nor on the tree's branch area. */
        QTreeWidget::item:selected:first, QTreeWidget::item:selected:only-one, QListWidget::item:selected { border-left: 2px solid palette(highlight); }
        QTreeWidget::branch:selected { background: palette(mid); border: 0; }
        QTreeWidget::item:focus:!selected, QListWidget::item:focus:!selected { border: 1px solid palette(highlight); }
        QTableView { background: palette(base); alternate-background-color: palette(window); border: 1px solid palette(dark); selection-background-color: palette(mid); selection-color: palette(text); gridline-color: palette(mid); }
        /* D490: a spreadsheet grid -- the view draws every cell line, the current cell has a frame,
           the row header numbers rows and a selected cell lights its row and column headers. */
        QTableView::item { min-height: @controlContentpx; padding: @padYpx @space1px; }
        QTableView::item:hover { background: palette(midlight); }
        QTableView::item:focus { border: 2px solid palette(highlight); }
        QHeaderView::section { min-height: @controlContentpx; background: palette(window); color: palette(text); border: 0; border-right: 1px solid palette(mid); border-bottom: 1px solid palette(mid); padding: @padYpx @space2px; font-size: @fontLabelpx; font-weight: 600; }
        QHeaderView::section:vertical { padding: 0 @space1px; color: palette(window-text); font-weight: 400; }
        QHeaderView::section:checked { background: palette(mid); color: palette(text); }
        QTableView QHeaderView { background: palette(base); } /* no grey band past the last row or column */
        QTableView QTableCornerButton::section { background: palette(window); border: 0; border-right: 1px solid palette(mid); border-bottom: 1px solid palette(mid); }
        QLabel#editorScope { background: palette(window); color: palette(text); padding: @space1px @space2px; border-bottom: 1px solid palette(mid); }
        QLabel#editorRunInfo { background: palette(base); color: palette(text); padding: @space1px @space2px; }
        QLabel#editorError, QLabel[validationState="invalid"], QLabel[status="error"] { color: palette(bright-text); padding: @space1px @space2px; }
        QToolButton#editorMessageClose, QToolButton#editorMessageClose:focus { min-height: 0; max-height: none; min-width: 0; padding: 0; border: none; background: transparent; } /* D429 */
        QLabel[status="warning"] { color: palette(link-visited); }
        QLabel[status="advisory"] { color: palette(link); }
        QLabel[status="ok"] { color: palette(shadow); }
        QLabel#editorPaletteHint, QLabel#editorToolHint { color: palette(window-text); font-size: @fontLabelpx; }
        QFrame#editorApplyDivider { color: palette(mid); }
        QLabel[editorEyebrow="true"] { color: palette(window-text); font-size: @fontLabelpx; font-weight: 600; }
        QLabel#editorCommandTitle { color: palette(text); font-size: @fontTitlepx; font-weight: 600; }
        QLabel[numeric="true"] { font-size: @fontNumericpx; }
        QStatusBar { background: palette(base); border-top: 1px solid palette(mid); color: palette(text); min-height: @controlHeightpx; }
        QStatusBar::item { border: 0; }
        QToolTip { background: palette(text); color: palette(base); border: 1px solid palette(text); padding: @space1px @space2px; }
    )");
    const int density=editorDesign::presentationDensity(),height=editorDesign::controlHeight+4*density;
    const std::pair<const char*,int> sizes[]={
        {"@fontLabel",editorDesign::fontSizeLabel+density},{"@fontNumeric",editorDesign::fontSizeNumeric+density},
        {"@fontBody",editorDesign::fontSizeBody+density},{"@fontTitle",editorDesign::fontSizeTitle},
        {"@fontSection",editorDesign::fontSizeSection},
        {"@panelGap",editorDesign::toolbarBand},{"@space1",editorDesign::space1},{"@space2",editorDesign::space2},{"@space4",editorDesign::space4},
        {"@space5",editorDesign::space5},{"@space6",editorDesign::space6},
        {"@controlHeight",height},{"@controlContent",height-2*editorDesign::controlPaddingY-2},
        {"@padY",editorDesign::controlPaddingY},
        {"@fieldContent",height-2},{"@fieldFocus",height-4},
        {"@radius",2},{"@toolbarIcon",editorDesign::iconSize+4*density},{"@icon",editorDesign::iconSize}
    };
    // No token is a prefix of another ("@space1" vs "@space6", "@controlHeight" vs "@controlContent").
    for(const auto& [name,value]:sizes)stylesheet.replace(QLatin1String(name),QString::number(value));
    stylesheet.replace(QLatin1String("@glyphs"),editorGlyphDirectory());
    return stylesheet;
}
void applyTableRecipe(QTableView* table) {
    table->setAlternatingRowColors(false);table->setShowGrid(true);table->setGridStyle(Qt::SolidLine);
    makeSpreadsheetTable(table);
    if(!table->property("editorCustomRows").toBool())table->verticalHeader()->setDefaultSectionSize(editorDesign::tableRowHeight+4*editorDesign::presentationDensity());
    table->horizontalHeader()->setMinimumSectionSize(editorDesign::controlHeight);
}
void applyEditorStyle(QWidget* window) {
    installEditorStyle();
    // Application-wide as well: with a stylesheet on the window, Qt 6.4 resolved a child's
    // palette(role) against the default palette (Mid #b8b8b8, Highlight #308cc6), not this one.
    QApplication::setPalette(editorDesign::editorPalette());
    window->setPalette(editorDesign::editorPalette());
    window->setStyleSheet(editorStyleSheet());
}
}
