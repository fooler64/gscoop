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
#include <QFrame>

#include "core/scoop_service.h"
#include "core/settings_store.h"
#include "ui/animated_stack.h"
#include "ui/activity_bar.h"
#include "ui/theme.h"
#include "ui/search_page.h"
#include "ui/installed_page.h"
#include "ui/bucket_page.h"
#include "ui/settings_page.h"

MainWindow::MainWindow(ScoopService* service, QWidget* parent)
    : QMainWindow(parent), m_service(service) {
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
    resize(1120, 740);
    setMinimumSize(880, 580);

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
    // 活动栏按钮索引映射：0=search 1=installed 2=bucket 3=settings
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

void MainWindow::applyTheme(const QString& theme) {
    bool dark = true;
    if (theme == "light") dark = false;
    else if (theme == "system") {
        const QColor bg = palette().window().color();
        dark = (bg.lightness() < 128);
    }
    m_dark = dark;

    // 纯代码调色板（无 QSS）
    Theme::applyToApplication(dark);
    if (m_activityBar) m_activityBar->setDark(dark);

    // 标题栏背景（代码绘制）
    if (m_titleBar) {
        QPalette p = m_titleBar->palette();
        p.setColor(QPalette::Window,
                   dark ? QColor("#10161d") : QColor("#e8eef2"));
        m_titleBar->setPalette(p);
        m_titleBar->setAutoFillBackground(true);
    }
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
