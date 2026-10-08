# 100310 SWC 与 TIFF 坐标排查

排查时间：2026-10-08 21:37:15 +08:00（Asia/Shanghai）。

## 结论

`/home/pzy/100310.swc` 与 Python 读取的
`/data/disk/C6.0/img/image_100310_0000.tif` 使用同一方向的坐标。
当前 Vaa3D 的默认旧 TIFF 读取器对这张 8 位 TIFF 产生 Y 轴翻转，
SWC 加载不进行相应转换，导致两者错位。

图像实际尺寸为 XYZ = 392×392×303；JSON 的 original_shape =
304×1401×1401 是原始尺寸，不能作为当前 TIFF 的尺寸。
SWC 共 150 个节点，XYZ 范围分别为 194–245、180–204、80–114。

## 脚本生成 MIP 的处理

检查脚本 `/data/disk/Neuron_trace/6_gcut_pruning_no_radius_jump_no_long_edge_copy.py`：

- 第 1644 行附近通过 `ImageParser(...).load()` 读取背景。
  导入的 `/home/pzy/Neuron_Trace/TraceFlow/core/io/image_parser.py`
  直接调用 `tifffile.imread`，数组顺序为 ZYX，没有翻转或裁剪。
- `_save_combined_grid_plot` 中沿 axis=0 求最大值生成 XY MIP，
  `imshow` 显示背景，直接用 SWC 的 x、y 绘制线段。
- 导入的 `core/visualization/debug_vis.py:264` 中 `plot_swc`
  同样取 `image.max(axis=0)`，使用 `origin='lower'`；图像和线段共用坐标轴，
  此设置不会改变两者的相对位置。`scale` 仅调整散点面积，不缩放坐标。
- 元数据中的 xy_resolution=280、z_resolution=997（纳米）用于计算胞体位置，
  不是 MIP 绘图时对 SWC 的坐标转换。
- 该脚本当前默认 DATASET_ROOT 是 `/data/disk2/B4.5`，并非 C6.0。
  本次仅检查其绘图逻辑，没有执行整个处理流程。

## Vaa3D 读取路径及验证

日志 `/tmp/vaa3d-files/restarted.log` 显示 b_useMylib=false，图像尺寸
392×392×303；Local 3D View 的起点实际为 0,0,0，包含全图，排除局部裁剪偏移。

读取路径为 `Image4DSimple::loadImage` → `loadTif2Stack` → `Read_Stack` →
`mg_image_lib.cpp` 的 `read_directory`。8 位图使用 `TIFFReadRGBAImage`，
它默认返回从底行开始的像素；当前代码顺序复制到数组，因此相对于 tifffile
读取结果沿 Y 轴翻转。SWC 的 `loadNeuronTree` 直接读取坐标。

独立 C++ 验证程序链接当前应用的 `mg_image_lib.o`、`mg_utilities.o` 和
`libv3dtiff`，读取同一 TIFF 并输出全部像素，与 Python 数组比较：

- 与原始数组不一致：37,260,330 个体素不同。
- 与 `image[:, ::-1, :]` 完全一致（全部体素逐一比较）。

这是该文件在当前读取器下的结论，不能自动推广到 16 位 TIFF、MYLIB、
TeraFly 或其他图像格式。这里没有改动全局读取行为，以免改变已有标注的坐标约定。

## 当前窗口的适配文件

已另存 `/home/pzy/100310.vaa3d_yflip.swc`，转换为：

```text
x' = x
y' = 392 - 1 - y = 391 - y
z' = z
```

原始 SWC 未修改。验证 150 个节点的编号、类型、X、Z、半径、父节点完全保持，
新旧 Y 坐标之和为 391。适配文件仅用于当前旧 TIFF 读取方向；
不要替换 Python 管线的原始 SWC，也不要直接用于原分辨率 TeraFly 图像。
在当前三维窗口移除原 SWC 叠加后，拖入适配文件即可检查。
尚未在交互窗口内人工确认最终叠加效果；未重启运行中的应用。

图像、SWC 和临时验证二进制均未纳入 Git。
