#include "presentation.hpp"
#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QApplication>
#include <QActionGroup>
#include <QHeaderView>
#include <QMainWindow>
#include <QMenu>
#include <QTableWidget>
#include <algorithm>
namespace multra {
int editorDensity(){return editorDesign::presentationDensity();}
void setEditorDensity(int density) {
    const int previous=editorDensity();
    density=std::clamp(density,0,2);qApp->setProperty("editorDensity",density);
    auto font=qApp->font();font.setPixelSize(editorDesign::fontSizeBody+density);qApp->setFont(font);
    for(auto* window:QApplication::topLevelWidgets())if(window->objectName()=="networkEditor"||window->objectName()=="labWindow") {
        applyEditorStyle(window);
        int selected=0;for(const auto* key:{"editorDensityCompact","editorDensityComfortable","editorDensityLarge"})
            if(auto* action=window->findChild<QAction*>(key))action->setChecked(selected++==density);
        for(auto* table:window->findChildren<QTableWidget*>()) {
            const int height=editorDesign::tableRowHeight+4*density;table->verticalHeader()->setDefaultSectionSize(height);
            for(int row=0;row<table->rowCount();++row) {
                int rowHeight=height;
                for(int col=0;col<table->columnCount();++col) {
                    if(auto* widget=table->cellWidget(row,col))rowHeight=std::max(rowHeight,widget->minimumSizeHint().height());
                    if(auto* item=table->item(row,col);item&&item->data(Qt::FontRole).isValid()) {
                        if(!item->data(Qt::UserRole+99).isValid())item->setData(Qt::UserRole+99,item->font().pixelSize()-previous);
                        auto f=item->font();f.setPixelSize(item->data(Qt::UserRole+99).toInt()+density);item->setFont(f);
                    }
                }
                table->setRowHeight(row,rowHeight);
            }
        }
    }
}
void installPresentationMenu(QMainWindow& window,const std::function<QString(const char*)>& text,const std::function<void()>& relayout) {
    auto* view=window.findChild<QMenu*>("editorViewMenu");if(!view)return;
    auto* menu=view->addMenu(text("editorDensity"));menu->setObjectName("editorDensityMenu");menu->setProperty("localeKey","editorDensity");
    auto* group=new QActionGroup(menu);int density=0;
    for(const auto* key:{"editorDensityCompact","editorDensityComfortable","editorDensityLarge"}) {
        auto* a=menu->addAction(text(key));a->setObjectName(key);a->setProperty("localeKey",key);a->setCheckable(true);a->setChecked(editorDensity()==density);group->addAction(a);
        QObject::connect(a,&QAction::triggered,&window,[density,relayout]{setEditorDensity(density);relayout();});++density;
    }
}
void translatePresentationMenu(QMainWindow& window,const std::function<QString(const char*)>& text) {
    for(auto* menu:window.findChildren<QMenu*>())if(menu->property("localeKey").isValid())menu->setTitle(text(menu->property("localeKey").toString().toUtf8().constData()));
    for(auto* action:window.findChildren<QAction*>())if(action->property("localeKey").isValid())action->setText(text(action->property("localeKey").toString().toUtf8().constData()));
}
}
