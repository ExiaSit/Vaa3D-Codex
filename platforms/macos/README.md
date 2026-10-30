# macOS

- 分支：`mac`。共享源码从 Ubuntu Qt 5 主线整合；本目录的 Info.plist 由 `v3d_essential.pro` 的 `macx` 条件引用，其他平台不使用。
- TIFF：注册 `public.tiff` 与 tif/tiff 扩展名，Alternate 角色，不自动修改用户默认应用。构建安装后可运行 macOS 的 `lsregister -f /absolute/path/Vaa3D.app` 刷新打开方式列表。
- 构建边界：这台 Mac 原先运行的是 QT6_NEW 基础上的独立 Qt 6 版本。此次共享代码整合没有完成整套应用的 Qt 5/6 统一和 macOS 完整构建验收；不能把 Linux 构建成功或 macOS 核心测试通过写成 Mac 应用发布成功。
- 原 Qt6 工作目录、应用、提交和未保存工作均保持原状。其源码变更（不含编译二进制）另存为 `archive/mac-qt6-local-20261030` 归档标签，以便后续移植。归档不代表它具有最新 Ubuntu 功能。
- 原 Mac 的 TeraFly 自动启动/全量 ROI、多视角 SAM2 面板依赖本机工作流，未作为跨平台默认功能合入；后续需要独立功能开关、外部资源配置和实际运行验收。
- 后续在 mac 分支继续做 Qt/依赖适配，保留 Ubuntu 新功能；不要直接用旧 Qt6 目录覆盖共享源码。验证后再合入 master。
