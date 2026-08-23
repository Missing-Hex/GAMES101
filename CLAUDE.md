# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

GAMES101（闫令琪《计算机图形学入门》）课程作业仓库。每个 `AssignmentN/` 目录是一份独立作业，含作业 PDF 和 `src/` 下的 C++17 CMake 工程（OpenCV + Eigen3，系统包安装，无需额外下载依赖）。

当前作业进展：Assignment1 已完成，Assignment2 框架已就位（待实现）。

## Build & run

```bash
cmake -B build -S src && make -C build -j   # 在 AssignmentN/src 下
./build/Rasterizer                          # 交互模式：a/d 旋转，ESC 退出
./build/Rasterizer -r 30 output.png         # 命令行模式：渲染指定角度到图片
```

- 依赖：`libopencv-dev`、`libeigen3-dev`（Assignment2 的 CMakeLists 只 `find_package(OpenCV)`，Eigen 走默认头文件路径）。
- 无测试框架，验证方式 = 编译 + 运行看输出图片。
- `**/build/` 和 `*.swp` 已在 .gitignore 中。

## Architecture

所有作业共享同一套骨架（Assignment1 代码即模板）：

- **main.cpp** — 定义三个变换矩阵函数 `get_model_matrix` / `get_view_matrix` / `get_projection_matrix`（透视投影的推导是作业核心），以及帧循环（清屏 → 设矩阵 → draw → 显示 → 处理按键）。
- **rasterizer.hpp/cpp** — `rst::rasterizer` 类，渲染管线核心在 `draw()`：`mvp * 顶点` → 透视除法（齐次坐标除以 w）→ 视口变换（NDC→屏幕，含 z 映射到深度范围）→ 光栅化。持有帧缓冲 `frame_buf` 和深度缓冲 `depth_buf`；`get_index`/`set_pixel` 注意 y 轴翻转（`(height-y)*width+x`）。
- **Triangle.hpp/cpp** — 三角形顶点/颜色/法线/纹理坐标数据结构。
- 作业进度以 `// TODO:` 注释标记，说明在作业 PDF 中；用户需要按 PDF 实现这些函数（Assignment2 的 rasterize_triangle 属于填充三角形，超出 Assignment1 的线框实现）。

## Notes

- 代码中的中文注释是用户的学习笔记，新增代码时请保持同样的中文注释风格。
- 提交信息习惯用英文，例如 `init: Assignment1 Learning Record`。
