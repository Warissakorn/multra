#include "../src/geo/projection.hpp"
#include "../src/geo/tile_grid.hpp"
#include <iostream>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace trafficsim;
namespace {
void require(bool condition,const char* message) {if (!condition) throw std::runtime_error(message);}
void near(double a,double b,double tolerance=1e-6) {if (!std::isfinite(a)||std::abs(a-b)>tolerance) throw std::runtime_error("Coordinate mismatch: "+std::to_string(a)+" expected "+std::to_string(b));}
void projection() {
    GeoReference r;r.anchorLatitude=0;r.anchorLongitude=99;r.anchorWorldX=500000;
    GeoProjection p(r);
    const auto origin=p.geographicToWorld({99,0});near(origin.x,500000);near(origin.y,0);
    // Independent control: 0.9996 times the WGS84 ellipsoidal meridional arc
    // at latitude 13 N (Simpson integration, a=6378137, f=1/298.257223563).
    const auto control=p.geographicToWorld({99,13});near(control.x,500000,1e-5);near(control.y,1437135.722171435,0.01);
    const auto back=p.worldToGeographic(control);near(back.longitude,99,1e-9);near(back.latitude,13,1e-9);
    const auto round=p.mercatorToWorld(p.worldToMercator(control));near(round.x,control.x);near(round.y,control.y);
    r.anchorWorldX=123;r.anchorWorldY=-456;r.rotationDegrees=90;GeoProjection rotated(r);
    const auto north=rotated.geographicToWorld({99,13});near(north.x,123-control.y,1e-5);near(north.y,-456,1e-5);
    require(rotated.insideStudy({123,-456}),"Anchor outside study");require(!rotated.insideStudy({100124,-456}),"Unbounded study");
    bool failed=false;try {p.worldToMercator({std::numeric_limits<double>::quiet_NaN(),0});}catch(const std::exception&){failed=true;}
    require(failed,"Invalid coordinate accepted");
    GeoReference south{"EPSG:32734",18,-34,0,0,0};GeoProjection southern(south);
    const auto s=southern.geographicToWorld({18,-34});near(s.x,0);near(s.y,0);
}
void tiles() {
    const auto world=tileBounds({"osm",0,0,0});near(world.left,-mercatorHalfWorld);near(world.top,mercatorHalfWorld);
    const auto a=tileBounds({"osm",1,0,0}),b=tileBounds({"osm",1,1,0});near(a.right,b.left);
    const auto k=tileAt("osm",19,{mercatorHalfWorld,-mercatorHalfWorld});require(k.x==(1<<19)-1 && k.y==(1<<19)-1,"Boundary tile overflow");
    const auto top=tilePoint({"osm",1,0,0},0,0),bottom=tilePoint({"osm",1,0,0},0,1);require(top.y>bottom.y,"Tile is upside down");
    require(tileZoom(4,0,19)>=tileZoom(2,0,19),"DPI zoom reversed");
    const double scale=256*std::pow(2.,15)/(2*mercatorHalfWorld);require(tileZoom(scale,0,19)==15,"Wrong zoom");
    require(tileZoom(scale*1.1,0,19,15)==15,"Zoom hysteresis failed");
}
}
int main() {try {projection();tiles();std::cout<<"map geo checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
