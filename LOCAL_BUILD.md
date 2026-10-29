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

## 二维三视图自适应与窗口状态回归

Qt 5 默认让三个切面按统一比例适应窗口。固定倍率仍可从三视图缩放菜单选择；选固定倍率后，再选择“自动适应窗口”恢复自适应。缩放保持原始图像、ROI 与切片位置。

完成常规构建后运行 `make -C v3d_main/v3d -f Makefile -f ../../tests/triview_resize_gui_test.mk triview-resize-gui-test`。在独立 Xvfb 和临时 `XDG_CONFIG_HOME` 下运行 `/tmp/vaa3d-triview-resize-gui`（本机需 `LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6`），输出 `TRIVIEW_RESIZE_PASS` 表示通过。测试验证标题栏点击、还原、大小变化、坐标/ROI/切片保持、手动倍率与自动适应及面板折叠，并在 `/tmp` 保存大小窗口截图。

## 大型 V3DRAW 后台加载与回归

二维打开至少 64 MiB 的 `.v3draw` / `.raw` 时，使用后台读取线程；每 8 MiB 一块，在读取时处理字节序并统计各通道灰度范围。进度窗口支持取消，取消后保留已有图像，不弹出插件读取器重试提示。原始像素保留，仍需容纳整个体积的内存。当前实现支持标准 32 位尺寸和旧版 16 位尺寸文件头，以及 uint8、uint16、float32。

大尺寸单通道 uint8/uint16 切面使用颜色查找表；加载初始化只渲染最终三视图状态，避免反复生成整张切面。浮点和多通道仍采用原切面混色算法；此更新不涵盖其他文件格式、自动转换/翻转、自动开启三维窗口或插件自身的耗时操作。取消会等待当前块的系统读取返回，异常缓慢的存储设备可能延迟取消完成。

读取器单测（无需显示服务器）：

```bash
mkdir -p /tmp/vaa3d-raw-reader-test
cd /tmp/vaa3d-raw-reader-test
/home/pzy/anaconda3/bin/qmake /home/pzy/pzy/Vaa3D/tests/raw_volume_reader_test.pro CONFIG+=debug CONFIG-=release
make
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 ./raw_volume_reader_test
```

完整 Debug 构建后，在仓库根目录运行 `make -C v3d_main/v3d -f Makefile -f ../../tests/raw_volume_gui_test.mk raw-volume-gui-test`，随后在独立 Xvfb、临时 `XDG_CONFIG_HOME`、系统 `LD_PRELOAD` 和 `LIBGL_ALWAYS_SOFTWARE=1` 下运行 `/tmp/vaa3d-raw-volume-gui`。不带参数使用临时 80 MiB 稀疏文件；可传入本次用户的 `P00165-T001-R001-S005-B1.v3draw` 路径测试真实文件（需要约 24 GB 额外可用内存）。该参数模式会检查此文件的已知尺寸和灰度范围。

GUI 测试验证逐像素颜色一致性、取消及数据保留、真实主窗口加载、文件字节抽样比对，并记录墙钟耗时及 20 ms 界面定时器的最大间隔；要求间隔小于 2 秒。输出 `RAW_VOLUME_GUI_PASS` 表示通过，截图保存到 `/tmp/vaa3d-raw-loaded.png`。测试读取原文件但不修改它；缓存状态会明显影响计时，热缓存结果不能代表机械硬盘首次读取速度。

## 多个 3D 窗口的 Object Manager

每个 3D 视窗独立持有一个 Object Manager；标题包含所属视窗名称。原来的 `Attached to 3D view` 开关改为固定归属说明，无需手动绑定。切换视窗、刷新对象列表或关闭一个管理器，不会把另一个管理器指向其他视窗；关闭所属 3D 视窗或重建渲染器时，会先释放其管理器，避免访问旧对象。

完整构建后，在仓库根目录运行 `make -C v3d_main/v3d -f Makefile -f ../../tests/object_manager_windows_gui_test.mk object-manager-windows-gui-test`。在独立 Xvfb、临时 `XDG_CONFIG_HOME`、系统 `LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6` 和 `LIBGL_ALWAYS_SOFTWARE=1` 下运行 `/tmp/vaa3d-object-manager-windows-gui`；输出 `OBJECT_MANAGER_WINDOWS_PASS` 表示通过。

测试使用两个合成图像的真实 3D 窗口，通过界面按钮打开管理器，验证不同对象列表、标题/页签、激活切换、拒绝跨窗口重新关联、神经元/Marker/表面显示开关与 Y 翻转、撤销、列表刷新、管理器隐藏/重开、所属窗口关闭及渲染器重建清理。截图保存在 `/tmp/vaa3d-manager-A.png` 和 `/tmp/vaa3d-manager-B.png`，不使用或修改用户图像/标注。

## TIFF Mask 后台导入与多窗口操作

3D 视窗的“导入 Mask”支持 TIFF 的后台读取、标签统计和表面生成；计算期间进度窗不锁定主窗口或其他视窗，同一视窗一次导入一个 Mask，不同视窗可同时导入。参数集中在一个窗口：表面类型、Marching Cubes/Tetrahedrons、Mesh density、范围、是否替换已有表面及可选 VANO APO 名称/注释。默认追加；选择替换也只在全部生成、上传成功后生效，取消或失败保留原有表面。

显卡显示列表按每批最多 8192 个三角形上传，批次之间处理界面事件。取消、关闭所属视窗或重建渲染器会终止任务并丢弃未发布结果；CPU 大缓冲和三角形链在后台释放。完成后仍是原生 Label Surface，可用所属 Object Manager 控制 on/off、Y flip 和导出。读取保持 TIFF 存储行序，不翻转 Y。

读取器支持标准多页 TIFF/BigTIFF、条带或 tile、压缩数据、unsigned 1/8/16 位、float32，以及连续或平面多通道（导入按钮默认第一通道）。非整数/负值标签须用 Range surface。密度 1000 仍代表每个标签约十亿个网格的计算量；后台化不减少算法总量及整个 Mask/网格所需内存。取消需等待当前磁盘读取或计算小批次返回。此更新针对 TIFF Mask；RAW/当前图像生成表面的旧入口及其他插件操作未改为后台。验证平台为本机 Qt 5、Linux、Xvfb 软件 OpenGL，未覆盖实际 GPU 的所有驱动行为。

验证入口（仓库根目录）：

```bash
mkdir -p /tmp/vaa3d-mask-reader-test
cd /tmp/vaa3d-mask-reader-test
/home/pzy/anaconda3/bin/qmake /home/pzy/pzy/Vaa3D/tests/mask_tiff_volume_test.pro CONFIG+=debug CONFIG-=release
make
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 ./mask_tiff_volume_test
cd /home/pzy/pzy/Vaa3D
g++ -std=c++11 -O2 -pthread tests/marching_parallel_test.cpp v3d_main/3drenderer/marchingcubes.cpp -o /tmp/vaa3d-marching-parallel-test
/tmp/vaa3d-marching-parallel-test
make -C v3d_main/v3d -f Makefile -f ../../tests/mask_async_gui_test.mk mask-async-gui-test
XDG_CONFIG_HOME="$(mktemp -d /tmp/vaa3d-mask-gui-XXXXXX)" LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1600x1200x24' /tmp/vaa3d-mask-async-gui
```

成功标记分别为 `MASK_TIFF_READER_PASS`、`MARCHING_PARALLEL_PASS`、`MASK_ASYNC_GUI_PASS`。读取器检查 44 组像素/通道组合、取消、错误通道和损坏/截断文件；算法回归比较并行与串行结果，并验证取消和异常后可重用。GUI 回归创建两个真实 3D 视窗，验证同时导入、主窗口及另一视窗响应、滚轮缩放、显示开关与 Y flip、计算中和上传中取消、替换、失败保留数据、关闭视窗及渲染器销毁。截图输出 `/tmp/vaa3d-mask-async.png`，无用户图像/标注写入。

## Label Surface 的源路径

Object Manager → Label Surface 的“源路径 / Source path”列显示每个表面所属的源文件绝对路径。多标签 Mask 的各标签分别记录同一源文件；追加多个 Mask 时，已有表面保留各自来源，即使文件名、标签值相同也可区分。列只读，悬浮显示完整路径，默认宽度限制为 220–480 像素；Y flip 继续位于 on/off 后的第二个可见列。

TIFF 后台导入在成功发布表面时写入来源；取消或失败不改动已有表面的来源。RAW/V3DRAW、从有源文件的当前图像生成表面，以及 OBJ/V3DS/VAA3DS 导入也记录对应输入文件。没有可确定源文件的表面显示“未知来源 / Unknown”，不把最后一次导入路径套用到全部对象。

路径是本次加载的显示元数据，现有 OBJ/V3DS 保存格式不增加字段：重新导入导出的网格时显示该网格文件路径，无法还原其生成前的 Mask 路径。旧进程不能热更新，使用新版并重新导入 Mask 后生效。

沿用上面的 `mask-async-gui-test` 验证入口。回归增加中文/空格目录下的同名 Mask、相对路径转绝对路径、多标签/多窗口来源、追加/替换、取消/失败保留、撤销与刷新、只读单元格、完整路径工具提示及 Y flip 列位置检查；界面截图输出 `/tmp/vaa3d-mask-source-paths.png`。

## Volume Cut / Surface Cut 裁切框平移

两个裁切页的滑条下方均增加六方向按钮与各自独立的“平移步长”。先调好裁切框大小，再点击左/右（X-/X+）、上/下（Y-/Y+）、前/后（Z-/Z+）平移整个范围。方向固定对应数据坐标轴，不跟随相机旋转。每次点击只走一步，长按不连续移动；接近边界时只走剩余距离，到边界停止，始终保留 min/max 间距。完整范围占满该轴时不能平移，需先缩小；单层范围也可平移。

- Volume Cut：步长为图像体素数，默认 5；只移动 X/Y/Z 裁切区间，Front 保持原值。现有三个区间锁定按钮保持原状态；已启用的 Z-Lock with Volume 继续按移动后的完整范围联动。
- Surface Cut：步长为该轴完整表面范围的百分比，默认 5%，范围 1–100%。底层沿用现有 0–200 裁切滑条（1% 对应 2 个刻度）。不重新生成网格，不移动对象坐标。
- 两个裁切页的步长及每个 3D 视窗独立；不会改变相机的旋转、缩放或 Shift。X-section 模式保留原切片控件，不增加这组 Volume Cut 按钮。

验证入口（仓库根目录，完成普通 Debug 构建后）：

```bash
make -C v3d_main/v3d -f Makefile -f ../../tests/cut_move_gui_test.mk cut-move-gui-test
XDG_CONFIG_HOME="$(mktemp -d /tmp/vaa3d-cut-move-config-XXXXXX)" LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1600x1200x24' /tmp/vaa3d-cut-move-gui
```

成功标记为 `CUT_MOVE_GUI_PASS`。真实双 3D 窗口验证两个裁切页的六方向、步长与单位、宽度保持、两端边界、步长大于框宽、完整范围、单层范围、Volume 三轴锁定、长按不连续移动、滑条/渲染器同步、Volume 渲染像素改变、Z-Lock 读取最终范围及另一窗口/相机/Front 保持。界面截图为 `/tmp/vaa3d-volume-cut-move.png` 和 `/tmp/vaa3d-surface-cut-move.png`。

## 三维坐标轴外侧 X / Y / Z 标签

Qt 5 的 3D 视窗沿三条彩色坐标轴外侧显示 X（红）、Y（绿）、Z（蓝），字母带黑白描边，适应深色和浅色背景。使用 Others → Axes 开关统一控制坐标轴与标签。

标签按当前相机、缩放、Shift 和窗口尺寸重新投影，在每条轴末端向外寻找位置；整个标签框与图像八角点投影形成的包围矩形保持间隙，并避免标签互相重叠或超出视口。极端放大、平移、近裁切或窗口空间不足时，无法安全放置的标签隐藏；缩小或调整视角后可恢复。优先保留图像显示区域，不把文字强行夹到图像内部。当前只补齐 Qt 5 的坐标轴标签，Qt 4 继续沿用原绘制路径。

验证入口（普通 Debug 构建后）：

```bash
make -C v3d_main/v3d -f Makefile -f ../../tests/axis_labels_gui_test.mk axis-labels-gui-test
XDG_CONFIG_HOME="$(mktemp -d /tmp/vaa3d-axis-config-XXXXXX)" LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1600x1200x24' /tmp/vaa3d-axis-labels-gui
```

成功标记为 `AXIS_LABELS_GUI_PASS`。回归检查真实三维视窗中的 RGB 字母像素、深/浅背景、投影图像避让、旋转、缩放、Shift、正交投影、Axes 开关，以及标签碰撞、视口占满、退化投影方向和 2 倍布局尺度。截图输出 `/tmp/vaa3d-axis-labels-default.png`、`/tmp/vaa3d-axis-labels-light.png`、`/tmp/vaa3d-axis-labels-rotated.png`、`/tmp/vaa3d-axis-labels-ortho.png`、`/tmp/vaa3d-axis-labels-zoom.png`。

## 三维图像局部暂时隐藏

3D 图像视窗顶部、导入按钮下方新增“局部隐藏”、半径（默认 8 体素，1–4096）、“撤销隐藏”“恢复全部”和隐藏框计数。

1. 勾选“局部隐藏”，鼠标移动到灰色浮块。沿当前视线的最亮信号估计深度，显示黄色三维框；切片模式使用已开启切片与视线的交点。可调半径控制 X/Y/Z 方向的框大小，前后信号重叠时可先旋转或用 Volume Cut 裁切，再悬停预览。
2. 黄色框出现后按 Delete，只隐藏框内原始图像的三维区域。按住不连走；鼠标刚移到新位置、预览未完成或移出图像视窗时不执行隐藏。框选不是自动分割整个连通块，也不是沿视线整列擦除；应确认黄色框大小。
3. Ctrl+Z（模式内、图像区域获得焦点时）或“撤销隐藏”恢复最后一框；“恢复全部”清除当前视窗所有局部隐藏框。Esc 或取消勾选退出悬停模式，已隐藏区域保持，仍可用按钮恢复。

只改变当前 3D 视窗绘制，不修改图像体素、RGBA 纹理缓冲、磁盘文件、独立的二维与 MIP 投影窗口、SWC 或 Mask 表面。排除框使用三维归一化位置并关联时间点，旋转后仍隐藏同一三维区域；关闭视窗、重建渲染器或重新加载图像时清空。源码数据和分析算法输入仍保留原内容；不新增隐藏区域持久化格式。

实现是在体绘制切片几何中扣除隐藏框并插值纹理坐标，无需改写或重新上传图像纹理；覆盖 2D/3D 纹理、着色器/固定管线及 F-slice。隐藏框很多时，CPU 几何裁切开销会增加；悬停采用 40 ms 合并事件，沿视线最多采样 4097 个现有显示缓冲点。定位受显示缓冲分辨率、当前通道和前后信号重叠影响，不保证自动识别生物学连通体。

验证入口（普通 Debug 构建后）：

```bash
make -C v3d_main/v3d -f Makefile -f ../../tests/local_volume_hide_gui_test.mk local-volume-hide-gui-test
XDG_CONFIG_HOME="$(mktemp -d /tmp/vaa3d-local-hide-config-XXXXXX)" LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1600x1200x24' /tmp/vaa3d-local-volume-hide-gui
```

成功标记为 `LOCAL_VOLUME_HIDE_GUI_PASS`。测试两个真实视窗中的独立体块，检查黄色框、实际像素隐藏与相邻块保留、原始/RGBA SHA256 不变、2D/3D 纹理及着色器/固定管线四路径、Alpha、Z 切片、F-slice、旋转及同屏不同深度的信号保留、撤销/恢复、关闭模式、Delete 自动重复/预览等待保护、离开视窗/Esc 和渲染器重建；几何回归检查面积守恒、80 个重叠框无碎片膨胀和时间点隔离。截图：`/tmp/vaa3d-local-hide-preview.png`、`/tmp/vaa3d-local-hide-deleted.png`。

## 最近目录只记录最终选择

文件选择器的目录下拉框只在用户确认打开文件或选择文件夹时更新：文件记其所在目录，文件夹记自身；浏览、转到、返回上级、刷新和取消不写入历史。保留最近 20 个最终目录并去重，文件名下拉框仍保留最近文件。

目录历史改用 `HHMI/Vaa3D` 设置中的 `FastFileDialog/recentOpenedDirectories`。首次使用从 `lastDirectory` 和 `recentFiles` 的所在目录重建，避免继承旧浏览路径；旧 `recentDirectories` 键原样保留，可供旧版本或恢复使用。旧记录无法区分“仅浏览”和“确认选中文件夹”，所以更早且无法从最近文件推断的文件夹需重新选择。删除新键可在下次打开文件选择器时按上述规则重建；无需删除整个用户配置。

## Object Manager 的 SWC 手动颜色

SWC 对象的 Alpha 值 0/1/2 在旧格式中表示按类型/分段/多神经元配色，并不表示透明度。双击 Object Manager 的 SWC 颜色格，或使用 `Color >> Single Color...` 后，会切换选中对象为固定 RGB 颜色；固定颜色优先于全局的祖先、审阅、置信度等节点配色。编辑工具的节点高亮继续生效。需要自动配色时，使用 `Neuron Segment Colorful` / `Multi-neuron Colorful` 菜单；取消颜色对话框不修改对象。此调整不改变 SWC 节点的类型、坐标及原文件。

完整构建后运行 `make -C v3d_main/v3d -f Makefile -f ../../tests/swc_color_gui_test.mk swc-color-gui-test`，在独立 Xvfb、临时 `XDG_CONFIG_HOME`、`LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6`、`LIBGL_ALWAYS_SOFTWARE=1` 下运行 `/tmp/vaa3d-swc-color-gui`。测试导入两个临时 SWC，经真实 QColorDialog 改色，在关闭坐标轴及边框后检查线条/管状渲染的实际颜色像素，并验证各对象独立性、全局配色优先级、取消、自动配色标签和批量单色操作。成功标记 `SWC_COLOR_GUI_PASS`，截图 `/tmp/vaa3d-swc-custom-colors.png`。验证平台为本机 Linux Qt 5 + 软件 OpenGL，未覆盖其他平台的原生颜色选择器。
