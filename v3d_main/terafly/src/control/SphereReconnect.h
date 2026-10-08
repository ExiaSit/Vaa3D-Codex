#ifndef TERAFLY_SPHERE_RECONNECT_H
#define TERAFLY_SPHERE_RECONNECT_H
#include "basic_surf_objs.h"
#include <QSet>
#include <QQueue>
#include <cmath>
#include <limits>

namespace terafly {
// Contract the part of an SWC inside a sphere, including edges crossing it.
// Outside geometry is preserved. Reject contractions that would introduce loops.
inline bool reconnectSphere(const NeuronTree &input, double x, double y, double z,
                            double radius, NeuronTree &output, QString &error,
                            int &removed, int &connections)
{
    removed = connections = 0;
    if (!std::isfinite(radius) || radius <= 0 || !std::isfinite(x) ||
        !std::isfinite(y) || !std::isfinite(z)) {
        error = QString::fromUtf8("球心和半径必须是有效数值，半径必须大于 0。"); return false;
    }
    QHash<V3DLONG, int> index;
    QSet<V3DLONG> inside, boundary;
    V3DLONG maxId = 0;
    for (int i=0; i<input.listNeuron.size(); ++i) {
        const NeuronSWC &n=input.listNeuron[i];
        if (index.contains(n.n) || n.n < 0) {
            error=QString::fromUtf8("标注含重复或无效节点编号，请先修复标注。"); return false;
        }
        index.insert(n.n,i); maxId=qMax(maxId,n.n);
        if ((n.x-x)*(n.x-x)+(n.y-y)*(n.y-y)+(n.z-z)*(n.z-z)<=radius*radius)
            inside.insert(n.n);
    }
    if (maxId > std::numeric_limits<V3DLONG>::max()-2*input.listNeuron.size()-1) {error="Node ID overflow";return false;}
    const V3DLONG center=maxId+1;
    V3DLONG nextId=center+1;
    QList<NeuronSWC> surfaceNodes;
    QHash<V3DLONG, QSet<V3DLONG> > adjacency;
    bool touched=!inside.isEmpty();
    for (const NeuronSWC &n: input.listNeuron) {
        if (n.pn<0) continue;
        if (!index.contains(n.pn)) {
            error=QString::fromUtf8("标注含缺失的父节点，无法安全重连。"); return false;
        }
        const NeuronSWC &p=input.listNeuron[index[n.pn]];
        double dx=p.x-n.x, dy=p.y-n.y, dz=p.z-n.z;
        double len=dx*dx+dy*dy+dz*dz;
        double t=len>0 ? qBound(0.0,((x-n.x)*dx+(y-n.y)*dy+(z-n.z)*dz)/len,1.0) : 0;
        double ex=n.x+t*dx-x, ey=n.y+t*dy-y, ez=n.z+t*dz-z;
        bool intersects=inside.contains(n.n)||inside.contains(p.n)||ex*ex+ey*ey+ez*ez<radius*radius;
        if (intersects) {
            touched=true;
            // Split crossing edges at the sphere surface so their outside
            // portions retain exactly the same geometry.
            double projection=len>0 ? ((x-n.x)*dx+(y-n.y)*dy+(z-n.z)*dz)/len : 0;
            double ox=n.x-x,oy=n.y-y,oz=n.z-z;
            double delta=len>0 ? sqrt(qMax(0.0,projection*projection-(ox*ox+oy*oy+oz*oz-radius*radius)/len)) : 0;
            auto attach=[&](const NeuronSWC &outside,double parameter) {
                NeuronSWC edge=outside;edge.n=nextId++;edge.pn=center;
                edge.x=n.x+parameter*dx;edge.y=n.y+parameter*dy;edge.z=n.z+parameter*dz;
                surfaceNodes.append(edge);boundary.insert(edge.n);
                adjacency[outside.n].insert(edge.n);adjacency[edge.n].insert(outside.n);
            };
            if (!inside.contains(n.n)) attach(n,qBound(0.0,projection-delta,1.0));
            if (!inside.contains(p.n)) attach(p,qBound(0.0,projection+delta,1.0));
        } else {
            adjacency[n.n].insert(p.n); adjacency[p.n].insert(n.n);
        }
    }
    if (!touched) {error=QString::fromUtf8("球内没有节点或穿过球体的线段。"); return false;}
    for (V3DLONG id: boundary) {adjacency[center].insert(id);adjacency[id].insert(center);}
    QHash<V3DLONG,V3DLONG> parent;
    QQueue<V3DLONG> queue; queue.enqueue(center);parent[center]=-1;
    while (!queue.isEmpty()) {
        V3DLONG id=queue.dequeue();
        for (V3DLONG next:adjacency[id]) {
            if(next==parent[id]) continue;
            if(parent.contains(next)) {
                error=QString::fromUtf8("这个球会使球外线段形成环路，请缩小或移动球体后再试。"); return false;
            }
            parent[next]=id;queue.enqueue(next);
        }
    }
    output=input;output.listNeuron.clear();output.hashNeuron.clear();
    NeuronSWC soma; soma.n=center;soma.pn=-1;soma.type=1;soma.x=x;soma.y=y;soma.z=z;soma.r=1;
    output.listNeuron.append(soma);
    for (NeuronSWC n:input.listNeuron) {
        if(inside.contains(n.n)) continue;
        if(parent.contains(n.n)) n.pn=parent[n.n];
        output.listNeuron.append(n);
    }
    for(NeuronSWC n:surfaceNodes) {n.pn=parent[n.n];output.listNeuron.append(n);}
    for(int i=0;i<output.listNeuron.size();++i) output.hashNeuron.insert(output.listNeuron[i].n,i);
    removed=inside.size();connections=boundary.size();return true;
}
}
#endif
