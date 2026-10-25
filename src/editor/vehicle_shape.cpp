#include "vehicle_shape.hpp"
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <utility>

namespace multra {
namespace {
// Coordinates are fractions of body length/width; x=0 is always the front bumper.
void box(QPainterPath& p,double from,double to,double halfWidth,double radius=.025) {
    p.addRoundedRect(-to,-halfWidth,to-from,2*halfWidth,radius,radius);
}
void window(QPainterPath& p,double from,double to,double frontWidth,double rearWidth) {
    p.moveTo(-from,-frontWidth);p.lineTo(-from,frontWidth);
    p.lineTo(-to,rearWidth);p.lineTo(-to,-rearWidth);p.closeSubpath();
}
QPainterPath body(std::string_view kind) {
    QPainterPath p;
    if(kind=="bus") {box(p,0,1,.5,.055);return p;}
    if(kind=="motorcycle") {
        QPainterPath tyres;box(tyres,0,.24,.12,.06);box(tyres,.73,1,.14,.06);
        p.moveTo(-.14,0);p.lineTo(-.27,-.5);p.lineTo(-.34,-.5);
        p.lineTo(-.33,-.22);p.cubicTo(-.48,-.38,-.72,-.33,-.84,-.16);
        p.lineTo(-.84,.16);p.cubicTo(-.72,.33,-.48,.38,-.33,.22);
        p.lineTo(-.34,.5);p.lineTo(-.27,.5);p.closeSubpath();return p.united(tyres);
    }
    if(kind=="truck") {
        // A rigid truck: cab and load box joined by a narrower chassis, no trailer joint.
        p.moveTo(0,-.33);p.quadTo(0,-.43,-.025,-.46);
        p.lineTo(-.20,-.46);p.quadTo(-.23,-.46,-.24,-.34);
        p.lineTo(-.30,-.34);p.lineTo(-.30,-.475);p.quadTo(-.30,-.5,-.315,-.5);
        p.lineTo(-.985,-.5);p.quadTo(-1,-.5,-1,-.46);p.lineTo(-1,.46);
        p.quadTo(-1,.5,-.985,.5);p.lineTo(-.315,.5);p.quadTo(-.30,.5,-.30,.475);
        p.lineTo(-.30,.34);p.lineTo(-.24,.34);p.quadTo(-.23,.46,-.20,.46);
        p.lineTo(-.025,.46);p.quadTo(0,.43,0,.33);
        p.closeSubpath();return p;
    }
    if(kind=="van") {
        // Short cab-over nose, nearly parallel sides and upright rear doors.
        p.moveTo(0,-.33);p.quadTo(0,-.45,-.045,-.48);
        p.lineTo(-.16,-.5);p.lineTo(-.965,-.5);p.quadTo(-1,-.5,-1,-.43);
        p.lineTo(-1,.43);p.quadTo(-1,.5,-.965,.5);p.lineTo(-.16,.5);
        p.lineTo(-.045,.48);p.quadTo(0,.45,0,.33);p.closeSubpath();return p;
    }
    if(kind=="pickup") {
        p.moveTo(0,-.36);p.quadTo(0,-.46,-.06,-.48);
        p.lineTo(-.30,-.5);p.lineTo(-.95,-.5);p.quadTo(-1,-.5,-1,-.44);
        p.lineTo(-1,.44);p.quadTo(-1,.5,-.95,.5);p.lineTo(-.30,.5);
        p.lineTo(-.06,.48);p.quadTo(0,.46,0,.36);p.closeSubpath();return p;
    }
    p.moveTo(0,-.30);p.quadTo(0,-.43,-.055,-.46);
    p.lineTo(-.25,-.5);p.lineTo(-.86,-.5);
    p.quadTo(-1,-.5,-1,-.34);p.lineTo(-1,.34);
    p.quadTo(-1,.5,-.86,.5);p.lineTo(-.25,.5);
    p.lineTo(-.055,.46);p.quadTo(0,.43,0,.30);p.closeSubpath();return p;
}
}
void VehicleDrawing::translate(double x,double y) {
    for(auto* p:{&body,&glass,&panels,&lamps})p->translate(x,y);
}
VehicleDrawing vehicleDrawing(double length,double width,bool detailed,std::string_view kind) {
    VehicleDrawing d;
    if(!std::isfinite(length)||!std::isfinite(width)||length<=0||width<=0)return d;
    if(kind.empty())kind=length>7?"truck":"car";
    d.body=body(kind);
    if(detailed) {
        if(kind=="motorcycle") {
            box(d.glass,.22,.32,.20,.035);box(d.glass,.53,.80,.17,.07);
            box(d.panels,.35,.52,.20,.07);
        } else if(kind=="truck") {
            window(d.glass,.045,.105,.34,.37);
            box(d.panels,.125,.205,.29,.018); // raised cab roof behind the windscreen
            d.glass.addRect(-.205,-.42,.085,.065);d.glass.addRect(-.205,.355,.085,.065);
            box(d.glass,.25,.285,.25,.008); // exposed chassis between cab and fixed load box
            box(d.panels,.335,.955,.43,.015);
            // Recesses in the roof highlight describe corrugations without adding scene items.
            for(int i=0;i<8;++i)d.panels.addRect(-.385-i*.072,-.405,.009,.81);
            d.glass.addRect(-.987,-.41,.013,.82); // rear door/header line
        } else if(kind=="bus") {
            window(d.glass,.025,.09,.37,.41);box(d.glass,.92,.965,.37);
            for(int i=0;i<7;++i) {
                const double x=.14+i*.105;
                d.glass.addRect(-x-.075,-.46,.075,.065);
                d.glass.addRect(-x-.075,.395,.075,.065);
            }
            box(d.panels,.32,.43,.23);box(d.panels,.66,.77,.23);
        } else if(kind=="van") {
            window(d.glass,.14,.25,.34,.40);
            box(d.glass,.935,.975,.38,.012); // upright tailgate glass
            // Cab door, sliding door and rear-quarter windows, divided by body pillars.
            for(const auto& span:{std::pair{.15,.27},std::pair{.32,.52},std::pair{.57,.76},std::pair{.81,.91}})
                for(const double side:{-1.,1.})
                    d.glass.addRoundedRect(-span.second,side<0?-.475:.405,
                        span.second-span.first,.07,.008,.008);
            box(d.panels,.285,.91,.34,.035); // continuous high roof
            box(d.panels,.04,.115,.29,.025); // shallow bonnet ahead of the windscreen
        } else {
            const bool pickup=kind=="pickup";
            const double start=.23,end=pickup?.62:.77;
            window(d.glass,start,start+.12,.36,.41);
            window(d.glass,end-.10,end,.41,.35);
            for(const double side:{-1.,1.})
                d.glass.addRect(-end+.12,side<0?-.46:.37,end-start-.26,.09);
            box(d.panels,start+.15,end-.13,.30);
            if(pickup)box(d.glass,.68,.94,.39); // open bed, visibly distinct from the cab
        }
        // Pale paired headlamps identify the nose without implying a signal/brake state.
        if(kind!="motorcycle")for(const double side:{-1.,1.})
            d.lamps.addRoundedRect(-.065,side<0?-.35:.23,.035,.12,.01,.01);
        else box(d.lamps,.015,.065,.08,.015);
    }
    if(kind=="truck") {
        // Keep the cab as short as the smaller rigid truck (nominally 2.04 m).
        // Move the load-box front forward to retain the full type length and rear bumper.
        constexpr double cabLength=2.04,boxFrom=.30;
        const double frontScale=std::min(1.,cabLength/(.24*length));
        const double boxStart=boxFrom*frontScale;
        for(auto* path:{&d.body,&d.glass,&d.panels,&d.lamps})for(int i=0;i<path->elementCount();++i) {
            const auto point=path->elementAt(i);const double from=-point.x;
            const double mapped=from<=boxFrom?from*frontScale:
                boxStart+(from-boxFrom)*(1-boxStart)/(1-boxFrom);
            path->setElementPositionAt(i,-mapped,point.y);
        }
    }
    const auto metres=QTransform::fromScale(length,width);
    for(auto* p:{&d.body,&d.glass,&d.panels,&d.lamps})*p=metres.map(*p);
    return d;
}
QPainterPath vehicleShape(double length,double width,bool detailed,std::string_view kind) {
    return vehicleDrawing(length,width,detailed,kind).body;
}
}
