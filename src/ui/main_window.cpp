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

#include "core/scoop_service.h"
#include "core/settings_store.h"
#include "core/theme_manager.h"
#include "ui/animated_stack.h"
#include "ui/activity_bar.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"
#include "ui/search_page.h"
#include "ui/installed_page.h"
#include "ui/bucket_page.h"
#include "ui/settings_page.h"
#include "ui/doctor_page.h"
#include "ui/explore_bucket_dialog.h"
#include "ui/add_bucket_dialog.h"
#include "ui/add_buckets_dialog.h"
#include "ui/package_info_dialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
// MinGW 旧头文件可能缺少这些常量
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif
#endif

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

    // ===== VSCode 风格布局：活动栏 | (标题栏 + 内容) =====
    auto* rootLayout = new QHBoxLayout;
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- 左侧活动栏 ----
    auto* activityBar = new ActivityBar(this);
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("🔍"), tr("搜索")));   // 0
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("📦"), tr("已安装"))); // 1
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("🗂️"), tr("Bucket"))); // 2
    m_navButtons.append(activityBar->addItem(QString::fromUtf8("⚙️"), tr("设置")));   // 3
    rootLayout->addWidget(activityBar);

    connect(activityBar, &ActivityBar::itemClicked, this,
            [this](int idx) { navigateTo(idx, true); });
    m_activityBar = activityBar;

    // ---- 右侧主体（标题栏 + 页面）----
    auto* rightSide = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightSide);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // 顶部标题栏（VSCode 风格：窄条 + 标题）
    m_titleBar = new QFrame(rightSide);
    m_titleBar->setFixedHeight(44);
    auto* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(20, 0, 20, 0);
    titleLayout->setSpacing(10);

    auto* titleLabel = new QLabel(tr("gScoop"), m_titleBar);
    QFont tf = titleLabel->font();
    tf.setPointSize(13);
    tf.setBold(true);
    titleLabel->setFont(tf);
    titleLayout->addWidget(titleLabel);

    auto* subLabel = new QLabel(tr("Scoop 包管理器 · 甘雨主题"), m_titleBar);
    titleLayout->addWidget(subLabel);
    titleLayout->addStretch();

    // ---- 自绘窗口按钮（最小化/最大化/关闭）----
    const int btnSize = 34;
    const QColor btnIconColor = Theme::textSub(false);

    m_minBtn = new QPushButton(m_titleBar);
    m_minBtn->setIcon(IconPainter::minimize(btnIconColor, 14));
    m_minBtn->setFixedSize(btnSize, btnSize);
    m_minBtn->setFlat(true);
    m_minBtn->setCursor(Qt::PointingHandCursor);
    m_minBtn->setToolTip(tr("最小化"));
    m_minBtn->setObjectName("winBtn");
    titleLayout->addWidget(m_minBtn);

    m_maxBtn = new QPushButton(m_titleBar);
    m_maxBtn->setIcon(IconPainter::maximize(btnIconColor, 14));
    m_maxBtn->setFixedSize(btnSize, btnSize);
    m_maxBtn->setFlat(true);
    m_maxBtn->setCursor(Qt::PointingHandCursor);
    m_maxBtn->setToolTip(tr("最大化"));
    m_maxBtn->setObjectName("winBtn");
    titleLayout->addWidget(m_maxBtn);

    m_closeBtn = new QPushButton(m_titleBar);
    m_closeBtn->setIcon(IconPainter::close(btnIconColor, 14));
    m_closeBtn->setFixedSize(btnSize, btnSize);
    m_closeBtn->setFlat(true);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setToolTip(tr("关闭"));
    m_closeBtn->setObjectName("winCloseBtn");
    titleLayout->addWidget(m_closeBtn);

    // 窗口按钮 hover/点击样式（代码调色板）
    connect(m_minBtn, &QPushButton::clicked, this, &MainWindow::onMinimizeClicked);
    connect(m_maxBtn, &QPushButton::clicked, this, &MainWindow::onMaximizeClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    m_minBtn->installEventFilter(this);
    m_maxBtn->installEventFilter(this);
    m_closeBtn->installEventFilter(this);

    rightLayout->addWidget(m_titleBar);

    // 页面堆栈
    m_stack = new AnimatedStackedWidget(rightSide);
    m_searchPage = new SearchPage(m_service, this);
    m_bucketPage = new BucketPage(m_service, this);
    m_installedPage = new InstalledPage(m_service, this);
    m_settingsPage = new SettingsPage(m_service, this);

    m_stack->addWidget(m_searchPage);      // 0
    m_stack->addWidget(m_bucketPage);      // 1
    m_stack->addWidget(m_installedPage);   // 2
    m_stack->addWidget(m_settingsPage);    // 3

    rightLayout->addWidget(m_stack, 1);
    rootLayout->addWidget(rightSide, 1);

    auto* central = new QWidget(this);
    central->setLayout(rootLayout);
    setCentralWidget(central);

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
    m_stack->setCurrentIndex(stackIdx, animate);

    if (m_activityBar) m_activityBar->setActive(pageIndex);
    m_currentPage = pageIndex;

    switch (pageIndex) {
    case 0: m_searchPage->onPageShown(); break;
    case 1: m_installedPage->onPageShown(); break;
    case 2: m_bucketPage->onPageShown(); break;
    case 3: m_settingsPage->onPageShown(); break;
    }
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

void MainWindow::updateMaximizeIcon() {
    if (!m_maxBtn) return;
    const bool max = isMaximized();
    const QColor c = Theme::textSub(false);
    m_maxBtn->setIcon(max ? IconPainter::restore(c, 14)
                          : IconPainter::maximize(c, 14));
    m_maxBtn->setToolTip(max ? tr("还原") : tr("最大化"));
}

// 窗口按钮 hover/关闭 hover 背景（自绘，非 QSS）
bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    const bool isWinBtn = (obj == m_minBtn || obj == m_maxBtn || obj == m_closeBtn);
    if (isWinBtn) {
        auto* btn = qobject_cast<QPushButton*>(obj);
        if (!btn) return QMainWindow::eventFilter(obj, event);
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            const bool hover = (event->type() == QEvent::Enter);
            const bool dark = ThemeManager::instance().isDark();
            const bool isClose = (obj == m_closeBtn);
            QColor bg;
            if (hover) {
                bg = isClose ? Theme::danger(dark) : Theme::highlight(dark);
            } else {
                bg = Qt::transparent;
            }
            QPalette pal = btn->palette();
            pal.setColor(QPalette::Button, bg);
            btn->setPalette(pal);
            btn->setAutoFillBackground(hover);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
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
