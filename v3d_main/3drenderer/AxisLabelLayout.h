#ifndef VAA3D_AXIS_LABEL_LAYOUT_H
#define VAA3D_AXIS_LABEL_LAYOUT_H

#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

namespace AxisLabelDetail {
inline double cross(QPointF a,QPointF b){return a.x()*b.y()-a.y()*b.x();}

inline QPolygonF convexHull(QPolygonF points)
{
    std::sort(points.begin(),points.end(),[](QPointF a,QPointF b){return a.x()<b.x()||(a.x()==b.x()&&a.y()<b.y());});
    points.erase(std::unique(points.begin(),points.end()),points.end());
    if(points.size()<3)return points;
    QPolygonF hull;
    for(auto p:points){while(hull.size()>1&&cross(hull.last()-hull[hull.size()-2],p-hull.last())<=0)hull.removeLast();hull<<p;}
    const int lower=hull.size();
    for(int i=points.size()-2;i>=0;--i){auto p=points[i];while(hull.size()>lower&&cross(hull.last()-hull[hull.size()-2],p-hull.last())<=0)hull.removeLast();hull<<p;}
    hull.removeLast();return hull;
}

inline bool contains(const QPolygonF &hull,QPointF point)
{
    for(int i=0;i<hull.size();++i)
        if(cross(hull[(i+1)%hull.size()]-hull[i],point-hull[i]) < -1e-7)return false;
    return true;
}
}

// Anchor each letter near its projected axis tip, extending away from the image
// center. The fixed input corner average varies continuously with the camera.
// Avoid the true silhouette; never switch to a discrete sideways fallback.
// If a nearby, nonoverlapping position is impossible, hide this letter.
inline std::array<QRectF,3> axisLabelLayout(const QRectF &viewport, const QPolygonF &image,
    const QPointF &origin, const std::array<QPointF,3> &tips, double scale=1)
{
    std::array<QRectF,3> result;
    if(image.isEmpty()||!std::isfinite(scale)||scale<=0)return result;
    const double w=16*scale,h=20*scale,gap=6*scale;
    const QRectF safe=viewport.adjusted(3*scale,3*scale,-3*scale,-3*scale);
    // Minkowski sum with a letter half-box and margin: forbidden label centers.
    QPolygonF expanded;
    QPointF imageCenter;
    const int count=image.size();
    for(int i=0;i<count;++i)imageCenter+=image[i];
    imageCenter/=count;
    for(auto p:image) {
        if(!std::isfinite(p.x())||!std::isfinite(p.y()))return result;
        for(int x:{-1,1})for(int y:{-1,1})expanded<<p+QPointF(x*(w/2+gap),y*(h/2+gap));
    }
    const auto blocked=AxisLabelDetail::convexHull(expanded);
    for(int axis=0;axis<3;++axis)
    {
        const QPointF projectedAxis=tips[axis]-origin;
        const double axisLength=std::hypot(projectedAxis.x(),projectedAxis.y());
        // An almost end-on axis has no stable screen direction.
        if(!std::isfinite(axisLength)||axisLength<4*scale)continue;
        QPointF direction=tips[axis]-imageCenter;
        const double length=std::hypot(direction.x(),direction.y());
        if(!std::isfinite(length)||length<4*scale)continue;
        direction/=length;
        double distance=8*scale+std::abs(direction.x())*w/2+std::abs(direction.y())*h/2;
        const QPointF base=tips[axis]+direction*distance;
        if(AxisLabelDetail::contains(blocked,base)) {
            double exit=std::numeric_limits<double>::infinity();
            for(int i=0;i<blocked.size();++i) {
                auto edge=blocked[(i+1)%blocked.size()]-blocked[i];
                double slope=AxisLabelDetail::cross(edge,direction);
                if(slope < -1e-9)exit=qMin(exit,-AxisLabelDetail::cross(edge,base-blocked[i])/slope);
            }
            // No extra pixel offset here: that would jump at the hull boundary.
            distance+=qMax(0.0,exit)+1e-5*scale;
        }
        if(!std::isfinite(distance)||distance>48*scale)continue;
        const QPointF center=tips[axis]+direction*distance;
        const QRectF box(center.x()-w/2,center.y()-h/2,w,h);
        if(!safe.contains(box))continue;
        bool overlaps=false;
        for(int i=0;i<axis;++i)
            if(!result[i].isEmpty()&&result[i].adjusted(-2*scale,-2*scale,2*scale,2*scale).intersects(box))overlaps=true;
        if(!overlaps)result[axis]=box;
    }
    return result;
}
#endif
