#include "editor_glyph.hpp"
#include "ui_design_tokens.hpp"
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>

namespace multra {
void paintEditorGlyph(QPainter& p,EditorIcon icon,const QColor& ink,double stroke) {
    p.setPen(QPen(ink,stroke,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    const auto line=[&](int x,int y,int a,int b){p.drawLine(x,y,a,b);};
    const auto box=[&](int x,int y,int w,int h){p.drawRoundedRect(QRectF(x,y,w,h),1.5,1.5);};
    switch(icon) {
    case EditorIcon::document: box(4,2,12,16);line(7,8,13,8);line(7,12,13,12);break;
    case EditorIcon::open: box(2,6,16,11);line(3,6,3,3);line(3,3,9,3);line(9,3,11,6);break;
    case EditorIcon::save: box(3,3,14,14);box(6,3,8,5);box(6,12,8,5);break;
    case EditorIcon::undo: case EditorIcon::redo: {
        if(icon==EditorIcon::redo){p.translate(20,0);p.scale(-1,1);}
        QPainterPath path;path.moveTo(3,7);path.cubicTo(18,1,21,16,10,17);p.drawPath(path);
        line(3,7,3,2);line(3,7,8,9);break;
    }
    case EditorIcon::fit:
        for(int i=0;i<4;++i){line(3,8,3,3);line(3,3,8,3);p.translate(20,0);p.rotate(90);}
        line(7,10,13,10);line(10,7,10,13);break;
    // The canvas frame with the docks pushed out to both sides: not fit's corners and cross (D192).
    case EditorIcon::focus: box(1,5,18,10);line(5,10,15,10);line(5,10,7,8);line(5,10,7,12);line(15,10,13,8);line(15,10,13,12);break;
    case EditorIcon::finish: line(3,10,8,15);line(8,15,17,5);break;
    // Rotate turns clockwise; Reset is the mirrored return arrow around an origin dot (D192).
    case EditorIcon::rotate: case EditorIcon::reset:
        if(icon==EditorIcon::reset){p.translate(20,0);p.scale(-1,1);}
        p.drawArc(QRectF(4,4,12,12),35*16,290*16);line(16,3,16,8);line(16,8,11,8);
        if(icon==EditorIcon::reset){p.setBrush(p.pen().color());p.drawEllipse(QPointF(10,10),1.,1.);}
        break;
    case EditorIcon::remove: line(3,5,17,5);line(7,2,13,2);box(5,5,10,13);line(8,9,8,14);line(12,9,12,14);break;
    case EditorIcon::select: {
        QPolygonF points;points<<QPointF(4,2)<<QPointF(16,10)<<QPointF(10,11)<<QPointF(7,17)<<QPointF(4,2);
        p.drawPolyline(points);break;
    }
    case EditorIcon::link: line(4,2,4,18);line(16,2,16,18);line(10,3,10,7);line(10,13,10,17);break;
    // Connector: two road ends joined by a curved two-rail neck, no arrow. Route: one trace from
    // a start dot to an arrowhead, no rails (D192).
    case EditorIcon::connector: {
        line(1,11,5,11);line(1,16,5,16);line(11,1,11,5);line(16,1,16,5);
        QPainterPath inner;inner.moveTo(5,11);inner.cubicTo(8,11,11,8,11,5);p.drawPath(inner);
        QPainterPath outer;outer.moveTo(5,16);outer.cubicTo(12,16,16,12,16,5);p.drawPath(outer);break;
    }
    case EditorIcon::route: {
        QPainterPath path;path.moveTo(4,16);path.cubicTo(4,7,17,14,17,3);p.drawPath(path);
        line(13,6,17,3);line(17,3,18,8);p.setBrush(p.pen().color());p.drawEllipse(QPointF(4,16),1.5,1.5);break;
    }
    case EditorIcon::input: box(3,6,14,9);line(5,6,7,3);line(7,3,13,3);line(13,3,15,6);line(6,15,6,17);line(14,15,14,17);break;
    case EditorIcon::signal: box(6,1,8,15);p.drawEllipse(QPointF(10,5),1.,1.);p.drawEllipse(QPointF(10,11),1.,1.);line(10,16,10,19);break;
    case EditorIcon::split: line(5,2,5,18);line(15,2,15,18);line(1,10,19,10);break;
    case EditorIcon::measure: box(2,5,16,10);for(int x=5;x<17;x+=3)line(x,5,x,9);break;
    case EditorIcon::image: box(2,3,16,14);line(3,15,8,9);line(8,9,13,14);line(13,14,17,9);p.drawEllipse(QPointF(13,7),1.5,1.5);break;
    case EditorIcon::run: case EditorIcon::step: {
        QPolygonF points;points<<QPointF(5,3)<<QPointF(15,10)<<QPointF(5,17);
        p.setBrush(editorDesign::accent());p.setPen(Qt::NoPen);p.drawPolygon(points);
        if(icon==EditorIcon::step){p.setPen(QPen(ink,stroke));line(17,4,17,16);}break;
    }
    case EditorIcon::pause: line(7,3,7,17);line(13,3,13,17);break;
    case EditorIcon::inspector: box(2,3,16,14);line(12,3,12,17);line(14,7,16,7);line(14,11,16,11);break;
    case EditorIcon::objects: box(2,3,16,14);line(2,8,18,8);line(2,12,18,12);line(8,3,8,17);break;
    case EditorIcon::conflict: line(2,7,18,7);line(2,13,18,13);line(7,2,7,18);line(13,2,13,18);box(7,7,6,6);break;
    case EditorIcon::counter: line(2,3,18,3);box(6,6,8,3);box(6,11,8,3);box(6,16,8,3);break;
    case EditorIcon::section: line(4,3,4,17);line(16,3,16,17);line(4,10,16,10);break; // two lines, timed between
    case EditorIcon::zone: line(3,6,10,3);line(10,3,17,8);line(17,8,13,17);line(13,17,4,15);line(4,15,3,6);break; // a polygon
    // A road going away: two kerbs closing towards the horizon and the dashed centre between (S3.3b).
    case EditorIcon::road: line(2,18,7,2);line(18,18,13,2);line(10,17,10,14);line(10,10,10,8);line(10,5,10,4);break;
    case EditorIcon::grid: for(int v:{4,10,16}){line(v,2,v,18);line(2,v,18,v);}break;
    }
}
}
