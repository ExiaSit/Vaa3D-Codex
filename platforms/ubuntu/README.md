# Ubuntu

- 分支：`ubuntu`；日常修改先在此分支提交，再验证并合入 master。
- 当前完整应用构建入口为根目录 `build-debug-local.sh`（Qt 5）；依赖和图形回归详见 `LOCAL_BUILD.md`。
- 桌面文件关联和 `vaa3d://` 协议集成继续使用 `scripts/` 中现有 Linux 实现，不在其他平台自动执行。
- 原运行目录中的 experiments、未跟踪测试、颜色设置和未保存标注不属于本次整合范围。
