# Vaa3D Mac Qt5 local-058 预发布版

适用于 M 系列 Mac，最低 macOS 26.0。应用版本号为 0.58.0。

## 本次新增

- 内置 56 个神经元追踪及相关 SWC/图像工具插件。可在 Plug-In → neuron_tracing 中找到 Vaa3D-Neuron2 APP1、APP2、MOST、SimpleTracing、SmartTracing、Rivulet、Advantra、UltraTracer、神经元半径、SWC 排序/重采样等入口。
- Help → Check for Updates… 可检查本仓库的 Mac 发布版本，确认后自动下载和校验。下载就绪后保存工作并退出 Vaa3D，安装器会替换应用并重新打开。
- 更新会在 `~/Library/Application Support/Vaa3D/Updates` 保存旧版 ZIP 和安装日志。安装器最多等待 10 分钟，不会强制关闭正在使用的 Vaa3D。替换或启动命令失败时尝试恢复旧应用。
- 更新检查会展示预发布版标记，跳过草稿、其他平台和旧版本，使用发布清单中的 SHA-256 校验下载内容，并检查应用身份、版本、系统要求、架构和签名完整性。

## 安装

下载 `Vaa3D-macOS26-arm64-local-058.zip`，解压后替换现有应用。local-056 及更早版本首次需要手动安装本版，之后可使用应用内更新。应用已内置运行库，更新时需要应用所在目录可写，例如用户自己的 Applications 文件夹。

应用使用 ad-hoc 签名，尚未获得 Apple 公证。首次运行若被 macOS 拦截，请在系统设置的“隐私与安全性”中允许打开。

## 验证范围

56 个插件均通过 arm64 编译、Qt5 加载及菜单枚举检查。APP1、APP2 实际追踪合成三维管状图像并输出有效 SWC，APP2 默认的重采样与排序调用链通过。其他插件的具体算法输出及第三方运行依赖仍需针对所用工作流验证。

更新功能已通过版本筛选、禁止降级、下载校验、损坏检测、取消下载，以及安装器等待退出、旧版备份和拒绝损坏应用的回归。

67 个候选中仍有 11 个因旧 ITK、MATLAB Runtime、第三方库或编译兼容问题未能纳入。详细菜单和阻塞清单见同版本的 `PLUGINS-local-058.md`。原生配色弹窗、大型真实数据性能和全部插件算法的完整验收仍待完成。

更新信息从本仓库 mac 分支的 platforms/macos/updates.json 获取，安装包从 GitHub Releases 下载。发布后续版本时需要同步此清单，内容包括标签、版本对应文件名、下载地址和 SHA-256。

UltraTracer 等组合插件可能包含 NEUTUBE、SNAKE、NeuroGPSTree 子入口，这些入口仍依赖未纳入本包的算法插件。菜单枚举通过不表示这些外部依赖已满足。
