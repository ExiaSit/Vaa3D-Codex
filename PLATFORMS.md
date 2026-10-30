# 四分支维护约定

## 分支与更新顺序

活动远端分支仅保留 `master`、`windows`、`mac`、`ubuntu`。`master` 是共同代码主线；平台分支从同一整合提交开始，未验证的平台不是可发布安装包。

1. 每次改动先在对应的平台分支提交，更新 `CHANGELOG_LOCAL.md`，分配唯一 `local-NNN` 标签（先检查远端已有最大编号，不能覆盖别的平台标签）。
2. 运行相关单元/集成测试及本平台应用构建和受影响的 GUI 测试。共享改动还需记录其他平台的结果或未验证范围。
3. 验证完成后将平台分支合入 `master`，通过 PR 展示范围及证据。平台专有实现用 `macx` / `win32` / `unix:!macx` 或 `Q_OS_*` 隔离，允许源码进入共享主线。
4. 其余平台再合入 `master`。不要以整目录覆盖解决冲突，不采用 ours 策略伪造通用功能合并，不强制覆盖别人提交。
5. 后续推送按用户指令执行；2026-10-30 用户明确授权本次合并、推送和分支整理，不将授权扩大为任意外部发布。

共享功能与平台功能按职责划分，SAM2/VR 等可选模块采用功能开关，不能仅因开发发生在某系统就强行绑定平台。构建产物、图像、自动保存标注、缓存和 SDK 不提交。

## 首次切换（保留未跟踪文件）

在现有仓库先 `git status --short`。若远端名为 codex：

```sh
git fetch codex --prune
git switch --track codex/ubuntu  # Ubuntu；Mac/Windows 分别换成 mac/windows
```

若平台本地分支已存在，使用 `git switch ubuntu` 后检查差异，再 `git merge --ff-only codex/ubuntu`。新克隆默认远端名为 origin，相应替换命令中的 codex。不要执行 clean/reset --hard；若未跟踪文件阻止切换，先单独备份该文件再处理。Ubuntu 原来的 master 和未跟踪数据本次没有被切换或修改。

## 验证分级

- `cmake -S tests -B build/shared-tests`、构建和 ctest：独立共用算法回归，不是全程序。
- `.github/workflows/shared-core.yml`：三个操作系统的同一核心测试，工作流结果以 GitHub 实际运行为准。
- Linux 完整 SWC 导航测试：完成应用构建后，在 `v3d_main/v3d` 执行 `make -f Makefile -f ../../tests/swc_navigation_gui_test.mk swc-navigation-gui-test`，在独立 Xvfb/临时 XDG_CONFIG_HOME 中运行生成的测试程序；另跑已有 swc-color 和 yflip 图形回归。
- macOS/Windows：分别按 [macOS](platforms/macos/README.md)、[Windows](platforms/windows/README.md) 补齐完整应用验证，不能用 Linux 结果代替。

## 旧分支可恢复归档

旧远端分支先保存为 `archive/pre-platform-20261030/*` 标签，再按原 SHA 校验删除分支引用。映射见 [归档清单](doc/platform-branch-archive-20261030.json)。编号防止 VROnUbuntu/VRonUbuntu 在大小写不敏感磁盘上冲突。现有 local-* 标签保留。

恢复某个旧版本可以按清单的标签执行 `git switch --detach <archive-tag>`；若确需恢复旧活动分支，要先与四分支约定协调。原 Mac Qt6 本地修改的源代码另保存在 `archive/mac-qt6-local-20261030`，不上传其二进制备份历史。
