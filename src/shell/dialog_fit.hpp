#pragma once
class QDialog;
class QScrollArea;
class QWidget;
namespace multra {
// A dialog taller than its screen hides its own buttons (a 1366x768 laptop, or 1080p at 150 %,
// leaves about 650 px). These keep the buttons on screen whatever the content's length.
//
// scrollPage: content in a scroll area that asks for the content's full size, so a dialog that
// fits opens without a scroll bar, and one that does not scrolls instead of growing off screen.
QScrollArea* scrollPage(QWidget* content, QWidget* parent);
// scrollingBody: gives `dialog` an outer layout with a scrolling body, and returns the body for the
// caller's own layout. A footer added with addFooter stays below the body and never scrolls.
QWidget* scrollingBody(QDialog& dialog);
void addFooter(QDialog& dialog, QWidget* footer);
// On its first show, the dialog's size is capped to 90 % of its screen's available area; Qt then
// centres it there. scrollingBody already calls it.
void fitToScreen(QDialog& dialog);
}
