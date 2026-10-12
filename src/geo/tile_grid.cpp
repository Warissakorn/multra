#include "tile_grid.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trafficsim {
namespace {
int count(int zoom) { if (zoom<0 || zoom>22) throw std::invalid_argument("EDIT_MAP_TILE");return 1<<zoom; }
}
TileBounds tileBounds(const TileKey& k) {
    const int n=count(k.zoom);
    if (k.x<0 || k.x>=n || k.y<0 || k.y>=n) throw std::invalid_argument("EDIT_MAP_TILE");
    const double span=2*mercatorHalfWorld/n;
    return {-mercatorHalfWorld+k.x*span,mercatorHalfWorld-(k.y+1)*span,
        -mercatorHalfWorld+(k.x+1)*span,mercatorHalfWorld-k.y*span};
}
MetrePoint tilePoint(const TileKey& k,double u,double v) {
    const auto b=tileBounds(k);return {b.left+(b.right-b.left)*u,b.top-(b.top-b.bottom)*v};
}
TileKey tileAt(const std::string& provider,int zoom,MetrePoint p) {
    const int n=count(zoom);
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) throw std::invalid_argument("EDIT_MAP_TILE");
    const double x=std::clamp((p.x+mercatorHalfWorld)/(2*mercatorHalfWorld),0.,1.);
    const double y=std::clamp((mercatorHalfWorld-p.y)/(2*mercatorHalfWorld),0.,1.);
    return {provider,zoom,std::min(n-1,static_cast<int>(x*n)),std::min(n-1,static_cast<int>(y*n))};
}
int tileZoom(double scale,int minimum,int maximum,int previous,int pixels) {
    count(minimum);count(maximum);
    if (maximum<minimum || pixels<1 || !std::isfinite(scale) || scale<=0) throw std::invalid_argument("EDIT_MAP_TILE");
    const double ideal=std::log2(2*mercatorHalfWorld*scale/pixels);
    if (previous>=minimum && previous<=maximum && std::abs(ideal-previous)<0.75) return previous;
    return static_cast<int>(std::clamp(std::round(ideal),double(minimum),double(maximum)));
}
}
