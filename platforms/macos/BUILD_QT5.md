# Mac Qt5 整合构建试验

时间：2026-10-30 16:18:25 +0800。源码基于 local-053，加入本次三处 Apple OpenGL 头文件路径修正；版本 local-054。

## 结论

Qt5 路线可行：共享源码在本机已完整编译、链接并启动，生成 arm64 应用，合成 TIFF/SWC 三维叠加和鼠标旋转正常。它保留了 Ubuntu 新增代码，未使用旧 Qt6 目录覆盖共享源码。但不是全部新增功能、插件和平台的完整验收。

## 环境与变更

- macOS 26.6.2，Xcode 27，Homebrew Qt 5.15.19，arm64。
- Qt5 已安装在 /opt/homebrew/Cellar/qt@5/5.15.19，通过 /opt/homebrew/opt/qt@5/bin/qmake 调用；默认 /opt/homebrew/bin/qmake 和 qmake6 仍输出 Qt 6.11.2。
- 普通 Homebrew 安装因 Qt6 冲突停止。随后 --force 安装成功解包 Qt5，但全局链接冲突返回失败；没有执行覆盖链接。再执行 brew unlink qt@5 返回 0 个链接被移除。直接调用 Qt5 独立路径成功。期间 brotli 从 1.2.0 升为 1.2.1；其余列出的依赖未据此声明升级。
- 初始化仓库要求的 TeraStitcher 子模块 1ef5a44bd9a74be86042da2265817ddf217e702f。
- glsl_r.h、CustomClipPlanes.h、CameraTransformGL.cpp 用 __APPLE__ 条件选 OpenGL/gl.h，其余平台仍用 GL/gl.h。
- 新增 scripts/build-macos-qt5.sh，显式 Qt5/Widgets/Concurrent、USE_Qt5、C++11 和依赖目录，构建到独立位置，不安装、不替换旧应用。
- 本次复用旧 Mac 构建已生成的 arm64 newmat/mylib 等库，以及 Boost 头文件。不是仅靠 Qt5 包就能从全新机器复现；依赖须先准备。

## 实测

1. 独立接口程序包含 QFileDialog、QColorDialog::getRgba、QGLWidget：Qt5 编译并以 offscreen 启动成功，退出 0。
2. 完整应用 make 退出 0（440 个目标文件）；file 确认为 Mach-O arm64，otool 确认 Qt 框架链接到 qt@5。
3. 构建脚本在同一目录重新生成并增量构建，退出 0。
4. 原生 GUI 启动显示 Ready；打开 64×64×16 单通道 TIFF + 三节点 SWC 的 ANO，看到体数据和 SWC，拖拽后旋转角从 15/340/358 变为 42/22/336。新增导入 Mask、坐标轴和 Rescale 控件可见；控件可见不等于其全部流程已测。
5. swc_navigation GUI 回归退出 0，SWC_NAVIGATION_GUI_PASS。
6. object_yflip GUI 回归退出 0，Y_FLIP_GUI_PASS。
7. swc_color 默认原生弹窗回归超时（退出 -6），不能计为通过。仅在测试注入头文件中启用 AA_DontUseNativeDialogs 后重跑退出 0，SWC_COLOR_GUI_PASS，覆盖像素/管理器一致、批量颜色、独立对象及取消；产品仍保留原生弹窗。
8. 主应用 Object Manager 及原生颜色窗口能够打开/关闭，但手动输入绿色后截图未确认变绿，因此原生颜色交互不记为验收通过。未将其归因为已确定的产品逻辑错误。

GUI 测试使用现有 tests/*.cpp 与完整构建对象链接，额外测试头文件将默认 QSettings 改为独立 Ini 目录；配色复测另外禁用原生弹窗。该改动只在工作目录的测试辅助文件里，没有改变产品默认设置。

## 尚未完成

- Mac 原生颜色窗口的可靠性/自动化适配；所有新增功能的全面 Mac 回归。
- TeraFly 真数据描线、保存、关闭重开；大型真实图像/高节点 SWC 性能；插件/Neuron2/SAM2 等外部流程。
- Windows 应用验证，及本次头文件修正后的 Linux 重编（非 Apple 分支文本保持原样）。
- 构建仍报告旧 Qt 示例目录、Qt5 对新 SDK 未测试，以及依赖库的最低 macOS 版本高于默认 11.0 等警告。本包仅作为当前机器开发测试版，不宣称支持旧系统或可分发。依赖 Homebrew/本机库，不是独立安装包。
- 未合入远端 master、未推送、未替换原 Qt6 app。源码已在本地 mac 分支保存。

## 复建命令（当前机器）

在仓库根目录执行：

```sh
MAC_QT5_NATIVE_LIB_DIR=/Users/pzy/Documents/Codex/2026-10-07/for-neuron-tracing-editing-short-keys/work/common_lib/lib_mac64 \
MAC_QT5_BOOST_INCLUDE_DIR=/Users/pzy/Documents/Codex/2026-10-09/vaa3d-swc-3d/work/qt5-macos-probe/deps \
MAC_QT5_BUILD_DIR=/Users/pzy/Documents/Codex/2026-10-09/vaa3d-swc-3d/work/qt5-macos-probe \
scripts/build-macos-qt5.sh
```

Boost 的 deps/boost 为原本地源码 common_lib/include/boost 的只读使用链接。其他机器需要设置自己的原生库目录及 Boost include 目录，且准备 libtiff/teem/hdf5/libaec 等依赖。

产物：`/Users/pzy/Documents/Codex/2026-10-09/vaa3d-swc-3d/work/qt5-macos-probe/Vaa3D-Qt5-probe.app`。

证据在同目录：api-build.log、build-opengl2.log、rebuild-script.log、qa-build.log、qa-swc_navigation.log、qa-object_yflip.log、qa-swc_color-native.log、qa-swc_color-nonnative.log、qa-results.json。JSON 保留最初原生配色测试失败，不用复测覆盖原始证据。
