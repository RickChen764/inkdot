<div align="center">
  <img src="resources/inkdot.png" width="112" alt="墨点 Inkdot 图标">
  <h1>墨点 Inkdot</h1>
  <p><em>轻落一笔，清爽阅读。</em></p>
  <p>适用于 Windows 的极轻量 Markdown 与 Mermaid 阅读/编辑器</p>

  <a href="https://github.com/RickChen764/inkdot/releases/latest">
    <img src="https://img.shields.io/github/v/release/RickChen764/inkdot?label=%E4%B8%8B%E8%BD%BD&style=for-the-badge&color=25272b" alt="下载最新版本">
  </a>
  <a href="https://github.com/RickChen764/inkdot/actions/workflows/build.yml">
    <img src="https://img.shields.io/github/actions/workflow/status/RickChen764/inkdot/build.yml?style=for-the-badge&label=%E6%9E%84%E5%BB%BA" alt="构建状态">
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/license-MIT-bc372b?style=for-the-badge" alt="MIT License">
  </a>
</div>

墨点使用 Windows 原生 Direct2D/DirectWrite 渲染，不携带 Electron 或 WebView。它以单个便携 EXE 分发，启动迅速，既适合作为 `.md`/`.mmd` 文件的默认阅读器，也提供带实时预览的轻量编辑模式。

在 [Tinta](https://github.com/oipoistar/tinta) 的轻盈基础上，墨点增加了 YAML Front Matter 原生渲染、跟随 Windows 系统语言的简体中文界面、输入法无关的编辑快捷键和独立品牌配置。应用图标采用真实书法侧点，保留宣纸渗化与枯笔露锋。

## 下载与兼容性

从 [Releases](https://github.com/RickChen764/inkdot/releases/latest) 下载 `inkdot.exe`，无需安装：

1. 运行 `inkdot.exe`，或把 Markdown/Mermaid 文件拖到窗口中。
2. 如需文件关联，运行 `inkdot.exe /register`，再在 Windows 设置中选择墨点。
3. 未签名便携版首次运行可能触发 SmartScreen；可展开“更多信息”后确认运行。

支持 Windows 10/11 x64。程序静态链接运行库，正常使用无需额外安装依赖。

> Microsoft Store 版本尚未发布。墨点使用独立的 MSIX Identity，不复用或关联 Tinta 的商店条目。

## 主要功能

- 原生 Direct2D/DirectWrite 硬件加速，无 Electron、无 WebView
- Markdown 阅读与专注编辑：左右实时预览、自动换行、撤销/重做、搜索
- YAML Front Matter 原生渲染：保留字段顺序、引号、缩进和多行文本
- Mermaid `flowchart`/`graph` 原生渲染，无需浏览器内核
- 10 套浅色/深色主题、目录、文件夹浏览、拖放打开和文件监视
- 简体中文/英文界面：跟随 Windows UI 语言自动选择
- 原生文本选择、复制、内部锚点、GitHub Alerts 和常用行内扩展
- 单个便携可执行文件，Release 构建目标体积低于 1 MiB

## YAML Front Matter

文件顶部由 `---` 包围的元数据会显示为紧凑、跟随主题的等宽信息块，而不会混入正文：

```yaml
---
title: 项目笔记
tags: [Markdown, Windows]
author:
  name: Ada
draft: false
---
```

内置轻量解析器支持常见映射、列表、嵌套值、流式集合、引号、注释及 `|`/`>` 多行文本，不增加运行时依赖。元数据无效时会显示本地化错误，正文仍可正常阅读和编辑。

## 编辑与快捷键

先打开一个真实的 `.md`、`.markdown` 或 `.mmd` 文件，再按 `Ctrl+E` 进入编辑模式。内置欢迎页没有对应磁盘文件，因此不可直接保存。

双击 `Ctrl` 可随时打开完整快捷键面板；面板支持滚轮浏览，按 `Esc`、`?` 或再次双击 `Ctrl` 关闭。

| 按键 | 功能 |
|---|---|
| `Ctrl+E` | 进入编辑模式 |
| 双击 `Ctrl` / `?` | 显示完整快捷键面板 |
| `Ctrl+S` | 保存 |
| `Ctrl+Z` / `Ctrl+Y` | 撤销 / 重做 |
| `Ctrl+X` / `Ctrl+C` / `Ctrl+V` | 剪切 / 复制 / 粘贴 |
| `Ctrl+P` | 显示 / 隐藏预览 |
| `Ctrl+W` | 切换编辑器自动换行 |
| 连按两次 `Esc` | 退出编辑模式 |
| `B` | 显示 / 隐藏文件夹浏览器 |
| `Tab` | 显示 / 隐藏目录 |
| `F` / `Ctrl+F` | 搜索 |
| `T` | 选择主题 |
| `Ctrl+滚轮` | 缩放 |
| `J` / `K`、方向键、`PgUp` / `PgDn` | 阅读导航 |
| `Q` | 退出 |

## 使用

```pwsh
# 打开 Markdown 或 Mermaid 文件
inkdot.exe document.md
inkdot.exe diagram.mmd

# 使用浅色主题打开
inkdot.exe -l document.md

# 启动时显示性能统计
inkdot.exe -s document.md

# 注册 .md、.markdown 和 .mmd 文件关联
inkdot.exe /register
```

设置独立保存在 `%APPDATA%\Inkdot\settings.ini`。首次运行且新设置不存在时，墨点可只读导入 `%APPDATA%\Tinta\settings.ini` 作为初始设置；之后两款应用互不影响。文件关联使用独立的 `Inkdot.MarkdownFile` ProgID，可与 Tinta 并存。

## 从源码构建

需要 Windows、Visual Studio 2019 或更高版本，以及 CMake 3.15 或更高版本：

```pwsh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

产物位于 `build/Release/inkdot.exe`。重新生成 ICO 和 MSIX 图标素材：

```pwsh
pwsh tools/generate_brand_assets.ps1
```

生成脚本以审核通过的 `resources/inkdot-source.png` 为源图，不会重新绘制书法笔触。

## 上游项目与许可

墨点基于 [oipoistar/Tinta](https://github.com/oipoistar/tinta) 修改，感谢原作者创建这一轻巧的原生 Markdown 阅读器。本项目是独立维护的社区分支，与 Tinta 原项目及其作者不存在官方关联或背书关系。

源代码依据 [MIT License](LICENSE) 发布，并完整保留上游原始版权声明。第三方依赖 [MD4C](https://github.com/mity/md4c) 在构建时由 CMake 获取。

---

<sub>English: Inkdot is an independent lightweight Windows Markdown/Mermaid reader and editor derived from Tinta. It adds native YAML Front Matter rendering, automatic Simplified Chinese localization, Ctrl+E editing, and a double-Ctrl shortcut panel. Download the portable executable from Releases.</sub>
