#include "canvas.hpp"
#include "canvas_labels.hpp"
#include "ui_design_tokens.hpp"
#include <QGraphicsLineItem>
#include <QLabel>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace trafficsim {
CanvasLabelItem::CanvasLabelItem(QString text,QFont font):QGraphicsSimpleTextItem(std::move(text)) {
    setFont(font);setFlag(ItemIgnoresTransformations);setCacheMode(NoCache);
    setBrush(editorDesign::role(QPalette::Text));
}
QRectF CanvasLabelItem::boundingRect() const {return QGraphicsSimpleTextItem::boundingRect().adjusted(-5,-3,5,3);}
void CanvasLabelItem::paint(QPainter* painter,const QStyleOptionGraphicsItem* option,QWidget* widget) {
    painter->save();painter->setRenderHint(QPainter::Antialiasing);painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setPen(QPen(editorDesign::role(QPalette::Mid),1));painter->setBrush(editorDesign::role(QPalette::Base));
    painter->drawRoundedRect(boundingRect(),3,3);painter->restore();
    QGraphicsSimpleTextItem::paint(painter,option,widget);
}
CanvasLabelItem* EditorCanvas::addCanvasLabel(QString text,const QString& tag,const std::string& id,Point anchor,int priority,QFont font) {
    auto* label=new CanvasLabelItem(text,font);scene_.addItem(label);label->setZValue(200020);
    label->setData(0,tag);label->setData(1,QString::fromStdString(id));
    label->setToolTip(id.empty()?text:QString::fromStdString(id)+QStringLiteral("\n")+text);
    QPen pen(editorDesign::role(QPalette::Dark),1);pen.setCosmetic(true);
    auto* leader=scene_.addLine({},pen);leader->setZValue(200019);leader->setData(0,QStringLiteral("label-leader"));leader->hide();
    labels_.push_back({label,leader,anchor,text,id,priority});return label;
}
void EditorCanvas::layoutCanvasLabels() {
    if(labels_.empty())return;
    const QRectF area=QRectF(viewport()->rect()).adjusted(8,8,-8,-8);
    if(area.width()<30 || area.height()<30)return;
    std::vector<QRectF> occupied;
    for(const auto* item:scene_.items()) {
        const auto tag=item->data(0).toString();
        if(tag=="input-marker" || tag=="decision-marker" || tag=="route-start" || tag=="route-end" ||
           tag=="geometry-point" || tag=="connector-end" || tag=="lane-resize" || tag=="rotation-pivot" ||
           tag=="stop-line" || tag=="waiting-line" || tag=="queue-counter" || tag=="travel-time-section")
            occupied.push_back(item->deviceTransform(viewportTransform()).mapRect(item->boundingRect()).adjusted(-3,-3,3,3));
    }
    for(auto* overlay:{mapCredit_,mapStatus_})if(overlay && overlay->isVisible())occupied.emplace_back(overlay->geometry());
    std::vector<CanvasLabelAnchor*> order;
    for(auto& label:labels_)order.push_back(&label);
    const auto priority=[&](const auto* label) {return label->priority+(QString::fromStdString(label->id)==hoverDemandId_?100:0);};
    std::stable_sort(order.begin(),order.end(),[&](const auto* a,const auto* b) {
        return priority(a)!=priority(b)?priority(a)>priority(b):a->id<b->id;
    });
    for(auto* anchor:order) {
        auto* label=anchor->item;anchor->leader->hide();label->hide();
        if(std::abs(transform().m11())<.12 && priority(anchor)<80)continue;
        const auto point=viewportTransform().map(QPointF(anchor->anchor.x,anchor->anchor.y));
        if(!area.adjusted(-20,-20,20,20).contains(point))continue;
        const int maxWidth=std::min(220,static_cast<int>(area.width())-10);
        label->setText(QFontMetrics(label->font()).elidedText(anchor->full,Qt::ElideRight,maxWidth));
        const auto bounds=label->boundingRect();std::optional<QRectF> picked;
        // Deterministic nearby rings; no two drawn labels share a rectangle.
        for(int ring=0;ring<=7 && !picked;++ring) {
            const double gap=8+ring*18,vertical=ring*(bounds.height()+6);
            const QPointF offsets[]={{gap,-bounds.height()/2},{-gap-bounds.width(),-bounds.height()/2},
                {gap,-bounds.height()-8-vertical},{gap,8+vertical},
                {-gap-bounds.width(),-bounds.height()-8-vertical},{-gap-bounds.width(),8+vertical}};
            for(const auto offset:offsets) {
                const QRectF candidate(point+offset,bounds.size());
                if(!area.contains(candidate))continue;
                if(std::any_of(occupied.begin(),occupied.end(),[&](const auto& rect){return rect.intersects(candidate.adjusted(-3,-3,3,3));}))continue;
                picked=candidate;break;
            }
        }
        if(!picked)continue; // Full values remain in object tables and marker tooltips.
        const double ratio=devicePixelRatioF();
        QPointF origin=picked->topLeft()-bounds.topLeft();
        origin.setX(std::round(origin.x()*ratio)/ratio);origin.setY(std::round(origin.y()*ratio)/ratio);
        const auto scenePoint=viewportTransform().inverted().map(origin);
        label->setPos(scenePoint);label->show();occupied.push_back(*picked);
        const QPointF nearest(std::clamp(point.x(),picked->left(),picked->right()),std::clamp(point.y(),picked->top(),picked->bottom()));
        if(QLineF(point,nearest).length()>14) {
            const auto end=viewportTransform().inverted().map(nearest);
            anchor->leader->setLine(QLineF(QPointF(anchor->anchor.x,anchor->anchor.y),end));anchor->leader->show();
        }
    }
}
void EditorCanvas::paintEvent(QPaintEvent* event) {
    layoutCanvasLabels();refreshCursor();QGraphicsView::paintEvent(event);
}
}
