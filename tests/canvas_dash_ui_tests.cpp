// D430: a dashed lane line is cut into dashes of its display type's laneDash and laneGap metres,
// so it keeps its true length at every zoom. A pen's own dash pattern counts in pen widths and Qt
// floors a thin pen at one device pixel, which stretched a 3 m dash to 15 m at 4 px/m.
#include "../src/editor/canvas.hpp"
#include "../src/editor/canvas_style.hpp"
#include "../src/project/display.hpp"
#include "ui_test_storage.hpp"
#include <QApplication>
#include <QGraphicsPathItem>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace multra;
namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(20);}
// Each subpath's length along its points.
std::vector<double> dashLengths(const QPainterPath& path) {
    std::vector<double> out;QPointF last;
    for(int i=0;i<path.elementCount();++i) {
        const auto e=path.elementAt(i);const QPointF p(e.x,e.y);
        if(e.isMoveTo())out.push_back(0);else out.back()+=QLineF(last,p).length();
        last=p;
    }
    return out;
}
// The repeat length, in metres, of a horizontal dashed line painted at `scale` px/m: the mean
// distance between dash starts along the painted row.
double paintedPeriod(const QPainterPath& path,const QPen& pen,double scale,double y) {
    QImage image(2400,40,QImage::Format_ARGB32);image.fill(Qt::black);
    QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(QTransform::fromScale(scale,-scale)*QTransform::fromTranslate(0,20+y*scale));
    QPen white(pen);white.setColor(Qt::white);painter.setPen(white);painter.drawPath(path);painter.end();
    int starts=0,first=-1,last=-1;bool on=false;
    for(int x=0;x<image.width();++x) {
        const bool lit=qGray(image.pixel(x,20))>20 || qGray(image.pixel(x,19))>20 || qGray(image.pixel(x,21))>20;
        if(lit && !on){++starts;if(first<0)first=x;last=x;}
        on=lit;
    }
    return starts>1?(last-first)/(starts-1.0)/scale:0;
}
void writeCatalog(const std::filesystem::path& data,const std::filesystem::path& to,const std::string& extra) {
    std::filesystem::create_directories(to/"display-types");
    std::filesystem::copy(data/"levels",to/"levels",std::filesystem::copy_options::recursive);
    std::ofstream(to/"display-types/default.json")<<R"({"id":"default","name":{"en":"Default","th":"ถนน"},"linkColor":"#49596d",)"
        R"("connectorColor":"#607d8b","laneColor":"#d0dfeb","vehicleColor":"#facc15")"<<extra<<"}";
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Data directory required");const std::filesystem::path data(argv[1]);
        // 1. The cut: a polyline bending inside a dash keeps 3 m per dash, the last dash partial.
        const std::vector<Point> bent{{0,0},{7,0},{7,13}};
        const auto lengths=dashLengths(canvasStyle::markingPath(bent,MarkingType::dashed,3,3));
        const std::vector<double> expected{3,3,3,2};
        require(lengths.size()==expected.size(),"The bent line was cut into "+std::to_string(lengths.size())+" dashes, not 4");
        for(std::size_t i=0;i<expected.size();++i)require(std::abs(lengths[i]-expected[i])<1e-9,"A dash has the wrong length");
        require(dashLengths(canvasStyle::markingPath(bent,MarkingType::solid,3,3)).size()==1,"A solid line was cut");
        // 2. The catalog: shipped types are 3/3; laneDash/laneGap are read, and refused out of range.
        for(const auto& type:loadDisplayCatalog(data).types)
            require(type.laneDash==3 && type.laneGap==3,"Shipped display type "+type.id+" is not 3 m / 3 m");
        QTemporaryDir dir;const std::filesystem::path root=dir.path().toStdString();
        writeCatalog(data,root/"ok",R"(,"laneDash":2,"laneGap":7)");
        const auto custom=loadDisplayCatalog(root/"ok").types.front();
        require(custom.laneDash==2 && custom.laneGap==7,"laneDash/laneGap were not read");
        for(const auto* bad:{R"(,"laneDash":0)",R"(,"laneGap":"3")",R"(,"laneGap":1000)"}) {
            const auto where=root/("bad"+std::to_string(std::hash<std::string>{}(bad)));writeCatalog(data,where,bad);
            bool refused=false;try{loadDisplayCatalog(where);}catch(const std::exception& e){refused=std::string(e.what())=="EDIT_DISPLAY_CATALOG";}
            require(refused,std::string("An invalid dash entry was accepted: ")+bad);
        }
        // 3. On the canvas, both driving sides: the drawn line repeats every dash+gap metres at every zoom.
        EditorCanvas canvas;canvas.resize(900,500);canvas.show();settle();
        auto catalog=loadDisplayCatalog(root/"ok");canvas.setDisplayCatalog(catalog);
        for(const auto side:{DrivingSide::left,DrivingSide::right}) {
            ProjectDocument d;d.network.drivingSide=side;
            Link link{"a",{{0,0},{400,0}},{{"a1",3.5},{"a2",3.5}}};
            link.boundaryMarkings={MarkingType::solid,MarkingType::dashed,MarkingType::solid};
            d.network.links={link};validateDocument(d);canvas.setDocument(&d);
            for(const double scale:{2.0,4.0,8.0,16.0}) {
                canvas.setTransform(QTransform::fromScale(scale,-scale));canvas.centerOn(200,0);canvas.redraw();settle();
                QGraphicsPathItem* dashed=nullptr;
                for(auto* item:canvas.scene()->items())if(item->data(0).toString()=="road-marking")
                    if(auto* p=dynamic_cast<QGraphicsPathItem*>(item);p && dashLengths(p->path()).size()>1)dashed=p;
                require(dashed,"No dashed lane line at "+std::to_string(scale)+" px/m");
                const double y=dashed->path().boundingRect().center().y();
                const double period=paintedPeriod(dashed->path(),dashed->pen(),scale,y);
                require(std::abs(period-9)<=1/scale+1e-9,"The dash period is "+std::to_string(period)+" m at "+std::to_string(scale)+" px/m, not 9 m");
                // The forcing: the old pen pattern measures wrong at this zoom, so the check can see it.
                if(scale==2.0) {
                    QPainterPath whole;whole.moveTo(0,y);whole.lineTo(400,y);
                    QPen old(dashed->pen());old.setDashPattern({3/canvasStyle::markingWidth,3/canvasStyle::markingWidth});
                    require(std::abs(paintedPeriod(whole,old,scale,y)-6)>1,"The forcing: the old pen measures true at 2 px/m");
                }
            }
            canvas.setDocument(nullptr);
        }
        std::cout<<"Canvas dashes: true length per display type at 2-16 px/m, both sides, catalog checks passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
