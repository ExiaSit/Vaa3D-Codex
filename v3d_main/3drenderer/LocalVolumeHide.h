#ifndef VAA3D_LOCAL_VOLUME_HIDE_H
#define VAA3D_LOCAL_VOLUME_HIDE_H
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

struct LocalHideBox {
    std::array<double,3> low,high;
    int frame=0;
    bool contains(const std::array<double,3> &p) const {
        for(int i=0;i<3;++i)if(p[i]<low[i]||p[i]>high[i])return false;
        return true;
    }
};
struct LocalHideVertex {
    std::array<double,3> unit,position,texture;
};
using LocalHidePolygon=std::vector<LocalHideVertex>;

inline bool localHidePolygonHasArea(const LocalHidePolygon &p)
{
    if(p.size()<3)return false;
    std::array<double,3> n{{0,0,0}};
    for(size_t i=0;i<p.size();++i)for(int axis=0;axis<3;++axis)
        n[axis]+=p[i].unit[(axis+1)%3]*p[(i+1)%p.size()].unit[(axis+2)%3]
            -p[i].unit[(axis+2)%3]*p[(i+1)%p.size()].unit[(axis+1)%3];
    return n[0]*n[0]+n[1]*n[1]+n[2]*n[2]>1e-24;
}

inline LocalHidePolygon clipLocalHidePlane(const LocalHidePolygon &p,int axis,double edge,bool less,bool strict)
{
    LocalHidePolygon out;if(p.empty())return out;
    auto inside=[&](const LocalHideVertex &v){double d=less?edge-v.unit[axis]:v.unit[axis]-edge;return strict?d>0:d>=0;};
    auto previous=p.back();bool was=inside(previous);
    for(const auto &current:p) {
        bool is=inside(current);
        if(is!=was) {
            double t=(edge-previous.unit[axis])/(current.unit[axis]-previous.unit[axis]);
            LocalHideVertex v;
            for(int i=0;i<3;++i) {
                v.unit[i]=previous.unit[i]+t*(current.unit[i]-previous.unit[i]);
                v.position[i]=previous.position[i]+t*(current.position[i]-previous.position[i]);
                v.texture[i]=previous.texture[i]+t*(current.texture[i]-previous.texture[i]);
            }
            out.push_back(v);
        }
        if(is)out.push_back(current);
        previous=current;was=is;
    }
    return out;
}

// Subtract boxes from the slice geometry, leaving source voxels and textures
// untouched. This works with 2D/3D textures, shaders and the fixed pipeline.
inline std::vector<LocalHidePolygon> subtractLocalHideBoxes(const LocalHidePolygon &quad,
    const std::vector<LocalHideBox> &boxes,int frame)
{
    std::vector<LocalHidePolygon> visible{quad};
    for(const auto &box:boxes) {
        if(box.frame!=frame)continue;
        std::vector<LocalHidePolygon> next;
        for(const auto &polygon:visible) {
            bool outside=false;
            for(int axis=0;axis<3;++axis) {
                double lo=polygon[0].unit[axis],hi=lo;
                for(const auto &v:polygon){lo=std::min(lo,v.unit[axis]);hi=std::max(hi,v.unit[axis]);}
                if(hi<box.low[axis]||lo>box.high[axis]){outside=true;break;}
            }
            if(outside){next.push_back(polygon);continue;}
            LocalHidePolygon remaining=polygon;
            for(int axis=0;axis<3&&!remaining.empty();++axis)for(int side=0;side<2&&!remaining.empty();++side) {
                double edge=side?box.high[axis]:box.low[axis];bool less=!side;
                auto piece=clipLocalHidePlane(remaining,axis,edge,less,true);
                if(localHidePolygonHasArea(piece))next.push_back(std::move(piece));
                remaining=clipLocalHidePlane(remaining,axis,edge,!less,false);
            }
        }
        visible.swap(next);if(visible.empty())break;
    }
    return visible;
}
#endif
