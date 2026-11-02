#include "table_layout.hpp"
#include <QAbstractSpinBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QTabWidget>
#include <algorithm>
namespace multra {
void sizeTabPages(QTabWidget* tabs) {
    // A hidden Results page must not impose its two-table minimum on another task group.
    for(int i=0;i<tabs->count();++i) {
        auto* page=tabs->widget(i);
        if(!page->property("editorPagePolicy").isValid())page->setProperty("editorPagePolicy",QVariant::fromValue(page->sizePolicy()));
        auto policy=page->property("editorPagePolicy").value<QSizePolicy>();
        if(i!=tabs->currentIndex())policy.setVerticalPolicy(QSizePolicy::Ignored);
        page->setSizePolicy(policy);
    }
}
void fitTableColumns(QTableWidget* table) {
    if(table->rowCount()==0||table->property("editorColumnsSized").toBool())return;
    table->resizeColumnsToContents();table->horizontalHeader()->setStretchLastSection(false);
    for(int column=0;column<table->columnCount();++column) {
        int minimum=40;
        for(int row=0;row<table->rowCount();++row)if(auto* widget=table->cellWidget(row,column))
            minimum=std::max(minimum,std::max(widget->minimumWidth(),widget->minimumSizeHint().width()));
        table->setColumnWidth(column,std::clamp(table->columnWidth(column),minimum,std::max(minimum,320)));
    }
    table->setProperty("editorColumnsSized",true);
}
}
