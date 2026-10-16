#include "../src/shell/editor_window.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QHeaderView>
#include <QHelpEvent>
#include <QLineEdit>
#include <QRegularExpression>
#include <QScreen>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTest>
#include <QTextEdit>
#include <QTimer>
#include <filesystem>
#include <iostream>
using namespace trafficsim;
// UI slice d2: one table recipe for every table, including those built after startup; a full-text
// tooltip on elided cells; figures that read the same in either language.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,"Missing table widget");return value;}
void settle(){QApplication::processEvents();QTest::qWait(20);}
bool recipe(const QTableView* table) {
    return table->verticalHeader()->defaultSectionSize()==editorDesign::tableRowHeight&&table->alternatingRowColors()&&!table->showGrid();
}
QTextEdit* popup(EditorWindow& window) {
    for(auto* widget:QApplication::topLevelWidgets())
        if(widget->objectName()=="editorBoundedTooltip"&&widget->isVisible())return qobject_cast<QTextEdit*>(widget);
    if(auto* own=window.findChild<QTextEdit*>("editorBoundedTooltip"))if(own->isVisible())return own;
    return nullptr;
}
void hover(QTableWidget* table,int row,int column) {
    const auto at=table->visualRect(table->model()->index(row,column)).center();
    QHelpEvent help(QEvent::ToolTip,at,table->viewport()->mapToGlobal(at));
    QApplication::sendEvent(table->viewport(),&help);settle();
}
const QRegularExpression figure(QStringLiteral("^-?[0-9]+(\\.[0-9]+)?$"));
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
        // Startup tables keep the recipe; a table built after the window is shown gets it too.
        require(recipe(item<QTableWidget>(window,"editorLinkTable")),"Startup table lost the recipe");
        auto* late=new QTableWidget(3,2,&window);late->setWindowFlags(Qt::Window);late->show();settle();
        require(recipe(late),"A table built after startup missed the recipe");
        delete late;
        // A real dialog's tables, built when its action runs.
        bool inspected=false;
        QTimer::singleShot(300,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if(!dialog)return;
            const auto tables=dialog->findChildren<QTableWidget*>();
            inspected=!tables.isEmpty();
            for(auto* table:tables)if(table->isVisibleTo(dialog)&&!table->property("editorCustomRows").toBool()&&!recipe(table))inspected=false;
            if(argc>2)dialog->grab().save(QString::fromUtf8(argv[2])+"-dialog.png");
            dialog->reject();
        });
        item<QAction>(window,"editorDemandCatalog")->trigger();settle();
        require(inspected,"A dialog table missed the recipe");
        // An elided name shows in full on hover; a cell that fits shows nothing.
        auto* links=item<QTableWidget>(window,"editorLinkTable");
        auto* canvas=window.canvas();canvas->select(window.history().document().network.links.front().id);settle();
        const QString longName=QStringLiteral("Approach from the western interchange, northbound slip lane");
        auto* name=item<QLineEdit>(window,"editorName");name->setFocus();name->setText(longName);QTest::keyClick(name,Qt::Key_Return);settle();
        int row=-1;for(int r=0;r<links->rowCount();++r)if(links->item(r,1)->text()==longName)row=r;
        require(row>=0,"Rename fixture did not reach the table");
        links->setColumnWidth(1,60);settle();
        require(links->fontMetrics().horizontalAdvance(longName)>links->visualRect(links->model()->index(row,1)).width(),"Elision fixture did not take effect");
        hover(links,row,1);
        auto* shown=popup(window);
        require(shown&&shown->toPlainText()==longName,"An elided cell has no full-text tooltip");
        if(argc>2)require(window.screen()->grabWindow(0).save(QString::fromUtf8(argv[2])+"-tooltip.png")||window.grab().save(QString::fromUtf8(argv[2])+"-tooltip.png"),"Screenshot failed");
        shown->hide();
        links->resizeColumnsToContents();settle();
        hover(links,row,2);
        require(!popup(window),"A cell that fits shows a tooltip");
        // Figures read the same in English and Thai: Latin digits, '.' decimals, right-aligned.
        item<QAction>(window,"editorStep")->trigger();
        for(int i=0;i<300;++i)item<QAction>(window,"editorStep")->trigger();
        settle();
        auto* language=item<QComboBox>(window,"editorLanguage");
        const auto collect=[&]{
            QStringList out;
            for(const auto* table:{"editorLinkTable","editorRouteTable","editorMovementTable"}) {
                auto* view=item<QTableWidget>(window,table);
                for(int r=0;r<view->rowCount();++r)for(int c=0;c<view->columnCount();++c)
                    if(auto* cell=view->item(r,c);cell&&figure.match(cell->text()).hasMatch()&&c>0) {
                        require(cell->textAlignment()&Qt::AlignRight,"A figure is not right-aligned");
                        out<<cell->text();
                    }
            }
            return out;
        };
        language->setCurrentIndex(0);settle();const auto english=collect();
        language->setCurrentIndex(1);settle();const auto thai=collect();
        require(english.size()>4,"Figure fixture is empty");
        require(english==thai,"A figure changed with the language");
        require(english.contains(QStringLiteral("150.00")),"Link length lost its fixed precision");
        std::cout<<"Tables: recipe after startup, elided tooltips and language-stable figures passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
