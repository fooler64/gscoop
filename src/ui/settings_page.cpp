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

#include "core/scoop_service.h"
#include "core/settings_store.h"
#include "core/theme_manager.h"
#include "ui/theme.h"

SettingsPage::SettingsPage(ScoopService* service, QWidget* parent)
    : QWidget(parent), m_service(service) {
    setupUi();

    connect(&SettingsStore::instance(), &SettingsStore::settingsChanged,
            this, &SettingsPage::onSettingsChanged);
}

// 主题化 GroupBox：surface（白）背景 + border 边框，代码调色板（无 QSS）
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

void SettingsPage::setupUi() {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scroll);

    auto* container = new QWidget(scroll);
    scroll->setWidget(container);
    // 容器背景跟随主题（surface2 浅蓝，与窗口 bg 区分）
    QPalette cp = container->palette();
    cp.setColor(QPalette::Window, Theme::surface2(ThemeManager::instance().isDark()));
    container->setPalette(cp);
    container->setAutoFillBackground(true);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(32, 28, 32, 20);
    layout->setSpacing(16);

    auto* title = new QLabel(tr("设置"), container);
    title->setObjectName("sectionTitle");
    layout->addWidget(title);

    // ---- 通用 ----
    auto* generalBox = makeGroupBox(tr("通用"), container);
    auto* generalForm = new QFormLayout(generalBox);

    m_themeCombo = new QComboBox(generalBox);
    // 动态扫描 themes/*.json（甘雨/浅色/未来新增）
    const auto themes = ThemeManager::instance().availableThemes();
    for (const auto& t : themes) {
        m_themeCombo->addItem(t.name, t.id);
    }
    generalForm->addRow(tr("主题:"), m_themeCombo);

    m_languageCombo = new QComboBox(generalBox);
    m_languageCombo->addItem(tr("简体中文"), "zh-CN");
    m_languageCombo->addItem(tr("English"), "en-US");
    generalForm->addRow(tr("语言:"), m_languageCombo);

    m_launchPageCombo = new QComboBox(generalBox);
    m_launchPageCombo->addItem(tr("搜索"), "search");
    m_launchPageCombo->addItem(tr("Bucket"), "buckets");
    m_launchPageCombo->addItem(tr("已安装"), "installed");
    m_launchPageCombo->addItem(tr("设置"), "settings");
    generalForm->addRow(tr("启动页:"), m_launchPageCombo);

    m_autoUpdateCheck = new QCheckBox(tr("启动时检查更新"), generalBox);
    generalForm->addRow(QString(), m_autoUpdateCheck);

    m_showUpdateBanner = new QCheckBox(tr("显示更新提示条"), generalBox);
    generalForm->addRow(QString(), m_showUpdateBanner);

    layout->addWidget(generalBox);

    // ---- 托盘 ----
    auto* trayBox = makeGroupBox(tr("托盘"), container);
    auto* trayForm = new QFormLayout(trayBox);
    m_minimizeToTray = new QCheckBox(tr("最小化到托盘"), trayBox);
    trayForm->addRow(QString(), m_minimizeToTray);
    m_closeToTray = new QCheckBox(tr("关闭到托盘"), trayBox);
    trayForm->addRow(QString(), m_closeToTray);
    layout->addWidget(trayBox);

    // ---- 网络 ----
    auto* netBox = makeGroupBox(tr("网络"), container);
    auto* netForm = new QFormLayout(netBox);
    m_useProxy = new QCheckBox(tr("使用代理"), netBox);
    netForm->addRow(QString(), m_useProxy);
    m_proxyEdit = new QLineEdit(netBox);
    m_proxyEdit->setPlaceholderText(tr("http://127.0.0.1:7897"));
    netForm->addRow(tr("代理地址:"), m_proxyEdit);
    layout->addWidget(netBox);

    // ---- 关于 ----
    auto* aboutBox = makeGroupBox(tr("关于"), container);
    auto* aboutForm = new QFormLayout(aboutBox);
    m_scoopStatusLabel = new QLabel(aboutBox);
    aboutForm->addRow(tr("Scoop:"), m_scoopStatusLabel);
    aboutForm->addRow(tr("gScoop"), new QLabel(tr("C++/Qt 重构版 v0.1.0 · 甘雨主题"), aboutBox));
    layout->addWidget(aboutBox);

    layout->addStretch();

    // ---- 初始值 ----
    const AppSettings& s = SettingsStore::instance().settings();
    int themeIdx = m_themeCombo->findData(s.theme);
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);
    int langIdx = m_languageCombo->findData(s.language);
    if (langIdx >= 0) m_languageCombo->setCurrentIndex(langIdx);
    int pageIdx = m_launchPageCombo->findData(s.defaultLaunchPage);
    if (pageIdx >= 0) m_launchPageCombo->setCurrentIndex(pageIdx);
    m_autoUpdateCheck->setChecked(s.autoUpdateCheck);
    m_showUpdateBanner->setChecked(s.showUpdateBanner);
    m_minimizeToTray->setChecked(s.minimizeToTray);
    m_closeToTray->setChecked(s.closeToTray);
    m_useProxy->setChecked(s.useProxy);
    m_proxyEdit->setText(s.proxyUrl);

    // ---- 信号 ----
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        SettingsStore::instance().setTheme(m_themeCombo->itemData(idx).toString());
    });
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        SettingsStore::instance().setLanguage(m_languageCombo->itemData(idx).toString());
    });
    connect(m_launchPageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        SettingsStore::instance().setLaunchPage(m_launchPageCombo->itemData(idx).toString());
    });
    connect(m_autoUpdateCheck, &QCheckBox::toggled,
            &SettingsStore::instance(), &SettingsStore::setAutoUpdateCheck);
    connect(m_showUpdateBanner, &QCheckBox::toggled,
            &SettingsStore::instance(), &SettingsStore::setShowUpdateBanner);
    connect(m_minimizeToTray, &QCheckBox::toggled,
            &SettingsStore::instance(), &SettingsStore::setMinimizeToTray);
    connect(m_closeToTray, &QCheckBox::toggled,
            &SettingsStore::instance(), &SettingsStore::setCloseToTray);
    connect(m_useProxy, &QCheckBox::toggled,
            this, [this](bool on) {
        SettingsStore::instance().setProxy(m_proxyEdit->text(), on);
    });
    connect(m_proxyEdit, &QLineEdit::textChanged,
            this, [this](const QString& text) {
        SettingsStore::instance().setProxy(text, m_useProxy->isChecked());
    });
}

void SettingsPage::onSettingsChanged() {
    const AppSettings& s = SettingsStore::instance().settings();
    // 同步界面（避免循环信号）
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
    m_scoopStatusLabel->setText(
        m_service->isScoopInstalled()
            ? tr("已检测到 (%1)").arg(m_service->scoopPath())
            : tr("未检测到 Scoop"));
}
