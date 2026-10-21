#include "../src/shell/editor_window.hpp"
#include "../src/editor/run_vehicle_item.hpp"
#include "../src/editor/road_crossbar.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QBuffer>
#include <QTemporaryDir>
#include <fstream>
#include <map>
#include <tuple>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineEdit>
#include <QScrollBar>
#include <QPainter>
#include <QStandardPaths>
#include <QTest>
#include <algorithm>
#include <string>
#include <filesystem>
#include <iostream>
using namespace multra;
// UI slice e1b (D199): run vehicles carry a neutral casing and run signal bars a shape per state,
// both drawn only: hit regions, poses and crossbars are what they were.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(20);}
double luminance(QRgb c){return (0.2126*qRed(c)+0.7152*qGreen(c)+0.0722*qBlue(c))/255;}
QImage render(QGraphicsItem* item,QColor background,double scale) {
    QGraphicsScene scene;scene.addItem(item);
    const auto bounds=item->sceneBoundingRect().adjusted(-2/scale,-2/scale,2/scale,2/scale);
    QImage image(QSize(int(bounds.width()*scale),int(bounds.height()*scale)),QImage::Format_RGB32);image.fill(background);
    QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);scene.render(&p,QRectF(image.rect()),bounds);p.end();
    scene.removeItem(item);
    return image;
}
QSet<int> ink(const QImage& image,QRgb background) {
    QSet<int> out;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)
        if(std::abs(luminance(image.pixel(x,y))-luminance(background))>0.05)out.insert(y*image.width()+x);
    return out;
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        // A vehicle the colour of its background still shows its edge; its hit shape is the bare path.
        QPainterPath body;body.addRect(-4.5,-0.9,4.5,1.8);
        for(const QColor fill:{QColor("#202020"),QColor("#f0f0f0")}) {
            auto* vehicle=new RunVehicleItem(body,fill,20);
            QGraphicsPathItem plain(body);plain.setPen(Qt::NoPen);
            require(vehicle->shape()==plain.shape(),"Casing changed the vehicle's hit shape");
            const auto image=render(vehicle,fill,20);
            double edge=0;for(int x=0;x<image.width();++x)edge=std::max(edge,std::abs(luminance(image.pixel(x,image.height()/2))-luminance(fill.rgb())));
            require(edge>=0.3,"Vehicle vanishes on a background of its own colour");
            delete vehicle;
        }
        // Red, amber and green bars differ in shape, not only hue; their hit shape is one.
        const RoadCrossbar bar{{0,-3.5},{0,3.5},0};
        const QColor background("#808080");
        std::vector<QSet<int>> masks;std::vector<QPainterPath> shapes;
        for(const auto cue:{SignalCue::stop,SignalCue::caution,SignalCue::go}) {
            auto* item=new RoadCrossbarItem(bar,Qt::black,20,false,cue);
            shapes.push_back(item->shape());masks.push_back(ink(render(item,background,20),background.rgb()));delete item;
        }
        require(shapes[0]==shapes[1]&&shapes[1]==shapes[2],"A signal cue changed the crossbar's hit shape");
        require(masks[0]!=masks[1]&&masks[1]!=masks[2]&&masks[0]!=masks[2],"Signal states share one shape");
        require(masks[0].contains(masks[1])&&masks[0].contains(masks[2]),"Red is not the full bar");
        // A real run: every vehicle is cased and a click on one still reaches the run.
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
        for(int i=0;i<300;++i)window.findChild<QAction*>("editorStep")->trigger();
        settle();
        auto* canvas=window.canvas();int vehicles=0;QGraphicsItem* first=nullptr;
        for(auto* item:canvas->scene()->items())if(item->data(0).toString()==QStringLiteral("run-vehicle")) {
            require(dynamic_cast<RunVehicleItem*>(item),"A run vehicle is not cased");
            ++vehicles;if(!first)first=item;
        }
        require(vehicles>0,"Run fixture has no vehicles");
        std::optional<std::uint64_t> clicked;canvas->runVehicleClicked=[&](std::uint64_t id){clicked=id;};
        canvas->centerOn(first);settle();
        const auto at=canvas->mapFromScene(first->mapToScene(first->shape().boundingRect().center()));
        QTest::mouseClick(canvas->viewport(),Qt::LeftButton,{},at);settle();
        require(clicked&&*clicked==first->data(2).toULongLong(),"A click on a cased vehicle missed it");
        // D283: a vehicle keeps its item from frame to frame; items of vehicles gone are removed,
        // and a followed outline leaves the vehicle that is no longer followed.
        const auto runVehicles=[&]{
            std::map<qulonglong,QGraphicsItem*> items;
            for(auto* item:canvas->scene()->items())if(item->data(0).toString()==QStringLiteral("run-vehicle")) {
                require(items.emplace(item->data(2).toULongLong(),item).second,"Two items for one vehicle");
            }
            return items;
        };
        const auto before=runVehicles();
        for(int i=0;i<200;++i)window.findChild<QAction*>("editorStep")->trigger();
        settle();
        const auto after=runVehicles();int kept=0,gone=0;
        for(const auto& [id,item]:before) {
            const auto still=after.find(id);
            if(still!=after.end()){require(still->second==item,"A vehicle's item was rebuilt");++kept;}
            else ++gone;
        }
        for(const auto& [id,item]:after)
            require(std::any_of(window.runState().vehicles.begin(),window.runState().vehicles.end(),[&](const Vehicle& v){return v.id==id;}),
                    "An item outlived its vehicle");
        require(kept>0&&gone>0,"The forcing: 200 Steps keep some vehicles and finish others");
        const auto ids=runVehicles();require(ids.size()>=2,"Need two vehicles to follow");
        auto pick=ids.begin();const auto one=pick->first;auto* oneItem=pick->second;const auto two=(++pick)->first;
        canvas->setRunFollow(one);settle();require(static_cast<QGraphicsPathItem*>(oneItem)->pen().style()!=Qt::NoPen,"The followed vehicle has no outline");
        canvas->setRunFollow(two);settle();require(static_cast<QGraphicsPathItem*>(oneItem)->pen().style()==Qt::NoPen,"A vehicle no longer followed keeps its outline");
        canvas->setRunFollow(std::nullopt);settle();
        // A frame drawn on reused items equals one drawn from scratch, after a zoom too.
        const auto frame=[&]{
            std::map<qulonglong,std::tuple<QPainterPath,QRgb,QPointF,double,double,int>> out;
            for(const auto& [id,item]:runVehicles()) {
                const auto* body=static_cast<QGraphicsPathItem*>(item);
                out[id]={body->path(),body->brush().color().rgb(),body->pos(),body->rotation(),body->zValue(),int(body->pen().style())};
            }
            return out;
        };
        const auto atStart=frame();const auto view=canvas->transform();
        const int across=canvas->horizontalScrollBar()->value(),down=canvas->verticalScrollBar()->value();
        canvas->setTransform(QTransform::fromScale(.5,-.5));window.findChild<QAction*>("editorStep")->trigger();settle();
        const auto reused=frame();
        require(std::get<0>(reused.begin()->second)!=std::get<0>(atStart.at(reused.begin()->first)) || !atStart.contains(reused.begin()->first),
                "The forcing: zooming out enlarges the floored bodies");
        canvas->redraw();settle();require(frame()==reused,"Reused vehicle items differ from rebuilt ones");
        canvas->setTransform(view);canvas->horizontalScrollBar()->setValue(across);canvas->verticalScrollBar()->setValue(down);canvas->redraw();settle(); // bodies follow the zoom at the next frame
        // D281: during a run the static network below the vehicles is painted from the cached
        // background, pixel for pixel what live items paint, and a selection still repaints it.
        const auto hidden=[&]{int n=0;for(auto* item:canvas->scene()->items())if(!item->isVisible())++n;return n;};
        require(hidden()>0,"The forcing: a run hides the static layer it bakes");
        const auto grab=[&]{settle();return canvas->viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);};
        // The cache is a premultiplied pixmap, so a blend can round a level or two apart from the live
        // viewport: antialiased edges, and every pixel of a half-transparent photo (Qt 6.5 on Windows).
        // A live casing over an edge differs a little more (≤24 levels on ≈40 px, Linux). A stale or
        // missing layer, or lost opacity, differs by tens of levels, so only pixels more than 2 levels
        // apart count toward the 1 % budget, and none may be more than 32 apart.
        struct Difference { long differing{}, beyond{}, pixels{}; int max{}; };
        const auto compare=[](const QImage& x,const QImage& y){
            Difference d{0,0,long(x.width())*x.height(),x.size()==y.size()?0:255};
            if(d.max)return d;
            for(int r=0;r<x.height();++r)for(int c=0;c<x.width();++c) {
                const auto p=x.pixel(c,r),q=y.pixel(c,r);
                const int level=std::max({std::abs(qRed(p)-qRed(q)),std::abs(qGreen(p)-qGreen(q)),std::abs(qBlue(p)-qBlue(q))});
                d.max=std::max(d.max,level);if(level>0)++d.differing;if(level>2)++d.beyond;
            }
            return d;
        };
        const auto same=[&](const QImage& x,const QImage& y){
            const auto d=compare(x,y);return d.max<=32&&d.beyond*100<d.pixels;
        };
        // A failed match names its numbers, so a CI log tells rounding from a lost layer.
        const auto requireSame=[&](const QImage& x,const QImage& y,const char* what){
            if(same(x,y))return;
            const auto d=compare(x,y);
            throw std::runtime_error(std::string(what)+": "+std::to_string(d.differing)+" of "+std::to_string(d.pixels)+
                " pixels differ, "+std::to_string(d.beyond)+" by more than 2 levels, at most "+std::to_string(d.max));
        };
        const auto baked=grab();
        canvas->setRunLayerCache(false);require(hidden()==0,"Without the cache every item is live");
        requireSame(grab(),baked,"The cached static layer paints differently from live items");
        canvas->setRunLayerCache(true);requireSame(grab(),baked,"Turning the cache back on changed the picture");
        const auto link=window.history().document().network.links.front().id;
        canvas->select(link);const auto selected=grab();
        require(!same(selected,baked),"The forcing: selecting a Link changes the picture");
        canvas->setRunLayerCache(false);requireSame(grab(),selected,"A selection during a run left the cached layer stale");
        canvas->setRunLayerCache(true);
        window.findChild<QLineEdit*>("editorSeed")->setText("43");settle(); // ends the run (clearRun)
        require(hidden()==0,"Leaving the run leaves static items hidden");
        // A half-transparent background image is baked with its opacity, as the scene paints it.
        {
            QTemporaryDir directory;require(directory.isValid(),"No temporary directory");
            auto document=window.history().document();QImage photo(400,400,QImage::Format_RGB32);photo.fill(QColor("#203040"));
            QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);require(photo.save(&buffer,"PNG"),"Image fixture");
            document.background.pngBase64=std::make_shared<const std::string>(png.toBase64().toStdString());
            document.background.x=-200;document.background.y=200;document.background.opacity=.5;
            const auto file=directory.filePath("photo.traffic.json");
            std::ofstream(file.toStdString())<<documentJson(document).dump(1);
            window.openFile(file);settle();
            for(int i=0;i<50;++i)window.findChild<QAction*>("editorStep")->trigger();
            const auto photoBaked=grab();
            canvas->setRunLayerCache(false);
            // Only the photo's interior: on Windows (Qt 6.5) its antialiased border lands a fraction of a
            // pixel apart between the cached and live paints (≤60 levels on ≈1.2k px, CI run 39378395930);
            // the rows above bound everything else. Lost opacity differs across the whole interior.
            const auto interior=canvas->mapFromScene(QRectF(-200,-200,400,400)).boundingRect()
                .intersected(canvas->viewport()->rect()).adjusted(3,3,-3,-3);
            require(interior.width()>=20&&interior.height()>=20,"The forcing: the photo's interior is on screen");
            requireSame(grab().copy(interior),photoBaked.copy(interior),"A background image's opacity is lost in the cached layer");
            canvas->setRunLayerCache(true);
        }
        if(argc>2) {
            canvas->setTransform(QTransform::fromScale(14,-14));canvas->centerOn(0,0);settle();
            auto image=canvas->viewport()->grab().toImage().convertToFormat(QImage::Format_Grayscale8);
            require(image.save(QString::fromUtf8(argv[2])+"-grayscale.png"),"Screenshot failed");
        }
        std::cout<<"Run presentation: casings, signal cues and unchanged hits passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
