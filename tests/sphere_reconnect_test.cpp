#include "../v3d_main/terafly/src/control/SphereReconnect.h"
#include <cassert>
#include <iostream>
NeuronSWC node(int id,int parent,double x,double y=0,double z=0) {NeuronSWC n;n.n=id;n.pn=parent;n.x=x;n.y=y;n.z=z;n.r=1;n.type=3;return n;}
bool run(const NeuronTree &in,NeuronTree &out,double radius=2) {QString error;int removed,connections;return terafly::reconnectSphere(in,0,0,0,radius,out,error,removed,connections);}
int main() {
 NeuronTree in,out;
 in.listNeuron<<node(1,-1,0)<<node(2,1,1)<<node(3,2,5)<<node(4,1,-5)<<node(5,3,10)<<node(6,-1,50);
 assert(run(in,out));assert(out.listNeuron.size()==7);
 const auto center=out.listNeuron[0];assert(center.type==1 && center.pn==-1);
 assert(out.listNeuron[out.hashNeuron[out.listNeuron[out.hashNeuron[3]].pn]].pn==center.n);
 assert(out.listNeuron[out.hashNeuron[out.listNeuron[out.hashNeuron[4]].pn]].pn==center.n);
 assert(out.listNeuron[out.hashNeuron[5]].pn==3);assert(out.listNeuron[out.hashNeuron[6]].pn==-1);
 assert(out.listNeuron[out.hashNeuron[5]].x==10);
 // An edge can pass through the sphere without either endpoint being inside.
 in.listNeuron.clear();in.listNeuron<<node(1,-1,-5)<<node(2,1,5);
 assert(run(in,out));assert(out.listNeuron.size()==5);
 assert(std::abs(out.listNeuron[3].x)==2 && out.listNeuron[3].x==-out.listNeuron[4].x);
 assert(out.listNeuron[3].pn==out.listNeuron[0].n && out.listNeuron[4].pn==out.listNeuron[0].n);
 // An outside path joining two boundary nodes must not become a cycle.
 in.listNeuron.clear();in.listNeuron<<node(1,-1,0)<<node(2,1,5)<<node(3,2,5,5)<<node(4,3,0,5)<<node(5,4,0,0,1);
 assert(!run(in,out));
 in.listNeuron.clear();in.listNeuron<<node(1,-1,10)<<node(2,1,15);assert(!run(in,out));
 in.listNeuron<<node(1,-1,0);assert(!run(in,out));
 in.listNeuron.clear();in.listNeuron<<node(1,99,0);assert(!run(in,out));
 in.listNeuron.clear();in.listNeuron<<node(1,-1,0)<<node(2,1,1);assert(run(in,out));assert(out.listNeuron.size()==1);
 assert(!run(in,out,0));
 std::cout<<"Sphere reconnect topology tests passed\n";
}
