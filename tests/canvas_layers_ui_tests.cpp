// Canvas style slice A (D423): one asphalt surface per level with a kerb stroked under it, Node
// polygons filled as junction road, and a resting Connector's lane centres as faint guides under
// the Link markings. Drawing only: hits, selection and the run are what they were.
#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/editor/canvas_style.hpp"
#include <QAction>
#include <QApplication>
#include <QGraphicsPathItem>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
using namespace multra;
namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(20);}
std::vector<QGraphicsPathItem*> tagged(EditorCanvas& c,const char* tag,const QString& id={}) {
    std::vector<QGraphicsPathItem*> out;
    for(auto* item:c.scene()->items())if(item->data(0).toString()==tag && (id.isEmpty() || item->data(1).toString()==id))
        if(auto* path=dynamic_cast<QGraphicsPathItem*>(item))out.push_back(path);
    return out;
}
int faint(EditorCanvas& c){return static_cast<int>(tagged(c,"connector-guide").size());}
int connectorLines(EditorCanvas& c,const QString& id,const char* kind) {
    int n=0;for(auto* item:tagged(c,"road-marking",id))n+=item->data(2).toString()==kind;return n;
}
double extreme(const std::vector<QGraphicsPathItem*>& items,bool highest) {
    double z=highest?-1e300:1e300;
    for(auto* i:items)z=highest?std::max(z,i->zValue()):std::min(z,i->zValue());
    return z;
}
ProjectDocument fixture(DrivingSide side) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={{"a",{{-80,0},{-50,0},{-20,0}},{{"a1",3.5},{"a2",3.5}}},
                     {"b",{{20,0},{80,0}},{{"b1",3.5},{"b2",3.5}}}};
    addConnectorRange(d,{"a","a1"},{"b","b1"},2,2);
    validateDocument(d);
    Link bridge{"bridge",{{0,-60},{0,60}},{{"u1",3.5}}};bridge.level=1;d.network.links.push_back(bridge);
    d.network.nodes.push_back(Node{"n","",{{-25,-12},{25,-12},{25,12},{-25,12}},false});
    return d;
}
void view(EditorCanvas& c,double scale){c.setTransform(QTransform::fromScale(scale,-scale));c.centerOn(0,0);c.redraw();settle();}
void standalone(EditorCanvas& c,DrivingSide side) {
    auto d=fixture(side);c.setDocument(&d);c.setTool(EditorCanvas::Tool::select);c.clearSelection(false);
    const auto connector=QString::fromStdString(d.network.connectors.front().id);
    view(c,4); // 14 px per lane
    // 1. Layers: every kerb of a level under every surface of it, lane guides over the surfaces and
    // under the Link markings; the level-1 kerb over the level-0 surfaces.
    const auto kerbs=tagged(c,"road-kerb"),surfaces=tagged(c,"road-surface"),nodes=tagged(c,"node-surface");
    require(!surfaces.empty() && nodes.size()==1,"Fixture lost its Connector or Node surface");
    const auto centre=surfaces.front()->sceneBoundingRect().center(); // items die at each redraw
    std::vector<QGraphicsPathItem*> ground,upper,linkMarks;
    for(auto* k:kerbs)(k->zValue()<100?ground:upper).push_back(k);
    require(ground.size()==4 && upper.size()==1,"Every Link, Connector and Node surface does not carry one kerb");
    for(auto* m:tagged(c,"road-marking"))if(m->data(1).toString()!=connector)linkMarks.push_back(m);
    require(extreme(ground,true)<extreme(surfaces,false) && extreme(ground,true)<extreme(nodes,false),
            "A kerb lies over a surface of its level");
    require(extreme(upper,false)>extreme(surfaces,true),"The bridge's kerb is under the ground surfaces");
    require(faint(c)>0,"A resting Connector at 14 px per lane shows no lane guides");
    const auto guides=tagged(c,"connector-guide",connector);
    require(guides.size()==2,"A resting two-lane Connector does not show one guide per lane");
    for(auto* g:guides)
        require(g->zValue()>extreme(surfaces,true) && g->zValue()<extreme(linkMarks,false) && g->pen().isCosmetic() &&
                g->pen().color().alpha()==canvasStyle::connectorGuideAlpha,"A lane guide is not a faint line between the surfaces and the Link markings");
    require(tagged(c,"road-marking",connector).empty(),"A resting Connector still draws lane markings");
    // 2. One asphalt: the Connector is its type's linkColor, not connectorColor.
    for(auto* s:surfaces)require(s->brush().color()==QColor("#49596d"),"A Connector surface is not road asphalt");
    for(auto* k:kerbs)require(k->pen().color()==canvasStyle::kerb() && !k->pen().isCosmetic() &&
        std::abs(k->pen().widthF()-canvasStyle::kerbWidth)<1e-12 && k->brush().style()==Qt::NoBrush,"A kerb is not the world-unit kerb pen");
    // 3. At rest no edge and no mouth edge; under 8 px no dividers either.
    require(connectorLines(c,connector,"edge")==0 && tagged(c,"connector-mouth-edge").empty(),
            "A resting Connector still draws its edges over the junction");
    view(c,8/3.5*(1-1e-3));
    require(faint(c)==0 && !tagged(c,"road-kerb").empty(),"Lane guides drawn under 8 px per lane");
    view(c,8/3.5*(1-1e-12)); // D205: one rounding below is no zoom change
    require(faint(c)>0,"A rounding of the scale hid the lane guides");
    // ...and selected, hovered or under the Connect tool it draws today's full set at z+4.5.
    view(c,4);
    c.select(connector.toStdString());c.redraw();settle();
    require(connectorLines(c,connector,"edge")==2 && faint(c)==0,"A selected Connector lost its full edges");
    for(auto* m:tagged(c,"road-marking",connector))require(std::abs(m->zValue()-4.5)<1e-12,"A selected Connector's lines moved");
    c.clearSelection(false);c.redraw();settle();
    require(connectorLines(c,connector,"edge")==0,"Clearing the selection kept the edges");
    QTest::mouseMove(c.viewport(),c.mapFromScene(QPointF(-12,centre.y())));settle(); // clear of the level-1 bridge at x=0
    require(connectorLines(c,connector,"edge")==2,"A hovered Connector does not draw its edges");
    QTest::mouseMove(c.viewport(),c.mapFromScene(QPointF(0,50)+QPointF(40,0)));settle();
    c.setTool(EditorCanvas::Tool::connect);c.redraw();settle();
    require(connectorLines(c,connector,"edge")==2,"The Connect tool does not show Connector edges");
    c.setTool(EditorCanvas::Tool::select);c.redraw();settle();
    // 4. Hits never read the scene: removing every kerb changes no answer.
    const std::vector<Point> probes{{-30,-1.75},{-30,-7.2},{-30,-7.6},{0,-1.75},{0,0},{0,40},{22,-12.1},{50,3.6}};
    std::vector<std::vector<std::pair<std::string,double>>> before;
    for(const auto p:probes)before.push_back(c.hitObjects(p));
    for(auto* k:tagged(c,"road-kerb")){c.scene()->removeItem(k);delete k;}
    for(std::size_t i=0;i<probes.size();++i)require(c.hitObjects(probes[i])==before[i],"A kerb changed a hit");
    // 5. Under 3 px per lane the kerb goes with the markings (D200); wireframe draws none at all.
    view(c,3/3.5*(1-1e-3));
    require(tagged(c,"road-kerb").empty() && !tagged(c,"road-surface").empty() && tagged(c,"node-surface").size()==1,
            "Under 3 px the kerb stayed or a surface went");
    view(c,4);c.setWireframe(true);
    require(tagged(c,"road-kerb").empty() && tagged(c,"node-surface").empty() && faint(c)==0,"Wireframe draws road surface");
    c.setWireframe(false);c.setDocument(nullptr);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Data directory required");const std::filesystem::path data(argv[1]);
        {
            EditorCanvas c;c.setDisplayCatalog({{},{{"default",{},"#49596d","#607d8b","#d0dfeb","#facc15"}}});
            c.resize(1000,650);c.show();settle();
            for(const auto side:{DrivingSide::left,DrivingSide::right})standalone(c,side);
        }
        // 6. A real run: the resting dividers go while the run is shown, the kerb and asphalt stay.
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));settle();
        auto& canvas=*window.canvas();
        view(canvas,9);
        require(faint(canvas)>=20 && !tagged(canvas,"road-kerb").empty(),"The shipped junction has no lane guide per Connector lane, or no kerb");
        if(argc>2)require(canvas.viewport()->grab().save(QString::fromUtf8(argv[2])+"-edit.png"),"Screenshot failed");
        for(int i=0;i<200;++i)window.findChild<QAction*>("editorStep")->trigger();
        settle();
        require(canvas.renderedVehicles()>0,"The forcing: the run shows vehicles");
        require(faint(canvas)==0 && !tagged(canvas,"road-kerb").empty() && !tagged(canvas,"road-surface").empty(),
                "A run kept the lane guides or lost the road");
        std::cout<<"Canvas layers: kerb under one asphalt, Node fill, faint resting lane guides, unchanged hits passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
