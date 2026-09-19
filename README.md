# gScoop

<img src="resources/gscoop-256.png" width="110" alt="gScoop icon">

![License](https://img.shields.io/badge/License-LGPL--3.0-blue.svg)

**Scoop 包管理器图形化客户端** —— 使用 C++ / Qt 6 重构的 [Rscoop](https://github.com/AmarBego/Rscoop)，甘雨主题界面，现代化卡片式 UI。

## ✨ 特性

### 🔍 搜索包
- 搜索本地所有 bucket 中的软件包，**相关性排序**：完全匹配 > 前缀匹配 > 子串匹配（搜 `git` 时 `git` 排第一）
- **卡片式结果展示**：每张卡片显示 名称 / 版本 / bucket / 简介，卡片高度按简介长度自动适配
- 未安装的包右侧显示**下载箭头**，已安装的显示绿色「已安装」徽标
- 单击卡片打开**包信息弹窗**，右键菜单可 查看信息 / 安装 / 卸载
- 搜索支持包名、描述、二进制（bin）匹配，分「包」和「二进制」两个标签

### 📦 已安装管理
- **卡片视图**展示所有已安装软件（名称 / 版本 / 来源 bucket）
- 每张卡片左上角：**锁图标**（点击锁定/解锁，锁定后不更新）+ **删除图标**（点击卸载）
- 有更新时卡片右上角显示「可更新」徽标
- 顶部工具栏：**搜索过滤**（按名称/描述）、**筛选下拉**（全部/可更新/已锁定/异常）、**刷新**、**更新全部**
- 点击卡片查看详细信息

### 🗂️ Bucket 管理
- **预置 12 个常用 bucket**（main / extras / versions / nirsoft / sysinternals / php / java / games / nonportable / nerd-fonts / extras-cn / cluttered-bucket），一键添加
- **国内镜像切换**：GitHub 直连 / ghproxy.com / gh-proxy.com / ghfast.top
- **探索新仓库**：搜索 GitHub 上的 scoop bucket 仓库，显示 owner/repo、描述、⭐Stars、更新时间，支持按 Stars/Apps/名称排序、隐藏中文 bucket、最小 Stars 过滤
- **添加 Bucket**：填写名称 + URL 添加单个仓库，或「添加多个 Buckets」批量添加（每行一个仓库）
- **已添加的 Buckets** 以卡片展示，点击卡片查看详情（类型/仓库/Manifests 数/分支/更新时间/路径），可打开仓库、删除 bucket

### 🧰 环境自检（Doctor）
- 一键检查：Git 是否安装 / Scoop 是否安装 / 7zip 是否安装 / Main bucket 是否安装 / Windows 长路径是否启用 / Scoop 是否位于 NTFS 磁盘 / 开发者模式是否启用
- 每项显示 通过/警告/失败 状态，附版本号或路径
- 附带**清理工具**：清理旧版本（scoop cleanup）+ 清理缓存（scoop cache rm）

### 🛡️ 安全
- **VirusTotal 查毒**：在包信息弹窗中一键调用 `scoop virustotal <package>` 扫描（需安装 scoop-virustotal 扩展并配置 API key）
- 支持网络代理设置

### ⚙️ 设置
- **自动化**：启动时检查更新、显示更新提示条
- **管理**：环境自检 + 自动清理
- **安全**：VirusTotal API key、网络代理
- **窗口**：主题选择（甘雨 / 雪白）、语言、启动页
- **托盘**：最小化到托盘、关闭到托盘
- **关于**：Scoop 状态、版本信息

### 📋 操作日志与进度
- **底部实时日志面板**：所有 scoop 操作（安装/更新/卸载/清理）的实时输出都显示在底部，可展开/收起、可清空
- **进度条**：解析 scoop 输出中的百分比，实时显示下载/解压进度

### ⚡ 批量操作
- 已安装页「批量选择」模式：多选卡片后**批量更新 / 批量卸载 / 批量锁定**，带全选功能

### 💾 配置备份
- **导出配置**：一键导出已安装软件列表为 JSON（scoop export）
- **导入配置**：从 JSON 还原并自动安装（scoop import），换机一键还原

### 🔧 一键修复
- 环境自检后可「一键修复」：自动安装缺失的 git / 7zip、补建 main bucket

### 📦 包详情增强
- 显示**安装占用大小**、**可用版本列表**、依赖、下载地址、备注

### ⭐ 搜索历史与收藏
- **搜索历史**：自动记录最近 20 条搜索词，下拉快速复用
- **收藏星标**：搜索卡片右上角星标可收藏常用包

### 🔄 Bucket 自动更新
- 可配置「每 24 小时 / 每 7 天」自动刷新 bucket 索引，或手动立即更新

### 🌐 多语言
- 内置**简体中文 / English** 切换（323 条界面文案完整翻译）

### 🎨 主题与 UI
- **甘雨主题**：全界面 JSON 主题文件驱动（`themes/*.json`），甘雨·蓝白冰系配色，可自由扩展新主题
- **现代化界面**：无边框窗口 + 自绘标题栏（最小化/最大化/关闭）、左侧活动栏、圆角卡片、页面切换过渡动画、全部图标用 QPainter 矢量自绘
- 不使用 QSS（Qt 私有样式标准），颜色全部收敛到 JSON 配置

## 📸 截图

（待补充截图）

## 📦 下载

前往 [Releases](https://github.com/fooler64/gscoop/releases) 下载：

| 版本 | 说明 |
|------|------|
| `gScoop-x.y.z-win64.zip` | 便携压缩版（解压后直接运行 `gscoop.exe`） |

## 🚀 使用

1. 安装 [Scoop](https://scoop.sh)（需要 Windows 7+ 与 PowerShell 5+）
2. 运行 gScoop
3. 首次进入可到「Bucket 管理」添加常用 bucket（main / extras / extras-cn 等）

## 🔨 从源码构建

### 环境要求

- Windows 10/11
- [Qt 6.x](https://www.qt.io/download-open-source)（需要 Widgets / Network / Concurrent 模块）
- CMake 3.16+ 与 Ninja（Qt 自带或单独安装）
- MinGW 13+（Qt 自带 mingw kit）或 MSVC

### 构建步骤

```bash
# 配置（以 Qt mingw kit 为例）
export PATH="/c/Qt/Tools/CMake_64/bin:/c/Qt/Tools/Ninja:/c/Qt/Tools/mingw1310_64/bin:$PATH"
export CMAKE_PREFIX_PATH="C:/Qt/6.11.2/mingw_64"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 部署

```bash
# 部署 Qt DLL（windeployqt）
cd build
C:/Qt/6.11.2/mingw_64/bin/windeployqt gscoop.exe --dir .
```

## 📁 项目结构

```
src/
├── main.cpp                 # 入口：ModernStyle + 全局字体
├── core/
│   ├── scoop_service.*      # scoop CLI 封装（搜索/安装/卸载/更新/hold/bucket）
│   ├── settings_store.*     # 设置持久化（QSettings）
│   └── theme_manager.*      # JSON 主题管理器
├── models/
│   └── scoop_models.*       # 数据模型 + JSON 解析
└── ui/
    ├── main_window.*        # 主窗口（无边框 + 自绘标题栏）
    ├── activity_bar.*       # 左侧活动栏
    ├── search_page.*        # 搜索页（卡片结果）
    ├── installed_page.*     # 已安装页（卡片 + hold/卸载）
    ├── bucket_page.*        # Bucket 管理页
    ├── settings_page.*      # 设置页（标签式）
    ├── package_info_dialog.*   # 包信息弹窗
    ├── bucket_info_dialog.*    # Bucket 详情弹窗
    ├── explore_bucket_dialog.* # 探索新仓库
    ├── add_bucket_dialog.*     # 添加 bucket
    ├── icon_painter.*       # 自绘矢量图标
    └── modern_style.*       # 现代化控件样式（QProxyStyle）
themes/
├── ganyu.json               # 甘雨·蓝白冰系（默认）
└── light.json               # 雪白·内衬
```

## 📜 许可证

本项目基于 **GNU Lesser General Public License v3.0 (LGPL-3.0)** 发布。

- 基于 [Rscoop](https://github.com/AmarBego/Rscoop)（MIT）的 UI 设计理念重写，代码为全新 C++/Qt 实现
- 使用 Qt 6（LGPL v3），动态链接，符合许可要求
- 完整许可证文本见 [LICENSE](LICENSE)

## 🙏 致谢

- [Rscoop](https://github.com/AmarBego/Rscoop) —— 功能参考与 UI 灵感
- [Scoop](https://scoop.sh) —— Windows 包管理器
- [Qt](https://www.qt.io) —— 跨平台 C++ 框架
