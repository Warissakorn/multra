#include "editor_layout.hpp"
#include <QApplication>
#include <QFormLayout>
#include <QLabel>
#include <QWidget>

namespace trafficsim {
namespace {
void normalize(QLayout* layout) {
    if(!layout)return;
    if(auto* form=qobject_cast<QFormLayout*>(layout)) {
        form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        form->setLabelAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        form->setFormAlignment(Qt::AlignLeft|Qt::AlignTop);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        for(int row=0;row<form->rowCount();++row)
            if(auto* item=form->itemAt(row,QFormLayout::LabelRole))
                if(auto* label=qobject_cast<QLabel*>(item->widget())) {
                    label->setTextFormat(Qt::PlainText);label->setWordWrap(true);
                    label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
                }
    }
    for(int i=0;i<layout->count();++i)if(auto* item=layout->itemAt(i))normalize(item->layout());
}
class LayoutPolicy final : public QObject {
public:
    explicit LayoutPolicy(QObject* parent):QObject(parent) {}
    bool eventFilter(QObject* object,QEvent* event) override {
        if(event->type()==QEvent::Polish || event->type()==QEvent::Show)
            if(auto* widget=qobject_cast<QWidget*>(object))normalize(widget->layout());
        return false;
    }
};
}
void installEditorLayoutPolicy() {
    auto* app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(!app || app->property("editorLayoutPolicy").toBool())return;
    app->setProperty("editorLayoutPolicy",true);app->installEventFilter(new LayoutPolicy(app));
}
}
