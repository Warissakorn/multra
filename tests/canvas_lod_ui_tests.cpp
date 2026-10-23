#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/editor/vehicle_shape.hpp"
#include "../src/editor/canvas_style.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QGraphicsPathItem>
#include <QStandardPaths>
#include <QTest>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
using namespace multra;
// UI slice e2 (D200): below 3 px per lane the 0.10 m lane markings are not drawn; nothing else
// changes -- surfaces, heads, selection, handles, hits and Ctrl+A are what they were. Vehicles keep
// their 4x2.5 px floor and show the windshield/cab only from 14 px.
namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(20);}
bool decoration(const QString& tag){return tag==QStringLiteral("road-marking")||tag==QStringLiteral("connector-mouth-edge");}
std::map<QString,int> tags(EditorCanvas& canvas) {
    std::map<QString,int> out;
    for(auto* item:canvas.scene()->items())++out[item->data(0).toString()];
    return out;
}
std::map<QString,int> essential(EditorCanvas& canvas) {
    auto out=tags(canvas);std::erase_if(out,[](const auto& entry){return decoration(entry.first);});return out;
}
int markings(EditorCanvas& canvas){int n=0;for(auto* item:canvas.scene()->items())n+=decoration(item->data(0).toString());return n;}
void view(EditorCanvas& canvas,const QTransform& transform){canvas.setTransform(transform);canvas.centerOn(0,0);canvas.redraw();settle();}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
        auto& canvas=*window.canvas();
        const auto fixture=nlohmann::json::parse(std::ifstream(data/"scenarios/crossing.json"))["network"];
        double narrowest=1e9;
        for(const auto& link:fixture["links"])for(const auto& lane:link["lanes"])narrowest=std::min(narrowest,lane["width"].get<double>());
        const double at=3/narrowest, under=at*(1-1e-3);
        // 0. D205: a scale one rounding below the threshold (a rotated view's hypot on MSVC) is not a
        // zoom change -- the forcing first: the rounded product really is below 3 px.
        const double rounded=at*(1-1e-12);
        require(narrowest*rounded<canvasStyle::minLanePixels,"Setup: the rounded scale is not below the threshold");
        require(canvasStyle::laneMarkingsReadable(narrowest,rounded),"A rounding of the scale hides the markings");
        // 1. The markings follow the lane threshold, and only they do.
        view(canvas,QTransform::fromScale(at,-at));
        const auto full=tags(canvas);
        require(full.count("road-marking")&&full.at("road-marking")>0,"No lane markings at 3 px per lane");
        require(full.count("connector-mouth-edge")&&full.at("connector-mouth-edge")>0,"No mouth edges at 3 px per lane");
        if(argc>2)require(canvas.viewport()->grab().save(QString::fromUtf8(argv[2])+"-at-3px.png"),"Screenshot failed");
        const auto kept=essential(canvas);
        view(canvas,QTransform::fromScale(under,-under));
        require(markings(canvas)==0,"Lane markings drawn below 3 px per lane");
        if(argc>2)require(canvas.viewport()->grab().save(QString::fromUtf8(argv[2])+"-under-3px.png"),"Screenshot failed");
        // 2. Every other item -- surfaces, heads, stop lines, conflicts -- is still there.
        require(essential(canvas)==kept,"Something besides lane decoration changed below 3 px");
        require(kept.count("road-surface")&&kept.count("stop-line"),"Fixture lost its surfaces or heads");
        // 6. Rotation and flip read the same rotation-safe scale.
        for(const double s:{at,under}) {
            const int expected=s==at?full.at("road-marking")+full.at("connector-mouth-edge"):0;
            view(canvas,QTransform().rotate(37)*QTransform::fromScale(s,-s));
            require(markings(canvas)==expected,"A rotated view decides the markings differently");
            view(canvas,QTransform::fromScale(s,s));
            require(markings(canvas)==expected,"A flipped view decides the markings differently");
        }
        // 3, 4. A selected Link keeps its outline, arrows and grips; hits are the model's.
        const auto& link=fixture["links"].front();const auto& line=link["geometry"];
        const auto middle=line[line.size()/2];
        const Point probe{(line.front()["x"].get<double>()+middle["x"].get<double>())/2,(line.front()["y"].get<double>()+middle["y"].get<double>())/2};
        canvas.select(link["id"].get<std::string>());
        std::vector<std::vector<std::pair<std::string,double>>> hits;std::vector<std::map<QString,int>> selected;
        for(const double s:{at,under}) {
            view(canvas,QTransform::fromScale(s,-s));
            selected.push_back(essential(canvas));hits.push_back(canvas.hitObjects(probe));
        }
        require(selected[0]==selected[1],"Selection feedback changed below 3 px");
        require(selected[0].count("object-feedback")&&selected[0].count("direction-arrow")&&selected[0].count("geometry-point"),
                "Selected Link lost its outline, direction or grips");
        require(!hits[0].empty()&&hits[0]==hits[1],"Hits changed with the lane threshold");
        // 5. Ctrl+A is the same drawing and the same picking either side.
        canvas.setWireframe(true);
        std::vector<std::map<QString,int>> wire;
        for(const double s:{at,under}){view(canvas,QTransform::fromScale(s,-s));wire.push_back(tags(canvas));require(markings(canvas)==0,"Ctrl+A drew markings");require(canvas.hitObjects(probe)==hits[0],"Ctrl+A picking changed");}
        require(wire[0]==wire[1],"Ctrl+A drawing changed with the lane threshold");
        canvas.setWireframe(false);canvas.select("");
        // 7. Vehicles: the 14 px detail boundary and the 4 x 2.5 px floor (already in, pinned here).
        for(int i=0;i<300;++i)window.findChild<QAction*>("editorStep")->trigger();
        settle();
        const auto vehicles=[&] {
            std::vector<QGraphicsPathItem*> out;
            for(auto* item:canvas.scene()->items())
                if(item->data(0).toString()==QStringLiteral("run-vehicle"))out.push_back(static_cast<QGraphicsPathItem*>(item));
            return out;
        };
        view(canvas,QTransform::fromScale(40,-40));
        auto cars=vehicles();require(!cars.empty(),"Run fixture has no vehicles");
        const auto type=cars.front()->data(1).toString();const auto bounds=cars.front()->path().boundingRect();
        const double length=bounds.width(),width=bounds.height(),count=double(cars.size());
        for(const double s:{14/length,14/length*(1-1e-3)}) {
            view(canvas,QTransform::fromScale(s,-s));cars=vehicles();
            require(double(cars.size())==count,"Vehicle count changed with zoom");
            for(auto* car:cars) {
                if(car->data(1).toString()!=type)continue;
                const auto expected=vehicleShape(length,width,s*length>=14).translated(car->data(3).toDouble(),0);
                require(car->path()==expected,"Vehicle detail does not switch at 14 px");
            }
        }
        view(canvas,QTransform::fromScale(0.05,-0.05));
        for(auto* car:vehicles()) {
            const auto r=car->path().boundingRect();
            require(r.width()*0.05>=4-1e-9&&r.height()*0.05>=2.5-1e-9,"A vehicle fell below the 4 x 2.5 px floor");
        }
        std::cout<<"Canvas LOD: lane-marking threshold, kept essentials, hits, Ctrl+A and vehicle detail passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
