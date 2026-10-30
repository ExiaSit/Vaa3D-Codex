#!/usr/bin/env bash
# Local Qt5 development build; does not install or replace another app.
set -euo pipefail
root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
[[ "$(uname -s)" == Darwin ]] || { echo 'This entry point requires macOS.' >&2; exit 1; }
qt5_prefix="${MAC_QT5_PREFIX:-$(brew --prefix qt@5)}"
qmake_bin="$qt5_prefix/bin/qmake"
[[ "$("$qmake_bin" -query QT_VERSION)" == 5.* ]] || { echo 'Qt 5 is required.' >&2; exit 1; }
: "${MAC_QT5_NATIVE_LIB_DIR:?Set to the directory containing native libv3dnewmat, libmylib, libv3dtiff, libteem, libhdf5 and libszip}"
: "${MAC_QT5_BOOST_INCLUDE_DIR:?Set to the parent directory of boost/config.hpp}"
[[ -f "$MAC_QT5_BOOST_INCLUDE_DIR/boost/config.hpp" ]] || { echo 'Missing Boost headers.' >&2; exit 1; }
[[ -f "$root_dir/v3d_main/terafly/src/terarepo/src/iomanager/iomanager.config.h" ]] || { echo 'Run git submodule update --init first.' >&2; exit 1; }
mac_brew_prefix="$(brew --prefix)"
build_dir="${MAC_QT5_BUILD_DIR:-$root_dir/build/macos-qt5}"
mkdir -p "$build_dir"
cd "$build_dir"
"$qmake_bin" "$root_dir/v3d_main/v3d/vaa3d.pro" \
    'QT+=widgets concurrent' 'CONFIG+=c++11' 'DEFINES+=USE_Qt5' \
    "INCLUDEPATH+=$mac_brew_prefix/include $mac_brew_prefix/opt/libtiff/include $mac_brew_prefix/opt/hdf5/include $MAC_QT5_BOOST_INCLUDE_DIR" \
    "QMAKE_LIBDIR+=$MAC_QT5_NATIVE_LIB_DIR" \
    'TARGET=Vaa3D-Qt5-probe' -o Makefile.qt5
make -f Makefile.qt5 -j "${JOBS:-4}"
echo "Built: $build_dir/Vaa3D-Qt5-probe.app"
