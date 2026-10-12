#include "projection.hpp"
#include <proj.h>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace trafficsim {
struct GeoProjection::Impl {
    PJ_CONTEXT* context{proj_context_create()};
    PJ* geographic{};PJ* mercator{};
    ~Impl() { proj_destroy(geographic);proj_destroy(mercator);proj_context_destroy(context); }
    PJ* pipeline(const char* source,const char* destination) {
        auto* raw=proj_create_crs_to_crs(context,source,destination,nullptr);
        if (!raw) throw std::invalid_argument("EDIT_MAP_PROJECTION");
        auto* result=proj_normalize_for_visualization(context,raw);proj_destroy(raw);
        if (!result) throw std::invalid_argument("EDIT_MAP_PROJECTION");
        return result;
    }
    static MetrePoint transform(PJ* pipeline,PJ_DIRECTION direction,MetrePoint p) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) throw std::invalid_argument("EDIT_MAP_PROJECTION");
        proj_errno_reset(pipeline);const auto q=proj_trans(pipeline,direction,proj_coord(p.x,p.y,0,0));
        if (proj_errno(pipeline) || !std::isfinite(q.xy.x) || !std::isfinite(q.xy.y))
            throw std::invalid_argument("EDIT_MAP_PROJECTION");
        return {q.xy.x,q.xy.y};
    }
};
GeoProjection::GeoProjection(const GeoReference& r):impl_(std::make_unique<Impl>()),reference_(r) {
    validateMapSettings(MapSettings{r});
    if (!impl_->context) throw std::invalid_argument("EDIT_MAP_PROJECTION");
    proj_context_set_enable_network(impl_->context,0);
    impl_->geographic=impl_->pipeline("EPSG:4326",r.projectedCrs.c_str());
    impl_->mercator=impl_->pipeline("EPSG:3857",r.projectedCrs.c_str());
    origin_=Impl::transform(impl_->geographic,PJ_FWD,{r.anchorLongitude,r.anchorLatitude});
    const double angle=r.rotationDegrees*std::numbers::pi/180;cosine_=std::cos(angle);sine_=std::sin(angle);
}
GeoProjection::~GeoProjection()=default;
MetrePoint GeoProjection::projectedToWorld(MetrePoint p) const {
    const double x=p.x-origin_.x,y=p.y-origin_.y;
    return {reference_.anchorWorldX+cosine_*x-sine_*y,reference_.anchorWorldY+sine_*x+cosine_*y};
}
MetrePoint GeoProjection::worldToProjected(MetrePoint p) const {
    const double x=p.x-reference_.anchorWorldX,y=p.y-reference_.anchorWorldY;
    return {origin_.x+cosine_*x+sine_*y,origin_.y-sine_*x+cosine_*y};
}
MetrePoint GeoProjection::geographicToWorld(LonLat p) const {
    if (p.longitude < -180 || p.longitude > 180 || p.latitude < -80 || p.latitude > 84)
        throw std::invalid_argument("EDIT_MAP_PROJECTION");
    return projectedToWorld(Impl::transform(impl_->geographic,PJ_FWD,{p.longitude,p.latitude}));
}
LonLat GeoProjection::worldToGeographic(MetrePoint p) const {
    const auto q=Impl::transform(impl_->geographic,PJ_INV,worldToProjected(p));return {q.x,q.y};
}
MetrePoint GeoProjection::mercatorToWorld(MetrePoint p) const { return projectedToWorld(Impl::transform(impl_->mercator,PJ_FWD,p)); }
MetrePoint GeoProjection::worldToMercator(MetrePoint p) const { return Impl::transform(impl_->mercator,PJ_INV,worldToProjected(p)); }
bool GeoProjection::insideStudy(MetrePoint p) const {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::hypot(p.x-reference_.anchorWorldX,p.y-reference_.anchorWorldY)<=100000;
}
}
