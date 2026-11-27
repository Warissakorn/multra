// UI_GEOMETRY (D680): each check of ui_geometry_support.hpp fires on a dialog built to break it, and a plain form passes.
// Rows UG1-UG6; the forcing rows are what make "no findings" on the real dialogs mean something.
#include "ui_geometry_support.hpp"
#include "../src/shell/editor_style.hpp"
#include "../src/shell/table_grid.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
bool has(const std::vector<std::string>& found, const char* check) {
    for (const auto& f : found) if (f.rfind(check, 0) == 0) return true;
    return false;
}
QTableWidget* table(QWidget* parent) {
    auto* t = new QTableWidget(2, 2, parent);
    for (int r = 0; r < 2; ++r) for (int c = 0; c < 2; ++c) t->setItem(r, c, new QTableWidgetItem("1.5"));
    return t;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        {   // UG1: the style's cell padding with Qt's own delegate puts the editor off its cell; the recipe's delegate does not.
            QDialog d; d.setStyleSheet(multra::editorStyleSheet()); auto* box = new QVBoxLayout(&d);
            auto* plain = table(&d); box->addWidget(plain); d.resize(400, 300); d.show(); QApplication::processEvents();
            require(has(uiGeometry::findings(d), "G1"), "UG1: Qt's delegate under the cell padding not caught");
            multra::makeSpreadsheetTable(plain); QApplication::processEvents();
            require(!has(uiGeometry::findings(d), "G1"), "UG1: the recipe's delegate still caught");
        }
        {   // UG2: a child placed past its parent's edge.
            QDialog d; d.resize(200, 100); auto* child = new QPushButton("x", &d); child->setGeometry(180, 10, 60, 24);
            d.show(); QApplication::processEvents();
            require(has(uiGeometry::findings(d), "G2"), "UG2: a clipped child not caught");
        }
        {   // UG3: two widgets in one grid cell.
            QDialog d; auto* grid = new QGridLayout(&d);
            grid->addWidget(new QLineEdit(&d), 0, 0); grid->addWidget(new QLineEdit(&d), 0, 0);
            d.show(); QApplication::processEvents();
            require(has(uiGeometry::findings(d), "G3"), "UG3: overlapping siblings not caught");
        }
        {   // UG4: a label and a button narrower than their text.
            QDialog d; auto* box = new QHBoxLayout(&d);
            auto* label = new QLabel("a label far wider than twenty pixels", &d); label->setFixedWidth(20); box->addWidget(label);
            d.show(); QApplication::processEvents();
            require(has(uiGeometry::findings(d), "G4"), "UG4: a clipped label not caught");
            QDialog e; auto* row = new QHBoxLayout(&e);
            auto* button = new QPushButton("a button far wider than twenty pixels", &e); button->setFixedWidth(20); row->addWidget(button);
            e.show(); QApplication::processEvents();
            require(has(uiGeometry::findings(e), "G4"), "UG4: a clipped button not caught");
        }
        {   // UG6 (D683): a widget set on a cell that cannot shrink to its row (the old 24 px field rule) is caught;
            // under the editor style a combo on a cell fills it.
            QDialog d; d.setStyleSheet(multra::editorStyleSheet()); auto* box = new QVBoxLayout(&d);
            auto* t = table(&d); multra::makeSpreadsheetTable(t); box->addWidget(t);
            auto* pinned = new QPushButton("x"); pinned->setFixedHeight(t->rowHeight(0) + 6); t->setCellWidget(0, 0, pinned);
            d.resize(400, 300); d.show(); QApplication::processEvents();
            require(has(uiGeometry::findings(d), "G1"), "UG6: a cell widget taller than its row not caught");
            auto* combo = new QComboBox; combo->addItems({"major", "minor"}); t->setCellWidget(0, 0, combo); QApplication::processEvents();
            const auto found = uiGeometry::findings(d);
            for (const auto& f : found) std::cerr << f << '\n';
            require(found.empty(), "UG6: a combo on a cell under the editor style does not fill it");
        }
        {   // UG5: a plain form under the editor style passes.
            QDialog d; d.setStyleSheet(multra::editorStyleSheet()); auto* form = new QFormLayout(&d);
            form->addRow("Name", new QLineEdit(&d)); form->addRow("Speed", new QLineEdit(&d)); form->addRow(new QPushButton("Confirm", &d));
            auto* t = table(&d); multra::makeSpreadsheetTable(t); form->addRow(t);
            d.show(); QApplication::processEvents();
            const auto found = uiGeometry::findings(d);
            for (const auto& f : found) std::cerr << f << '\n';
            require(found.empty(), "UG5: a plain form has findings");
        }
        std::cout << "UI geometry tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
