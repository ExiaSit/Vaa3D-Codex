#include "../v3d_main/3drenderer/marchingcubes.h"
#include <thread>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <stdexcept>
static float sphere(float x,float y,float z){return (x-.4f)*(x-.4f)+(y-.5f)*(y-.5f)+(z-.6f)*(z-.6f);}
static float plane(float x,float y,float z){return x+2*y+z;}
static uint64_t result(int mode)
{
    Triangle *t=MarchingCubes(mode?41:37,mode?1.4f:.08f,mode?plane:sphere,mode);
    uint64_t hash=1469598103934665603ULL;int n=0;
    for(auto p=t;p;p=p->next){++n;for(auto data:{&p->vertex[0][0],&p->normal[0][0]})for(int i=0;i<9;++i){uint32_t bits;memcpy(&bits,data+i,4);hash=(hash^bits)*1099511628211ULL;}}
    assert(n>0);delTriangles(t);return hash;
}
int main()
{
    uint64_t a=result(0),b=result(1),parallelA=0,parallelB=0;
    std::thread one([&](){parallelA=result(0);});std::thread two([&](){parallelB=result(1);});one.join();two.join();assert(a==parallelA&&b==parallelB);
    assert(!MarchingCubes(1000,.08f,sphere,0,[](int p,void*){return p<1;},nullptr));
    bool threw=false;try{MarchingCubes(30,.08f,sphere,0,[](int p,void*){if(p>60)throw std::runtime_error("test");return true;},nullptr);}catch(const std::runtime_error&){threw=true;}
    assert(threw&&result(0)==a);std::cout<<"MARCHING_PARALLEL_PASS"<<std::endl;
}
