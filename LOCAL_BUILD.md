# 本机 Debug 构建

源码：`https://github.com/Vaa3D/v3d_external.git`，`master` 分支，提交 `215c7803a3aeb7c7a5772f3db2380ee6737d4edc`。TeraStitcher 子模块已初始化。

本次使用 Anaconda 的 Qt 5.15.2 和 GNU C++11；编译模式为 `CONFIG+=debug CONFIG-=release`。本机缺少的 OpenGL/X11 开发包提取在 `.local-deps/sysroot`，没有安装到系统目录。首次编译所需的 Boost、TIFF、MYLIB 和 newmat 库已经在源码树内准备好。

以后修改 C++ 源文件后，在项目根目录执行：

```bash
./build-debug-local.sh
```

可执行文件入口为 `bin/vaa3d`，实际构建产物为 `v3d_main/v3d/vaa3d`。可用 `JOBS=4 ./build-debug-local.sh` 调整并行度，或用 `QMAKE_BIN=/path/to/qmake` 指定另一套兼容的 Qt 5。

首次准备步骤记录：

1. `git submodule update --init --depth 1`
2. 从仓库的 `v3d_main/common_lib/src_packages/boost_1_57_0.tar.gz` 提取 `boost` 到 `v3d_main/common_lib/include/`。
3. 编译 `v3d_main/jba/c++/jba.makefile` 和 `v3d_main/common_lib/src_packages/mylib_tiff/mylib_linux.makefile`。
4. 从仓库的 `tiff-4.0.2.tar.gz` 编译并安装到 `v3d_main/common_lib/`，建立 `libv3dtiff.so` 软链接。
5. 将 Ubuntu 22.04 的 OpenGL、GLU、freeglut 和 X11 开发包用 `dpkg-deb -x` 提取到 `.local-deps/sysroot`。

检查 Debug 符号：`readelf -S bin/vaa3d | grep .debug_info`。检查命令行入口：`QT_QPA_PLATFORM=offscreen bin/vaa3d --help`。

## 本机渲染设置

`config/rendering-rtx5090.ini` 保存 RTX 5090 上验证过的三个渲染选项。用户实际配置位于 `~/.config/HHMI/Vaa3D.conf` 的 `[General]` 段；恢复时只合并这三个键，保留其余偏好。此配置片段不能替代完整配置文件。设置在重新启动 Vaa3D 后生效。

## Object Manager 的 Y 翻转显示

Object Manager 六类对象表的第二列为 `Y flip`，紧跟 `on/off`，每行可独立切换。适用于 Mask/OBJ 表面、Marker、SWC/ESWC 神经元、神经元片段、APO 点及 APO 点集。点集开关会统一设置其成员；树与片段同时翻转时，两次翻转相互抵消。

有图像时以当前图像数据范围的 Y 中心为镜面；没有图像时使用场景表面包围盒的 Y 中心。只变换三维显示，原始图像、对象坐标和导出文件不变。此状态不写入 SWC/ESWC/APO/OBJ 文件，重新导入需重新勾选。当前不映射几何编辑工具的坐标，进行坐标编辑或描线前应取消翻转；二维 MIP 不使用此三维显示开关。

验证入口：

```bash
mkdir -p /tmp/vaa3d-yflip-test
cd /tmp/vaa3d-yflip-test
/home/pzy/anaconda3/bin/qmake /home/pzy/pzy/Vaa3D/tests/object_yflip_test.pro
make
./object_yflip_test
cd /home/pzy/pzy/Vaa3D
make -C v3d_main/v3d -f Makefile -f ../../tests/object_yflip_gui_test.mk yflip-gui-test
# 在独立 Xvfb 显示和临时 XDG_CONFIG_HOME 下运行，避免改变日常设置：
XDG_CONFIG_HOME="$(mktemp -d /tmp/yflip-config-XXXXXX)" LIBGL_ALWAYS_SOFTWARE=1 LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 xvfb-run -a /tmp/vaa3d-object-yflip-gui
```

## 系统和 Firefox 使用 Vaa3D 打开 TIFF

运行 `python3 scripts/install-desktop-integration.py`，为当前用户安装 `~/.local/share/applications/vaa3d-local.desktop`，并将 `image/tiff`、`image/x-tiff` 默认程序设为 Vaa3D。安装脚本遵循 `XDG_DATA_HOME`；不需要 sudo。桌面入口通过 `scripts/open-vaa3d.sh` 将完整文件路径作为 `-i` 参数传入，支持多个文件和包含中文、空格的路径。需要 Python 3、xdg-utils、desktop-file-utils，以及已完成的本地构建；移动仓库后重新运行安装脚本。

在 Firefox 的 TIFF 弹窗中选择“打开，通过 → System Handler（默认）”；可勾选“以后自动采用相同的动作”。如果 Firefox 已记住“保存文件”，在“设置 → 常规 → 应用程序 → TIFF 图像”中改为“使用系统默认应用程序”。已经打开的弹窗若仍显示旧默认程序，取消后重新点击文件链接。浏览器可能先下载临时副本再交给 Vaa3D，这属于正常行为。本配置没有修改 Firefox 配置，也不关闭现有 Vaa3D 窗口；每次调用打开一个新程序实例。

安装后验证：`xdg-mime query default image/tiff` 应返回 `vaa3d-local.desktop`；`gio mime image/tiff` 应显示 Vaa3D 为默认程序。

恢复此次安装前的 TIFF 默认程序：运行 `xdg-mime default org.gnome.Evince.desktop image/tiff`；对于新增的 `image/x-tiff` 默认项，从 `~/.config/mimeapps.list` 的 `[Default Applications]` 中只移除 `image/x-tiff=vaa3d-local.desktop`（若尚未被用户修改）。然后删除 `~/.local/share/applications/vaa3d-local.desktop` 并运行 `update-desktop-database ~/.local/share/applications`；保留其余 MIME 设置。若采用自定义 XDG 路径，请使用相应路径。

## 三维滚轮缩放回归

完成常规构建后，运行 `make -C v3d_main/v3d -f Makefile -f ../../tests/wheel_zoom_gui_test.mk wheel-zoom-gui-test`。随后在独立 Xvfb、临时 `XDG_CONFIG_HOME` 和系统 `LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6` 下运行 `/tmp/vaa3d-wheel-zoom-gui`；输出 `WHEEL_ZOOM_GUI_PASS` 表示通过。测试使用合成 32³ 图像，检查微小滚轮增量累积、像素滚动、焦点、修饰键、缩放边界及滑条反馈。TeraFly 的跨分辨率切换不在此回归覆盖范围。

桌面图标与程序窗口使用同一 Vaa3D logo；程序嵌入 256×256 图标，主窗口和三维窗口自动继承。Linux 桌面入口的 `StartupWMClass=vaa3d` 与窗口类型匹配，Qt desktop file name 为 `vaa3d-local`，用于任务栏识别。更新图标设置后运行桌面安装脚本并启动新版即可生效。
