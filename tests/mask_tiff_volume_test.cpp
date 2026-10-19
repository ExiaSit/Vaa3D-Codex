#include "../v3d_main/basic_c_fun/MaskTiffVolume.h"
#include <QTemporaryDir>
#include <cassert>
#include <iostream>
#include <thread>

static void fixture(QString path,int bits,int samples,bool separate,bool tiled)
{
    TIFF *t=TIFFOpen(QFile::encodeName(path).constData(),"w");assert(t);
    const int w=19,h=17;
    for(int z=0;z<3;++z)
    {
        TIFFSetField(t,TIFFTAG_IMAGEWIDTH,w);TIFFSetField(t,TIFFTAG_IMAGELENGTH,h);
        TIFFSetField(t,TIFFTAG_BITSPERSAMPLE,bits);TIFFSetField(t,TIFFTAG_SAMPLESPERPIXEL,samples);
        TIFFSetField(t,TIFFTAG_SAMPLEFORMAT,bits==32?SAMPLEFORMAT_IEEEFP:SAMPLEFORMAT_UINT);
        TIFFSetField(t,TIFFTAG_PLANARCONFIG,separate?PLANARCONFIG_SEPARATE:PLANARCONFIG_CONTIG);
        TIFFSetField(t,TIFFTAG_PHOTOMETRIC,samples==3?PHOTOMETRIC_RGB:PHOTOMETRIC_MINISBLACK);
        TIFFSetField(t,TIFFTAG_COMPRESSION,COMPRESSION_LZW);
        if(tiled){TIFFSetField(t,TIFFTAG_TILEWIDTH,16);TIFFSetField(t,TIFFTAG_TILELENGTH,16);}
        else TIFFSetField(t,TIFFTAG_ROWSPERSTRIP,8);
        int tw=tiled?16:w,th=tiled?16:1,stride=separate?1:samples;
        size_t rowBytes=(tw*stride*bits+7)/8;
        std::vector<unsigned char> buf(rowBytes*th);
        for(int plane=0;plane<(separate?samples:1);++plane)
        for(int y=0;y<h;y+=th)for(int x=0;x<w;x+=tw)
        {
            std::fill(buf.begin(),buf.end(),0);
            for(int r=0;r<th;++r)for(int c=0;c<tw;++c)for(int ch=0;ch<stride;++ch)
            {
                int v=(z*17+(y+r)*3+x+c+(separate?plane:ch)*43)%127;
                size_t i=r*rowBytes+(c*stride+ch)*(bits/8);
                if(bits==1){if(v%2)buf[r*rowBytes+c/8]|=1<<(7-c%8);}
                else if(bits==8)buf[i]=v;
                else if(bits==16){uint16 value=v*257;memcpy(buf.data()+i,&value,2);}
                else {float value=v+0.25f;memcpy(buf.data()+i,&value,4);}
            }
            if(tiled)assert(TIFFWriteTile(t,buf.data(),x,y,0,plane)>0);
            else assert(TIFFWriteScanline(t,buf.data(),y,plane)>=0);
        }
        assert(TIFFWriteDirectory(t));
    }
    TIFFClose(t);
}
int main()
{
    QTemporaryDir dir;assert(dir.isValid());int cases=0;
    for(int bits:{1,8,16,32})for(bool tiled:{false,true})for(int mode=0;mode<(bits==1?1:3);++mode)
    {
        int samples=mode?3:1;QString path=dir.path()+"/mask.tif";fixture(path,bits,samples,mode==2,tiled);
        for(int ch=0;ch<samples;++ch)
        {
            MaskTiffVolume volume;std::atomic<bool> cancel{false};std::atomic<int> progress{0};volume.read(path,ch,cancel,progress);
            assert(volume.x==19&&volume.y==17&&volume.z==3);
            for(int z=0;z<3;++z)for(int y=0;y<17;++y)for(int x=0;x<19;++x)
            {
                int v=(z*17+y*3+x+ch*43)%127;size_t i=((z*17+y)*19+x)*volume.bytes;
                if(bits==1)assert(volume.data[i]==v%2);
                else if(bits==8)assert(volume.data[i]==v);
                else if(bits==16){uint16 value;memcpy(&value,volume.data.data()+i,2);assert(value==v*257);}
                else {float value;memcpy(&value,volume.data.data()+i,4);assert(value==v+0.25f);}
            }
            ++cases;
        }
    }
    std::atomic<bool> cancel{true};std::atomic<int> progress{0};MaskTiffVolume volume;
    volume.read(dir.path()+"/mask.tif",0,cancel,progress);assert(volume.data.empty());cancel=false;
    bool failed=false;try{volume.read(dir.path()+"/mask.tif",99,cancel,progress);}catch(const std::exception&){failed=true;}assert(failed);
    QFile bad(dir.path()+"/bad.tif");assert(bad.open(QIODevice::WriteOnly));bad.write("bad");bad.close();failed=false;
    try{volume.read(bad.fileName(),0,cancel,progress);}catch(const std::exception&){failed=true;}assert(failed);
    QFile truncated(dir.path()+"/mask.tif");assert(truncated.open(QIODevice::ReadWrite));assert(truncated.resize(truncated.size()-10));truncated.close();failed=false;
    try{volume.read(truncated.fileName(),0,cancel,progress);}catch(const std::exception&){failed=true;}assert(failed);
    std::cout<<"MASK_TIFF_READER_PASS cases="<<cases<<std::endl;
}
