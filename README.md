# Vaa3D Codex 自用修改版

这是 **ExiaSit** 使用 Codex 维护的个人 Vaa3D 版本，基于 [Vaa3D/v3d_external](https://github.com/Vaa3D/v3d_external)，保留上游源码历史与许可证。此仓库不是 Vaa3D 官方发行版。

## 本地新增与修复

- SWC/ESWC 导入、批量自动配色、Object Manager 各窗口独立管理、逐对象颜色与 Y 翻转显示。
- TIFF Mask 导入为标签表面、后台网格生成、来源路径显示，以及图像/SWC/Mask 的 MIP 叠加。
- 大目录文件选择、最近路径、大型 V3DRAW 后台加载和三维交互修复。
- 三维方向步进、裁剪窗口移动、XYZ 轴标签、逐通道配色与体素标定。
- 全通道图像重采样到 1 µm 体素；保留源图，另建输出视窗。
- Linux `vaa3d://` 协议一次加载原图、多个 SWC 和 Mask，并复用已运行的兼容程序。

完整变更、时间、验证与限制见 [CHANGELOG_LOCAL.md](CHANGELOG_LOCAL.md)。`local-NNN` 是本维护版的版本标签，不是官方版本号。

## 获取与构建

```bash
git clone --recurse-submodules git@github.com:ExiaSit/Vaa3D-Codex.git
cd Vaa3D-Codex
```

私有仓库需要相应 GitHub 访问权限。当前维护版在 Linux、Qt 5.15.2 下构建与验证；依赖准备和运行方法见 [LOCAL_BUILD.md](LOCAL_BUILD.md)。准备好依赖后执行：

```bash
./build-debug-local.sh
```

构建脚本及协议注册包含当前 Linux 环境设置，换机器需按文档调整。Windows/macOS 尚未验证本维护版，系统集成与构建依赖需要适配；仓库不提供这两个平台的安装包。

## 使用说明

- [Mask 导入](MASK_IMPORT.md)
- [MIP 浏览器](MIP_VIEWER.md)
- [SWC 与 TIFF 坐标对齐](SWC_TIFF_ALIGNMENT.md)
- [上游 README](README)

本次同步保留本地维护提交及 `local-*` 标签；用户图像、自动保存标注、缓存和本机构建产物不纳入维护提交。软件各部分继续适用原有 [LICENSE](LICENSE) 及各子目录的许可证；“自用修改版”名称不改变这些许可条件。
