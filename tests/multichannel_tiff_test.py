#!/usr/bin/env python3
"""Check multichannel TIFF pixels against tifffile; optionally include a real TIFF.

Requires numpy, tifffile, imagecodecs, g++, and the local libtiff build.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import numpy as np
import tifffile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include "MultiChannelTiff.h"
#include "mg_image_lib.h"
int main(int argc, char **argv) {
    if (argc!=4) return 2;
    unsigned char *data=new unsigned char[1]; data[0]=123;
    V3DLONG *sz=new V3DLONG[4]{1,1,1,1}; int type=1;
    auto oldData=data; auto oldSize=sz;
    int result=readMultiChannelTiff(argv[1],data,sz,type);
    if (result!=0) {
        bool preserved=data==oldData && sz==oldSize && data[0]==123 && type==1;
        delete[] data; delete[] sz;
        return preserved ? (result==-1 ? 4 : 3) : 5;
    }
    FILE *out=fopen(argv[2],"wb"); if(!out) return 6;
    size_t n=size_t(sz[0])*sz[1]*sz[2]*sz[3]*type;
    bool ok=fwrite(data,1,n,out)==n; fclose(out);
    out=fopen(argv[3],"w"); if(!out) return 7;
    fprintf(out,"%ld %ld %ld %ld %d",long(sz[0]),long(sz[1]),long(sz[2]),long(sz[3]),type);fclose(out);
    delete[] data; delete[] sz;
    // Direct legacy callers must reject this unsupported Stack layout safely.
    Stack *legacy=Read_Stack(argv[1]);
    if(legacy) { Kill_Stack(legacy); return 8; }
    return ok ? 0 : 9;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix='vaa3d-multichannel-') as folder:
        work = Path(folder)
        cpp = work / 'reader.cpp'
        cpp.write_text(HARNESS)
        exe = work / 'reader'
        lib = ROOT / 'v3d_main/common_lib/lib'
        qmake = os.environ.get('QMAKE_BIN', '/home/pzy/anaconda3/bin/qmake')
        qt = Path(subprocess.check_output([qmake, '-query', 'QT_INSTALL_HEADERS'], text=True).strip())
        subprocess.run([
            'g++', '-std=c++11', '-g', '-w', '-fsanitize=address,undefined',
            '-fno-omit-frame-pointer', '-no-pie', '-fPIC', '-DQT_NO_VERSION_TAGGING',
            '-I' + str(qt), '-I' + str(qt / 'QtCore'),
            '-I' + str(ROOT / 'v3d_main/basic_c_fun'),
            '-I' + str(ROOT / 'v3d_main/common_lib/include'), str(cpp),
            str(ROOT / 'v3d_main/basic_c_fun/mg_image_lib.cpp'),
            str(ROOT / 'v3d_main/basic_c_fun/mg_utilities.cpp'),
            '-L' + str(lib), '-Wl,-rpath,' + str(lib), '-lv3dtiff',
            '-o', str(exe)], check=True)

        def run(path):
            return subprocess.run([str(exe), str(path), str(work / 'raw'), str(work / 'shape')],
                                  capture_output=True, text=True)

        def check(path, expected):
            result = run(path)
            assert result.returncode == 0, result.stderr
            x, y, z, c, unit = map(int, (work / 'shape').read_text().split())
            assert (c, z, y, x) == expected.shape and unit == expected.dtype.itemsize
            actual = np.fromfile(work / 'raw', expected.dtype).reshape(expected.shape)
            np.testing.assert_array_equal(actual, expected)
            print('PASS all pixels + legacy rejection:', path.name, expected.shape)

        for dtype in (np.uint8, np.uint16, np.float32):
            for channels in (2, 4):
                data = (np.arange(channels*3*19*23).reshape(channels, 3, 19, 23)*17+3).astype(dtype)
                for planar in ('contig', 'separate'):
                    disk = data.transpose(1, 2, 3, 0) if planar == 'contig' else data.transpose(1, 0, 2, 3)
                    for tiled in (False, True):
                        path = work / f'{dtype.__name__}-{channels}-{planar}-{tiled}.tif'
                        tifffile.imwrite(path, disk, photometric='minisblack', planarconfig=planar,
                                         compression='deflate', tile=(16, 16) if tiled else None,
                                         rowsperstrip=None if tiled else 5, metadata=None,
                                         bigtiff=tiled, byteorder='>' if tiled else '<')
                        check(path, data)
        # Nikon-like alpha tags do not discard fluorescence channels.
        data = np.arange(3*19*23*4, dtype=np.uint16).reshape(3,19,23,4)
        path = work / 'nikon.tif'
        tifffile.imwrite(path, data, photometric='minisblack', planarconfig='contig',
                         extrasamples=('unassalpha',)*3, rowsperstrip=1, metadata=None)
        check(path, data.transpose(3,0,1,2))
        for name, kwargs in [('gray', {}), ('rgb', {'photometric': 'rgb'})]:
            path = work / (name+'.tif')
            tifffile.imwrite(path, np.zeros((7,11) if name=='gray' else (7,11,3), np.uint8), **kwargs)
            assert run(path).returncode == 4  # retain legacy routing
        path = work / 'signed.tif'
        tifffile.imwrite(path, data.astype(np.int16), photometric='minisblack', planarconfig='contig', metadata=None)
        assert run(path).returncode == 3
        path = work / 'mismatch.tif'
        with tifffile.TiffWriter(path) as t:
            t.write(data[0], photometric='minisblack', planarconfig='contig', metadata=None)
            t.write(data[0,:8], photometric='minisblack', planarconfig='contig', metadata=None)
        assert run(path).returncode == 3
        path = work / 'truncated.tif'
        tifffile.imwrite(path, data, photometric='minisblack', planarconfig='contig', metadata=None)
        with path.open('r+b') as f:
            f.truncate(path.stat().st_size//2)
        assert run(path).returncode == 3
        print('PASS fallback and rejection with unchanged output buffers')
        if len(sys.argv)>1:
            path = Path(sys.argv[1])
            with tifffile.TiffFile(path) as tf:
                data = np.stack([page.asarray() for page in tf.pages])
            check(path, data.transpose(3,0,1,2))
        print('MULTICHANNEL_TIFF_PASS (ASan + UBSan)')


if __name__ == '__main__':
    main()
