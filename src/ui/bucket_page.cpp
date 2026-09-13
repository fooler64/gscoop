// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/bucket_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QMenu>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QPen>
#include <QSizePolicy>
#include <QMouseEvent>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"
#include "ui/add_bucket_dialog.h"
#include "ui/explore_bucket_dialog.h"
#include "ui/bucket_info_dialog.h"

// 预置常用 buckets（GitHub 官方地址）
static const QVector<PresetBucket> kPresets = {
    {"main",        "https://github.com/ScoopInstaller/Main",        "Scoop 官方主库（必备）"},
    {"extras",      "https://github.com/ScoopInstaller/Extras",      "扩展软件库"},
    {"versions",    "https://github.com/ScoopInstaller/Versions",    "历史/多版本软件"},
    {"nirsoft",     "https://github.com/ScoopInstaller/Nirsoft",     "NirSoft 系统工具"},
    {"sysinternals", "https://github.com/ScoopInstaller/Sysinternals", "Sysinternals 工具"},
    {"php",         "https://github.com/ScoopInstaller/PHP",         "PHP 版本"},
    {"java",        "https://github.com/ScoopInstaller/Java",        "Java 各版本"},
    {"games",       "https://github.com/Calinou/scoop-games",        "游戏"},
    {"nonportable", "https://github.com/ScoopInstaller/Nonportable", "非便携软件"},
    {"nerd-fonts",  "https://github.com/matthewjberger/scoop-nerd-fonts", "Nerd Fonts 字体"},
    {"extras-cn",   "https://github.com/Scoopforge/Extras-CN.git",   "中国软件库（微信/QQ等）"},
    {"cluttered-bucket", "https://github.com/Paxxs/Cluttered-bucket.git", "常用杂项"},
};

const QVector<PresetBucket>& BucketPage::presets() {
    return kPresets;
}

// ==================== BucketCard ====================
BucketCard::BucketCard(const QString& name, const QString& url, const QString& desc,
                       bool installed, QWidget* parent)
    : QFrame(parent), m_name(name), m_url(url), m_installed(installed) {
    setCursor(Qt::PointingHandCursor);
    setToolTip(url);
    setMinimumHeight(52);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 内部布局：两个 QLabel（QLabel 富文本渲染可靠）
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(12, 8, 12, 8);
    lay->setSpacing(2);

    // QLabel 不拦截鼠标事件（点击穿透到卡片，触发 clicked）
    auto transparentForMouse = [](QLabel* lbl) {
        lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    };

    const bool dark = ThemeManager::instance().isDark();
    auto* nameLbl = new QLabel(name, this);
    QFont nf = nameLbl->font();
    nf.setBold(true);
    nameLbl->setFont(nf);
    nameLbl->setStyleSheet(QString("color:%1;").arg(
        (installed ? Theme::success(dark) : Theme::text(dark)).name()));
    transparentForMouse(nameLbl);

    auto* descLbl = new QLabel(installed ? tr("✓ 已添加") : desc, this);
    QFont df = descLbl->font();
    df.setPointSizeF(df.pointSizeF() - 1.5);
    descLbl->setFont(df);
    descLbl->setStyleSheet(QString("color:%1;").arg(
        (installed ? Theme::success(dark) : Theme::textSub(dark)).name()));
    transparentForMouse(descLbl);

    lay->addWidget(nameLbl);
    lay->addWidget(descLbl);
}

void BucketCard::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    emit clicked(m_name, m_url);
    QFrame::mousePressEvent(event);
}

void BucketCard::enterEvent(QEnterEvent* event) {
    m_hover = true;
    update();
    QFrame::enterEvent(event);
}

void BucketCard::leaveEvent(QEvent* event) {
    m_hover = false;
    update();
    QFrame::leaveEvent(event);
}

void BucketCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    const bool dark = ThemeManager::instance().isDark();
    const QColor base = m_installed ? Theme::surface3(dark) : Theme::surface2(dark);
    QColor bg = m_hover ? Theme::blend(base, Theme::accent(dark), 0.08) : base;
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(m_installed
                      ? Theme::success(dark)
                      : Theme::border(dark), 1));
    p.setBrush(bg);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
}

// ==================== InstalledBucketCard ====================
InstalledBucketCard::InstalledBucketCard(const BucketInfo& bucket, QWidget* parent)
    : QFrame(parent), m_bucket(bucket) {
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(m_bucket.git_url);

    // 内部布局：名称 + manifest 数 + URL
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 10, 14, 10);
    lay->setSpacing(3);

    const bool dark = ThemeManager::instance().isDark();

    // 所有子 QLabel 不拦截鼠标事件（点击穿透到卡片本身，处理点击逻辑）
    auto transparentForMouse = [](QLabel* lbl) {
        lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    };

    auto* nameRow = new QHBoxLayout;
    auto* nameLbl = new QLabel(m_bucket.name, this);
    QFont nf = nameLbl->font();
    nf.setBold(true);
    nf.setPointSizeF(nf.pointSizeF() + 0.5);
    nameLbl->setFont(nf);
    nameLbl->setStyleSheet(QString("color:%1;").arg(Theme::text(dark).name()));
    transparentForMouse(nameLbl);
    nameRow->addWidget(nameLbl);

    auto* countLbl = new QLabel(tr("%1 manifests").arg(m_bucket.manifest_count), this);
    countLbl->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::success(dark).name()));
    transparentForMouse(countLbl);
    nameRow->addWidget(countLbl);
    nameRow->addStretch();

    // 删除图标（右上角，自绘）
    auto* trashBtn = new QLabel(this);
    trashBtn->setPixmap(IconPainter::trash(Theme::textSub(dark), 14).pixmap(14, 14));
    trashBtn->setToolTip(tr("删除此 bucket"));
    transparentForMouse(trashBtn);
    nameRow->addWidget(trashBtn);

    lay->addLayout(nameRow);

    auto* urlLbl = new QLabel(m_bucket.git_url.isEmpty() ? m_bucket.path : m_bucket.git_url, this);
    QFont uf = urlLbl->font();
    uf.setPointSizeF(uf.pointSizeF() - 1.5);
    urlLbl->setFont(uf);
    urlLbl->setStyleSheet(QString("color:%1;").arg(Theme::textSub(dark).name()));
    urlLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    transparentForMouse(urlLbl);
    lay->addWidget(urlLbl);
}

void InstalledBucketCard::mousePressEvent(QMouseEvent* event) {
    // 点删除图标区域（右上角 ~28px）→ 删除；其他 → 查看介绍
    const QPoint pos = event->pos();
    const bool onTrash = (pos.x() > width() - 34 && pos.y() < 34);
    if (onTrash) {
        emit removeRequested(m_bucket.name);
    } else {
        emit infoRequested(m_bucket.name);
    }
    QFrame::mousePressEvent(event);
}

void InstalledBucketCard::enterEvent(QEnterEvent* event) {
    m_hover = true;
    update();
    QFrame::enterEvent(event);
}

void InstalledBucketCard::leaveEvent(QEvent* event) {
    m_hover = false;
    update();
    QFrame::leaveEvent(event);
}

void InstalledBucketCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const bool dark = ThemeManager::instance().isDark();
    // 圆角无边框：bg=surface3（比页面背景深一档，卡片感更强），hover 时 blend accent
    QColor bg = Theme::surface3(dark);
    if (m_hover) bg = Theme::blend(bg, Theme::accent(dark), 0.1);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);
}

BucketPage::BucketPage(ScoopService* service, QWidget* parent)
    : QWidget(parent), m_service(service) {
    setupUi();

    connect(m_service, &ScoopService::bucketsLoaded,
            this, &BucketPage::onBucketsLoaded);
    connect(m_service, &ScoopService::opFinished,
            this, &BucketPage::onOpFinished);
}

void BucketPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 20);
    layout->setSpacing(16);

    // ---- 标题行 ----
    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(tr("Bucket 管理"), this);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    titleRow->addWidget(title);
    titleRow->addStretch();

    // 探索新仓库
    m_exploreBtn = new QPushButton(this);
    m_exploreBtn->setIcon(IconPainter::globe(Theme::text(false), 16));
    m_exploreBtn->setText(tr("探索新仓库"));
    m_exploreBtn->setToolTip(tr("搜索 GitHub 上的 bucket 仓库"));
    m_exploreBtn->setMinimumHeight(32);
    titleRow->addWidget(m_exploreBtn);

    // 添加 Bucket
    m_addBtn = new QPushButton(this);
    m_addBtn->setIcon(IconPainter::plus(Theme::text(false), 16));
    m_addBtn->setText(tr("添加 Bucket"));
    m_addBtn->setToolTip(tr("添加单个或批量添加 bucket"));
    m_addBtn->setMinimumHeight(32);
    titleRow->addWidget(m_addBtn);

    // 镜像切换
    titleRow->addWidget(new QLabel(tr("下载镜像:"), this));
    m_mirrorCombo = new QComboBox(this);
    m_mirrorCombo->addItem(tr("GitHub 直连"), QString());
    m_mirrorCombo->addItem(tr("ghproxy.com"), QString("https://ghproxy.com/"));
    m_mirrorCombo->addItem(tr("gh-proxy.com"), QString("https://gh-proxy.com/"));
    m_mirrorCombo->addItem(tr("ghfast.top"), QString("https://ghfast.top/"));
    m_mirrorCombo->setMinimumWidth(140);
    titleRow->addWidget(m_mirrorCombo);

    // 刷新按钮移到"已添加的 Buckets"标题行（标题行空间不足）
    layout->addLayout(titleRow);

    // ---- 预置 buckets 网格（卡片）----
    auto* presetTitle = new QLabel(tr("常用 Buckets（点击一键添加）"), this);
    QFont pf = presetTitle->font();
    pf.setBold(true);
    presetTitle->setFont(pf);
    layout->addWidget(presetTitle);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* scrollInner = new QWidget(scroll);
    m_presetLayout = new QGridLayout(scrollInner);
    m_presetLayout->setContentsMargins(0, 0, 0, 0);
    m_presetLayout->setSpacing(10);
    scroll->setWidget(scrollInner);
    scroll->setFixedHeight(220);
    layout->addWidget(scroll);

    // ---- 已安装 buckets 列表 ----
    auto* installedRow = new QHBoxLayout;
    auto* installedTitle = new QLabel(tr("已添加的 Buckets"), this);
    QFont ipf = installedTitle->font();
    ipf.setBold(true);
    installedTitle->setFont(ipf);
    installedRow->addWidget(installedTitle);
    installedRow->addStretch();

    // 刷新按钮（放这里空间充足）
    m_refreshBtn = new QPushButton(this);
    m_refreshBtn->setIcon(IconPainter::refresh(Theme::text(false), 16));
    m_refreshBtn->setText(tr("刷新"));
    m_refreshBtn->setToolTip(tr("刷新 bucket 列表"));
    m_refreshBtn->setMinimumHeight(30);
    installedRow->addWidget(m_refreshBtn);
    layout->addLayout(installedRow);

    m_countLabel = new QLabel(tr("0 个"), this);
    layout->addWidget(m_countLabel);

    // 已安装 buckets 卡片网格（滚动区域）
    m_installedScroll = new QScrollArea(this);
    m_installedScroll->setWidgetResizable(true);
    m_installedScroll->setFrameShape(QFrame::NoFrame);
    m_installedHost = new QWidget(m_installedScroll);
    m_installedLayout = new QGridLayout(m_installedHost);
    m_installedLayout->setContentsMargins(0, 0, 0, 0);
    m_installedLayout->setSpacing(10);
    m_installedLayout->setAlignment(Qt::AlignTop);
    m_installedScroll->setWidget(m_installedHost);
    layout->addWidget(m_installedScroll, 1);

    m_removeBtn = new QPushButton(tr("删除选中 Bucket"), this);
    layout->addWidget(m_removeBtn, 0, Qt::AlignRight);

    // 信号
    connect(m_mirrorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BucketPage::onMirrorChanged);
    connect(m_removeBtn, &QPushButton::clicked, this, &BucketPage::onRemoveBucket);
    connect(m_refreshBtn, &QPushButton::clicked, this, &BucketPage::onRefreshClicked);
    connect(m_addBtn, &QPushButton::clicked, this, &BucketPage::onAddBucketClicked);
    connect(m_exploreBtn, &QPushButton::clicked, this, &BucketPage::onExploreClicked);

    // 默认镜像
    m_mirror.clear();
}

QString BucketPage::mirrorUrl(const QString& githubUrl) const {
    if (m_mirror.isEmpty()) return githubUrl;
    // 去掉末尾 .git 再拼镜像前缀（scoop bucket add 需要 .git 结尾，镜像通常保留）
    if (githubUrl.endsWith(".git")) {
        return m_mirror + githubUrl;
    }
    return m_mirror + githubUrl;
}

void BucketPage::onMirrorChanged(int idx) {
    m_mirror = m_mirrorCombo->itemData(idx).toString();
    rebuildPresetGrid();
}

void BucketPage::rebuildPresetGrid() {
    if (!m_presetLayout) return;
    // 清空旧按钮
    while (QLayoutItem* item = m_presetLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    const int cols = 3;
    for (int i = 0; i < kPresets.size(); ++i) {
        const PresetBucket& p = kPresets[i];
        const bool installed = m_installedNames.contains(p.name);

        auto* card = new BucketCard(p.name, p.url, p.desc, installed, this);
        QObject::connect(card, &BucketCard::clicked, this,
                         [this](const QString& name, const QString& url) {
            onAddPreset(name, url);
        });

        m_presetLayout->addWidget(card, i / cols, i % cols);
    }
}

void BucketPage::onBucketsLoaded(QVector<BucketInfo> buckets) {
    m_buckets = buckets;
    m_installedNames.clear();
    for (const BucketInfo& b : buckets) {
        m_installedNames.insert(b.name);
    }
    populateInstalledList(buckets);
    m_countLabel->setText(tr("%1 个").arg(buckets.size()));
    rebuildPresetGrid();
}

void BucketPage::populateInstalledList(const QVector<BucketInfo>& buckets) {
    if (!m_installedLayout) return;
    // 清空旧卡片
    while (QLayoutItem* item = m_installedLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    const int cols = qMax(1, 2);
    for (int i = 0; i < buckets.size(); ++i) {
        auto* card = new InstalledBucketCard(buckets[i], m_installedHost);
        QObject::connect(card, &InstalledBucketCard::infoRequested, this,
                         [this](const QString& name) {
            // 点击卡片主体 → 查看 bucket 介绍
            for (const auto& b : m_buckets) {
                if (b.name == name) {
                    BucketInfoDialog dlg(m_service, b, this);
                    connect(&dlg, &BucketInfoDialog::removeRequested, this,
                            [this](const QString& rname) {
                        m_pendingRemove = rname;
                        onRemoveBucket();
                    });
                    dlg.exec();
                    break;
                }
            }
        });
        QObject::connect(card, &InstalledBucketCard::removeRequested, this,
                         [this](const QString& name) {
            // 点击删除图标 → 直接请求删除
            m_pendingRemove = name;
            onRemoveBucket();
        });
        m_installedLayout->addWidget(card, i / cols, i % cols);
    }
    m_installedLayout->setColumnStretch(cols - 1, 1);
    m_installedHost->updateGeometry();
}

void BucketPage::onAddPreset(const QString& name, const QString& url) {
    if (m_installedNames.contains(name)) {
        QMessageBox::information(this, tr("提示"), tr("Bucket \"%1\" 已添加。").arg(name));
        return;
    }
    const QString target = mirrorUrl(url);
    m_service->addBucket(name, target);
}

void BucketPage::onRemoveBucket() {
    const QString name = m_pendingRemove;
    if (name.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一个 bucket。"));
        return;
    }
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("删除 Bucket"), tr("确定要删除 bucket \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) m_service->removeBucket(name);
    m_pendingRemove.clear();
}

void BucketPage::onRefreshClicked() {
    m_service->fetchBuckets();
}

void BucketPage::onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error) {
    if (type == ScoopOpType::BucketAdd || type == ScoopOpType::BucketRemove) {
        m_service->fetchBuckets();
        if (!success) {
            QMessageBox::warning(this, tr("操作失败"), tr("bucket 操作失败：%1").arg(error));
        }
    }
    Q_UNUSED(package);
}

void BucketPage::onPageShown() {
    if (!m_loaded) {
        m_loaded = true;
        m_service->fetchBuckets();
        rebuildPresetGrid();
    } else {
        m_service->fetchBuckets();
    }
}

void BucketPage::onAddBucketClicked() {
    AddBucketDialog dlg(m_service, this);
    dlg.exec();
    // 添加后刷新
    m_service->fetchBuckets();
}

void BucketPage::onExploreClicked() {
    // 探索新仓库弹窗（独立实现）
    ExploreBucketDialog dlg(m_service, this);
    dlg.exec();
    m_service->fetchBuckets();
}
