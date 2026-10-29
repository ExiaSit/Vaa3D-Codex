#ifndef VAA3D_RESCALE_ONE_MICRON_H
#define VAA3D_RESCALE_ONE_MICRON_H
#include <array>
#include <vector>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace micron {
struct Plan {
    std::array<int64_t,4> in, out;
    std::array<double,3> spacing;
    int bytes;
    size_t inputBytes, outputBytes;
};
inline Plan plan(std::array<int64_t,4> dims, std::array<double,3> spacing, int bytes) {
    if(bytes!=1 && bytes!=2 && bytes!=4) throw std::runtime_error("Only uint8, uint16 and float32 images are supported.");
    Plan p{dims,dims,spacing,bytes,0,0};
    const uint64_t budget=uint64_t(2)*1024*1024*1024;
    uint64_t input=bytes,output=bytes,weights=0;
    for(int a=0;a<4;++a){
        if(dims[a]<1 || dims[a]>1000000) throw std::runtime_error("Invalid or oversized image dimensions.");
        if(a<3){
            double n=dims[a]*spacing[a];
            if(!std::isfinite(spacing[a]) || spacing[a]<1e-6 || spacing[a]>1e6 || !std::isfinite(n) || n>1000000)
                throw std::runtime_error("Invalid spacing or output axis exceeds 1000000 voxels.");
            p.out[a]=std::max<int64_t>(1,std::llround(n));
            weights+=80*uint64_t(dims[a]+p.out[a]);
        }
        if(input>budget/uint64_t(dims[a]) || output>budget/uint64_t(p.out[a]))
            throw std::runtime_error("Rescale requires more than the 2 GiB working-memory limit.");
        input*=dims[a];output*=p.out[a];
    }
    if(input+output+weights>budget) throw std::runtime_error("Rescale requires more than the 2 GiB working-memory limit.");
    p.inputBytes=input;p.outputBytes=output;return p;
}
struct Weight {int64_t index;double value;};
using Axis=std::vector<std::vector<Weight>>;
inline Axis axis(int64_t n,int64_t count,double spacing){
    Axis result(count);
    for(int64_t i=0;i<count;++i){
        auto &w=result[i];
        if(spacing>=1){ // Linear interpolation at target voxel centres; edge extension.
            double p=std::max(0.,std::min(double(n-1),(i+.5)/spacing-.5));
            int64_t lo=int64_t(std::floor(p)),hi=std::min(n-1,lo+1);
            w.push_back({lo,1-(p-lo)});if(hi!=lo && p>lo)w.push_back({hi,p-lo});
        }else{ // Integrate source voxel cells over each target cell to suppress aliasing.
            double left=i/spacing,right=(i+1)/spacing,width=right-left;
            double end=std::min(double(n),right);
            for(int64_t j=int64_t(std::floor(left));j<end;++j){
                double overlap=std::min(end,double(j+1))-std::max(left,double(j));
                if(overlap>0)w.push_back({j,overlap/width});
            }
            if(right>n)w.push_back({n-1,(right-std::max(left,double(n)))/width});
        }
    }
    return result;
}
struct Job {
    Plan spec;
    std::unique_ptr<unsigned char[]> input, output;
    std::atomic<bool> cancel{false};
    std::atomic<int> percent{0};
    std::string error;
    explicit Job(const Plan &p):spec(p){}
};
template<class T> void runTyped(Job &job,const std::array<Axis,3> &weights){
    const auto &p=job.spec;const T *src=reinterpret_cast<const T*>(job.input.get());
    T *dst=reinterpret_cast<T*>(job.output.get());
    const int64_t ix=p.in[0],iy=p.in[1],iz=p.in[2],ox=p.out[0],oy=p.out[1],oz=p.out[2];
    for(int64_t c=0;c<p.in[3];++c)for(int64_t z=0;z<oz;++z)for(int64_t y=0;y<oy;++y){
        if(job.cancel)return;
        for(int64_t x=0;x<ox;++x){
            double sum=0;
            for(const auto &wz:weights[2][z])for(const auto &wy:weights[1][y])for(const auto &wx:weights[0][x])
                sum+=double(src[((c*iz+wz.index)*iy+wy.index)*ix+wx.index])*wz.value*wy.value*wx.value;
            dst[((c*oz+z)*oy+y)*ox+x]=p.bytes==4?T(sum):T(std::floor(std::max(0.,sum)+.5));
        }
        job.percent=int(100*((c*oz+z)*oy+y+1)/(p.in[3]*oz*oy));
    }
}
inline void run(Job &job){
    try{
        if(job.cancel)return;
        std::array<Axis,3> weights;
        for(int a=0;a<3;++a)weights[a]=axis(job.spec.in[a],job.spec.out[a],job.spec.spacing[a]);
        if(job.cancel)return;
        job.output.reset(new unsigned char[job.spec.outputBytes]);
        if(job.spec.bytes==1)runTyped<uint8_t>(job,weights);
        else if(job.spec.bytes==2)runTyped<uint16_t>(job,weights);
        else runTyped<float>(job,weights);
        if(job.cancel)job.output.reset();
    }catch(const std::exception &e){job.output.reset();job.error=e.what();}
}
}
#endif
