#include "canvas.hpp"
#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <cmath>

namespace trafficsim {
namespace {
QCursor glyphCursor(const QString& state,double ratio) {
    constexpr int size=32;
    QPixmap image(static_cast<int>(std::ceil(size*ratio)),static_cast<int>(std::ceil(size*ratio)));
    image.setDevicePixelRatio(ratio);image.fill(Qt::transparent);
    QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::black,1,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::white);
    if(state=="move") {
        QPolygonF arrow;arrow<<QPointF(1,1)<<QPointF(1,17)<<QPointF(5,13)<<QPointF(8,20)
            <<QPointF(10.5,19)<<QPointF(7.5,12)<<QPointF(13,12);p.drawPolygon(arrow);
        QPainterPath hand;hand.setFillRule(Qt::WindingFill);hand.addRoundedRect(QRectF(15,21,12,10),3,3);
        const double tops[]={15.5,14,15,16.5};
        for(int i=0;i<4;++i)hand.addRoundedRect(QRectF(15+i*2.9,tops[i],2.6,9),1.3,1.3);
        hand.addRoundedRect(QRectF(12,23,6,3),1.5,1.5);p.drawPath(hand.simplified());
    } else {
        p.setPen(QPen(Qt::white,3));p.drawLine(1,9,17,9);p.drawLine(9,1,9,17);
        p.setPen(QPen(Qt::black,1));p.drawLine(1,9,17,9);p.drawLine(9,1,9,17);
        if(state=="rotate") {
            p.drawArc(QRectF(18,18,11,11),30*16,290*16);p.drawLine(29,17,29,22);p.drawLine(29,22,24,22);
        } else if(state=="link") {
            p.drawLine(20,18,20,30);p.drawLine(29,18,29,30);p.drawLine(24,19,24,23);p.drawLine(24,27,24,30);
        } else if(state=="point-insert") {
            p.drawEllipse(QRectF(20,20,8,8));p.drawLine(18,24,30,24);p.drawLine(24,18,24,30);
        } else {
            QPainterPath path;path.moveTo(19,30);path.cubicTo(19,20,29,28,29,18);p.drawPath(path);
            p.drawLine(25,21,29,18);p.drawLine(29,18,30,23);
        }
    }
    p.end();return QCursor(image,state=="move"?1:9,state=="move"?1:9);
}
}
void EditorCanvas::refreshCursor() {
    QString state="cross";Qt::CursorShape shape=Qt::CrossCursor;
    const bool control=cursorModifiers_&Qt::ControlModifier;
    if(panning_){state="pan";shape=Qt::ClosedHandCursor;}
    else if(rotationPivot_){state="rotate";shape=Qt::BitmapCursor;}
    else if(laneResize_) {
        const auto direction=laneResize_->direction;const double x=std::abs(direction.x),y=std::abs(direction.y);
        state="resize";shape=x>2*y?Qt::SizeHorCursor:y>2*x?Qt::SizeVerCursor:direction.x*direction.y>0?Qt::SizeBDiagCursor:Qt::SizeFDiagCursor;
    } else if(movingGeometry() || copyDragging_ || groupDragging_ ||
              (headDrag_ && headDrag_->moved) || (lineDrag_ && lineDrag_->moved) || (decisionDrag_ && decisionDrag_->moved)) {
        state="move";shape=Qt::BitmapCursor;
    } else if(creating_) {
        if(tool_==Tool::select && (cursorPosition_-creationStart_).manhattanLength()<QApplication::startDragDistance())state="point-insert";
        else if(creationTool_==Tool::draw)state="link";
        else {
            const auto target=hitLanePosition(world(cursorPosition_,false),false);
            state=target?"connector":"unavailable";
        }
        shape=state=="unavailable"?Qt::ForbiddenCursor:Qt::BitmapCursor;
    } else if(connectorFrom_) {
        state=hitLanePosition(world(cursorPosition_,false),false)?"connector":"unavailable";
        shape=state=="unavailable"?Qt::ForbiddenCursor:Qt::BitmapCursor;
    } else if(control && (tool_==Tool::draw || tool_==Tool::connect)) {
        const bool lane=hitLanePosition(world(cursorPosition_,false),true).has_value();
        state=lane?"connector":tool_==Tool::draw?"link":"unavailable";
        shape=state=="unavailable"?Qt::ForbiddenCursor:Qt::BitmapCursor;
    } else if(tool_==Tool::select) {
        state="select";shape=Qt::ArrowCursor;
        if(const auto h=laneHandleAt(cursorPosition_)) {
            const double x=std::abs(h->direction.x),y=std::abs(h->direction.y);
            state="resize";shape=x>2*y?Qt::SizeHorCursor:y>2*x?Qt::SizeVerCursor:h->direction.x*h->direction.y>0?Qt::SizeBDiagCursor:Qt::SizeFDiagCursor;
        } else if(hoverVertex_>=0){state="point-move";shape=Qt::SizeAllCursor;}
    } else if((tool_==Tool::route || tool_==Tool::input) && !hoverSegment_.empty() && !hoverReachable_) {
        state="unavailable";shape=Qt::ForbiddenCursor;
    } else if(tool_==Tool::conflict && !hoverWaitingLine_.empty()){state="point-move";shape=Qt::SizeAllCursor;}
    const auto key=state+QString::number(static_cast<int>(shape))+QString::number(devicePixelRatioF(),'f',2);
    if(cursorKey_==key)return;
    cursorKey_=key;setProperty("editorCursorState",state);
    setCursor(shape==Qt::BitmapCursor?glyphCursor(state,devicePixelRatioF()):QCursor(shape));
}
void EditorCanvas::showMoveCursor() {refreshCursor();}
void EditorCanvas::keyReleaseEvent(QKeyEvent* event) {
    cursorModifiers_=event->modifiers();
    if(event->key()==Qt::Key_Control)cursorModifiers_&=~Qt::ControlModifier;
    if(event->key()==Qt::Key_Alt)cursorModifiers_&=~Qt::AltModifier;
    refreshCursor();QGraphicsView::keyReleaseEvent(event);
}
}
