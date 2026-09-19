// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/main_window.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QMessageBox>
#include <QApplication>
#include <QCoreApplication>
#include <QFrame>
#include <QScreen>
#include <QGuiApplication>
#include <QShowEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QRegularExpression>

#include "core/scoop_service.h"
#include "core/settings_store.h"
#include "core/theme_manager.h"
#include "ui/animated_stack.h"
#include "ui/activity_bar.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"
#include "ui/window_button.h"
#include "ui/log_panel.h"
#include "ui/search_page.h"
#include "ui/installed_page.h"
#include "ui/bucket_page.h"
#include "ui/settings_page.h"
#include "ui/doctor_page.h"
#include "ui/explore_bucket_dialog.h"
#include "ui/add_bucket_dialog.h"
#include "ui/add_buckets_dialog.h"
#include "ui/package_info_dialog.h"
#include "ui/bucket_info_dialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
// MinGW 旧头文件可能缺少这些常量
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif
#endif

// 操作类型 → 标题
static QString opTitle(ScoopOpType type) {
    switch (type) {
    case ScoopOpType::Install: return QObject::tr("安装");
    case ScoopOpType::Uninstall: return QObject::tr("卸载");
    case ScoopOpType::Update: return QObject::tr("更新");
    case ScoopOpType::UpdateAll: return QObject::tr("更新全部");
    case ScoopOpType::Hold: return QObject::tr("锁定");
    case ScoopOpType::Unhold: return QObject::tr("解除锁定");
    case ScoopOpType::BucketAdd: return QObject::tr("添加 Bucket");
    case ScoopOpType::BucketRemove: return QObject::tr("删除 Bucket");
    case ScoopOpType::Cleanup: return QObject::tr("清理旧版本");
    case ScoopOpType::CacheRm: return QObject::tr("清理缓存");
    case ScoopOpType::VirusTotal: return QObject::tr("VirusTotal 查毒");
    default: return QObject::tr("操作");
    }
}

// 从 scoop 输出里解析进度百分比（如 "Downloading ... 45%"）
static int parseProgress(const QString& output) {
    static const QRegularExpression re("(\\d{1,3})\\s*%");
    int last = -1;
    auto it = re.globalMatch(output);
    while (it.hasNext()) {
        const auto m = it.next();
        const int v = m.captured(1).toInt();
        if (v >= 0 && v <= 100) last = v;
    }
    return last;
}

// 设置 Windows 原生标题栏为深色（VSCode 风格）
static void setDarkTitleBar(QWidget* w, bool dark) {
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(w->winId());
    const BOOL value = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &value, sizeof(value));
#endif
}

MainWindow::MainWindow(ScoopService* service, QWidget* parent)
    : QMainWindow(parent), m_service(service) {
    // 无边框窗口：使用自绘标题栏 + 自绘窗口按钮
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setMouseTracking(true);
    setupUi();
    setupTray();

    connect(m_service, &ScoopService::scoopDetected,
            this, &MainWindow::onScoopDetected);
    connect(&SettingsStore::instance(), &SettingsStore::settingsChanged,
            this, &MainWindow::onSettingsChanged);

    // 初始主题
    applyTheme(SettingsStore::instance().settings().theme);
    m_service->probeEnvironment();

}


void MainWindow::setupUi() {
    setWindowTitle(tr("gScoop"));
    resize(1180, 740);
    setMinimumSize(900, 580);

    // ===== 布局：顶部通栏标题栏 + (活动栏 | 内容) =====
    auto* rootLayout = new QVBoxLayout;
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- 顶部通栏标题栏（gScoop 在最左上，窗口按钮在最右）----
    m_titleBar = new QFrame(this);
    m_titleBar->setFixedHeight(48);
    auto* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(16, 0, 8, 0);
    titleLayout->setSpacing(10);

    auto* titleLabel = new QLabel(tr("gScoop"), m_titleBar);
    QFont tf = titleLabel->font();
    tf.setPointSize(14);
    tf.setBold(true);
    titleLabel->setFont(tf);
    titleLayout->addWidget(titleLabel);

    auto* subLabel = new QLabel(tr("Scoop 包管理器 · 甘雨主题"), m_titleBar);
    titleLayout->addWidget(subLabel);
    titleLayout->addStretch();

    // ---- 自绘窗口按钮（最小化/最大化-还原/关闭，带 hover 过渡动画）----
    auto* winBtns = new QHBoxLayout;
    winBtns->setContentsMargins(0, 0, 0, 0);
    winBtns->setSpacing(2);

    // 以管理员身份运行（权限不足时一键提权重启）
    m_elevateBtn = new QPushButton(m_titleBar);
    m_elevateBtn->setIcon(IconPainter::shield(Theme::textSub(false), 16));
    m_elevateBtn->setFixedSize(34, 30);
    m_elevateBtn->setFlat(true);
    m_elevateBtn->setCursor(Qt::PointingHandCursor);
    m_elevateBtn->setToolTip(tr("以管理员身份重新启动（解决权限问题）"));
    winBtns->addWidget(m_elevateBtn);

    m_minBtn = new WindowButton(WindowButton::Minimize, m_titleBar);
    m_maxBtn = new WindowButton(WindowButton::MaximizeRestore, m_titleBar);
    m_closeBtn = new WindowButton(WindowButton::Close, m_titleBar);
    for (WindowButton* b : {m_minBtn, m_maxBtn, m_closeBtn}) {
        b->setFixedSize(44, 32);
        winBtns->addWidget(b);
    }
    titleLayout->addLayout(winBtns);

    connect(m_elevateBtn, &QPushButton::clicked, this, &MainWindow::onElevateClicked);
    connect(m_minBtn, &WindowButton::clicked, this, &MainWindow::onMinimizeClicked);
    connect(m_maxBtn, &WindowButton::clicked, this, &MainWindow::onMaximizeClicked);
    connect(m_closeBtn, &WindowButton::clicked, this, &MainWindow::onCloseClicked);

    rootLayout->addWidget(m_titleBar);

    // ---- 下方：活动栏 | 内容区 ----
    auto* bodyLayout = new QHBoxLayout;
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // 左侧活动栏
    auto* activityBar = new ActivityBar(this);
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("🔍"), tr("搜索")));   // 0
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("📦"), tr("已安装"))); // 1
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("🗂️"), tr("Bucket"))); // 2
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("⚙️"), tr("设置")));   // 3
    bodyLayout->addWidget(activityBar);

    connect(activityBar, &ActivityBar::itemClicked, this,
            [this](int idx) { navigateTo(idx, true); });
    m_activityBar = activityBar;

    // 右侧内容（页面堆栈）
    m_stack = new AnimatedStackedWidget(this);
    // 主页面：水平滑动（活动栏在左侧，左右切换更符合直觉）
    m_stack->setSlideAxis(Qt::Vertical);   // 主页面：上→下 / 下→上 垂直切换
    m_stack->setDuration(220);
    m_stack->setSlideExtent(1.0);
    m_searchPage = new SearchPage(m_service, this);
    m_bucketPage = new BucketPage(m_service, this);
    m_installedPage = new InstalledPage(m_service, this);
    m_settingsPage = new SettingsPage(m_service, this);

    m_stack->addWidget(m_searchPage);      // 0
    m_stack->addWidget(m_bucketPage);      // 1
    m_stack->addWidget(m_installedPage);   // 2
    m_stack->addWidget(m_settingsPage);    // 3

    bodyLayout->addWidget(m_stack, 1);
    rootLayout->addLayout(bodyLayout, 1);

    // ---- 底部操作日志面板（跨整宽）----
    m_logPanel = new LogPanel(this);
    rootLayout->addWidget(m_logPanel);

    auto* central = new QWidget(this);
    central->setLayout(rootLayout);
    setCentralWidget(central);

    // 连接 scoop 操作 → 日志面板
    connect(m_service, &ScoopService::opStarted, this,
            [this](ScoopOpType type, const QString& package) {
        m_logPanel->beginOperation(opTitle(type), package);
    });
    connect(m_service, &ScoopService::opProgress, this,
            [this](const ScoopOpProgress& p) {
        if (!p.output.isEmpty()) m_logPanel->appendRaw(p.output);
        const int pct = parseProgress(p.output);
        if (pct >= 0) m_logPanel->setProgress(pct);
        if (!p.stage.isEmpty()) m_logPanel->setStage(p.stage);
    });
    connect(m_service, &ScoopService::opFinished, this,
            [this](ScoopOpType, const QString&, bool success, const QString& error) {
        m_logPanel->finishOperation(success, error);
    });
    // 设置页（自检/一键修复）请求写日志
    connect(m_settingsPage, &SettingsPage::logRequested, this,
            [this](const QString& title, const QString& text) {
        m_logPanel->beginOperation(title, QString());
        m_logPanel->appendRaw(text);
        m_logPanel->setExpanded(true);
    });

    // 启动页
    const QString launch = SettingsStore::instance().settings().defaultLaunchPage;
    int idx = 0;
    if (launch == "installed") idx = 1;
    else if (launch == "buckets") idx = 2;
    else if (launch == "settings") idx = 3;
    navigateTo(idx, false);
}

void MainWindow::setupTray() {
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon::fromTheme("applications-utilities"));
    m_tray->setToolTip(tr("gScoop"));

    auto* menu = new QMenu(this);
    menu->addAction(tr("显示主窗口"), this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    menu->addSeparator();
    menu->addAction(tr("退出"), this, [this]() {
        m_exiting = true;
        close();
    });
    m_tray->setContextMenu(menu);
    m_tray->show();
}

void MainWindow::navigateTo(int pageIndex, bool animate) {
    if (!m_stack) return;
    // 活动栏按钮索引：0=search 1=installed 2=bucket 3=settings
    // 页面堆栈索引：0=search 1=bucket 2=installed 3=settings
    int stackIdx = pageIndex;
    if (pageIndex == 1) stackIdx = 2;   // installed
    else if (pageIndex == 2) stackIdx = 1;  // bucket

    // 先触发页面内容加载（onPageShown），再动画切换，避免动画期间空白页
    switch (pageIndex) {
    case 0: m_searchPage->onPageShown(); break;
    case 1: m_installedPage->onPageShown(); break;
    case 2: m_bucketPage->onPageShown(); break;
    case 3: m_settingsPage->onPageShown(); break;
    }

    m_stack->setCurrentIndex(stackIdx, animate);

    if (m_activityBar) m_activityBar->setActive(pageIndex);
    m_currentPage = pageIndex;
}

// 依据设置配置 bucket 自动更新定时器
void MainWindow::configureBucketTimer() {
    if (!m_bucketTimer) return;
    const AppSettings& s = SettingsStore::instance().settings();
    if (s.autoBucketUpdate && s.bucketUpdateHours > 0) {
        m_bucketTimer->start(s.bucketUpdateHours * 3600 * 1000);
    } else {
        m_bucketTimer->stop();
    }
}

void MainWindow::onAutoBucketUpdateTick() {
    // 静默刷新全部 bucket（不弹窗，进度显示在底部日志）
    m_service->updateAllBuckets();
}

void MainWindow::onScoopDetected(bool installed) {
    if (!installed) {
        statusBar()->showMessage(tr("未检测到 Scoop，请先安装 Scoop"), 10000);
    } else {
        statusBar()->showMessage(tr("Scoop 已就绪"), 5000);
    }
}

void MainWindow::onSettingsChanged() {
    const AppSettings& s = SettingsStore::instance().settings();
    applyTheme(s.theme);
    configureBucketTimer();
}

void MainWindow::applyTheme(const QString& themeId) {
    // 主题 id（ganyu / light / 其他 JSON 文件）→ ThemeManager
    ThemeManager::instance().loadTheme(themeId);
    const bool dark = ThemeManager::instance().isDark();
    m_dark = dark;

    // 代码调色板（JSON 驱动，无 QSS）
    ThemeManager::instance().apply();
    if (m_activityBar) m_activityBar->setDark(dark);

    // 内容区页面背景跟随主题（surface2 面板色，与窗口 bg 分层）
    const QColor pageBg = Theme::surface2(dark);
    for (QWidget* w : {qobject_cast<QWidget*>(m_searchPage),
                       qobject_cast<QWidget*>(m_bucketPage),
                       qobject_cast<QWidget*>(m_installedPage),
                       qobject_cast<QWidget*>(m_settingsPage)}) {
        if (!w) continue;
        QPalette wp = w->palette();
        wp.setColor(QPalette::Window, pageBg);
        w->setPalette(wp);
        w->setAutoFillBackground(true);
    }

    // 标题栏背景
    if (m_titleBar) {
        QPalette p = m_titleBar->palette();
        p.setColor(QPalette::Window, Theme::titleBarBg(dark));
        m_titleBar->setPalette(p);
        m_titleBar->setAutoFillBackground(true);
    }

    // 窗口按钮跟随主题重绘
    for (WindowButton* b : {m_minBtn, m_maxBtn, m_closeBtn}) {
        if (b) b->update();
    }

    // 日志面板跟随主题
    if (m_logPanel) m_logPanel->refreshTheme();

    // Windows 原生标题栏跟随主题
    setDarkTitleBar(this, dark);
}

void MainWindow::showEvent(QShowEvent* event) {
    // 首次显示时居中（避免右侧超出屏幕导致控件被裁）
    if (!m_centered) {
        m_centered = true;
        if (QScreen* screen = QGuiApplication::primaryScreen()) {
            const QRect avail = screen->availableGeometry();
            move(avail.center() - QPoint(width() / 2, height() / 2));
        }
    }
    QMainWindow::showEvent(event);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeIcon();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    const AppSettings& s = SettingsStore::instance().settings();
    if (!m_exiting && s.closeToTray && m_tray && m_tray->isVisible()) {
        hide();
        m_tray->showMessage(tr("gScoop"), tr("已最小化到托盘"), QSystemTrayIcon::Information, 2000);
        event->ignore();
        return;
    }
    event->accept();
}

// ===== 自绘窗口按钮 =====
void MainWindow::onMinimizeClicked() {
    showMinimized();
}

void MainWindow::onMaximizeClicked() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    updateMaximizeIcon();
}

void MainWindow::onCloseClicked() {
    close();
}

// 以管理员身份重新启动（UAC 提权）
void MainWindow::onElevateClicked() {
#ifdef Q_OS_WIN
    // 已经是管理员则直接提示
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    if (isAdmin) {
        QMessageBox::information(this, tr("管理员权限"),
            tr("gScoop 当前已以管理员身份运行。"));
        return;
    }

    const QString exe = QCoreApplication::applicationDirPath() + "/gscoop.exe";
    // ShellExecuteW "runas" 触发 UAC 提权
    const HINSTANCE r = ShellExecuteW(nullptr, L"runas", reinterpret_cast<LPCWSTR>(exe.utf16()),
                                      nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<qintptr>(r) <= 32) {
        QMessageBox::warning(this, tr("提权失败"),
            tr("无法以管理员身份启动（可能被用户取消）。"));
        return;
    }
    // 提权成功：退出当前实例，避免两个实例并存
    m_exiting = true;
    QCoreApplication::quit();
#else
    QMessageBox::information(this, tr("提示"), tr("该功能仅支持 Windows。"));
#endif
}

void MainWindow::updateMaximizeIcon() {
    if (!m_maxBtn) return;
    m_maxBtn->setMaximized(isMaximized());
}

// ===== 无边框窗口拖动 =====
void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->pos().y() <= m_titleBar->height()) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        if (isMaximized()) {
            // 从最大化拖动时先还原（近似：直接还原到还原尺寸）
            showNormal();
        }
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->pos().y() <= m_titleBar->height()) {
        onMaximizeClicked();
        event->accept();
        return;
    }
    QMainWindow::mouseDoubleClickEvent(event);
}
