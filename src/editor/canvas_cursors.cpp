#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../shell/editor_style.hpp"
#include <QGraphicsPathItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <map>

// D306: the canvas's pointers and preview lines. Every pointer is the ordinary arrow; a move adds a
// hand beside it and an authoring tool adds its own toolbar glyph, so the pointer says what a
// Ctrl+right-click will do without the crosshair hiding the tip.
namespace multra {
namespace {
constexpr int cursorSize=32,cursorRatio=2;
QPixmap cursorCanvas() {
    QPixmap pixmap(cursorSize*cursorRatio,cursorSize*cursorRatio);pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(cursorRatio);return pixmap;
}
void drawArrow(QPainter& p) {
    QPolygonF arrow;
    arrow<<QPointF(1,1)<<QPointF(1,17)<<QPointF(5,13)<<QPointF(8,20)<<QPointF(10.5,19)
         <<QPointF(7.5,12)<<QPointF(13,12)<<QPointF(1,1);
    p.setPen(QPen(Qt::black,1,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::white);p.drawPolygon(arrow);
}
// The pointer while something is being moved: the arrow with a small open hand beside it, so
// pointing stays an arrow and only a move says "you are carrying this".
const QCursor& moveCursor() {
    static const QCursor cursor=[]{
        auto pixmap=cursorCanvas();
        QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);drawArrow(p);
        // Palm, four fingers and a thumb merged into one outline (winding fill, so the overlaps
        // join instead of cutting holes).
        QPainterPath hand;hand.setFillRule(Qt::WindingFill);
        hand.addRoundedRect(QRectF(15,21,11.5,9.5),3,3);
        const double tops[]={15.5,14,15,16.5};
        for(int i=0;i<4;++i)hand.addRoundedRect(QRectF(15+i*2.9,tops[i],2.6,9),1.3,1.3);
        QPainterPath thumb;thumb.addRoundedRect(QRectF(-1.3,-5,2.6,7),1.3,1.3);
        hand.addPath(QTransform().translate(14.8,25).rotate(-40).map(thumb));
        p.setBrush(Qt::white);p.drawPath(hand.simplified());
        p.end();
        return QCursor(pixmap,1,1);
    }();
    return cursor;
}
// The palette's glyph for each tool (editor_palette.cpp), in Tool order.
EditorIcon toolGlyph(EditorCanvas::Tool tool) {
    using T=EditorCanvas::Tool;
    switch(tool) {
    case T::select: return EditorIcon::select;
    case T::draw: return EditorIcon::link;
    case T::split: return EditorIcon::split;
    case T::measure: return EditorIcon::measure;
    case T::calibrate: return EditorIcon::image;
    case T::connect: return EditorIcon::connector;
    case T::route: return EditorIcon::route;
    case T::input: return EditorIcon::input;
    case T::head: return EditorIcon::signal;
    case T::conflict: return EditorIcon::conflict;
    case T::counter: return EditorIcon::counter;
    case T::section: return EditorIcon::section;
    case T::zone: case T::node: return EditorIcon::zone;
    case T::road: return EditorIcon::road;
    }
    return EditorIcon::select;
}
// The arrow with the tool's glyph on a white badge at its lower right, black ink on white like
// the arrow itself, so it reads on a light or a dark canvas alike.
const QCursor& toolCursor(EditorCanvas::Tool tool) {
    static std::map<EditorCanvas::Tool,QCursor> cache;
    if(const auto found=cache.find(tool);found!=cache.end())return found->second;
    auto pixmap=cursorCanvas();
    QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);drawArrow(p);
    const QRectF badge(13.5,13.5,17,17);
    p.setPen(QPen(Qt::black,1));p.setBrush(Qt::white);p.drawRoundedRect(badge,3.5,3.5);
    p.translate(badge.left()+1.5,badge.top()+1.5);p.scale(14/20.,14/20.);p.setBrush(Qt::NoBrush);
    paintEditorGlyph(p,toolGlyph(tool),Qt::black,1.6*20/14);
    p.end();
    return cache.emplace(tool,QCursor(pixmap,1,1)).first->second;
}
}
QCursor EditorCanvas::idleCursor() const {
    return tool_==Tool::select?QCursor(Qt::ArrowCursor):toolCursor(tool_);
}
void EditorCanvas::showMoveCursor() {
    if(cursor().pixmap().cacheKey()!=moveCursor().pixmap().cacheKey())setCursor(moveCursor());
}
bool EditorCanvas::authoringPress(const QMouseEvent* e,bool draftOpen) const {
    if(e->button()==Qt::RightButton)return (e->modifiers()&Qt::ControlModifier)!=0;
    return draftOpen && e->button()==Qt::LeftButton &&
           !(e->modifiers()&(Qt::ControlModifier|Qt::ShiftModifier|Qt::AltModifier));
}
QGraphicsPathItem* EditorCanvas::addPreviewPath(const QPainterPath& shape,double z,const QPen& pen,const QBrush& fill) {
    scene_.addPath(shape,canvasStyle::previewCasing(pen.widthF()))->setZValue(z-.01);
    auto* item=scene_.addPath(shape,pen,fill);item->setZValue(z);
    return item;
}
}
