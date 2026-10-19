#ifndef VAA3D_MASK_TIFF_VOLUME_H
#define VAA3D_MASK_TIFF_VOLUME_H
#include <tiffio.h>
#include <QString>
#include <QFile>
#include <atomic>
#include <algorithm>
#include <climits>
#include <memory>
#include <vector>
#include <limits>
#include <cstring>
#include <stdexcept>

// Independent TIFF handles and buffers; no widgets or legacy global Stack pools.
struct MaskTiffVolume
{
    int x=0,y=0,z=0,bytes=0;
    std::vector<unsigned char> data;
    void read(const QString &path,int channel,std::atomic<bool> &cancel,std::atomic<int> &progress)
    {
        std::unique_ptr<TIFF,decltype(&TIFFClose)> tif(TIFFOpen(QFile::encodeName(path).constData(),"r"),TIFFClose);
        if(!tif) throw std::runtime_error("Cannot open TIFF mask");
        uint32 w=0,h=0;uint16 bits=0,samples=1,format=1,planar=1;
        size_t pages=0;
        for(;;)
        {
            if(cancel.load())return;
            ++pages;
            if(TIFFLastDirectory(tif.get()))break;
            if(!TIFFReadDirectory(tif.get()))throw std::runtime_error("Damaged TIFF page directory");
        }
        uint16 firstBits=0,firstSamples=0,firstFormat=0;
        if(!TIFFSetDirectory(tif.get(),0))throw std::runtime_error("Invalid TIFF directory");
        for(size_t page=0;page<pages;++page)
        {
            if(cancel.load())return;
            TIFFGetField(tif.get(),TIFFTAG_IMAGEWIDTH,&w);TIFFGetField(tif.get(),TIFFTAG_IMAGELENGTH,&h);
            TIFFGetFieldDefaulted(tif.get(),TIFFTAG_BITSPERSAMPLE,&bits);
            TIFFGetFieldDefaulted(tif.get(),TIFFTAG_SAMPLESPERPIXEL,&samples);
            TIFFGetFieldDefaulted(tif.get(),TIFFTAG_SAMPLEFORMAT,&format);
            TIFFGetFieldDefaulted(tif.get(),TIFFTAG_PLANARCONFIG,&planar);
            if(!w||!h||w>INT_MAX||h>INT_MAX||pages>INT_MAX||channel<0||channel>=samples ||
               !((format==SAMPLEFORMAT_UINT&&(bits==1||bits==8||bits==16))||(format==SAMPLEFORMAT_IEEEFP&&bits==32)) ||
               (bits==1&&samples!=1) || (planar!=PLANARCONFIG_CONTIG&&planar!=PLANARCONFIG_SEPARATE))
                throw std::runtime_error("Unsupported TIFF mask dimensions/type (use 1/8/16-bit unsigned or float32)");
            int unit=bits==1?1:bits/8;
            if(page==0)
            {
                x=w;y=h;z=pages;bytes=unit;firstBits=bits;firstSamples=samples;firstFormat=format;
                size_t n=size_t(x);
                for(size_t factor:{size_t(y),size_t(z),size_t(bytes)})
                {if(n>std::numeric_limits<size_t>::max()/factor)throw std::runtime_error("TIFF mask size overflow");n*=factor;}
                data.resize(n);
            }
            else if(w!=uint32(x)||h!=uint32(y)||bits!=firstBits||samples!=firstSamples||format!=firstFormat)throw std::runtime_error("Inconsistent TIFF mask pages");
            const int stride=planar==PLANARCONFIG_CONTIG?samples:1;
            const int component=planar==PLANARCONFIG_CONTIG?channel:0;
            auto copyRow=[&](const unsigned char *src,uint32 row,uint32 column,uint32 count){
                unsigned char *out=data.data()+((page*size_t(y)+row)*x+column)*bytes;
                if(bits==1){for(uint32 i=0;i<count;++i)out[i]=(src[i/8]>>(7-i%8))&1;}
                else if(stride==1)std::memcpy(out,src,size_t(count)*bytes);
                else for(uint32 i=0;i<count;++i)std::memcpy(out+size_t(i)*bytes,src+(size_t(i)*stride+component)*bytes,bytes);
            };
            if(TIFFIsTiled(tif.get()))
            {
                uint32 tw=0,th=0;TIFFGetField(tif.get(),TIFFTAG_TILEWIDTH,&tw);TIFFGetField(tif.get(),TIFFTAG_TILELENGTH,&th);
                tsize_t size=TIFFTileSize(tif.get()), rowSize=TIFFTileRowSize(tif.get());
                if(!tw||!th||tw>INT_MAX||th>INT_MAX||size<=0||rowSize<=0||uint64(rowSize)*th>uint64(size)||uint64(rowSize)<(uint64(tw)*stride*bits+7)/8)throw std::runtime_error("Invalid TIFF tile size");
                std::vector<unsigned char> tile(size);
                for(uint32 row=0;row<h;row+=th)for(uint32 col=0;col<w;col+=tw)
                {
                    if(cancel.load())return;
                    if(TIFFReadTile(tif.get(),tile.data(),col,row,0,planar==PLANARCONFIG_SEPARATE?channel:0)<size)
                        throw std::runtime_error("Cannot decode TIFF tile");
                    for(uint32 r=0;r<th&&row+r<h;++r)copyRow(tile.data()+size_t(r)*rowSize,row+r,col,std::min(tw,w-col));
                    progress.store(int(30*(page+double(row)/h)/pages));
                }
            }
            else
            {
                tsize_t size=TIFFScanlineSize(tif.get());
                if(size<=0||uint64(size)<(uint64(w)*stride*bits+7)/8)throw std::runtime_error("Invalid TIFF scanline size");
                std::vector<unsigned char> rowBuffer(size);
                for(uint32 row=0;row<h;++row)
                {
                    if(cancel.load())return;
                    if(TIFFReadScanline(tif.get(),rowBuffer.data(),row,planar==PLANARCONFIG_SEPARATE?channel:0)<0)
                        throw std::runtime_error("Cannot decode TIFF scanline");
                    copyRow(rowBuffer.data(),row,0,w);
                    progress.store(int(30*(page+double(row)/h)/pages));
                }
            }
            if(page+1<pages&&!TIFFReadDirectory(tif.get()))throw std::runtime_error("Missing TIFF page");
        }
    }
};
#endif
