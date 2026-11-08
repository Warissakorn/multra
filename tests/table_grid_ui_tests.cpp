#include "../src/shell/editor_window.hpp"
#include "../src/shell/table_grid.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QTest>
#include <filesystem>
#include <iostream>
using namespace multra;
// D490: every table reads like a spreadsheet -- grid, numbered rows, cell blocks where several
// cells may be selected, Ctrl+C as Excel's tab-separated text and Ctrl+A kept by a focused table.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,"Missing widget");return value;}
void settle(){QApplication::processEvents();QTest::qWait(20);}
void block(QTableWidget* table,int top,int left,int bottom,int right) {
    table->selectionModel()->setCurrentIndex(table->model()->index(top,left),QItemSelectionModel::NoUpdate);
    table->selectionModel()->select(QItemSelection(table->model()->index(top,left),table->model()->index(bottom,right)),QItemSelectionModel::ClearAndSelect);
    settle();
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1360,860);window.show();
        window.activateWindow();require(QTest::qWaitForWindowActive(&window),"Window never became active");settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
        item<QAction>(window,"editorObjects")->setChecked(true);settle();

        // The grid look: lines drawn, no alternating rows, numbered rows, a select-all corner.
        auto* links=item<QTableWidget>(window,"editorLinkTable");
        require(links->showGrid()&&!links->alternatingRowColors(),"The link table is not a grid");
        require(links->verticalHeader()->isVisibleTo(links)&&links->isCornerButtonEnabled(),"The link table has no row header");
        require(links->model()->headerData(0,Qt::Vertical).toString()=="1","Rows are not numbered from 1");
        // Cell blocks where several cells may be selected; a one-object list keeps its row.
        require(links->selectionBehavior()==QAbstractItemView::SelectItems,"The link table selects whole rows");
        auto* routes=item<QTableWidget>(window,"editorRouteTable");
        require(routes->selectionMode()==QAbstractItemView::SingleSelection&&routes->selectionBehavior()==QAbstractItemView::SelectRows,"A single-selection table lost row selection");

        // A block of cells away from the ID column picks every row it touches on the canvas.
        require(links->rowCount()>=3,"Fixture has too few links");
        block(links,0,2,1,3);
        require(links->selectionModel()->selectedIndexes().size()==4,"The cell block did not take");
        require(links->selectionModel()->selectedRows().isEmpty(),"The block selected whole rows");
        require(selectedTableRows(links)==std::vector<int>{0,1},"Selected rows of a cell block are wrong");
        auto* canvas=window.canvas();
        const auto id=[&](int row){return links->item(row,0)->data(Qt::UserRole).toString().toStdString();};
        require(canvas->selection().size()==2&&canvas->isSelected(id(0))&&canvas->isSelected(id(1)),"A cell block did not select its rows' objects");

        // Ctrl+C on the focused table: headers, then the block, tab-separated.
        QApplication::clipboard()->setText(QStringLiteral("sentinel"));
        require(QApplication::clipboard()->text()=="sentinel","The test clipboard does not hold text");
        block(links,0,2,1,3);links->setFocus();settle();
        require(links->hasFocus(),"The link table did not take focus");
        QTest::keyClick(links,Qt::Key_C,Qt::ControlModifier);settle();
        const auto header=[&](int c){return links->horizontalHeaderItem(c)->text();};
        const QString expected=header(2)+'\t'+header(3)+'\n'+links->item(0,2)->text()+'\t'+links->item(0,3)->text()+'\n'
            +links->item(1,2)->text()+'\t'+links->item(1,3)->text()+'\n';
        require(QApplication::clipboard()->text()==expected,"Ctrl+C did not copy the block as Excel text");
        // One cell copies only its value.
        block(links,1,3,1,3);
        QTest::keyClick(links,Qt::Key_C,Qt::ControlModifier);settle();
        require(QApplication::clipboard()->text()==links->item(1,3)->text(),"One cell did not copy its value");

        // Ctrl+A belongs to the focused table; elsewhere it still toggles the wireframe view.
        auto* wireframe=item<QAction>(window,"editorToggleWireframe");
        canvas->setFocus();settle();const bool before=wireframe->isChecked();
        QTest::keyClick(canvas,Qt::Key_A,Qt::ControlModifier);settle();
        require(wireframe->isChecked()!=before,"Ctrl+A on the canvas no longer toggles the wireframe");
        QTest::keyClick(canvas,Qt::Key_A,Qt::ControlModifier);settle();
        links->setFocus();settle();
        QTest::keyClick(links,Qt::Key_A,Qt::ControlModifier);settle();
        require(wireframe->isChecked()==before,"Ctrl+A in a table toggled the wireframe");
        require(links->selectionModel()->selectedIndexes().size()==links->rowCount()*links->columnCount(),"Ctrl+A did not select every cell");

        // The Excel text: a field with a tab or quote is quoted, an unselected cell in the block
        // stays empty, named rows lead with their label, and a cell's editor gives its text.
        QTableWidget sheet(2,3);sheet.show();settle();
        require(sheet.showGrid()&&sheet.selectionBehavior()==QAbstractItemView::SelectItems,"A late table missed the grid recipe");
        sheet.setHorizontalHeaderLabels({"A","B","C"});sheet.setVerticalHeaderLabels({"Zone 1","Zone 2"});
        sheet.setItem(0,0,new QTableWidgetItem("a\tb"));sheet.setItem(0,1,new QTableWidgetItem("say \"hi\""));
        sheet.setItem(1,0,new QTableWidgetItem("x"));
        auto* combo=new QComboBox;combo->addItems({"left","right"});combo->setCurrentIndex(1);sheet.setCellWidget(1,2,combo);
        sheet.selectionModel()->select(sheet.model()->index(0,0),QItemSelectionModel::Select);
        sheet.selectionModel()->select(sheet.model()->index(0,1),QItemSelectionModel::Select);
        sheet.selectionModel()->select(sheet.model()->index(1,2),QItemSelectionModel::Select);
        require(sheet.selectionModel()->selectedIndexes().size()==3,"The scattered selection did not take");
        require(tableSelectionText(&sheet)==QStringLiteral("\tA\tB\tC\nZone 1\t\"a\tb\"\t\"say \"\"hi\"\"\"\t\nZone 2\t\t\tright\n"),"Excel text is wrong");
        std::cout<<"Tables: grid, cell blocks, Ctrl+C as Excel text and table-owned Ctrl+A passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
