#include "../src/editor/canvas.hpp"
#include "../src/editor/run_vehicle_item.hpp"
#include "../src/editor/vehicle_color.hpp"
#include "../src/project/display.hpp"
#include "../src/project/load.hpp"
#include "../src/core/simulation.hpp"
#include <QApplication>
#include <QGraphicsTextItem>
#include <QImage>
#include <QPainter>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <set>
using namespace multra;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(double actual,double expected){require(std::abs(actual-expected)<1e-7,"Vehicle geometry differs");}
void settle(){QApplication::processEvents();}
QImage raster(RunVehicleItem& item) {
    const auto bounds=item.boundingRect();const double scale=24;
    QImage image(int(std::ceil(bounds.width()*scale))+8,int(std::ceil(bounds.height()*scale))+8,QImage::Format_ARGB32);
    image.fill(Qt::transparent);QPainter painter(&image);
    painter.translate(4-bounds.left()*scale,4-bounds.top()*scale);painter.scale(scale,scale);
    item.paint(&painter,nullptr,nullptr);painter.end();return image;
}
int glassPixels(RunVehicleItem& item) {
    const auto image=raster(item);int count=0;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)
        count+=image.pixelColor(x,y)==QColor("#172c3b");
    return count;
}
void drawingContract() {
    const std::map<std::string,double> windshield{{"car",.29},{"motorcycle",.27},{"pickup",.29},
        {"van",.20},{"bus",.055},{"truck",.075}};
    for(const auto& [kind,probe]:windshield)for(const auto dimensions:{QSizeF(4.5,1.8),QSizeF(12,2.5),QSizeF(2.1,.8)}) {
        const double length=dimensions.width(),width=dimensions.height();
        const auto detailed=vehicleDrawing(length,width,true,kind);
        const auto plain=vehicleDrawing(length,width,false,kind);
        const auto bounds=detailed.body.boundingRect();
        near(bounds.left(),-length);near(bounds.right(),0);near(bounds.top(),-width/2);near(bounds.bottom(),width/2);
        require(!detailed.body.isEmpty(),"A valid archetype is empty");
        require(plain.body==detailed.body,"LOD changes a vehicle's outer silhouette");
        require(plain.glass.isEmpty()&&plain.panels.isEmpty()&&plain.lamps.isEmpty(),"Small symbols keep interior decoration");
        require(!detailed.glass.isEmpty()&&!detailed.lamps.isEmpty(),"Detailed vehicles lack glass or nose cues");
        const QPointF window(-probe*(kind=="truck"?std::min(length,8.5):length),0);
        require(detailed.glass.contains(window),"Setup: windshield probe is not on glass");
        require(detailed.body.contains(window),"Glass punches a hole in the vehicle body");
        RunVehicleItem item(detailed,QColor("#38bdf8"),20);
        require(item.contains(window),"A click on glass misses the vehicle");
        require(!item.contains(QPointF(.01,0)),"Painted casing expands the hit shape");
        require(item.childItems().empty(),"Decoration adds scene children per vehicle");
    }
    // Equal dimensions must still communicate the archetype: a bus has no truck cab notch.
    const auto bus=vehicleDrawing(12,2.5,true,"bus"),truck=vehicleDrawing(12,2.5,true,"truck");
    require(bus.body!=truck.body,"An explicit bus and truck share a silhouette");
    require(bus.body.contains(QPointF(-2.38,1.1))&&!truck.body.contains(QPointF(-2.38,1.1)),
        "A bus inherits the truck's cab notch");
    require(truck.glass.contains(QPointF(-.265*8.5,0)) && truck.body.contains(QPointF(-.265*8.5,0)),
        "Truck chassis detail breaks the rigid body hit area");
    require(truck.panels.contains(QPointF(-4.17,0)) &&
            !truck.panels.contains(QPointF(-3.65,0)),"Truck roof lacks recessed corrugations");
    const auto lightTruck=vehicleDrawing(8.5,2.5,true,"truck");
    for(double x=.03;x<2.04;x+=.07)for(double y=-1.2;y<1.2;y+=.11) {
        const QPointF probe(-x,y);
        require(truck.body.contains(probe)==lightTruck.body.contains(probe),"Heavy and light truck cab sizes differ");
        require(truck.glass.contains(probe)==lightTruck.glass.contains(probe),"Truck cab glazing is stretched");
        require(truck.lamps.contains(probe)==lightTruck.lamps.contains(probe),"Truck cab headlights are stretched");
    }
    require(vehicleDrawing(5,2,true,"pickup").glass!=vehicleDrawing(5,2,true,"van").glass,
        "A pickup bed is indistinguishable from a van's roof");
    const auto van=vehicleDrawing(5.5,2,true,"van");
    require(van.body!=vehicleDrawing(5.5,2,true,"pickup").body,"Van shares a pickup nose");
    require(van.glass.contains(QPointF(-.42*5.5,.44*2)) &&
            !van.glass.contains(QPointF(-.54*5.5,.44*2)),"Van side windows lack body pillars");
    require(van.glass.contains(QPointF(-.95*5.5,0)) &&
            !van.glass.contains(QPointF(-.6*5.5,0)),"Van lacks upright rear glass and a solid roof");
    const auto bike=vehicleDrawing(2.1,.8,true,"motorcycle");
    for(const double station:{.10,.18,.43,.68,.85,.93})
        require(bike.body.contains(QPointF(-station*2.1,0)),"Motorcycle body has a disconnected centreline");
    require(bike.panels.contains(QPointF(-.40*2.1,0)) &&
            bike.glass.contains(QPointF(-.68*2.1,0)),"Motorcycle tank and saddle are indistinct");
    require(bike.lamps.contains(QPointF(-.04*2.1,0)),"Motorcycle lacks its original nose light cue");
    for(const double invalid:{0.,-1.,std::nan(""),static_cast<double>(INFINITY)})
        require(vehicleDrawing(invalid,2,true,"car").body.isEmpty(),"Invalid dimensions produce a drawable body");
}
std::map<std::uint64_t,RunVehicleItem*> vehicles(EditorCanvas& canvas) {
    std::map<std::uint64_t,RunVehicleItem*> result;
    for(auto* item:canvas.scene()->items())if(item->data(0).toString()=="run-vehicle") {
        auto* vehicle=dynamic_cast<RunVehicleItem*>(item);require(vehicle,"Run uses an uncased item");
        require(result.emplace(item->data(2).toULongLong(),vehicle).second,"A vehicle has multiple items");
    }
    return result;
}
void canvasContract(const std::filesystem::path& data) {
    const auto source=loadScenario(data/"scenarios/crossing.json",data).scenario;
    const auto display=loadDisplayCatalog(data);
    ProjectDocument document;document.network.links={{"road",{{0,0},{500,0}},{{"lane",3.5}}}};
    Scenario scenario;scenario.duration=60;scenario.timeStep=.1;
    scenario.segments={{"lane",500,{}}};scenario.routes={{"route",{"lane"}}};
    scenario.vehicleTypes=source.vehicleTypes;scenario.behaviours=source.behaviours;
    auto frame=createSimulation(scenario,42);
    for(std::uint32_t i=0;i<frame.scenario->vehicleTypes.size();++i) {
        Vehicle vehicle;vehicle.id=i+1;vehicle.typeIndex=i;vehicle.distance=40+50*i;
        frame.vehicles.push_back(vehicle);
    }
    std::set<std::string> installed;for(const auto& type:frame.scenario->vehicleTypes)installed.insert(type.id);
    for(const auto* id:{"bus","car","heavy-vehicle","motorcycle","pickup","rigid-truck","van"})
        require(installed.contains(id),"Setup: installed catalog omits an expected type");
    EditorCanvas canvas;canvas.resize(1200,600);canvas.setDisplayCatalog(display);canvas.setDocument(&document);
    canvas.setTransform(QTransform::fromScale(12,-12));canvas.setRunNetwork(document.network);canvas.setRunFrame(frame);
    canvas.show();settle();const auto originals=vehicles(canvas);
    require(originals.size()==frame.vehicles.size(),"The canvas omits a vehicle type");
    std::optional<std::uint64_t> clicked;canvas.runVehicleClicked=[&](std::uint64_t id){clicked=id;};
    for(const auto& vehicle:frame.vehicles) {
        const auto& type=frame.scenario->vehicleTypes[vehicle.typeIndex];auto* item=originals.at(vehicle.id);
        const auto bounds=item->path().boundingRect();const auto axles=vehicleAxles(type);
        const double lever=axles.wheelbase+axles.frontOverhang;
        near(bounds.width(),type.length);near(bounds.height(),type.width);near(bounds.right(),lever);
        near(item->pos().x(),vehicle.distance-lever);near(item->rotation(),0);
        require(item->brush().color()==QColor(QString::fromStdString(display.vehicleColors.at(type.id))),"Wrong type colour");
        require(glassPixels(*item)>0,"Detailed canvas vehicles lack painted glass");
        const auto kind=display.vehicleShapes.at(type.id);
        const double windshield=kind=="bus"?.055:kind=="truck"?.075:kind=="van"?.20:kind=="motorcycle"?.27:.29;
        const QPointF onGlass(lever-windshield*(kind=="truck"?std::min(type.length,8.5):type.length),0);
        require(item->contains(onGlass),"Canvas glass has a hole in its hit area");
        canvas.centerOn(item);settle();clicked.reset();
        QTest::mouseClick(canvas.viewport(),Qt::LeftButton,{},canvas.mapFromScene(item->mapToScene(onGlass)));
        require(clicked&&*clicked==vehicle.id,"A viewport click on glass does not select the correct vehicle");
    }
    canvas.setRunFrame(frame);require(vehicles(canvas)==originals,"A stopped frame rebuilds vehicle items");
    canvas.setRunFollow(frame.vehicles.front().id);settle();
    require(originals.begin()->second->pen().style()!=Qt::NoPen,"Following loses the vehicle outline");
    canvas.setRunFollow(std::nullopt);
    const auto idFor=[&](const std::string& type) {
        for(const auto& vehicle:frame.vehicles)if(frame.scenario->vehicleTypes[vehicle.typeIndex].id==type)return vehicle.id;
        throw std::runtime_error("Setup: missing threshold-test type");
    };
    const auto threshold=[&](const std::string& type,double scale) {
        for(const bool readable:{true,false}) {
            canvas.setTransform(QTransform::fromScale(scale*(readable?1.:1.-1e-3),-scale*(readable?1.:1.-1e-3)));
            canvas.setRunFrame(frame);const auto at=vehicles(canvas);
            require(at==originals,"Crossing a detail threshold rebuilds scene items");
            require((glassPixels(*at.at(idFor(type)))>0)==readable,"Painted detail does not switch at its pixel threshold");
        }
    };
    // Independent raster probes pin both inequalities; the solid body's path is unchanged.
    threshold("car",14/4.5);threshold("bus",5/2.5);
    // At 1.5 px/m a bus is 18 px long but narrower than 5 px: details must stay hidden.
    canvas.setTransform(QTransform::fromScale(1.5,-1.5));canvas.setRunFrame(frame);
    auto small=vehicles(canvas);require(small==originals,"Zooming rebuilds vehicle items");
    bool checkedBus=false;
    for(const auto& vehicle:frame.vehicles)if(frame.scenario->vehicleTypes[vehicle.typeIndex].id=="bus") {
        require(glassPixels(*small.at(vehicle.id))==0,"A narrow bus shows unreadable subpixel windows");checkedBus=true;
    }
    require(checkedBus,"Setup: no bus exercises the width threshold");
    canvas.setTransform(QTransform::fromScale(.05,-.05));canvas.setRunFrame(frame);
    for(const auto& vehicle:frame.vehicles) {
        const auto* item=small.at(vehicle.id);const auto bounds=item->path().boundingRect();
        require(bounds.width()*.05>=4-1e-7&&bounds.height()*.05>=2.5-1e-7,"A symbol falls below the visible pixel floor");
        near(item->mapToScene(QPointF(item->data(3).toDouble(),0)).x(),vehicle.distance);
        require(glassPixels(*small.at(vehicle.id))==0,"Overview symbols retain interior decoration");
    }
    // A style replacement while stopped must immediately replace the drawn shape.
    canvas.setTransform(QTransform::fromScale(12,-12));canvas.setRunFrame(frame);
    auto beforeStyle=vehicles(canvas);std::uint64_t carId=0;
    for(const auto& vehicle:frame.vehicles)if(frame.scenario->vehicleTypes[vehicle.typeIndex].id=="car")carId=vehicle.id;
    require(carId!=0,"Setup: no car to restyle");const auto carBody=beforeStyle.at(carId)->path();
    auto restyled=display;restyled.vehicleShapes["car"]="truck";canvas.setDisplayCatalog(restyled);settle();
    require(vehicles(canvas).at(carId)->path()!=carBody,"A stopped appearance replacement keeps the old shape");
    // A new immutable Scenario can reuse the same IDs/type slots while changing dimensions.
    auto replacement=*frame.scenario;replacement.vehicleTypes.front().length+=1;
    replacement.vehicleTypes.front().axles.reset();auto changed=createSimulation(replacement,42);changed.vehicles=frame.vehicles;
    canvas.setRunFrame(changed);
    const auto* replaced=vehicles(canvas).at(frame.vehicles.front().id);
    near(replaced->path().boundingRect().width(),replacement.vehicleTypes.front().length);
    const auto lever=vehicleAxles(replacement.vehicleTypes.front());
    near(replaced->path().boundingRect().right(),lever.wheelbase+lever.frontOverhang);
    // Missing appearance and legacy colour-only entries choose their archetype from physical
    // length, even when the overview floor enlarges a car symbol beyond the truck threshold.
    auto legacyScenario=*frame.scenario;
    const auto heavy=std::find_if(legacyScenario.vehicleTypes.begin(),legacyScenario.vehicleTypes.end(),
        [](const auto& type){return type.id=="heavy-vehicle";});
    require(heavy!=legacyScenario.vehicleTypes.end(),"Setup: no rigid long type to copy");
    auto custom=*heavy;custom.id="zz-colour-only-long";legacyScenario.vehicleTypes.push_back(custom);
    auto legacyFrame=createSimulation(legacyScenario,42);legacyFrame.vehicles=frame.vehicles;
    Vehicle customVehicle;customVehicle.id=999;customVehicle.distance=450;
    for(std::uint32_t i=0;i<legacyFrame.scenario->vehicleTypes.size();++i)
        if(legacyFrame.scenario->vehicleTypes[i].id==custom.id)customVehicle.typeIndex=i;
    legacyFrame.vehicles.push_back(customVehicle);
    auto coloursOnly=display;coloursOnly.vehicleShapes.erase("car");coloursOnly.vehicleColors.erase("car");
    coloursOnly.vehicleColors[custom.id]="#ffcc00";
    require(!coloursOnly.vehicleShapes.contains(custom.id),"Setup: colour-only custom type has a shape");
    canvas.setDisplayCatalog(coloursOnly);canvas.setTransform(QTransform::fromScale(.05,-.05));canvas.setRunFrame(legacyFrame);
    const auto legacyItems=vehicles(canvas);
    for(const auto id:{carId,customVehicle.id}) {
        const auto vehicle=std::find_if(legacyFrame.vehicles.begin(),legacyFrame.vehicles.end(),[&](const auto& v){return v.id==id;});
        const auto& type=legacyFrame.scenario->vehicleTypes[vehicle->typeIndex];const auto axles=vehicleAxles(type);
        const auto expected=vehicleDrawing(std::max(type.length,80.),std::max(type.width,50.),false,id==carId?"car":"truck");
        require(legacyItems.at(id)->path()==expected.body.translated(axles.wheelbase+axles.frontOverhang,0),
            "Overview flooring changes an unmapped type's archetype");
    }
    require(legacyItems.at(customVehicle.id)->brush().color()==QColor("#ffcc00"),"A legacy colour-only type loses its colour");
    auto mixedDisplay=display;mixedDisplay.vehicleColorPalettes["car"]={"#ff0000","#00ff00","#0000ff"};
    auto mixedFrame=frame;
    const auto car=std::find_if(frame.vehicles.begin(),frame.vehicles.end(),[&](const auto& v){return v.id==carId;});
    for(int i=0;i<20;++i){auto extra=*car;extra.id=100+i;extra.distance=50+15*i;mixedFrame.vehicles.push_back(extra);}
    canvas.setDisplayCatalog(mixedDisplay);canvas.setRunFrame(mixedFrame);
    std::map<std::uint64_t,QColor> beforeColours;std::set<QString> fleetColours;
    for(const auto& [id,item]:vehicles(canvas)) {
        beforeColours[id]=item->brush().color();if(id>=100)fleetColours.insert(item->brush().color().name());
    }
    require(fleetColours.size()==3,"A single vehicle type does not show mixed colours");
    canvas.setTransform(QTransform::fromScale(12,-12));canvas.setRunFrame(mixedFrame);
    for(const auto& [id,item]:vehicles(canvas))require(item->brush().color()==beforeColours.at(id),"Zoom changes a vehicle's colour");
    canvas.setRunFrame(frame);canvas.setRunFrame(mixedFrame);
    for(const auto& [id,item]:vehicles(canvas))require(item->brush().color()==beforeColours.at(id),"Seeking changes a vehicle's colour");
    mixedDisplay.vehicleColorPalettes["car"]={"#123456"};canvas.setDisplayCatalog(mixedDisplay);
    for(const auto& [id,item]:vehicles(canvas))if(id>=100)require(item->brush().color()==QColor("#123456"),"Single colour not applied while stopped");
    canvas.clearRunFrame();require(vehicles(canvas).empty(),"Clearing Run leaves vehicle items");
}
void gallery(const std::filesystem::path& data,const QString& path,bool mixed) {
    const auto source=loadScenario(data/"scenarios/crossing.json",data).scenario;
    auto catalog=loadDisplayCatalog(data);QGraphicsScene scene;scene.setBackgroundBrush(QColor("#101923"));
    if(mixed)for(const auto& type:source.vehicleTypes)catalog.vehicleColorPalettes[type.id]=catalog.availableVehicleColors;
    auto* title=scene.addText(mixed?"MULTRA  /  MIXED VEHICLE COLOURS":"MULTRA  /  VEHICLE LIBRARY",QFont("Sans",20,QFont::Bold));title->setDefaultTextColor(Qt::white);title->setPos(28,15);
    int row=0;
    for(const auto& type:source.vehicleTypes) {
        const double y=105+row++*90;
        scene.addRect(170,y-30,900,62,Qt::NoPen,QColor("#34414e"));
        auto* label=scene.addText(QString::fromStdString(type.id),QFont("Sans",11));label->setDefaultTextColor(QColor("#dde5ef"));label->setPos(25,y-12);
        const std::vector<std::pair<double,double>> placements=mixed?
            std::vector<std::pair<double,double>>{{390,10},{560,10},{730,10},{900,10},{1070,10}}:
            std::vector<std::pair<double,double>>{{540,26},{900,10}};
        std::uint64_t vehicle=row*8;
        for(const auto& [front,scale]:placements) {
            auto* item=new RunVehicleItem(vehicleDrawing(type.length,type.width,true,catalog.vehicleShapes.at(type.id)),
                QColor(QString::fromStdString(vehicleDisplayColor(catalog,type.id,vehicle++,catalog.vehicleColors.at(type.id)))),scale);
            item->setScale(scale);item->setPos(front,y);scene.addItem(item);
        }
    }
    const QRectF bounds(0,0,1100,760);scene.setSceneRect(bounds);
    QImage image(1100,760,QImage::Format_RGB32);image.fill(QColor("#101923"));QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);scene.render(&painter,bounds,bounds);painter.end();
    require(image.save(path),"Could not save appearance gallery");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"Expected data directory");const std::filesystem::path data(argv[1]);
        drawingContract();canvasContract(data);
        if(argc>2)gallery(data,QString::fromUtf8(argv[2]),argc>3);
        std::cout<<"Vehicle appearance: seven types, silhouettes, glass hits, LOD, reuse and replacement passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
