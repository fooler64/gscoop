# gScoop

![License](https://img.shields.io/badge/License-LGPL--3.0-blue.svg)

**Scoop 包管理器图形化客户端** —— 使用 C++ / Qt 6 重构的 [Rscoop](https://github.com/AmarBego/Rscoop)，甘雨主题界面，现代化卡片式 UI。

## ✨ 特性

- 🔍 **搜索包**：搜索本地 bucket 中的软件包，卡片式结果展示，简介自动适配高度
- 📦 **已安装管理**：卡片视图，支持 更新 / 卸载 / 锁定（hold）/ 更新全部 / 搜索过滤
- 🗂️ **Bucket 管理**：
  - 预置 12 个常用 bucket，一键添加
  - 国内镜像切换（ghproxy / gh-proxy / ghfast）
  - **探索新仓库**：搜索 GitHub 上的 scoop bucket 仓库（按 Stars 排序、过滤中文）
  - 批量添加多个 bucket
  - 点击已添加 bucket 查看详情
- 🧰 **环境自检（Doctor）**：检查 Git / Scoop / 7zip / Main bucket / 长路径 / NTFS / 开发者模式
- 🛡️ **安全**：VirusTotal 查毒集成（需 scoop-virustotal 扩展 + API key）
- 🎨 **甘雨主题**：全界面 JSON 主题文件驱动（`themes/*.json`），冰蓝 + 白 + 金配色，无 QSS
- 🪟 **现代化 UI**：无边框窗口、自绘窗口按钮、圆角卡片、过渡动画、自绘图标（QPainter 矢量）

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
