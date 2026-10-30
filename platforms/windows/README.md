# Windows

- 分支：`windows`，由用户维护。初始源码与新的共享 master 一致，尚未完成 Windows 应用构建和运行验收。
- 上游已有 `build_msvc.bat`、`v3d_main/v3d/vaa3d_msvc.pro` 等入口；它们是既有入口，不代表已适配当前 Visual Studio/Qt/依赖版本。
- 本机 SDK、Qt 和库路径通过本地配置或构建参数提供，不提交绝对机器路径；专用代码放在本目录或 win32/Q_OS_WIN 条件下。
- CI 中 Windows shared-core 只测试独立 C++ 核心算法，不等于完整 GUI 构建。首次可运行版本需记录工具链、Qt版本、依赖、TIFF/SWC/3D 验收结果。
