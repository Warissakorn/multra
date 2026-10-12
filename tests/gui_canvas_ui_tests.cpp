#include "../src/editor/canvas.hpp"
#include "../src/editor/canvas_labels.hpp"
#include "../src/shell/editor_style.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <QApplication>
#include <QGraphicsLineItem>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QTest>
#include <cmath>
#include <iostream>
#include <map>

using namespace trafficsim;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
QPoint pixel(EditorCanvas& c,Point p) {
    const auto point=c.mapFromScene(p.x,p.y);require(c.viewport()->rect().contains(point),"Gesture forcing outside viewport");return point;
}
void mouse(EditorCanvas& c,QEvent::Type type,QPoint point,Qt::MouseButton button=Qt::NoButton,Qt::MouseButtons buttons={},Qt::KeyboardModifiers modifiers={}) {
    require(c.viewport()->rect().contains(point),"Mouse forcing outside viewport");
    QMouseEvent event(type,QPointF(point),QPointF(c.viewport()->mapToGlobal(point)),button,buttons,modifiers);
    QApplication::sendEvent(c.viewport(),&event);QApplication::processEvents();
}
void click(EditorCanvas& c,Point p,Qt::MouseButton button=Qt::LeftButton,Qt::KeyboardModifiers modifiers={}) {
    const auto at=pixel(c,p);mouse(c,QEvent::MouseButtonPress,at,button,button,modifiers);
    mouse(c,QEvent::MouseButtonRelease,at,button,{},modifiers);
}
void hover(EditorCanvas& c,Point p,Qt::KeyboardModifiers modifiers={}) {mouse(c,QEvent::MouseMove,pixel(c,p),Qt::NoButton,{},modifiers);}
QString state(EditorCanvas& c){return c.property("editorCursorState").toString();}
void creationCursors(EditorCanvas& c) {
    ProjectDocument d;const auto a=addLink(d,{{-80,0},{-20,0}},1,3.5),b=addLink(d,{{20,0},{80,0}},1,3.5);
    c.setDocument(&d);c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);c.snap=false;
    int links=0,connectors=0,points=0;c.createLinkGesture=[&](const auto&){++links;};
    c.createRangeGesture=[&](auto from,auto to,const auto&){require(from.linkId==a && to.linkId==b,"Gesture changed lane ownership");++connectors;};
    c.editGeometry=[&](const auto&,const auto&){++points;};
    c.setTool(EditorCanvas::Tool::draw);hover(c,{-50,-40},Qt::ControlModifier);require(state(c)=="link","Draw over empty space has no Link cursor");
    const auto p=pixel(c,{-50,-40}),q=pixel(c,{40,-40});
    mouse(c,QEvent::MouseButtonPress,p,Qt::RightButton,Qt::RightButton,Qt::ControlModifier);
    mouse(c,QEvent::MouseMove,q,Qt::NoButton,Qt::RightButton,Qt::ControlModifier);require(state(c)=="link","Link drag lost its cursor");
    mouse(c,QEvent::MouseButtonRelease,q,Qt::RightButton,{},Qt::ControlModifier);require(links==1,"Existing Link gesture changed");
    hover(c,{-30,0},Qt::ControlModifier);require(state(c)=="connector","Draw on a lane does not predict Connector creation");
    mouse(c,QEvent::MouseButtonPress,pixel(c,{-30,0}),Qt::RightButton,Qt::RightButton,Qt::ControlModifier);
    mouse(c,QEvent::MouseMove,pixel(c,{40,0}),Qt::NoButton,Qt::RightButton,Qt::ControlModifier);
    require(state(c)=="connector","Actual Connector drag has the Link cursor");
    mouse(c,QEvent::MouseButtonRelease,pixel(c,{40,0}),Qt::RightButton,{},Qt::ControlModifier);require(connectors==1,"Existing Connector gesture changed");
    c.setTool(EditorCanvas::Tool::select);hover(c,{-50,0});require(state(c)=="select" && c.cursor().shape()==Qt::ArrowCursor,"Select pointing changed");
    mouse(c,QEvent::MouseButtonPress,pixel(c,{-50,0}),Qt::RightButton,Qt::RightButton,Qt::ControlModifier);
    require(state(c)=="point-insert","Select Ctrl-click has no point insertion cursor");
    mouse(c,QEvent::MouseButtonRelease,pixel(c,{-50,0}),Qt::RightButton,{},Qt::ControlModifier);require(points==1,"Select Ctrl-click stopped inserting points");
    c.setTool(EditorCanvas::Tool::connect);click(c,{-30,0},Qt::RightButton,Qt::ControlModifier);
    require(c.pickingConnectorTarget(),"Two-click source was not picked");hover(c,{0,-35});require(state(c)=="unavailable","Unavailable Connector target is not indicated");
    hover(c,{40,0});require(state(c)=="connector","Valid two-click target has no Connector cursor");
    QTest::keyClick(&c,Qt::Key_Escape);require(!c.pickingConnectorTarget(),"Escape did not cancel the source");
    c.setTool(EditorCanvas::Tool::select);mouse(c,QEvent::MouseButtonPress,p,Qt::MiddleButton,Qt::MiddleButton);
    require(state(c)=="pan" && c.cursor().shape()==Qt::ClosedHandCursor,"Pan cursor is inconsistent");
    mouse(c,QEvent::MouseButtonRelease,p,Qt::MiddleButton);require(state(c)=="select","Pan cursor outlived its gesture");
    c.createLinkGesture={};c.createRangeGesture={};c.editGeometry={};c.setDocument(nullptr);
}
void routeEndpoints(EditorCanvas& c) {
    ProjectDocument d;const auto road=addLink(d,{{-80,0},{80,0}},1,3.5);c.setDocument(&d);
    c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);c.setTool(EditorCanvas::Tool::route);c.snap=false;
    int commits=0;std::optional<RouteEndMarker> committed;
    c.markedRouteCommitted=[&](const auto& path,auto start,auto marker){require(path==std::vector<std::string>{road} && start,"Route contract changed");committed=marker;++commits;};
    click(c,{-40,0});hover(c,{20,0});require(c.routeEndMarker() && std::abs(c.routeEndMarker()->station-100)<1,"Route hover snapped to road end");
    hover(c,{40,0});require(std::abs(c.routeEndMarker()->station-120)<1,"Endpoint did not move within the same road");
    bool crossbar=false;for(auto* item:c.scene()->items())if(item->data(0)=="route-end") {
        const auto* line=dynamic_cast<QGraphicsLineItem*>(item);require(line && line->pen().style()==Qt::DashLine,"Preview endpoint is not dashed");
        require(std::abs(line->line().center().x()-40)<1,"Preview line is not at its chosen station");crossbar=true;
    }
    require(crossbar,"Route destination preview missing");hover(c,{-60,0});require(state(c)=="unavailable","Upstream destination is not rejected");
    click(c,{-60,0});require(commits==0 && !c.routeDraft().empty(),"Upstream click committed a route");
    hover(c,{30,0});require(state(c)!="unavailable","Route did not recover after upstream rejection");
    click(c,{50,0});require(commits==1 && committed && std::abs(committed->station-130)<1,"Final click did not determine destination");
    require(c.routeDraft().empty() && !c.routeEndMarker(),"Endpoint state outlived commit");
    c.markedRouteCommitted={};c.setDocument(nullptr);
}
void labels(EditorCanvas& c,const QString& screenshot) {
    ProjectDocument d;const auto a=addLink(d,{{-80,0},{80,0}},1,3.5);d.definition.emplace();
    for(int i=0;i<8;++i) {
        VehicleInput input;input.id="input-"+std::to_string(i);input.linkId=a;input.vehicleTypeId="car";input.vehiclesPerHour=600+i*137;input.endTime=60;
        putInput(d,input);
    }
    RoutingDecision decision;decision.id="decision";decision.linkId=a;decision.position=80;
    decision.name="จุดตัดสินใจเส้นทางที่มีชื่อยาวมากสำหรับตรวจการจัดวางข้อความบนพื้นที่แก้ไขโครงข่าย";putRoutingDecision(d,decision);
    c.setDocument(&d);c.setDemandLabels("veh/h","%1 periods");c.setTool(EditorCanvas::Tool::select);
    c.setTransform(QTransform::fromScale(4,-4));c.centerOn(0,0);QApplication::processEvents();c.viewport()->repaint();
    std::vector<QRectF> rectangles;std::map<QString,QPointF> positions;int count=0;
    for(auto* item:c.scene()->items())if(auto* label=dynamic_cast<CanvasLabelItem*>(item);label && label->isVisible()) {
        const auto bounds=label->deviceTransform(c.viewportTransform()).mapRect(label->boundingRect());
        require(QRectF(c.viewport()->rect()).contains(bounds),"Canvas label escaped viewport");
        for(const auto& previous:rectangles)require(!previous.intersects(bounds),"Canvas labels overlap");
        rectangles.push_back(bounds);
        const auto id=item->data(1).toString();positions[id]=item->pos();
        require(c.demandObjectAt(bounds.center().toPoint()).second==id.toStdString(),"Displaced label lost object identity");
        const auto origin=c.viewportTransform().map(label->pos());const double ratio=c.devicePixelRatioF();
        require(std::abs(origin.x()*ratio-std::round(origin.x()*ratio))<.01 && std::abs(origin.y()*ratio-std::round(origin.y()*ratio))<.01,"Label text is not snapped to device pixels");
        require(label->toolTip().contains(id) && label->cacheMode()==QGraphicsItem::NoCache,"Label lost full text or uses a bitmap cache");++count;
    }
    require(count==9,"Dense fixture did not retain all labels when space was available");
    c.viewport()->repaint();for(auto* item:c.scene()->items())if(dynamic_cast<CanvasLabelItem*>(item) && item->isVisible())
        require(positions.at(item->data(1).toString())==item->pos(),"Label layout is unstable between frames");
    int moves=0;c.decisionMoved=[&](const auto&,double){++moves;};
    for(auto* item:c.scene()->items())if(item->data(0)=="decision-label" && item->isVisible()) {
        const auto bounds=item->deviceTransform(c.viewportTransform()).mapRect(item->boundingRect());
        const auto start=bounds.center().toPoint(),end=start+QPoint(20,0);
        mouse(c,QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);mouse(c,QEvent::MouseMove,end,Qt::NoButton,Qt::LeftButton);
        mouse(c,QEvent::MouseButtonRelease,end,Qt::LeftButton);break;
    }
    require(moves==0,"Dragging a displaced name moved the physical decision");c.decisionMoved={};
    c.viewport()->grab().save(screenshot);c.setTransform(QTransform::fromScale(.08,-.08));c.redraw();c.viewport()->repaint();
    for(auto* item:c.scene()->items())if(dynamic_cast<CanvasLabelItem*>(item))require(!item->isVisible(),"Low zoom did not suppress crowded secondary labels");
    c.setDocument(nullptr);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=4,"Expected data, screenshot and scale");loadEditorFont(std::filesystem::path(argv[1]));
        EditorCanvas canvas;applyEditorStyle(&canvas);canvas.resize(960,640);canvas.show();canvas.setFocus();QApplication::processEvents();
        require(std::abs(canvas.devicePixelRatioF()-QString::fromLocal8Bit(argv[3]).toDouble())<.02,"Canvas DPI forcing failed");
        const auto icon=editorApplicationIcon();for(int size:{16,24,32,48,64,128,256})require(!icon.pixmap(size,size).isNull(),"Embedded app icon missing a size");
        creationCursors(canvas);routeEndpoints(canvas);labels(canvas,QString::fromLocal8Bit(argv[2]));
        std::cout<<"GUI canvas passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
