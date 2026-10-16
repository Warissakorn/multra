#include "editor_interaction.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "editor_style.hpp"
#include <QHeaderView>
#include <QStyle>
#include <QTableView>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QLineEdit>
#include <QLayout>
#include <QResizeEvent>
#include <QPointer>
#include <QScreen>
#include <QScrollBar>
#include <QTextEdit>
#include <QTextDocument>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <vector>

namespace trafficsim {
void setReflowPolicy(QLabel* label, QSizePolicy::Policy horizontal) {
    auto policy=label->sizePolicy();policy.setHorizontalPolicy(horizontal);
    policy.setVerticalPolicy(QSizePolicy::Preferred);policy.setHeightForWidth(label->wordWrap());
    label->setSizePolicy(policy);label->updateGeometry();
}
// D205: the column is 120 px unless a label's unbreakable word (QLabel's minimum width) is wider.
// Thai on Windows has no break points (no libthai), so "ระยะห่างถนนทิศตรงข้าม" is one 123 px word;
// QFormLayout keeps such a row beside its label at the word's width, which moved only that form's
// fields. Every form under the root now widens alike. A word more than a quarter wider widens no
// other form; its own row lays out as before D205 (usually wrapped below its label).
struct FormColumn {
    std::vector<QPointer<ReflowLabel>> labels;
    std::vector<QPointer<QWidget>> hosts;
    int width=editorDesign::formLabelWidth;
    void refresh() {
        int next=editorDesign::formLabelWidth;
        for(const auto& label:labels)if(label) {
            const int word=label->QLabel::minimumSizeHint().width();
            if(word<editorDesign::formLabelWidth*5/4)next=std::max(next,word);
        }
        if(next==width)return;
        width=next;
        for(const auto& label:labels)if(label)label->updateGeometry();
        // A hidden form (the closed appearance fold) is not laid out on the posted request, so it would
        // keep the old column until shown; lay it out now.
        for(const auto& host:hosts)if(host&&!host->isVisible()&&host->layout())host->layout()->activate();
    }
};
QSize ReflowLabel::sizeHint() const {
    if(!wordWrap() || textFormat()==Qt::RichText)return QLabel::sizeHint();
    auto natural=fontMetrics().size(Qt::TextExpandTabs,text());
    if(property("editorFormLabel").toBool())natural.setWidth(column_?column_->width:editorDesign::formLabelWidth);
    const auto padding=contentsMargins();
    auto size=natural+QSize(padding.left()+padding.right()+2*margin(),padding.top()+padding.bottom()+2*margin());
    if(auto* parent=parentWidget()) {
        int available=parent->contentsRect().width();
        if(auto* layout=parent->layout()) {
            const auto margins=layout->contentsMargins();available-=margins.left()+margins.right();
        }
        if(available>0)size.setWidth(std::min(size.width(),available));
    }
    size.setHeight(std::max(size.height(),QLabel::heightForWidth(size.width())));
    return size;
}
namespace {
// Below two label columns a field beside its label would be squeezed to a sliver, so the whole
// form stacks each field under its label instead.
class FormReflow : public QObject {
public:
    FormReflow(QFormLayout* form,QWidget* host,std::shared_ptr<FormColumn> column)
        :QObject(host),form_(form),column_(std::move(column)){host->installEventFilter(this);apply(host);}
    bool eventFilter(QObject* watched,QEvent* event) override {
        if(event->type()==QEvent::Resize)apply(static_cast<QWidget*>(watched));
        // A label's text or font changed (a language switch): its form asks for a new layout.
        if(event->type()==QEvent::LayoutRequest)column_->refresh();
        return false;
    }
private:
    void apply(QWidget* host) {
        const auto margins=form_->contentsMargins();
        const int width=host->contentsRect().width()-margins.left()-margins.right();
        const auto policy=width<2*editorDesign::formLabelWidth?QFormLayout::WrapAllRows:QFormLayout::WrapLongRows;
        if(form_->rowWrapPolicy()!=policy)form_->setRowWrapPolicy(policy);
    }
    QFormLayout* form_;
    std::shared_ptr<FormColumn> column_;
};
}
void alignFormLabels(QWidget* root) {
    auto column=std::make_shared<FormColumn>();
    for(auto* form:root->findChildren<QFormLayout*>()) {
        for(int row=0;row<form->rowCount();++row)
            if(auto* cell=form->itemAt(row,QFormLayout::LabelRole))
                if(auto* label=dynamic_cast<ReflowLabel*>(cell->widget())) {
                    label->setProperty("editorFormLabel",true);label->column_=column;column->labels.emplace_back(label);
                }
        if(auto* host=form->parentWidget()){new FormReflow(form,host,column);column->hosts.emplace_back(host);}
    }
    column->refresh();
}
QSize ReflowLabel::minimumSizeHint() const {
    // A form label keeps its wrapped height: a short dock shrinks the scrolling pages, not a label.
    if(property("editorFormLabel").toBool()&&wordWrap())return {QLabel::minimumSizeHint().width(),sizeHint().height()};
    return QLabel::minimumSizeHint();
}
namespace {
class EditorInteraction : public QObject {
public:
    explicit EditorInteraction(QObject* parent):QObject(parent) {
        expiry_.setSingleShot(true);connect(&expiry_,&QTimer::timeout,this,[this]{hide();});
        leave_.setSingleShot(true);connect(&leave_,&QTimer::timeout,this,[this]{hide();});
    }
    ~EditorInteraction() override { delete popup_; }
private:
    QPointer<QTextEdit> popup_;
    QPointer<QWidget> source_;
    QTimer expiry_,leave_;
    std::vector<QMetaObject::Connection> scrolls_; // the source's scroll areas, while a popup shows
    void unwatch() { for(const auto& c:scrolls_)disconnect(c);scrolls_.clear(); }
    void hide() { if(popup_)popup_->hide();source_.clear();expiry_.stop();leave_.stop();unwatch(); }
    QString tooltip(QWidget* widget,QPoint position) {
        // Item views and scene objects carry their tooltip on the item, not the viewport.
        if(auto* view=qobject_cast<QAbstractItemView*>(widget->parentWidget())) {
            const auto index=view->indexAt(position);
            const auto value=index.data(Qt::ToolTipRole).toString();
            if(!value.isEmpty())return value;
            // An elided cell shows its full text (D196); a cell that fits needs no popup.
            const auto shown=index.data(Qt::DisplayRole).toString();
            if(index.isValid()&&!shown.isEmpty()) {
                const auto font=index.data(Qt::FontRole).isValid()?index.data(Qt::FontRole).value<QFont>():view->font();
                const int margin=view->style()->pixelMetric(QStyle::PM_FocusFrameHMargin,nullptr,view)+1;
                if(QFontMetrics(font).horizontalAdvance(shown)>view->visualRect(index).width()-2*margin)return shown;
            }
        }
        if(auto* view=qobject_cast<QGraphicsView*>(widget->parentWidget()))
            for(auto* item:view->items(position))if(!item->toolTip().isEmpty())return item->toolTip();
        for(auto* at=widget;at;at=at->parentWidget())if(!at->toolTip().isEmpty())return at->toolTip();
        return {};
    }
    void show(QWidget* source,const QString& text,QPoint pointer) {
        auto* owner=source->window();
        QRect bounds(owner->mapToGlobal(QPoint()),owner->size());
        if(auto* screen=owner->screen())bounds=bounds.intersected(screen->availableGeometry());
        bounds.adjust(4,4,-4,-4);
        if(bounds.width()<40 || bounds.height()<40){hide();return;}
        if(!popup_) {
            popup_=new QTextEdit(owner);
            popup_->setObjectName("editorBoundedTooltip");
            popup_->setWindowFlags(Qt::ToolTip|Qt::WindowDoesNotAcceptFocus);
            popup_->setAttribute(Qt::WA_ShowWithoutActivating);
            popup_->setReadOnly(true);popup_->setFocusPolicy(Qt::NoFocus);
            popup_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            popup_->setStyleSheet(QStringLiteral("QTextEdit { background: palette(text); color: palette(base); border: 1px solid palette(text); padding: 4px; }"));
        }
        if(popup_->parentWidget()!=owner)
            popup_->setParent(owner,Qt::ToolTip|Qt::WindowDoesNotAcceptFocus);
        popup_->setPalette(owner->palette());popup_->setFont(source->font());
        QTextOption option;option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        popup_->document()->setDefaultTextOption(option);
        popup_->setPlainText(text); // No HTML interpretation of object names or localized text.
        const int width=std::min(360,bounds.width());
        popup_->document()->setTextWidth(width-32);
        const int height=std::min(bounds.height(),static_cast<int>(std::ceil(popup_->document()->size().height()))+16);
        popup_->resize(width,std::max(32,height));
        int x=pointer.x()+12,y=pointer.y()+18;
        if(y+popup_->height()>bounds.bottom()+1)y=pointer.y()-popup_->height()-8;
        x=std::clamp(x,bounds.left(),bounds.right()+1-popup_->width());
        y=std::clamp(y,bounds.top(),bounds.bottom()+1-popup_->height());
        popup_->move(x,y);popup_->show();source_=source;
        // Content scrolled under a still pointer (wheel, keys, kinetic or programmatic) moves away
        // from the popup without any event on the source, so any scroll of an enclosing area hides it.
        unwatch();
        for(auto* at=source;at;at=at->parentWidget())if(auto* area=qobject_cast<QAbstractScrollArea*>(at))
            for(auto* bar:{area->horizontalScrollBar(),area->verticalScrollBar()})
                scrolls_.push_back(connect(bar,&QScrollBar::valueChanged,this,[this]{hide();}));
        QToolTip::hideText();leave_.stop();expiry_.start(15000);
    }
    bool eventFilter(QObject* object,QEvent* event) override {
        auto* widget=qobject_cast<QWidget*>(object);if(!widget)return false;
        if(event->type()==QEvent::Polish)if(auto* table=qobject_cast<QTableView*>(widget))applyTableRecipe(table);
        if(popup_ && (widget==popup_ || popup_->isAncestorOf(widget))) {
            if(event->type()==QEvent::Enter || event->type()==QEvent::Wheel || event->type()==QEvent::MouseMove) {
                leave_.stop();expiry_.start(15000);
            }
            if(event->type()==QEvent::Leave)leave_.start(200);
            return false;
        }
        if(event->type()==QEvent::Wheel) {
            QWidget* field=nullptr;
            for(auto* at=widget;at;at=at->parentWidget()) {
                // An open combo's list can scroll; its closed field never changes a value.
                if(qobject_cast<QAbstractItemView*>(at))break;
                if(qobject_cast<QAbstractSpinBox*>(at) || qobject_cast<QComboBox*>(at)){field=at;break;}
            }
            if(field) {
                const auto* wheel=static_cast<QWheelEvent*>(event);
                for(auto* at=field->parentWidget();at;at=at->parentWidget())
                    if(auto* area=qobject_cast<QAbstractScrollArea*>(at)) {
                        const auto position=area->viewport()->mapFromGlobal(wheel->globalPosition().toPoint());
                        QWheelEvent forwarded(position,wheel->globalPosition(),wheel->pixelDelta(),wheel->angleDelta(),
                            wheel->buttons(),wheel->modifiers(),wheel->phase(),wheel->inverted(),wheel->source());
                        QApplication::sendEvent(area->viewport(),&forwarded);break;
                    }
                event->accept();return true;
            }
        }
        if(event->type()==QEvent::ToolTip) {
            auto* help=static_cast<QHelpEvent*>(event);const auto text=tooltip(widget,help->pos());
            if(text.isEmpty()){hide();return false;}
            show(widget,text,help->globalPos());event->accept();return true;
        }
        if(event->type()==QEvent::Show || event->type()==QEvent::FontChange) {
            if(auto* label=qobject_cast<QLabel*>(widget);label && label->wordWrap())
                setReflowPolicy(label,label->sizePolicy().horizontalPolicy());
            const bool internalEdit=qobject_cast<QLineEdit*>(widget) &&
                (qobject_cast<QAbstractSpinBox*>(widget->parentWidget()) || qobject_cast<QComboBox*>(widget->parentWidget()));
            if(qobject_cast<QAbstractSpinBox*>(widget) || qobject_cast<QComboBox*>(widget) ||
               (qobject_cast<QLineEdit*>(widget) && !internalEdit)) {
                if(!widget->property("editorOriginalMinimumWidth").isValid())
                    widget->setProperty("editorOriginalMinimumWidth",widget->minimumWidth());
                widget->setMinimumWidth(std::max(widget->property("editorOriginalMinimumWidth").toInt(),
                    std::min(widget->minimumSizeHint().width(),widget->maximumWidth())));
            }
        }
        if(event->type()==QEvent::Resize && static_cast<QResizeEvent*>(event)->size().width()!=
           static_cast<QResizeEvent*>(event)->oldSize().width())
            for(auto* label:widget->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly))
                if(label->wordWrap())label->updateGeometry();
        if(event->type()==QEvent::Leave && widget==source_)leave_.start(200);
        if(event->type()==QEvent::Enter && widget==source_){leave_.stop();expiry_.start(15000);}
        if(event->type()==QEvent::MouseMove && widget==source_ && popup_ &&
           tooltip(widget,static_cast<QMouseEvent*>(event)->position().toPoint())!=popup_->toPlainText())hide();
        if(source_ && widget==source_->window() &&
           (event->type()==QEvent::Resize || event->type()==QEvent::Move))hide();
        if(event->type()==QEvent::MouseButtonPress || event->type()==QEvent::KeyPress ||
           (event->type()==QEvent::Hide && widget==source_) || event->type()==QEvent::WindowDeactivate)hide();
        return false;
    }
};
}
void installEditorInteraction() {
    static QPointer<EditorInteraction> interaction;
    if(!interaction && qApp){interaction=new EditorInteraction(qApp);qApp->installEventFilter(interaction);}
}
}
