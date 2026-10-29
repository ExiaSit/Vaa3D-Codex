"""Generate a full-size TIFF fixture or independently verify GUI rescale output."""
import argparse
import math
from pathlib import Path
import numpy as np
import tifffile

parser = argparse.ArgumentParser()
parser.add_argument('action', choices=['generate', 'verify'])
parser.add_argument('path', type=Path)
args = parser.parse_args()
if args.action == 'generate':
    if args.path.exists():
        parser.error('Refusing to overwrite an existing fixture')
    x = np.arange(1024, dtype=np.uint32)[None, :, None]
    y = np.arange(1024, dtype=np.uint32)[:, None, None]
    c = np.arange(4, dtype=np.uint32)[None, None, :]
    metadata = ('<OME><Image><Pixels SizeX="1024" SizeY="1024" SizeZ="28" '
                'SizeC="4" SizeT="1" PhysicalSizeX="0.564542" '
                'PhysicalSizeY="0.564542" PhysicalSizeZ="1.444444" /></Image></OME>')
    with tifffile.TiffWriter(args.path) as writer:
        for z in range(28):
            plane = ((x + 10*y + 100*z + 1000*c) % 4096).astype(np.uint16)
            writer.write(plane, photometric='minisblack', planarconfig='contig',
                         metadata=None, description=metadata if z == 0 else None)
    print(args.path)
else:
    assert args.path.stat().st_size == 4*40*578*578*2
    output = np.memmap(args.path, mode='r', dtype=np.uint16, shape=(4, 40, 578, 578))
    def weights(i, n, spacing):
        if spacing >= 1:
            p = np.clip((i + .5)/spacing - .5, 0, n-1)
            lo = int(np.floor(p))
            return [(lo, 1-(p-lo)), (min(lo+1, n-1), p-lo)]
        left, right = i/spacing, (i+1)/spacing
        result = []
        for j in range(min(n-1, int(left)), min(n-1, int(np.ceil(right))-1)+1):
            a, b = max(left, j), min(right, j+1)
            if j == n-1:
                b = right
            if b > a:
                result.append((j, (b-a)/(right-left)))
        return result
    rng = np.random.default_rng(42)
    samples = [(c, z, y, x) for c in range(4) for z, y, x in
               [(0, 0, 0), (39, 577, 577), (10, 345, 200), (20, 100, 500)]]
    samples += [(int(rng.integers(4)), int(rng.integers(40)),
                 int(rng.integers(578)), int(rng.integers(578))) for _ in range(1000)]
    maximum_error = 0
    for c, z, y, x in samples:
        expected = sum(((ix+10*iy+100*iz+1000*c) % 4096)*wx*wy*wz
                       for ix, wx in weights(x, 1024, .564542)
                       for iy, wy in weights(y, 1024, .564542)
                       for iz, wz in weights(z, 28, 1.444444))
        error = abs(int(output[c, z, y, x]) - math.floor(expected+.5))
        maximum_error = max(maximum_error, error)
        assert error <= 1, (c, z, y, x, expected, output[c, z, y, x])
    print('RESCALE_INDEPENDENT_REFERENCE_PASS:', len(samples), 'samples; max error', maximum_error)
