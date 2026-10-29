#include "../v3d_main/basic_c_fun/RescaleOneMicron.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace micron;
template<class T> void check(int bytes){
    auto p=plan({{4,2,2,4}},{{.5,1,2}},bytes);
    assert(p.out[0]==2 && p.out[1]==2 && p.out[2]==4 && p.out[3]==4);
    Job job(p);job.input.reset(new unsigned char[p.inputBytes]);
    auto src=reinterpret_cast<T*>(job.input.get());
    for(int c=0;c<4;++c)for(int z=0;z<2;++z)for(int y=0;y<2;++y)for(int x=0;x<4;++x)
        src[((c*2+z)*2+y)*4+x]=T(c*40+z*16+y*8+x*2);
    std::vector<unsigned char> before(job.input.get(),job.input.get()+p.inputBytes);
    run(job);assert(job.output && job.error.empty() && job.percent==100);
    auto out=reinterpret_cast<T*>(job.output.get());
    double offsets[]={0,4,12,16};
    for(int c=0;c<4;++c)for(int z=0;z<4;++z)for(int y=0;y<2;++y)for(int x=0;x<2;++x)
        assert(out[((c*4+z)*2+y)*2+x]==T(c*40+offsets[z]+y*8+x*4+1));
    assert(!memcmp(before.data(),job.input.get(),p.inputBytes));
    auto identity=plan({{4,2,2,4}},{{1,1,1}},bytes);Job id(identity);
    id.input.reset(new unsigned char[identity.inputBytes]);memcpy(id.input.get(),src,identity.inputBytes);
    run(id);assert(!memcmp(id.output.get(),src,identity.inputBytes));
    Job canceled(p);canceled.cancel=true;run(canceled);assert(!canceled.output);
}
int main(){
    check<uint8_t>(1);check<uint16_t>(2);check<float>(4);
    auto real=plan({{1024,1024,28,4}},{{.564542,.564542,1.444444}},2);
    assert(real.out[0]==578&&real.out[1]==578&&real.out[2]==40&&real.outputBytes==106906880);
    for(double s:{0.,-1.,double(NAN),double(INFINITY),1e-300,1000001.}){
        bool failed=false;try{plan({{4,4,4,1}},{{s,1,1}},1);}catch(...){failed=true;}assert(failed);
    }
    bool failed=false;try{plan({{10000,10000,10000,4}},{{1,1,1}},2);}catch(...){failed=true;}assert(failed);
    // Fractional final cells extend the edge; normalization preserves a constant field.
    auto tiny=plan({{3,2,1,1}},{{.2,.8,1.4}},1);Job j(tiny);j.input.reset(new unsigned char[tiny.inputBytes]);
    memset(j.input.get(),77,tiny.inputBytes);run(j);
    for(size_t i=0;i<tiny.outputBytes;++i)assert(j.output[i]==77);
    std::cout<<"RESCALE_ONE_MICRON_PASS\n";
}
