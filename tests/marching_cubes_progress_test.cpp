#include "../v3d_main/3drenderer/marchingcubes.h"
#include <cassert>
#include <chrono>
#include <iostream>
static float sphere(float x,float y,float z){return (x-.5f)*(x-.5f)+(y-.5f)*(y-.5f)+(z-.5f)*(z-.5f);}
struct Progress {int calls=0;int last=-1;int stop;};
static bool report(int n,void *p){auto &v=*static_cast<Progress*>(p);assert(n>=v.last&&n>=0&&n<=100);v.last=n;return ++v.calls<v.stop;}
int main(){
 auto start=std::chrono::steady_clock::now();
 Progress cancelled;cancelled.stop=8;
 auto t=MarchingCubes(1000,.12f,sphere,0,report,&cancelled);
 assert(!t && cancelled.calls==8);
 Progress partial;partial.stop=300;
 assert(MarchingCubes(24,.12f,sphere,0,report,&partial)==0);
 Progress good;good.stop=100000;
 t=MarchingCubes(24,.12f,sphere,0,report,&good);int count=numTriangles(t);assert(count>0);delTriangles(t);
 auto baseline=MarchingCubes(24,.12f,sphere);assert(numTriangles(baseline)==count);delTriangles(baseline);
 std::cout<<"Cancellation at density 1000 and regeneration passed; triangles="<<count<<" elapsed_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()<<"\n";
}
