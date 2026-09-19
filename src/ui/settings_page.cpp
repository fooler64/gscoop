// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/settings_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QListWidget>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QMessageBox>
#include <QFontMetrics>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QLayoutItem>
#include <QVariantAnimation>
#include <QTimer>
#include <QFileDialog>
#include <QDir>

#include "core/scoop_service.h"
#include "core/settings_store.h"
#include "core/i18n.h"
#include "ui/toggle_switch.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"
#include "ui/animated_stack.h"
#include "ui/doctor_page.h"   // DoctorItemCard

// ==================== SettingsTabButton ====================
SettingsTabButton::SettingsTabButton(const QString& text, const QString& icon, QWidget* parent)
    : QFrame(parent), m_text(text), m_icon(icon) {
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(42);
}

void SettingsTabButton::setActive(bool active) {
    // 复用一个动画对象（parent=this），避免频繁创建/销毁
    if (!m_activeAnim) {
        m_activeAnim = new QVariantAnimation(this);
        m_activeAnim->setDuration(160);
        m_activeAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_activeAnim, &QVariantAnimation::valueChanged, this,
                [this](const QVariant& v) {
            m_activeProgress = v.toDouble();
            update();
        });
    }
    m_activeAnim->stop();
    m_activeAnim->setStartValue(m_activeProgress);
    m_activeAnim->setEndValue(active ? 1.0 : 0.0);
    m_activeAnim->start();
}

void SettingsTabButton::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    emit clicked(property("tabIndex").toInt());
    QFrame::mousePressEvent(event);
}

void SettingsTabButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const bool dark = ThemeManager::instance().isDark();
    const bool hover = underMouse();

    // 背景：active 插值 active色 / hover 高亮
    QColor bg;
    if (m_activeProgress > 0.01) {
        bg = Theme::blend(Theme::highlight(dark), Theme::active(dark), m_activeProgress);
    } else if (hover) {
        bg = Theme::highlight(dark);
    } else {
        bg = Qt::transparent;
    }

    // 左侧指示条（宽度随进度生长）
    if (m_activeProgress > 0.01) {
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::accent(dark));
        const int barW = qMax(2, int(4 * m_activeProgress));
        p.drawRoundedRect(QRect(0, 8, barW, height() - 16), 2, 2);
    }

    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(QRect(8, 2, width() - 16, height() - 4), 8, 8);

    // 图标（emoji）
    QFont iconFont = font();
    iconFont.setPointSize(12);
    p.setFont(iconFont);
    p.setPen(Theme::text(dark));
    const QRect iconRect(14, 0, 28, height());
    p.drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter, m_icon);

    // 文字（选中时加粗，颜色过渡）
    QFont textFont = font();
    textFont.setPointSize(10);
    textFont.setBold(m_activeProgress > 0.5);
    p.setFont(textFont);
    const QColor textColor = Theme::blend(Theme::textSub(dark), Theme::text(dark), m_activeProgress);
    p.setPen(textColor);
    const QRect textRect(44, 0, width() - 52, height());
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_text);
}

// ==================== SettingsPage ====================
SettingsPage::SettingsPage(ScoopService* service, QWidget* parent)
    : QWidget(parent), m_service(service) {
    setupUi();

    connect(&SettingsStore::instance(), &SettingsStore::settingsChanged,
            this, &SettingsPage::onSettingsChanged);
}

QGroupBox* SettingsPage::makeGroupBox(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    const bool dark = ThemeManager::instance().isDark();
    QPalette bp = box->palette();
    bp.setColor(QPalette::Window, Theme::surface(dark));
    bp.setColor(QPalette::Base, Theme::surface(dark));
    bp.setColor(QPalette::Button, Theme::surface(dark));
    bp.setColor(QPalette::Text, Theme::text(dark));
    bp.setColor(QPalette::WindowText, Theme::text(dark));
    bp.setColor(QPalette::ButtonText, Theme::text(dark));
    box->setPalette(bp);
    box->setAutoFillBackground(true);
    return box;
}

QWidget* SettingsPage::makeTabPage() {
    auto* page = new QWidget(this);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(14);
    lay->addStretch(1);
    return page;
}

void SettingsPage::setupUi() {
    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ---- 左侧标签导航（竖排）----
    m_tabList = new QListWidget(this);
    m_tabList->setFixedWidth(170);
    m_tabList->setFrameShape(QFrame::NoFrame);
    m_tabList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tabList->setFocusPolicy(Qt::NoFocus);
    m_tabList->setSpacing(2);
    m_tabList->setContentsMargins(0, 20, 0, 0);

    const struct { QString icon; QString label; } kTabs[] = {
        {QString::fromUtf8("🤖"), tr("自动化")},
        {QString::fromUtf8("🧰"), tr("管理")},
        {QString::fromUtf8("🛡️"), tr("安全")},
        {QString::fromUtf8("🖥️"), tr("窗口")},
        {QString::fromUtf8("📌"), tr("托盘")},
        {QString::fromUtf8("ℹ️"), tr("关于")},
    };
    for (int i = 0; i < 6; ++i) {
        auto* item = new QListWidgetItem(m_tabList);
        item->setSizeHint(QSize(0, 42));
        item->setData(Qt::UserRole, i);
        auto* btn = new SettingsTabButton(kTabs[i].label, kTabs[i].icon, m_tabList);
        btn->setProperty("tabIndex", i);
        btn->setActive(i == 0);
        connect(btn, &SettingsTabButton::clicked, this, &SettingsPage::onTabChanged);
        m_tabList->setItemWidget(item, btn);
    }
    m_tabList->setCurrentRow(0);
    outer->addWidget(m_tabList);

    // ---- 右侧内容区（竖向滑动：与左侧竖直标签列表方向一致）----
    m_stack = new AnimatedStackedWidget(this);
    m_stack->setSlideAxis(Qt::Vertical);
    m_stack->setDuration(200);
    m_stack->setSlideExtent(1.0);

    auto* automationPage = makeTabPage();
    auto* managementPage = makeTabPage();
    auto* securityPage = makeTabPage();
    auto* windowPage = makeTabPage();
    auto* trayPage = makeTabPage();
    auto* aboutPage = makeTabPage();

    buildAutomationTab(automationPage);
    buildManagementTab(managementPage);
    buildSecurityTab(securityPage);
    buildWindowTab(windowPage);
    buildTrayTab(trayPage);
    buildAboutTab(aboutPage);

    m_stack->addWidget(automationPage);  // 0
    m_stack->addWidget(managementPage);  // 1
    m_stack->addWidget(securityPage);    // 2
    m_stack->addWidget(windowPage);      // 3
    m_stack->addWidget(trayPage);        // 4
    m_stack->addWidget(aboutPage);       // 5

    outer->addWidget(m_stack, 1);
}

void SettingsPage::buildAutomationTab(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());

    auto* title = new QLabel(tr("自动化"), page);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->insertWidget(0, title);

    auto* box = makeGroupBox(tr("更新"), page);
    auto* form = new QFormLayout(box);
    m_autoUpdateCheck = new ToggleSwitch(box);
    m_showUpdateBanner = new ToggleSwitch(box);
    form->addRow(tr("启动时检查更新:"), m_autoUpdateCheck);
    form->addRow(tr("显示更新提示条:"), m_showUpdateBanner);
    layout->insertWidget(1, box);

    auto* hint = new QLabel(tr("自动检查 gScoop 和相关工具是否有新版本。"), page);
    hint->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::textSub(false).name()));
    layout->insertWidget(2, hint);

    connect(m_autoUpdateCheck, &QAbstractButton::toggled,
            &SettingsStore::instance(), &SettingsStore::setAutoUpdateCheck);
    connect(m_showUpdateBanner, &QAbstractButton::toggled,
            &SettingsStore::instance(), &SettingsStore::setShowUpdateBanner);

    // ---- bucket 自动更新 ----
    auto* bBox = makeGroupBox(tr("Bucket 自动更新"), page);
    auto* bForm = new QFormLayout(bBox);
    m_autoBucketUpdateCheck = new ToggleSwitch(bBox);
    bForm->addRow(tr("定期自动更新 bucket 索引:"), m_autoBucketUpdateCheck);

    m_bucketUpdateInterval = new QComboBox(bBox);
    m_bucketUpdateInterval->addItem(tr("每 24 小时"), 24);
    m_bucketUpdateInterval->addItem(tr("每 7 天"), 168);
    bForm->addRow(tr("更新间隔:"), m_bucketUpdateInterval);

    m_bucketUpdateNowBtn = new QPushButton(tr("立即更新全部 Bucket"), bBox);
    m_bucketUpdateNowBtn->setIcon(IconPainter::refresh(Theme::text(false), 15));
    bForm->addRow(QString(), m_bucketUpdateNowBtn);
    layout->insertWidget(3, bBox);

    auto* bHint = new QLabel(tr("自动执行 scoop update 刷新 bucket 索引，保持软件信息最新。"), page);
    bHint->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::textSub(false).name()));
    layout->insertWidget(4, bHint);

    connect(m_autoBucketUpdateCheck, &QAbstractButton::toggled, this, [this](bool on) {
        SettingsStore::instance().setAutoBucketUpdate(
            on, m_bucketUpdateInterval->currentData().toInt());
    });
    connect(m_bucketUpdateInterval, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        SettingsStore::instance().setAutoBucketUpdate(
            m_autoBucketUpdateCheck->isChecked(),
            m_bucketUpdateInterval->currentData().toInt());
    });
    connect(m_bucketUpdateNowBtn, &QPushButton::clicked, this,
            &SettingsPage::onUpdateBucketsNow);
}

void SettingsPage::buildManagementTab(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());

    auto* title = new QLabel(tr("管理"), page);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->insertWidget(0, title);

    // ---- 环境自检 ----
    auto* doctorBox = makeGroupBox(tr("环境自检"), page);
    auto* doctorLay = new QVBoxLayout(doctorBox);

    auto* toolRow = new QHBoxLayout;
    m_runDoctorBtn = new QPushButton(tr("运行自检"), doctorBox);
    m_runDoctorBtn->setIcon(IconPainter::bug(Theme::text(false), 16));
    toolRow->addWidget(m_runDoctorBtn);
    m_cleanupAppsBtn = new QPushButton(tr("清理旧版本"), doctorBox);
    m_cleanupAppsBtn->setIcon(IconPainter::broom(Theme::text(false), 16));
    toolRow->addWidget(m_cleanupAppsBtn);
    m_cleanupCacheBtn = new QPushButton(tr("清理缓存"), doctorBox);
    m_cleanupCacheBtn->setIcon(IconPainter::trash(Theme::text(false), 16));
    toolRow->addWidget(m_cleanupCacheBtn);
    toolRow->addStretch();
    doctorLay->addLayout(toolRow);

    m_doctorStatusLabel = new QLabel(tr("点击\"运行自检\"检查环境"), doctorBox);
    m_doctorStatusLabel->setStyleSheet(QString("color:%1;").arg(Theme::textSub(false).name()));
    doctorLay->addWidget(m_doctorStatusLabel);

    // 结果容器（可滚动：结果多时可下滑）
    auto* doctorScroll = new QScrollArea(doctorBox);
    doctorScroll->setWidgetResizable(true);
    doctorScroll->setFrameShape(QFrame::NoFrame);
    doctorScroll->setMinimumHeight(150);
    doctorScroll->setMaximumHeight(320);
    m_doctorResultsHost = new QWidget(doctorScroll);
    m_doctorResultsLayout = new QVBoxLayout(m_doctorResultsHost);
    m_doctorResultsLayout->setContentsMargins(0, 0, 0, 0);
    m_doctorResultsLayout->setSpacing(6);
    m_doctorResultsLayout->setAlignment(Qt::AlignTop);
    doctorScroll->setWidget(m_doctorResultsHost);
    doctorLay->addWidget(doctorScroll);

    layout->insertWidget(1, doctorBox);

    // 一键修复按钮（追加到工具行）
    m_fixAllBtn = new QPushButton(tr("一键修复"), doctorBox);
    m_fixAllBtn->setIcon(IconPainter::shield(Theme::text(false), 16));
    m_fixAllBtn->setToolTip(tr("自动修复：安装缺失的 git/7zip、补建 main bucket"));
    m_fixAllBtn->setProperty("primary", true);
    toolRow->insertWidget(3, m_fixAllBtn);
    toolRow->removeItem(toolRow->itemAt(toolRow->count() - 1));  // 移除 stretch

    connect(m_runDoctorBtn, &QPushButton::clicked, this, &SettingsPage::onRunDoctor);
    connect(m_cleanupAppsBtn, &QPushButton::clicked, this, &SettingsPage::onCleanupApps);
    connect(m_cleanupCacheBtn, &QPushButton::clicked, this, &SettingsPage::onCleanupCache);
    connect(m_fixAllBtn, &QPushButton::clicked, this, &SettingsPage::onFixAll);

    // ---- 配置导出 / 导入 ----
    auto* cfgBox = makeGroupBox(tr("配置备份"), page);
    auto* cfgForm = new QFormLayout(cfgBox);
    auto* cfgRow = new QHBoxLayout;
    m_exportBtn = new QPushButton(tr("导出配置"), cfgBox);
    m_exportBtn->setToolTip(tr("导出已安装软件列表为 JSON（scoop export）"));
    m_importBtn = new QPushButton(tr("导入配置"), cfgBox);
    m_importBtn->setToolTip(tr("从 JSON 文件还原软件列表（scoop import）"));
    cfgRow->addWidget(m_exportBtn);
    cfgRow->addWidget(m_importBtn);
    cfgRow->addStretch();
    cfgForm->addRow(cfgRow);
    auto* cfgHint = new QLabel(tr("换机时导出配置，新机器导入即可一键还原全部软件。"), cfgBox);
    cfgHint->setWordWrap(true);
    cfgHint->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::textSub(false).name()));
    cfgForm->addRow(cfgHint);
    layout->insertWidget(2, cfgBox);

    connect(m_exportBtn, &QPushButton::clicked, this, &SettingsPage::onExportConfig);
    connect(m_importBtn, &QPushButton::clicked, this, &SettingsPage::onImportConfig);
}

void SettingsPage::buildSecurityTab(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());

    auto* title = new QLabel(tr("安全"), page);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->insertWidget(0, title);

    // ---- VirusTotal ----
    auto* vtBox = makeGroupBox(tr("VirusTotal"), page);
    auto* vtForm = new QFormLayout(vtBox);
    m_vtApiKeyEdit = new QLineEdit(vtBox);
    m_vtApiKeyEdit->setPlaceholderText(tr("输入 VirusTotal API key（可选）"));
    m_vtApiKeyEdit->setEchoMode(QLineEdit::Password);
    vtForm->addRow(tr("API Key:"), m_vtApiKeyEdit);
    layout->insertWidget(1, vtBox);

    connect(m_vtApiKeyEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        SettingsStore::instance().setVirusTotalApiKey(text.trimmed());
    });

    auto* vtHint = new QLabel(
        tr("配置 API key 后可在包信息中调用 scoop virustotal 查毒。\n"
           "需要先安装扩展：scoop install virustotal"), vtBox);
    vtHint->setWordWrap(true);
    vtHint->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::textSub(false).name()));
    layout->insertWidget(2, vtHint);

    // ---- 网络代理 ----
    auto* netBox = makeGroupBox(tr("网络"), page);
    auto* netForm = new QFormLayout(netBox);
    m_useProxy = new ToggleSwitch(netBox);
    netForm->addRow(tr("使用代理:"), m_useProxy);
    m_proxyEdit = new QLineEdit(netBox);
    m_proxyEdit->setPlaceholderText(tr("http://127.0.0.1:7897"));
    netForm->addRow(tr("代理地址:"), m_proxyEdit);
    layout->insertWidget(3, netBox);

    connect(m_useProxy, &QAbstractButton::toggled, this, [this](bool on) {
        SettingsStore::instance().setProxy(m_proxyEdit->text(), on);
    });
    connect(m_proxyEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        SettingsStore::instance().setProxy(text, m_useProxy->isChecked());
    });
}

void SettingsPage::buildWindowTab(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());

    auto* title = new QLabel(tr("窗口"), page);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->insertWidget(0, title);

    auto* box = makeGroupBox(tr("外观与启动"), page);
    auto* form = new QFormLayout(box);
    form->setSpacing(12);

    // 主题
    m_themeCombo = new QComboBox(box);
    const auto themes = ThemeManager::instance().availableThemes();
    for (const auto& t : themes) {
        m_themeCombo->addItem(t.name, t.id);
    }
    form->addRow(tr("主题:"), m_themeCombo);

    // 语言
    m_languageCombo = new QComboBox(box);
    m_languageCombo->addItem(tr("简体中文"), "zh-CN");
    m_languageCombo->addItem(tr("English"), "en-US");
    form->addRow(tr("语言:"), m_languageCombo);

    // 启动页
    m_launchPageCombo = new QComboBox(box);
    m_launchPageCombo->addItem(tr("搜索"), "search");
    m_launchPageCombo->addItem(tr("已安装"), "installed");
    m_launchPageCombo->addItem(tr("Bucket"), "buckets");
    m_launchPageCombo->addItem(tr("设置"), "settings");
    form->addRow(tr("启动页:"), m_launchPageCombo);

    layout->insertWidget(1, box);

    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        SettingsStore::instance().setTheme(m_themeCombo->itemData(idx).toString());
    });
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        const QString lang = m_languageCombo->itemData(idx).toString();
        SettingsStore::instance().setLanguage(lang);
        I18n::instance().applyLanguage(lang);
        QMessageBox::information(this, tr("语言"),
            tr("语言已切换为 %1。\n部分界面需要重启 gScoop 后完全生效。")
                .arg(m_languageCombo->currentText()));
    });
    connect(m_launchPageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        SettingsStore::instance().setLaunchPage(m_launchPageCombo->itemData(idx).toString());
    });
}

void SettingsPage::buildTrayTab(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());

    auto* title = new QLabel(tr("托盘"), page);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->insertWidget(0, title);

    auto* box = makeGroupBox(tr("系统托盘"), page);
    auto* form = new QFormLayout(box);
    m_minimizeToTray = new ToggleSwitch(box);
    m_closeToTray = new ToggleSwitch(box);
    form->addRow(tr("最小化到托盘:"), m_minimizeToTray);
    form->addRow(tr("关闭到托盘:"), m_closeToTray);
    layout->insertWidget(1, box);

    auto* hint = new QLabel(tr("启用后，关闭/最小化窗口时程序将继续在系统托盘运行。"), page);
    hint->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::textSub(false).name()));
    layout->insertWidget(2, hint);

    connect(m_minimizeToTray, &QAbstractButton::toggled,
            &SettingsStore::instance(), &SettingsStore::setMinimizeToTray);
    connect(m_closeToTray, &QAbstractButton::toggled,
            &SettingsStore::instance(), &SettingsStore::setCloseToTray);
}

void SettingsPage::buildAboutTab(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());

    auto* title = new QLabel(tr("关于"), page);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->insertWidget(0, title);

    auto* box = makeGroupBox(tr("gScoop"), page);
    auto* form = new QFormLayout(box);
    form->setSpacing(12);

    m_scoopStatusLabel = new QLabel(box);
    form->addRow(tr("Scoop:"), m_scoopStatusLabel);
    form->addRow(tr("版本:"), new QLabel(tr("gScoop v0.1.0（C++/Qt 重构版）"), box));
    form->addRow(tr("主题:"), new QLabel(tr("甘雨 · 蓝白冰系"), box));
    form->addRow(tr("开源:"), new QLabel(tr("复刻自 AmarBego/Rscoop 的 C++/Qt 实现"), box));
    layout->insertWidget(1, box);
}

void SettingsPage::onTabChanged(int index) {
    m_stack->setCurrentIndex(index, true);   // 带淡入淡出动画
    for (int i = 0; i < m_tabList->count(); ++i) {
        if (auto* w = m_tabList->itemWidget(m_tabList->item(i))) {
            if (auto* btn = qobject_cast<SettingsTabButton*>(w)) {
                btn->setActive(i == index);
            }
        }
    }
}

// 一键修复：根据自检结果自动安装缺失依赖 / 补建 bucket（真正执行）
void SettingsPage::onFixAll() {
    if (QMessageBox::question(this, tr("一键修复"),
            tr("将自动执行以下修复：\n"
               "· 缺少 git → scoop install git\n"
               "· 缺少 7zip → scoop install 7zip\n"
               "· 缺少 main bucket → scoop bucket add main\n\n"
               "是否继续？")) != QMessageBox::Yes) return;

    if (m_service->isBusy()) {
        QMessageBox::information(this, tr("提示"), tr("已有操作在进行中，请等待完成。"));
        return;
    }

    const QVector<DoctorCheckItem> items = m_service->runDoctor();
    QStringList installs;
    bool needMain = false;
    for (const auto& it : items) {
        if (it.passed) continue;
        if (it.title.contains("Git", Qt::CaseInsensitive)) installs << "git";
        else if (it.title.contains("7zip")) installs << "7zip";
        else if (it.title.contains("Main bucket")) needMain = true;
    }

    if (installs.isEmpty() && !needMain) {
        m_doctorStatusLabel->setText(tr("没有可自动修复的问题（其余项需手动处理）"));
        emit logRequested(tr("一键修复"), tr("未发现可自动修复的问题。\n"));
        return;
    }

    QString log = tr("开始自动修复…\n");
    if (!installs.isEmpty()) log += tr("安装缺失依赖：%1\n").arg(installs.join(", "));
    if (needMain) log += tr("补建 main bucket\n");
    emit logRequested(tr("一键修复"), log);

    // 真正执行：缺失依赖走批量安装队列，main bucket 单独添加
    if (!installs.isEmpty()) m_service->installPackages(installs);
    if (needMain) m_service->addBucket("main", QString());

    m_doctorStatusLabel->setText(tr("已提交修复，请在底部日志查看进度"));
    // 稍后刷新自检结果
    QTimer::singleShot(3000, this, [this]() { onRunDoctor(); });
}

// 导出配置
void SettingsPage::onExportConfig() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出配置"), QDir::homePath() + "/gscoop-export.json",
        tr("JSON 文件 (*.json)"));
    if (path.isEmpty()) return;
    m_service->exportConfig(path);
    m_doctorStatusLabel->setText(tr("已导出配置到 %1").arg(path));
    QMessageBox::information(this, tr("导出成功"), tr("配置已导出到：\n%1").arg(path));
}

// 导入配置
void SettingsPage::onImportConfig() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("导入配置"), QDir::homePath(), tr("JSON 文件 (*.json)"));
    if (path.isEmpty()) return;
    if (QMessageBox::question(this, tr("导入配置"),
            tr("将从该文件还原软件列表并开始安装：\n%1\n\n是否继续？").arg(path))
        != QMessageBox::Yes) return;
    m_service->importConfig(path);
    m_doctorStatusLabel->setText(tr("正在导入配置，请在底部日志查看进度"));
}

// 立即刷新全部 bucket
void SettingsPage::onUpdateBucketsNow() {
    m_service->updateAllBuckets();
    m_doctorStatusLabel->setText(tr("正在刷新全部 bucket…"));
}

void SettingsPage::onRunDoctor() {
    m_doctorStatusLabel->setText(tr("正在检查..."));
    m_runDoctorBtn->setEnabled(false);
    emit logRequested(tr("环境自检"), tr("开始检查环境…\n"));

    QFuture<QVector<DoctorCheckItem>> future = QtConcurrent::run([this]() {
        return m_service->runDoctor();
    });
    auto* watcher = new QFutureWatcher<QVector<DoctorCheckItem>>(this);
    connect(watcher, &QFutureWatcher<QVector<DoctorCheckItem>>::finished, this, [this, watcher]() {
        const QVector<DoctorCheckItem> items = watcher->result();
        populateDoctorResults(items);
        m_runDoctorBtn->setEnabled(true);
        watcher->deleteLater();

        // 结果写入底部日志
        QString log;
        int passed = 0;
        for (const auto& it : items) {
            if (it.passed) ++passed;
            const QString state = it.passed ? tr("通过") : (it.warning ? tr("警告") : tr("失败"));
            log += QString("[%1] %2").arg(state, it.title);
            if (!it.detail.isEmpty()) log += QString("  (%1)").arg(it.detail);
            log += "\n";
        }
        log += tr("自检完成：%1/%2 通过").arg(passed).arg(items.size());
        emit logRequested(tr("环境自检"), log);
    });
    watcher->setFuture(future);
}

void SettingsPage::populateDoctorResults(const QVector<DoctorCheckItem>& items) {
    while (QLayoutItem* item = m_doctorResultsLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    int passed = 0;
    for (const auto& item : items) {
        if (item.passed) ++passed;
        m_doctorResultsLayout->addWidget(new DoctorItemCard(item, m_doctorResultsHost));
    }
    m_doctorStatusLabel->setText(tr("自检完成：%1/%2 通过").arg(passed).arg(items.size()));
}

void SettingsPage::onCleanupApps() {
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("清理旧版本"), tr("确定要运行 scoop cleanup 清理所有应用的旧版本吗？"));
    if (res == QMessageBox::Yes) {
        m_service->cleanupApps();
        m_doctorStatusLabel->setText(tr("正在清理旧版本..."));
    }
}

void SettingsPage::onCleanupCache() {
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("清理缓存"), tr("确定要清空 scoop 下载缓存吗？"));
    if (res == QMessageBox::Yes) {
        m_service->cleanupCache();
        m_doctorStatusLabel->setText(tr("正在清理缓存..."));
    }
}

void SettingsPage::onSettingsChanged() {
    const AppSettings& s = SettingsStore::instance().settings();
    QSignalBlocker b1(m_themeCombo);
    QSignalBlocker b2(m_languageCombo);
    QSignalBlocker b3(m_launchPageCombo);
    int ti = m_themeCombo->findData(s.theme);
    if (ti >= 0) m_themeCombo->setCurrentIndex(ti);
    int li = m_languageCombo->findData(s.language);
    if (li >= 0) m_languageCombo->setCurrentIndex(li);
    int pi = m_launchPageCombo->findData(s.defaultLaunchPage);
    if (pi >= 0) m_launchPageCombo->setCurrentIndex(pi);
}

void SettingsPage::onPageShown() {
    // 同步所有设置项到界面
    const AppSettings& s = SettingsStore::instance().settings();
    m_autoUpdateCheck->setChecked(s.autoUpdateCheck);
    m_showUpdateBanner->setChecked(s.showUpdateBanner);
    m_minimizeToTray->setChecked(s.minimizeToTray);
    m_closeToTray->setChecked(s.closeToTray);
    m_useProxy->setChecked(s.useProxy);
    m_proxyEdit->setText(s.proxyUrl);
    m_vtApiKeyEdit->setText(s.virusTotalApiKey);
    if (m_autoBucketUpdateCheck) {
        m_autoBucketUpdateCheck->setChecked(s.autoBucketUpdate);
        const int idx = m_bucketUpdateInterval->findData(s.bucketUpdateHours);
        if (idx >= 0) m_bucketUpdateInterval->setCurrentIndex(idx);
    }
    m_scoopStatusLabel->setText(
        m_service->isScoopInstalled()
            ? tr("已检测到 (%1)").arg(m_service->scoopPath())
            : tr("未检测到 Scoop"));
}
