# local-058 发布后验收

验收时间为 2026-11-12 14:00:58 +0800（Asia/Shanghai）。应用源码为 local-058 / 3f5e7c4d，本文随 local-059 保存，仅补充验收记录。

## GitHub 发布

预发布页面为 https://github.com/ExiaSit/Vaa3D-Codex/releases/tag/local-058 。安装包、SHA256SUMS、manifest、发布说明与插件清单五个附件已上传，GitHub 返回的全部 SHA-256 与本地一致。

安装包 SHA-256 为 `a9daf97ec0219ee6181d3d66d9aee44c7b09b42dd8f1c0a0e93ad52fec4651cc`，大小 31,263,973 字节。签名/依赖审计覆盖 113 个 Mach-O 文件，全部为 arm64 可用且非系统依赖位于应用内部。

## 实际在线升级

使用含新更新器、将版本标记设为 0.57.0 的隔离副本进行测试。这是更新器测试夹具，历史 local-057 程序没有此更新能力。

1. 在 Help → Check for Updates 中读取本仓库公开更新清单，界面正确提示已安装 0.57.0、可用 0.58.0（pre-release）。
2. 确认下载，实际从公开 Release 获取 ZIP。界面确认下载及校验完成，并提示保存工作、退出后安装。
3. 通过应用菜单退出，安装器完成旧包备份、替换与自动重启。日志含 `Installed and launched successfully.`，读取新 Info.plist 确认为 0.58.0，重新打开的界面显示 Ready。
4. 将实际下载更新后的包安装到本机日常路径，应用内再次检查显示 `Installed version 0.58.0 is up to date for this Mac release channel.`。

安装器备份位于 `~/Library/Application Support/Vaa3D/Updates/Vaa3D-0.57.0-20261112-055509521.zip`，对应 `.log` 保存执行结果。

## 本机安装和清理

唯一日常应用为 `/Users/pzy/Applications/Vaa3D-Qt5.app`，版本 0.58.0，包含 56 个插件。旧日常应用及本轮开发/打包应用副本移入废纸篓的 Vaa3D-pre-058 和 Vaa3D-test-058 文件夹，发布 ZIP、源码和构建对象保留。初次清理在注销未注册的试打包应用时返回 -10814，随后完成恢复性安装及其余清理，最终安装签名检查通过。

LaunchServices 核对结果：

```
path:                       /Users/pzy/.Trash/Vaa3D-test-058-20261112-135917/2-Vaa3D.app (0x4f94)
path:                       /Users/pzy/Applications/Vaa3D-Qt5.app (0x4f98)
```

菜单、APP1/APP2、默认重采样/排序与更新验证之外，其余算法的完整数据验收仍见 PLUGINS-local-058.md 的限制。APP1/APP2 最终打包应用追踪输出分别为 17/8 个节点，运行时加载日志无 Homebrew 库引用。
