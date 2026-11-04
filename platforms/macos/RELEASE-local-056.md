# Vaa3D Mac Qt5 local-056 预发布版

面向 Apple Silicon（M 系列）Mac，最低 macOS 26.0。本机启动验证环境为 macOS 26.6.2，其他 Mac 尚待实机验证。

## 下载和安装

下载 `Vaa3D-macOS26-arm64-local-056.zip`，解压，将 Vaa3D.app 拖到“应用程序”，双击打开。已内置 Qt5 和动态库。首次运行如被系统拦截，在系统设置的“隐私与安全性”中允许该应用打开。应用使用 ad-hoc 签名，未获得 Developer ID 签名或 Apple 公证。

文件使用 `SHA256SUMS.txt` 校验。源码对应仓库 mac 分支的 local-056 标签，应用主体复用 local-055 的 Qt5 构建。本次增加发布打包脚本、应用版本标记、依赖和许可证收集。安装包内版本号为 0.56.0。

## 本次验证

- 57 个 Mach-O 文件均包含 arm64，所有非系统动态库依赖在包内可解析。
- 最低系统版本由所含二进制要求计算，结果为 26.0。
- 本地签名完整性校验通过。
- 清除 Qt/DYLD 环境设置后启动打包应用，12 秒时进程仍运行，实际加载日志没有 Homebrew 动态库路径。本次启动检查使用合成 TIFF 输入，尚未重新完成可视化交互验收。
- 既有 Mac Qt5 测试覆盖 TIFF/SWC 叠加及旋转、SWC 导航和 Y 翻转。原生配色弹窗、大规模真实数据性能及全部插件流程仍待验证，详见 BUILD_QT5.md。

## 打包复现

准备现有 Qt5 应用、Homebrew Qt5 和所用依赖后，在仓库根目录运行以下命令。输出目录必须尚未包含 Vaa3D.app。

```sh
python3 scripts/package-macos-qt5.py /path/to/Vaa3D-Qt5.app /path/to/release --version 0.56.0
cd /path/to/release
ditto -c -k --sequesterRsrc --keepParent Vaa3D.app Vaa3D-macOS26-arm64-local-056.zip
shasum -a 256 Vaa3D-macOS26-arm64-local-056.zip > SHA256SUMS.txt
```

打包脚本包含 WebP sharpyuv 补充步骤，Qt5 的 macdeployqt 在本机遗漏该间接依赖。发布附件另附 manifest.json、本文和校验和。源码和应用的许可证随包位于 Contents/Resources/Licenses。
