#!/usr/bin/env python3
"""Test the actual legacy TIFF reader/writer against tifffile (numpy required).

Run: python tests/tiff_orientation_test.py [optional real TIFF]
Requires the local TIFF dependency built as described in LOCAL_BUILD.md.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

import numpy as np
import tifffile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include <cstdio>
#include "mg_image_lib.h"
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    Stack* s = Read_Stack(argv[1]);
    if (!s) return 3;
    FILE* f = fopen(argv[2], "wb");
    if (!f) return 4;
    size_t n = (size_t)s->width * s->height * s->depth * s->kind;
    size_t written = fwrite(s->array, 1, n, f);
    fclose(f);
    Write_Stack(argv[3], s);
    Kill_Stack(s);
    return written == n ? 0 : 5;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix='vaa3d-tiff-test-') as folder:
        work = Path(folder)
        cpp = work / 'reader.cpp'
        cpp.write_text(HARNESS)
        exe = work / 'reader'
        lib = ROOT / 'v3d_main/common_lib/lib'
        subprocess.run([
            'g++', '-w', '-I' + str(ROOT / 'v3d_main/basic_c_fun'),
            '-I' + str(ROOT / 'v3d_main/common_lib/include'), str(cpp),
            str(ROOT / 'v3d_main/basic_c_fun/mg_image_lib.cpp'),
            str(ROOT / 'v3d_main/basic_c_fun/mg_utilities.cpp'),
            '-L' + str(lib), '-Wl,-rpath,' + str(lib), '-lv3dtiff',
            '-o', str(exe),
        ], check=True)

        def check(path):
            expected = tifffile.imread(path)
            raw, saved = work / 'read.bin', work / 'saved.tif'
            subprocess.run([str(exe), str(path), str(raw), str(saved)], check=True)
            actual = np.fromfile(raw, expected.dtype).reshape(expected.shape)
            np.testing.assert_array_equal(actual, expected)
            np.testing.assert_array_equal(tifffile.imread(saved), expected)
            with tifffile.TiffFile(saved) as tf:
                assert all(p.tags['Orientation'].value == 1 for p in tf.pages)
            subprocess.run([str(exe), str(saved), str(raw), str(work / 'again.tif')], check=True)
            np.testing.assert_array_equal(
                np.fromfile(raw, expected.dtype).reshape(expected.shape), expected)
            print('PASS read + save + reload:', path.name, expected.shape)

        for name, dtype, shape, photo in [
            ('gray8', np.uint8, (3, 7, 11), 'minisblack'),
            ('gray16', np.uint16, (3, 7, 11), 'minisblack'),
            ('rgb8', np.uint8, (3, 7, 11, 3), 'rgb'),
        ]:
            data = (np.arange(np.prod(shape)).reshape(shape) * 17 + 3).astype(dtype)
            path = work / (name + '.tif')
            tifffile.imwrite(path, data, photometric=photo, metadata=None)
            check(path)
        if len(sys.argv) > 1:
            check(Path(sys.argv[1]))


if __name__ == '__main__':
    main()
