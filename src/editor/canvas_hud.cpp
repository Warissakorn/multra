#include "canvas_hud.hpp"
#include "ui_design_tokens.hpp"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace trafficsim {
namespace {
constexpr int inset=editorDesign::space2, pad=editorDesign::space1, gap=editorDesign::space2;
constexpr int arrowSize=2*editorDesign::space4, barHeight=editorDesign::space1+2; // arrow box: arrow south of centre, N north
}
CanvasHud::CanvasHud(QWidget* viewport):QWidget(viewport) {
    setObjectName("editorCanvasHud");
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAutoFillBackground(true);setBackgroundRole(QPalette::Base);
    setFont(editorDesign::numericFont());
}
double CanvasHud::scaleStep(double pixelsPerMetre) {
    if(!(pixelsPerMetre>0))return 1;
    const double most=160/pixelsPerMetre, decade=std::pow(10.0,std::floor(std::log10(most)));
    for(double m:{5.0,2.0,1.0})if(m*decade<=most*(1+1e-12))return m*decade;
    return decade;
}
void CanvasHud::setUnits(const QString& metres,const QString& zoom) {
    metresFormat_=metres;zoomFormat_=zoom;update();
}
void CanvasHud::setView(double pixelsPerMetre,std::optional<double> northDegrees) {
    perMetre_=pixelsPerMetre;metres_=scaleStep(pixelsPerMetre);pixels_=metres_*pixelsPerMetre;north_=northDegrees;
    setProperty("scaleMetres",metres_);setProperty("scalePixels",pixels_);
    setProperty("northDegrees",north_?QVariant(*north_):QVariant());
    update();
}
QString CanvasHud::metresText() const {
    // Below a metre a step is 0.5/0.2/0.1…: as many decimals as the step needs, no more.
    const int decimals=metres_>=1?0:static_cast<int>(std::ceil(-std::log10(metres_)-1e-9));
    return metresFormat_.arg(editorDesign::formatValue(metres_,decimals));
}
QString CanvasHud::zoomText() const {
    return zoomFormat_.arg(editorDesign::formatValue(perMetre_,perMetre_<10?2:perMetre_<100?1:0));
}
QSize CanvasHud::contentSize(bool stacked) const {
    const QFontMetrics m(font());
    const int scaleWidth=std::max(static_cast<int>(std::ceil(pixels_)),m.horizontalAdvance(metresText()));
    const int arrow=north_?arrowSize+gap:0, zoom=m.horizontalAdvance(zoomText());
    const int text=m.height();
    if(stacked)return {2*pad+arrow+std::max(scaleWidth,zoom),2*pad+barHeight+2*text+pad};
    return {2*pad+arrow+scaleWidth+gap+zoom,2*pad+std::max(barHeight+text,arrow)};
}
void CanvasHud::place(const QRect& viewport,const QRect& avoid) {
    stacked_=contentSize(false).width()>viewport.width()-2*inset;
    const auto size=contentSize(stacked_);resize(size);
    QRect target(viewport.right()+1-inset-size.width(),viewport.bottom()+1-inset-size.height(),size.width(),size.height());
    if(avoid.isValid()&&target.intersects(avoid))target.moveBottom(avoid.top()-inset);
    move(std::max(viewport.left(),target.left()),std::max(viewport.top(),target.top()));
    show();raise();
}
void CanvasHud::paintEvent(QPaintEvent*) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    const auto ink=palette().color(QPalette::WindowText);
    p.setPen(QPen(palette().color(QPalette::Mid),1));p.drawRect(QRectF(rect()).adjusted(0.5,0.5,-0.5,-0.5));
    const QFontMetrics m(font());
    int x=pad;
    if(north_) {
        // An arrow pointing at an upright N, both turned to true north.
        const QPointF centre(x+arrowSize/2.0,height()/2.0);
        p.save();p.translate(centre);p.rotate(*north_);
        QPainterPath arrow;arrow.moveTo(0,-1);arrow.lineTo(arrowSize/5.0,arrowSize/2.0-2);
        arrow.lineTo(0,arrowSize/3.0);arrow.lineTo(-arrowSize/5.0,arrowSize/2.0-2);arrow.closeSubpath();
        p.setPen(Qt::NoPen);p.setBrush(ink);p.drawPath(arrow);p.restore();
        const double r=*north_*M_PI/180, reach=arrowSize/2.0-m.height()/2.0;
        QFont bold=font();bold.setWeight(QFont::DemiBold);p.setFont(bold);p.setPen(ink);
        const QPointF letter=centre+QPointF(std::sin(r)*reach,-std::cos(r)*reach);
        p.drawText(QRectF(letter.x()-m.height()/2.0,letter.y()-m.height()/2.0,m.height(),m.height()),Qt::AlignCenter,QStringLiteral("N"));
        p.setFont(font());x+=arrowSize+gap;
    }
    // Scale bar: a solid bar with end ticks, its length the metres it stands for.
    const double barTop=pad;
    p.setPen(Qt::NoPen);p.setBrush(ink);p.drawRect(QRectF(x,barTop+barHeight/2.0-1,pixels_,2));
    p.drawRect(QRectF(x,barTop,1.5,barHeight));p.drawRect(QRectF(x+pixels_-1.5,barTop,1.5,barHeight));
    p.setPen(ink);
    p.drawText(QRectF(x,barTop+barHeight,std::max(pixels_,1.0*m.horizontalAdvance(metresText())),m.height()),Qt::AlignLeft|Qt::AlignVCenter,metresText());
    const int scaleWidth=std::max(static_cast<int>(std::ceil(pixels_)),m.horizontalAdvance(metresText()));
    if(stacked_)p.drawText(QRectF(x,barTop+barHeight+m.height(),width()-x-pad,m.height()),Qt::AlignLeft|Qt::AlignVCenter,zoomText());
    else p.drawText(QRectF(x+scaleWidth+gap,0,width()-x-scaleWidth-gap-pad,height()),Qt::AlignLeft|Qt::AlignVCenter,zoomText());
}
}
