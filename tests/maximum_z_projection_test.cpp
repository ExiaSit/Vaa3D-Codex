#include "../v3d_main/basic_c_fun/MaximumZProjection.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <iterator>
#include <iostream>
template<class T> void check() {
    // Two channels, three Z slices, two pixels: maxima occur at different depths.
    const T src[]={1,9,7,2,4,3, 8,2,3,6,5,4};
    T out[4];maximumZProjection(src,out,2,3,2);
    assert(out[0]==7 && out[1]==9 && out[2]==8 && out[3]==6);
    maximumZProjection(src,out,2,1,2);
    for(int i=0;i<4;++i)assert(out[i]==src[i]);
}
int main(int argc,char**argv) {
    check<unsigned char>();check<unsigned short>();check<float>();
    const float negatives[]={-10,-2,-3,-8};float out[2];
    maximumZProjection(negatives,out,2,2,1);assert(out[0]==-3 && out[1]==-2);
    const unsigned short mask[]={0,255,0,0, 2,0,0,0, 0,0,0,3};
    unsigned char projected[4];nonzeroZProjection(mask,projected,4,3);
    assert(projected[0]==1 && projected[1]==1 && projected[2]==0 && projected[3]==1);
    if(argc>=3) {
        std::ifstream f(argv[1],std::ios::binary), e(argv[2],std::ios::binary);
        std::vector<unsigned char> src((std::istreambuf_iterator<char>(f)),{});
        std::vector<unsigned char> expected((std::istreambuf_iterator<char>(e)),{});
        assert(!expected.empty() && src.size()%expected.size()==0);
        std::vector<unsigned char> actual(expected.size());
        if(argc==4)nonzeroZProjection(src.data(),actual.data(),expected.size(),src.size()/expected.size());
        else maximumZProjection(src.data(),actual.data(),expected.size(),src.size()/expected.size(),1);
        assert(actual==expected);
    }
    std::cout<<"Maximum Z projection tests passed\n";
}
