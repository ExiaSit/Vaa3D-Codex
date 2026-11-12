# Mac 插件验证清单 local-058

来源为 Vaa3D/vaa3d_tools 的固定提交 `475e4ca92d4e51de10f1c05d80cef6615432c087`。在 released_plugins/v3d_plugins 中筛选 neuron、trac、swc、bigneuron 相关目录，共 67 个候选。56 个插件库完成 arm64 编译与 Qt5 加载/菜单枚举检查。

APP1、APP2 使用 32×32×32 合成管状体数据执行追踪，分别生成 17 与 48 个 SWC 节点，父节点引用和坐标范围检查通过。APP2 默认后处理链实际调用 resample_swc 与 sort_neuron_swc，输出 8 个节点。其他插件本轮验证范围为加载与菜单枚举，算法输出和第三方运行依赖仍需按具体工作流验证。

## 已编译并加载

| 插件目录 | 菜单 |
| --- | --- |
| Janelia_refextract | RefExtract、About |
| SWC_Resample_plugin | Resample SWC Displayed in 3D Views、about |
| affine_transform_swc | apply_transform、about |
| bigneuron_ECondeSousa_N3DFix | N3DFix、about |
| bigneuron_chingwei_CWlab_method1_version1 | tracing_menu、about |
| bigneuron_chingwei_ENT | tracing_menu、about |
| bigneuron_chingwei_EnsembleNeuronTracerBasic | tracing_menu、about |
| bigneuron_chingwei_EnsembleNeuronTracerV2n | tracing_menu、about |
| bigneuron_chingwei_EnsembleNeuronTracerV2s | tracing_menu、about |
| bigneuron_hide_ikeno_SIGEN | Trace、About |
| bigneuron_miroslav_r_Advantra | advantra_menu、about |
| bigneuron_miroslav_r_NeuronChaser | nc_menu、about |
| bigneuron_qul_anisodiffusion | anisodiff_menu(general)、about |
| bigneuron_qul_anisodiffusion_littlequick | anisodiff_littlequick_menu、about |
| bigneuron_siqi_rivuletv3d | tracing、about |
| bigneuron_ting_dezhe_neutu_autotrace | tracing、about |
| bigneuron_ytc_nctuTW | start tracing、about |
| bigneuron_ytc_zz_nctuTW_tips | tracing_menu、about |
| bigneuron_zhijiang_zn_bjut_MeanshiftSpanningtree | meanshift、about |
| bigneuron_zhijiang_zn_bjut_fastmarching_spanningtree_vaa3d | tracing_menu、about |
| bigneuron_zz_PSF_tracing | trace、about |
| bigneuron_zz_neurontracing_TReMAP | trace_mip、about |
| bigneuron_zz_neurontracing_mst | tracing、about |
| blastneuron_plugin | about |
| connect_neuron_fragments_extractor | extract_by_markers、extract_spines、label_components、about |
| convert_neuron_format_to_swc | asc_to_swc、about |
| crop_image_trace | crop、about |
| eswc_converter | swc_to_eswc、eswc_to_swc、check_eswc_format、about |
| extract_z_slices | extract a subset of Z slices to a stack、about |
| global_neuron_feature | compute global features、compute global features (only the first connected tree)、Help |
| neuron_color_display | 1_click_color、about |
| neuron_color_seperator | pick_neuron_by_color、seperate_result_explorer、about |
| neuron_connector | connect_neuron_SWC、about |
| neuron_distance | neuron_dist、neuron_dist_mask_area、about |
| neuron_image_profiling | profile_swc、about |
| neuron_populator | populate、prune、detect、about |
| neuron_radius | neuron_radius、neuron_radius_current_window、neuron_radius_terafly、about |
| neuron_reliability_score | calculate_score、calculate_score_terafly、about |
| neuron_render_ESWC_features | color_render_ESWC_features、about |
| neuron_stitch | find_border_tips_SWC_image、about |
| neuron_tile_display | neuron_tile_display、neuron_tile_XY、neuron_tile_groups、tile_multi_windows (swc and image files selected)、tile_multi_windows (swc and image files under one folder)、set configuration (tile_multi_windows options only)、about |
| neuron_toolbox | Neuron Toolbox、about |
| neurontracing_MOST | Start tracing、About |
| neurontracing_simpletracing | Tracing、Segmentation、Help、-------------------、Other method 1: ray_shooting、Other method 2: dfs |
| neurontracing_smarttracing | tracing_menu、about |
| neurontracing_vn2 | Vaa3D-Neuron2-APP2、Vaa3D-Neuron2-APP1、about and help |
| pruning_swc | pruning、pruning_iterative、pruning based on markers、about |
| resample_swc | resample、about |
| shape_extractor | shape_extractor、about |
| sort_neuron_swc | sort_swc、sort_swc_batch、about |
| standardize_swc | standardize、about |
| swc2mask_cylinder | swc2mask、about |
| swc_to_maskimage | swc_to_maskimage、swc_filter、About |
| sync_swc_terafly | sync、about |
| ultratracer | trace_APP2、trace_APP1、trace_MOST、trace_NEUTUBE、trace_SNAKE、trace_MST、trace_NeuroGPSTree、trace_Rivulet2、trace_TReMAP、generate_final_result(3D)、about |
| vis_ESWC_features | color_render_ESWC_features、about |

## 未纳入安装包

| 候选目录 | 当前阻塞 |
| --- | --- |
| bigneuron_AmosSironi_PrzemyslawGlowacki_RegMST_tracing_plugin | 依赖旧 ITK/OpenMP 及 Linux 第三方库，需要独立移植 |
| bigneuron_AmosSironi_PrzemyslawGlowacki_SQBTree_plugin | 依赖旧 ITK/OpenMP 及 Linux 第三方库，需要独立移植 |
| bigneuron_LinGU_LCM_boost | 存在流比较和默认参数重定义等 C++ 编译错误 |
| bigneuron_jiezhou_recon_3d_thin_v1 | 缺失 ITK 开发依赖 |
| bigneuron_pengyu_zhihao_3DTraceSWC | 需要 MATLAB Runtime 头文件 mclmcrrt.h |
| bigneuron_siqi_stalker_v3d | 所附旧 ITK 头文件在当前编译器中失败 |
| bigneuron_ting_neutube | 缺失 neurolabi 的 zqtheader.h 及对应构建依赖 |
| bigneuron_zhouhang_hust_NeuroGPSTree | 所附旧 Eigen 模板在当前编译器中失败 |
| bigneuron_zz_snake_tracing | 缺失 ITK 开发依赖 |
| neuron_assemble_live | 旧版 Boost 模板与当前编译器不兼容 |
| neurontracing_neutube | 缺少顶层插件构建入口，需要单独移植 neurolabi 构建 |

## 复现

先按 BUILD_QT5.md 构建主程序，再设置 MAC_QT5_NATIVE_LIB_DIR 和 MAC_QT5_BOOST_INCLUDE_DIR，运行以下脚本。TOOLS_SOURCE 应为上述固定提交且没有源码修改。

```sh
python3 scripts/build-macos-neuron-plugins.py "$TOOLS_SOURCE" "$PLUGIN_BUILD_DIR" --app-build-dir "$APP_BUILD_DIR"
```

插件输出位于 PLUGIN_BUILD_DIR/plugins，加载验证程序为 tests/macos/plugin_probe.pro。通过加载检查后，将 plugins 复制到应用 Contents/MacOS/plugins，再执行 scripts/package-macos-qt5.py。构建脚本将 Qt5 接口、路径及少量编译兼容修改应用到隔离源码副本，原始 vaa3d_tools 保持可核对。

组合插件的限制：UltraTracer 的 NEUTUBE、SNAKE、NeuroGPSTree 子入口，以及部分 Ensemble 调用链可能依赖上述未纳入的算法。本轮未将这些调用链计为功能通过。
