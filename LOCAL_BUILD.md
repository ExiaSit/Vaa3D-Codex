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

## 网页通过 vaa3d 协议打开本机文件（Linux）

当前维护版提供 `vaa3d://open?path=<URL 编码后的绝对路径>`。使用 Python 3 标准库校验参数，再调用现有 `open-vaa3d.sh`，以 `-i` 加载文件；每次打开新实例，图像遵循原有二维/自动三维偏好，SWC/ESWC 使用三维对象浏览器。现有实例无需退出，未保存标注不受影响。

仅注册协议，不改变当前 TIFF 文件关联：

```bash
python3 scripts/install-desktop-integration.py --protocol-only
xdg-mime query default x-scheme-handler/vaa3d
# 预期：vaa3d-protocol.desktop
xdg-open 'vaa3d://open?path=%2Fhome%2Fpzy%2F100310.swc'
```

安装到当前用户的 `$XDG_DATA_HOME/applications/vaa3d-protocol.desktop`（默认 `~/.local/share/applications`），依赖 Python 3、xdg-utils、desktop-file-utils、GIO 及已构建的本机 Vaa3D；错误提示在有桌面和 Zenity 时显示对话框，同时写入标准错误。不需要 sudo。原来的安装命令不带参数时同时注册 TIFF 和协议；移动仓库后重新运行安装脚本。

网页按钮参考 [examples/open-in-vaa3d.html](examples/open-in-vaa3d.html)，核心代码：

```javascript
button.addEventListener('click', () => {
  window.location.href = 'vaa3d://open?path=' + encodeURIComponent(absolutePath);
});
```

`absolutePath` 是**运行浏览器的电脑上**可读文件的绝对路径，例如 `/data/disk/example.tif`；共享目录须先在该电脑挂载到对应路径，服务器独有路径不能直接访问。浏览器通常会先询问是否允许打开 Vaa3D，用户允许后启动，无需网页读取文件内容或运行本地服务。本协议不下载 HTTP 文件、不执行插件或脚本，也不接受相对路径或未知参数；单文件模式的 `path` 只能出现一次。仅接受 SWC、ESWC、TIF、TIFF、V3DPBD（扩展名不区分大小写）。文件格式内容仍由 Vaa3D 加载器检查。路径必须整体用 `encodeURIComponent` 编码一次，空格、中文、`+`、`#`、`&`、`%` 均可保留。

### 一次打开原图、多个神经元和多个 Mask

使用同一个协议地址，改传一个 `image`，以及可重复的 `swc` / `mask` 参数；不能与单文件 `path` 混用。原图支持 TIFF/V3DPBD，神经元支持 SWC/ESWC，Mask 支持 TIFF 标签图（0 为背景，通道 0）。`mesh_density` 可选，范围 1–1000，默认 100。网页示例已提供整组表单：

```javascript
const p = new URLSearchParams();
p.set('image', '/data/43199.tif');
for (const path of ['/data/43199_a.swc', '/data/43199_b.eswc']) p.append('swc', path);
for (const path of ['/data/43199_mask_a.tif', '/data/43199_mask_b.tif']) p.append('mask', path);
p.set('mesh_density', '100');
window.location.href = 'vaa3d://open?' + p.toString();
```

一组只打开一个新 Vaa3D 实例、一个原图及其三维视窗，按原图 → 各 SWC → 各 Mask 的顺序导入；每个 Mask 在后台生成 Marching Cubes 标签表面并追加，不替换前一个。无需逐个确认 Mesh/VANO 选项，不读取 VANO 注释。完成后打开该视窗的 Object Manager：神经元在 Neuron/line Structure，Mask 的每个非零标签在 Label Surface，名称包含 Mask 文件名及标签值，源路径列保留完整来源，各行的显隐、颜色、Y flip 可分别控制。原图通过 Volume 控件管理。各窗口的队列和对象独立，手动导入仍保留原选项对话框。

网页按所选 ID 显式提供这一组路径；程序不根据文件名猜测关联，不变换/重采样坐标，SWC/Mask 应与原图处于相同坐标系，Mask 三维尺寸必须相同。所有路径/角色先校验，缺失、重复（包含符号链接别名）或错误类型会在启动前拒绝；同一个文件不能兼作原图和 Mask。解析/加载内容失败、尺寸不符或取消某个 Mask 后停止余下队列并提示，之前加载成功的内容保留，文件组不会假报成功；关闭/重建目标三维视窗时取消队列。此模式只负责当前会话加载，不生成持久化工程文件。

命令行也可调用 `scripts/open-vaa3d.sh --open-set --image /data/43199.tif --swc /data/43199_a.swc --mask /data/43199_mask.tif --mesh-density 100`，角色参数可重复（原图与密度除外）；路径必须是绝对路径，含空格时按 shell 规则加引号。协议注册记录的脚本位置不变，已注册电脑完成构建即可使用新模式；无需关闭已有实例。大量路径需注意浏览器/操作系统对 URL 长度的限制。

回归：`python3 tests/vaa3d_protocol_test.py`，测试在临时 XDG 目录中注册协议，以真实 `gio open` 调用模拟程序，验证特殊字符路径、Desktop Exec 引号、参数完整传递、非法链接拒绝及 TIFF 关联保持；不修改日常桌面配置。

整组 GUI 回归：常规构建后，执行 `make -C v3d_main/v3d -f Makefile -f ../../tests/file_set_gui_test.mk file-set-gui-test`，在临时 `XDG_CONFIG_HOME`、Xvfb、`LIBGL_ALWAYS_SOFTWARE=1`、`LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6` 下运行 `/tmp/vaa3d-file-set-gui`。测试使用合成数据，不修改用户数据。

卸载协议：删除上述 `vaa3d-protocol.desktop`，在 `$XDG_CONFIG_HOME/mimeapps.list`（默认 `~/.config/mimeapps.list`）中仅移除值为 `vaa3d-protocol.desktop` 的 `x-scheme-handler/vaa3d` 项，保留其他配置，然后对相应 applications 目录运行 `update-desktop-database`。如安装前已有其他处理程序，请先记录 `xdg-mime query default x-scheme-handler/vaa3d`，卸载时用 `xdg-mime default 原处理程序.desktop x-scheme-handler/vaa3d` 恢复。此功能只配置 Linux 桌面；Windows/macOS 尚无注册脚本。

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

## 最近目录只记录最终选择

文件选择器的目录下拉框只在用户确认打开文件或选择文件夹时更新：文件记其所在目录，文件夹记自身；浏览、转到、返回上级、刷新和取消不写入历史。保留最近 20 个最终目录并去重，文件名下拉框仍保留最近文件。

目录历史改用 `HHMI/Vaa3D` 设置中的 `FastFileDialog/recentOpenedDirectories`。首次使用从 `lastDirectory` 和 `recentFiles` 的所在目录重建，避免继承旧浏览路径；旧 `recentDirectories` 键原样保留，可供旧版本或恢复使用。旧记录无法区分“仅浏览”和“确认选中文件夹”，所以更早且无法从最近文件推断的文件夹需重新选择。删除新键可在下次打开文件选择器时按上述规则重建；无需删除整个用户配置。

## Object Manager 的 SWC 手动颜色

SWC 对象的 Alpha 值 0/1/2 在旧格式中表示按类型/分段/多神经元配色，并不表示透明度。双击 Object Manager 的 SWC 颜色格，或使用 `Color >> Single Color...` 后，会切换选中对象为固定 RGB 颜色；固定颜色优先于全局的祖先、审阅、置信度等节点配色。编辑工具的节点高亮继续生效。需要自动配色时，使用 `Neuron Segment Colorful` / `Multi-neuron Colorful` 菜单；取消颜色对话框不修改对象。此调整不改变 SWC 节点的类型、坐标及原文件。

完整构建后运行 `make -C v3d_main/v3d -f Makefile -f ../../tests/swc_color_gui_test.mk swc-color-gui-test`，在独立 Xvfb、临时 `XDG_CONFIG_HOME`、`LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6`、`LIBGL_ALWAYS_SOFTWARE=1` 下运行 `/tmp/vaa3d-swc-color-gui`。测试导入两个临时 SWC，经真实 QColorDialog 改色，在关闭坐标轴及边框后检查线条/管状渲染的实际颜色像素，并验证各对象独立性、全局配色优先级、取消、自动配色标签和批量单色操作。成功标记 `SWC_COLOR_GUI_PASS`，截图 `/tmp/vaa3d-swc-custom-colors.png`。验证平台为本机 Linux Qt 5 + 软件 OpenGL，未覆盖其他平台的原生颜色选择器。

## 3D 浏览器逐通道颜色

点击 3D 窗口的“通道颜色 / Colormap”按钮。在每个 Channel 上方点击“颜色 / Color…”选择显示颜色，“显示 / Show”独立控制该通道显隐；更改立即生效，仅作用于所属 3D 视窗。原有 RGBA 强度曲线仍可编辑，对比度调节保留通道颜色。单通道默认白色，多通道默认红、绿、蓝、白，控件仅显示实际存在的通道。

当前支持前四个通道；Channel 1–4 按原图顺序排列，不自动读取 OME 通道名称或染料配色。`P209-T1-R1-S19-4color.tif` 的原文件顺序为 DAPI、FITC、TexasRed、Cy5，可自行选择蓝、绿、红、洋红。颜色与显隐可通过 Save 保存为 `.vaa3dcm`，通过 Load 重用；兼容旧三通道曲线文件，缺少的通道保持现有曲线。Undo 恢复首次打开此窗口颜色面板时的状态。关闭/重建视窗后未保存的颜色设置不保留。

第四通道现在作为独立标量参与 MIP、Alpha 和切面渲染，不再只映射前三个通道。此功能使用 GLSL 体渲染；四通道完整 3D 纹理固定采用 RGBA8，避免本机软件驱动在 S3TC 压缩 3D 纹理中错误采样 alpha，显存用量可能高于压缩纹理。保留既有显示下采样及 8 位纹理映射，不改变原图位深、像素、二维通道颜色、SWC 或 Mask 颜色。旧进程不能热更新，需打开新版实例。

完整构建后运行：

```bash
make -C v3d_main/v3d -f Makefile -f ../../tests/volume_channel_colors_gui_test.mk volume-channel-colors-gui-test
XDG_CONFIG_HOME="$(mktemp -d /tmp/vaa3d-channel-colors-XXXXXX)" LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1800x1200x24' /tmp/vaa3d-volume-channel-colors-gui
```

可附加真实四通道 uint16 TIFF 路径，使用同一界面验证其通道。测试检查真实选色对话框、各通道 framebuffer 颜色/显隐、取消、对比度、二维/三维纹理、压缩开关、PBO streaming（合成数据）、MIP/Alpha/切面、颜色文件保存/载入与旧文件/无效索引、图像内存哈希不变；合成模式还检查单通道默认白色、Undo、两视窗隔离及渲染器销毁时面板清理。成功标记 `VOLUME_CHANNEL_COLORS_GUI_PASS`，截图位于 `/tmp/vaa3d-channel-controls*.png` 和 `/tmp/vaa3d-channel-volume*.png`，不覆盖源图像。
