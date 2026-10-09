#ifndef VAA3D_MAXIMUM_Z_PROJECTION_H
#define VAA3D_MAXIMUM_Z_PROJECTION_H
#include <cstddef>
#include <algorithm>
#include <cmath>
// Channel-major XYZ volumes; caller supplies positive dimensions and output storage.
template<class T>
void maximumZProjection(const T *src,T *dst,std::size_t plane,std::size_t depth,std::size_t channels)
{
    for(std::size_t c=0;c<channels;++c) {
        const T *volume=src+c*plane*depth;
        T *out=dst+c*plane;
        std::copy(volume,volume+plane,out);
        for(std::size_t z=1;z<depth;++z)
            for(std::size_t i=0;i<plane;++i)
                if(volume[z*plane+i]>out[i])out[i]=volume[z*plane+i];
    }
}
template<class T>
void nonzeroZProjection(const T *src,unsigned char *dst,std::size_t plane,std::size_t depth)
{
    std::fill(dst,dst+plane,0);
    for(std::size_t z=0;z<depth;++z)
        for(std::size_t i=0;i<plane;++i)
            if(std::isfinite(double(src[z*plane+i])) && src[z*plane+i]!=0)dst[i]=1;
}
#endif
