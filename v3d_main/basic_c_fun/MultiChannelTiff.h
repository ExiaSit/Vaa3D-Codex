#ifndef VAA3D_MULTI_CHANNEL_TIFF_H
#define VAA3D_MULTI_CHANNEL_TIFF_H

#include <tiffio.h>
#include "v3d_basicdatatype.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

// The legacy Stack has no representation for multichannel grayscale data.
// Return -1 for its existing formats, 0 on success, 1 on a read error.
inline int readMultiChannelTiff(const char *path, unsigned char *&image,
                                V3DLONG *&dimensions, int &datatype)
{
    try
    {
        std::unique_ptr<TIFF, decltype(&TIFFClose)> tif(TIFFOpen(path, "r"), TIFFClose);
        if (!tif) return 1;
        uint16 photo=0, samples=1, bits=0, format=1;
        TIFFGetFieldDefaulted(tif.get(), TIFFTAG_PHOTOMETRIC, &photo);
        TIFFGetFieldDefaulted(tif.get(), TIFFTAG_SAMPLESPERPIXEL, &samples);
        if (photo > PHOTOMETRIC_MINISBLACK || samples <= 1) return -1;
        uint32 width=0, height=0;
        TIFFGetField(tif.get(), TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif.get(), TIFFTAG_IMAGELENGTH, &height);
        TIFFGetFieldDefaulted(tif.get(), TIFFTAG_BITSPERSAMPLE, &bits);
        TIFFGetFieldDefaulted(tif.get(), TIFFTAG_SAMPLEFORMAT, &format);
        if (!width || !height ||
            !((format==SAMPLEFORMAT_UINT && (bits==8 || bits==16)) ||
              (format==SAMPLEFORMAT_IEEEFP && bits==32)))
            throw std::runtime_error("Unsupported multichannel grayscale TIFF type");

        size_t pages=0;
        do
        {
            uint32 w=0, h=0;
            uint16 s=1, b=0, f=1, p=0, planar=1;
            TIFFGetField(tif.get(), TIFFTAG_IMAGEWIDTH, &w);
            TIFFGetField(tif.get(), TIFFTAG_IMAGELENGTH, &h);
            TIFFGetFieldDefaulted(tif.get(), TIFFTAG_SAMPLESPERPIXEL, &s);
            TIFFGetFieldDefaulted(tif.get(), TIFFTAG_BITSPERSAMPLE, &b);
            TIFFGetFieldDefaulted(tif.get(), TIFFTAG_SAMPLEFORMAT, &f);
            TIFFGetFieldDefaulted(tif.get(), TIFFTAG_PHOTOMETRIC, &p);
            TIFFGetFieldDefaulted(tif.get(), TIFFTAG_PLANARCONFIG, &planar);
            if (w!=width || h!=height || s!=samples || b!=bits || f!=format || p!=photo ||
                (planar!=PLANARCONFIG_CONTIG && planar!=PLANARCONFIG_SEPARATE))
                throw std::runtime_error("Inconsistent multichannel TIFF pages");
            ++pages;
            if (TIFFLastDirectory(tif.get())) break;
            if (!TIFFReadDirectory(tif.get())) throw std::runtime_error("Damaged TIFF directory");
        } while (true);

        const size_t bytes=bits/8;
        size_t total=width;
        const size_t limit=std::min(size_t(std::numeric_limits<V3DLONG>::max()),
                                    std::numeric_limits<size_t>::max());
        for (size_t factor : {size_t(height), pages, size_t(samples), bytes})
        {
            if (total > limit/factor) throw std::runtime_error("TIFF dimensions overflow");
            total *= factor;
        }
        std::unique_ptr<unsigned char[]> output(new unsigned char[total]);
        std::unique_ptr<V3DLONG[]> size(new V3DLONG[4]);
        size[0]=width; size[1]=height; size[2]=pages; size[3]=samples;
        const size_t plane=size_t(width)*height, volume=plane*pages;
        if (!TIFFSetDirectory(tif.get(), 0)) throw std::runtime_error("Invalid TIFF directory");
        for (size_t z=0; z<pages; ++z)
        {
            uint16 planar=1;
            TIFFGetFieldDefaulted(tif.get(), TIFFTAG_PLANARCONFIG, &planar);
            const size_t stride=planar==PLANARCONFIG_CONTIG ? samples : 1;
            const uint16 passes=planar==PLANARCONFIG_SEPARATE ? samples : 1;
            auto copyRow = [&](const unsigned char *src, uint32 y, uint32 x, uint32 count, uint16 pass) {
                for (size_t c=0; c<stride; ++c)
                {
                    unsigned char *dst=output.get()+((c+pass)*volume+z*plane+size_t(y)*width+x)*bytes;
                    if (stride==1) std::memcpy(dst, src, size_t(count)*bytes);
                    else for (uint32 i=0; i<count; ++i)
                        std::memcpy(dst+size_t(i)*bytes, src+(size_t(i)*stride+c)*bytes, bytes);
                }
            };
            if (TIFFIsTiled(tif.get()))
            {
                uint32 tw=0, th=0;
                TIFFGetField(tif.get(), TIFFTAG_TILEWIDTH, &tw);
                TIFFGetField(tif.get(), TIFFTAG_TILELENGTH, &th);
                const tsize_t n=TIFFTileSize(tif.get()), row=TIFFTileRowSize(tif.get());
                if (!tw || !th || n<=0 || row<=0 || uint64(row)*th>uint64(n) ||
                    uint64(row)<uint64(tw)*stride*bytes)
                    throw std::runtime_error("Invalid TIFF tile size");
                std::vector<unsigned char> buffer(n);
                for (uint16 pass=0; pass<passes; ++pass)
                    for (uint64 y=0; y<height; y+=th) for (uint64 x=0; x<width; x+=tw)
                    {
                        if (TIFFReadTile(tif.get(), buffer.data(), x, y, 0, pass)<n)
                            throw std::runtime_error("Cannot decode TIFF tile");
                        for (uint32 r=0; r<std::min(uint64(th), uint64(height)-y); ++r)
                            copyRow(buffer.data()+size_t(r)*row, y+r, x,
                                    std::min(uint64(tw), uint64(width)-x), pass);
                    }
            }
            else
            {
                const tsize_t n=TIFFScanlineSize(tif.get());
                if (n<=0 || uint64(n)<uint64(width)*stride*bytes)
                    throw std::runtime_error("Invalid TIFF scanline size");
                std::vector<unsigned char> buffer(n);
                for (uint16 pass=0; pass<passes; ++pass) for (uint32 y=0; y<height; ++y)
                {
                    if (TIFFReadScanline(tif.get(), buffer.data(), y, pass)<0)
                        throw std::runtime_error("Cannot decode TIFF scanline");
                    copyRow(buffer.data(), y, 0, width, pass);
                }
            }
            if (z+1<pages && !TIFFReadDirectory(tif.get()))
                throw std::runtime_error("Missing TIFF page");
        }
        // Publish only a complete volume, preserving raw intensities and stored row order.
        delete[] image; delete[] dimensions;
        image=output.release(); dimensions=size.release(); datatype=bytes;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "Multichannel TIFF: %s\n", e.what());
        return 1;
    }
}
#endif
