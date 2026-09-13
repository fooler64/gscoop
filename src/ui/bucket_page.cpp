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

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"

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

    const bool dark = ThemeManager::instance().isDark();
    auto* nameLbl = new QLabel(name, this);
    QFont nf = nameLbl->font();
    nf.setBold(true);
    nameLbl->setFont(nf);
    nameLbl->setStyleSheet(QString("color:%1;").arg(
        (installed ? Theme::success(dark) : Theme::text(dark)).name()));

    auto* descLbl = new QLabel(installed ? tr("✓ 已添加") : desc, this);
    QFont df = descLbl->font();
    df.setPointSizeF(df.pointSizeF() - 1.5);
    descLbl->setFont(df);
    descLbl->setStyleSheet(QString("color:%1;").arg(
        (installed ? Theme::success(dark) : Theme::textSub(dark)).name()));

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

    // 镜像切换
    titleRow->addWidget(new QLabel(tr("下载镜像:"), this));
    m_mirrorCombo = new QComboBox(this);
    m_mirrorCombo->addItem(tr("GitHub 直连"), QString());
    m_mirrorCombo->addItem(tr("ghproxy.com"), QString("https://ghproxy.com/"));
    m_mirrorCombo->addItem(tr("gh-proxy.com"), QString("https://gh-proxy.com/"));
    m_mirrorCombo->addItem(tr("ghfast.top"), QString("https://ghfast.top/"));
    m_mirrorCombo->setMinimumWidth(140);
    titleRow->addWidget(m_mirrorCombo);

    m_refreshBtn = new QPushButton(tr("刷新"), this);
    titleRow->addWidget(m_refreshBtn);
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
    auto* installedTitle = new QLabel(tr("已添加的 Buckets"), this);
    QFont ipf = installedTitle->font();
    ipf.setBold(true);
    installedTitle->setFont(ipf);
    layout->addWidget(installedTitle);

    m_countLabel = new QLabel(tr("0 个"), this);
    layout->addWidget(m_countLabel);

    m_list = new QListWidget(this);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_list, 1);

    m_removeBtn = new QPushButton(tr("删除选中 Bucket"), this);
    layout->addWidget(m_removeBtn, 0, Qt::AlignRight);

    // 信号
    connect(m_mirrorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BucketPage::onMirrorChanged);
    connect(m_removeBtn, &QPushButton::clicked, this, &BucketPage::onRemoveBucket);
    connect(m_refreshBtn, &QPushButton::clicked, this, &BucketPage::onRefreshClicked);
    connect(m_list, &QListWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) {
        if (!m_list->itemAt(pos)) return;
        auto* menu = new QMenu(this);
        menu->addAction(tr("删除"), this, [this]() { onRemoveBucket(); });
        menu->exec(m_list->viewport()->mapToGlobal(pos));
    });

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
    m_list->clear();
    for (const BucketInfo& b : buckets) {
        QString text = QString("%1  (%2 manifests)").arg(b.name).arg(b.manifest_count);
        if (!b.git_url.isEmpty()) text += tr("  [%1]").arg(b.git_url);
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, b.name);
        m_list->addItem(item);
    }
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
    QListWidgetItem* item = m_list->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一个 bucket。"));
        return;
    }
    const QString name = item->data(Qt::UserRole).toString();
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("删除 Bucket"), tr("确定要删除 bucket \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) m_service->removeBucket(name);
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
