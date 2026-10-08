#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
qmake_bin="${QMAKE_BIN:-/home/pzy/anaconda3/bin/qmake}"
jobs="${JOBS:-12}"
gl_root="$root_dir/.local-deps/sysroot/usr"

if [[ ! -x "$qmake_bin" ]]; then
    echo "找不到 qmake: $qmake_bin" >&2
    exit 1
fi
if [[ ! -f "$root_dir/v3d_main/common_lib/lib/libv3dtiff.so" || ! -f "$root_dir/v3d_main/common_lib/src_packages/mylib_tiff/libmylib.a" ]]; then
    echo "缺少首次编译所需的 TIFF/MYLIB 库；请参阅 LOCAL_BUILD.md。" >&2
    exit 1
fi

make -C "$root_dir/v3d_main/jba/c++" -f jba.makefile -j "$jobs"

cd "$root_dir/v3d_main/v3d"
"$qmake_bin" vaa3d.pro \
    'CONFIG+=debug' 'CONFIG-=release' \
    'DEFINES+=USE_Qt5' 'QMAKE_CXXFLAGS+=-std=gnu++11' \
    'QMAKE_LFLAGS+=-no-pie' \
    "INCLUDEPATH+=$gl_root/include" \
    "LIBS+=-L$gl_root/lib/x86_64-linux-gnu" \
    "QMAKE_RPATHDIR+=$gl_root/lib/x86_64-linux-gnu"
make -j "$jobs"

mkdir -p "$root_dir/bin"
ln -sfn ../v3d_main/v3d/vaa3d "$root_dir/bin/vaa3d"
echo "Debug 构建完成: $root_dir/bin/vaa3d"
