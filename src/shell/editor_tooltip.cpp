#include "editor_tooltip.hpp"
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QHeaderView>
#include <QHelpEvent>
#include <QPainter>
#include <QPointer>
#include <QScreen>
#include <QTabBar>
#include <QTextBoundaryFinder>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTimer>
#include <QToolTip>
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
class WrappedTooltip final : public QWidget {
    QTextDocument text_;
public:
    WrappedTooltip():QWidget(nullptr,Qt::ToolTip|Qt::FramelessWindowHint) {
        setObjectName("editorWrappedTooltip");setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TransparentForMouseEvents);setFocusPolicy(Qt::NoFocus);
        text_.setDocumentMargin(0);
        auto option=text_.defaultTextOption();option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        text_.setDefaultTextOption(option);
    }
    void present(QWidget* owner,const QString& full,QPoint point) {
        auto* window=owner->window();auto* screen=owner->screen();
        QRect bounds=screen?screen->availableGeometry():QRect(point,QSize(640,480));
        if(window->isVisible())bounds=bounds.intersected(QRect(window->mapToGlobal(QPoint()),window->size()));
        bounds=bounds.adjusted(8,8,-8,-8);
        if(bounds.width()<32 || bounds.height()<32){hide();return;}
        setPalette(owner->palette());setFont(owner->font());text_.setDefaultFont(font());
        setAccessibleName(full);
        const int width=std::min(420,bounds.width())-16,height=bounds.height()-16;
        text_.setTextWidth(width);text_.setPlainText(full);
        if(text_.size().height()>height) {
            // Binary search followed by a grapheme boundary: never cut a Thai combining mark.
            qsizetype lo=0,hi=full.size();
            while(lo<hi) {
                const auto mid=(lo+hi+1)/2;text_.setPlainText(full.left(mid)+QStringLiteral("…"));
                if(text_.size().height()<=height)lo=mid;else hi=mid-1;
            }
            QTextBoundaryFinder boundary(QTextBoundaryFinder::Grapheme,full);boundary.setPosition(lo);
            if(!boundary.isAtBoundary())lo=std::max<qsizetype>(0,boundary.toPreviousBoundary());
            text_.setPlainText(full.left(lo)+QStringLiteral("…"));
        }
        const int contentWidth=std::min(width,static_cast<int>(std::ceil(text_.idealWidth())));
        text_.setTextWidth(std::max(1,contentWidth));
        resize(std::min(bounds.width(),contentWidth+16),std::min(bounds.height(),static_cast<int>(std::ceil(text_.size().height()))+16));
        QPoint position=point+QPoint(14,18);
        if(position.y()+this->height()>bounds.bottom()+1)position.setY(point.y()-this->height()-10);
        if(position.x()+this->width()>bounds.right()+1)position.setX(point.x()-this->width()-10);
        position.setX(std::clamp(position.x(),bounds.left(),bounds.right()+1-this->width()));
        position.setY(std::clamp(position.y(),bounds.top(),bounds.bottom()+1-this->height()));
        move(position);show();raise();update();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);painter.setRenderHint(QPainter::TextAntialiasing);
        painter.fillRect(rect(),palette().color(QPalette::ToolTipBase));
        painter.setPen(palette().color(QPalette::Mid));painter.drawRect(rect().adjusted(0,0,-1,-1));
        painter.translate(8,8);QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text,palette().color(QPalette::ToolTipText));
        text_.documentLayout()->draw(&painter,context);
    }
};
QString tooltipText(QWidget* widget,QPoint point) {
    for(auto* at=widget;at;at=at->parentWidget()) {
        const auto local=at->mapFrom(widget,point);
        if(auto* graphics=qobject_cast<QGraphicsView*>(at)) {
            const auto viewport=graphics->viewport()->mapFrom(widget,point);
            for(const auto* item:graphics->items(viewport))if(!item->toolTip().isEmpty())return item->toolTip();
        }
        if(auto* header=qobject_cast<QHeaderView*>(at)) {
            const int section=header->logicalIndexAt(local);
            if(section>=0 && header->model()) {
                auto text=header->model()->headerData(section,header->orientation(),Qt::ToolTipRole).toString();
                if(text.isEmpty())text=header->model()->headerData(section,header->orientation(),Qt::DisplayRole).toString();
                if(!text.isEmpty())return text;
            }
        } else if(auto* view=qobject_cast<QAbstractItemView*>(at)) {
            const auto index=view->indexAt(view->viewport()->mapFrom(widget,point));
            if(index.isValid()) {
                auto text=index.data(Qt::ToolTipRole).toString();
                if(text.isEmpty())text=index.data(Qt::DisplayRole).toString();
                if(!text.isEmpty())return text;
            }
        }
        if(auto* tabs=qobject_cast<QTabBar*>(at)) {
            const int tab=tabs->tabAt(local);
            if(tab>=0)return tabs->tabToolTip(tab).isEmpty()?tabs->tabText(tab):tabs->tabToolTip(tab);
        }
        if(!at->toolTip().isEmpty())return at->toolTip();
        if(auto* combo=qobject_cast<QComboBox*>(at))return combo->currentText();
        if(at->isWindow())break;
    }
    return {};
}
class TooltipPolicy final : public QObject {
    QPointer<WrappedTooltip> popup_;
    QPointer<QWidget> owner_;
    QTimer timer_;
public:
    explicit TooltipPolicy(QObject* parent):QObject(parent) {
        timer_.setSingleShot(true);connect(&timer_,&QTimer::timeout,this,[this]{dismiss();});
    }
    ~TooltipPolicy() override {delete popup_.data();}
    void dismiss() {if(popup_)popup_->hide();timer_.stop();owner_.clear();}
    bool eventFilter(QObject* object,QEvent* event) override {
        if(event->type()==QEvent::ToolTip) {
            auto* widget=qobject_cast<QWidget*>(object);if(!widget)return false;
            auto* help=static_cast<QHelpEvent*>(event);const auto text=tooltipText(widget,help->pos());
            QToolTip::hideText();
            if(text.isEmpty()){dismiss();return false;}
            if(!popup_)popup_=new WrappedTooltip;
            owner_=widget;popup_->present(widget,text,help->globalPos());
            timer_.start(std::clamp(static_cast<int>(text.size()*35),6000,20000));
            event->accept();return true;
        }
        if(popup_ && popup_->isVisible() && object!=popup_)switch(event->type()) {
        case QEvent::Wheel:case QEvent::MouseButtonPress:case QEvent::KeyPress:
        case QEvent::FocusOut:case QEvent::WindowDeactivate:dismiss();break;
        case QEvent::Leave:if(object==owner_)dismiss();break;
        case QEvent::Hide:case QEvent::Resize:case QEvent::Move:
            if(owner_ && (object==owner_ || object==owner_->window()))dismiss();
            break;
        default:break;
        }
        return false;
    }
};
}
void installEditorTooltipPolicy() {
    auto* app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(!app || app->property("editorTooltipPolicy").toBool())return;
    app->setProperty("editorTooltipPolicy",true);app->installEventFilter(new TooltipPolicy(app));
}
}
