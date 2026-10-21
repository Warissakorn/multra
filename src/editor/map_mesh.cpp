#include "map_layer.hpp"
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <array>
#include <stdexcept>
namespace multra {
namespace {
QPointF project(const GeoProjection& projection,const TileKey& key,double u,double v) {
    const auto p=projection.mercatorToWorld(tilePoint(key,u,v));return {p.x,p.y};
}
double distance(QPointF a,QPointF b) { return std::hypot(a.x()-b.x(),a.y()-b.y()); }
QPolygonF quad(QPointF a,QPointF b,QPointF c) { return {a,b,b+c-a,c}; }
}
MapMesh buildMapMesh(const GeoProjection& projection,const TileKey& key,double pixelsPerMetre) {
    if (!std::isfinite(pixelsPerMetre) || pixelsPerMetre<=0) throw std::invalid_argument("EDIT_MAP_PROJECTION");
    for (int n=1;n<=64;n*=2) {
        MapMesh mesh;mesh.divisions=n;mesh.vertices.reserve((n+1)*(n+1));
        for (int y=0;y<=n;++y) for (int x=0;x<=n;++x)
            mesh.vertices.push_back(project(projection,key,double(x)/n,double(y)/n));
        const auto at=[&](int x,int y){return mesh.vertices[y*(n+1)+x];};
        for (int y=0;y<n;++y) for (int x=0;x<n;++x) {
            const auto a=at(x,y),b=at(x+1,y),c=at(x+1,y+1),d=at(x,y+1);
            const auto check=[&](double u,double v,QPointF linear) {
                mesh.sampledError=std::max(mesh.sampledError,distance(project(projection,key,u/n,v/n),linear));
            };
            check(x+.5,y,(a+b)/2);check(x+1,y+.5,(b+c)/2);
            check(x+.5,y+1,(c+d)/2);check(x,y+.5,(d+a)/2);
            check(x+.5,y+.5,(a+c)/2);
            check(x+2./3,y+1./3,(a+b+c)/3);check(x+1./3,y+2./3,(a+c+d)/3);
        }
        if (mesh.sampledError*pixelsPerMetre<=.5) return mesh;
    }
    throw std::invalid_argument("EDIT_MAP_RENDER_LIMIT");
}
void paintMapMesh(QPainter& painter,const QImage& image,const MapMesh& mesh) {
    if (image.isNull() || mesh.divisions<=0) return;
    painter.save();painter.setRenderHint(QPainter::Antialiasing,false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
    const auto world=painter.worldTransform();const int n=mesh.divisions;
    for (int y=0;y<n;++y) for (int x=0;x<n;++x) {
        const std::array<int,4> idx{y*(n+1)+x,y*(n+1)+x+1,(y+1)*(n+1)+x+1,(y+1)*(n+1)+x};
        const double u=double(image.width())/n,v=double(image.height())/n;
        const std::array<QPointF,4> src{QPointF(x*u,y*v),QPointF((x+1)*u,y*v),
            QPointF((x+1)*u,(y+1)*v),QPointF(x*u,(y+1)*v)};
        for (const auto triangle:{std::array<int,3>{0,1,2},std::array<int,3>{0,2,3}}) {
            const auto a=triangle[0],b=triangle[1],c=triangle[2];
            QTransform affine;
            if (!QTransform::quadToQuad(quad(src[a],src[b],src[c]),
                quad(mesh.vertices[idx[a]],mesh.vertices[idx[b]],mesh.vertices[idx[c]]),affine)) continue;
            painter.save();painter.setWorldTransform(world);
            QPainterPath clip;clip.moveTo(mesh.vertices[idx[a]]);clip.lineTo(mesh.vertices[idx[b]]);
            clip.lineTo(mesh.vertices[idx[c]]);clip.closeSubpath();painter.setClipPath(clip,Qt::IntersectClip);
            painter.setWorldTransform(affine,true);painter.drawImage(QPointF(0,0),image);painter.restore();
        }
    }
    painter.restore();
}
}
