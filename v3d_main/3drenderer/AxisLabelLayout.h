#ifndef VAA3D_AXIS_LABEL_LAYOUT_H
#define VAA3D_AXIS_LABEL_LAYOUT_H

#include <QPointF>
#include <QRectF>
#include <array>
#include <cmath>
#include <limits>

// Screen-space labels stay outside the projected image's enclosing rectangle.
// An empty rectangle means there is no room: never clamp a label onto the image.
inline std::array<QRectF,3> axisLabelLayout(const QRectF &viewport, const QRectF &image,
    const QPointF &origin, const std::array<QPointF,3> &tips, double scale=1)
{
    std::array<QRectF,3> result;
    const double w=16*scale,h=20*scale,gap=6*scale;
    const QRectF safe=viewport.adjusted(3*scale,3*scale,-3*scale,-3*scale);
    const QRectF blocked=image.adjusted(-gap,-gap,gap,gap);
    const QRectF centers=blocked.adjusted(-w/2,-h/2,w/2,h/2);
    for(int axis=0;axis<3;++axis)
    {
        QPointF direction=tips[axis]-origin;
        double length=std::hypot(direction.x(),direction.y());
        if(length<1) {direction=tips[axis]-image.center();length=std::hypot(direction.x(),direction.y());}
        if(length<1) {direction=QPointF(axis==1?0:axis==0?-1:1,axis==1?1:0);length=1;}
        direction/=length;
        const QPointF tangent(-direction.y(),direction.x());
        for(double offset:{0.0,1.5,-1.5,3.0,-3.0})
        {
            const QPointF base=tips[axis]+tangent*(offset*h);
            double distance=8*scale;
            if(centers.contains(base))
            {
                double exit=std::numeric_limits<double>::infinity();
                if(direction.x()>1e-6)exit=qMin(exit,(centers.right()-base.x())/direction.x());
                if(direction.x()<-1e-6)exit=qMin(exit,(centers.left()-base.x())/direction.x());
                if(direction.y()>1e-6)exit=qMin(exit,(centers.bottom()-base.y())/direction.y());
                if(direction.y()<-1e-6)exit=qMin(exit,(centers.top()-base.y())/direction.y());
                distance=qMax(distance,exit+2*scale);
            }
            const QPointF center=base+direction*distance;
            const QRectF box(center.x()-w/2,center.y()-h/2,w,h);
            if(!safe.contains(box)||blocked.intersects(box))continue;
            bool overlaps=false;
            for(int i=0;i<axis;++i)
                if(!result[i].isEmpty()&&result[i].adjusted(-2*scale,-2*scale,2*scale,2*scale).intersects(box))overlaps=true;
            if(!overlaps) {result[axis]=box;break;}
        }
    }
    return result;
}
#endif
