#include "ui/explore_bucket_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QListWidget>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QListWidgetItem>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

// 内置已验证 buckets（来自 rscoop 的 VERIFIED_BUCKETS_DATA）
static const QVector<SearchableBucket> kVerifiedBuckets() {
    return {
        {"main", "ScoopInstaller/Main", "📦 The default bucket for Scoop.", "https://github.com/ScoopInstaller/Main", 1733, 1069, 1402, "2025-09-16", true},
        {"extras", "ScoopInstaller/Extras", "📦 The Extras bucket for Scoop.", "https://github.com/ScoopInstaller/Extras", 2245, 1300, 2383, "2025-09-16", true},
        {"games", "Calinou/scoop-games", "Scoop bucket for open source games and game-related tools.", "https://github.com/Calinou/scoop-games", 1233, 180, 310, "2025-09-16", true},
        {"nerd-fonts", "matthewjberger/scoop-nerd-fonts", "A scoop bucket for installing Nerd Fonts.", "https://github.com/matthewjberger/scoop-nerd-fonts", 900, 70, 500, "2025-09-16", true},
        {"sysinternals", "niheaven/scoop-sysinternals", "Scoop bucket for Sysinternals tools.", "https://github.com/niheaven/scoop-sysinternals", 110, 40, 80, "2025-09-16", true},
        {"java", "ScoopInstaller/Java", "📦 A bucket for installing Java LTS versions.", "https://github.com/ScoopInstaller/Java", 300, 130, 50, "2025-09-16", true},
        {"nirsoft", "ScoopInstaller/Nirsoft", "📦 A bucket for installing NirSoft utilities.", "https://github.com/ScoopInstaller/Nirsoft", 150, 60, 90, "2025-09-16", true},
        {"nonportable", "ScoopInstaller/Nonportable", "📦 Nonportable apps that require admin privileges.", "https://github.com/ScoopInstaller/Nonportable", 250, 90, 120, "2025-09-16", true},
        {"php", "ScoopInstaller/PHP", "📦 PHP versions bucket for Scoop.", "https://github.com/ScoopInstaller/PHP", 180, 80, 60, "2025-09-16", true},
        {"versions", "ScoopInstaller/Versions", "📦 Versions bucket for Scoop.", "https://github.com/ScoopInstaller/Versions", 400, 180, 400, "2025-09-16", true},
    };
}

ExploreBucketDialog::ExploreBucketDialog(ScoopService* service, QWidget* parent)
    : QDialog(parent), m_service(service) {
    setupUi();
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished, this, &ExploreBucketDialog::onReplyFinished);
}

void ExploreBucketDialog::setupUi() {
    setWindowTitle(tr("探索新仓库"));
    setModal(true);
    resize(760, 560);

    const bool dark = ThemeManager::instance().isDark();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(12);

    // 标题
    auto* title = new QLabel(tr("探索新仓库"), this);
    QFont tf = title->font();
    tf.setPointSize(15);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    // 搜索行
    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索 bucket 仓库（关键词，如：game / font / chinese）"));
    m_searchEdit->setMinimumHeight(34);
    searchRow->addWidget(m_searchEdit, 1);

    m_searchBtn = new QPushButton(this);
    m_searchBtn->setIcon(IconPainter::search(Theme::text(dark), 16));
    m_searchBtn->setText(tr("搜索"));
    m_searchBtn->setMinimumHeight(34);
    searchRow->addWidget(m_searchBtn);

    layout->addLayout(searchRow);

    // 过滤行
    auto* filterRow = new QHBoxLayout;
    filterRow->setSpacing(8);

    filterRow->addWidget(new QLabel(tr("排序:"), this));
    m_sortCombo = new QComboBox(this);
    m_sortCombo->addItem(tr("Stars"), "stars");
    m_sortCombo->addItem(tr("Apps 数量"), "apps");
    m_sortCombo->addItem(tr("名称"), "name");
    m_sortCombo->setMinimumWidth(110);
    filterRow->addWidget(m_sortCombo);

    filterRow->addSpacing(12);
    filterRow->addWidget(new QLabel(tr("最小 Stars:"), this));
    m_minStarsSpin = new QSpinBox(this);
    m_minStarsSpin->setRange(0, 10000);
    m_minStarsSpin->setValue(2);
    m_minStarsSpin->setSingleStep(1);
    filterRow->addWidget(m_minStarsSpin);

    m_hideChineseCheck = new QCheckBox(tr("隐藏中文 bucket"), this);
    m_hideChineseCheck->setToolTip(tr("过滤掉描述包含中文的仓库"));
    filterRow->addWidget(m_hideChineseCheck);

    filterRow->addStretch();
    layout->addLayout(filterRow);

    // 结果列表
    m_resultList = new QListWidget(this);
    m_resultList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_resultList, 1);

    // 状态 + 底部按钮
    m_statusLabel = new QLabel(tr("输入关键词开始搜索，或直接查看热门仓库"), this);
    m_statusLabel->setStyleSheet(QString("color:%1;").arg(Theme::textSub(dark).name()));
    layout->addWidget(m_statusLabel);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();

    auto* closeBtn = new QPushButton(tr("关闭"), this);
    btnRow->addWidget(closeBtn);

    m_addBtn = new QPushButton(this);
    m_addBtn->setIcon(IconPainter::plus(Theme::text(dark), 16));
    m_addBtn->setText(tr("添加选中"));
    m_addBtn->setEnabled(false);
    m_addBtn->setMinimumWidth(110);
    btnRow->addWidget(m_addBtn);

    layout->addLayout(btnRow);

    // 信号
    connect(m_searchBtn, &QPushButton::clicked, this, &ExploreBucketDialog::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &ExploreBucketDialog::onSearch);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExploreBucketDialog::onSortChanged);
    connect(m_minStarsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { if (!m_searching) onSearch(); });
    connect(m_hideChineseCheck, &QCheckBox::toggled,
            this, [this](bool) { if (!m_searching) onSearch(); });
    connect(m_resultList, &QListWidget::currentRowChanged,
            this, [this](int row) { m_addBtn->setEnabled(row >= 0); });
    connect(m_addBtn, &QPushButton::clicked, this, &ExploreBucketDialog::onAddClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    // 初始显示热门仓库
    populateResults(kVerifiedBuckets());
}

QString ExploreBucketDialog::buildQuery() const {
    QString q = m_searchEdit->text().trimmed();
    QStringList terms;
    if (!q.isEmpty()) terms << "\"" + q + "\"";
    terms << "scoop" << "bucket";
    QString query = terms.join("+");
    if (m_hideChineseCheck->isChecked()) {
        query += "+NOT+description:[\u4e00-\u9fa5]";
    }
    return query;
}

void ExploreBucketDialog::onSearch() {
    const QString q = m_searchEdit->text().trimmed();
    if (q.isEmpty()) {
        // 空搜索：显示热门仓库（内置 verified）
        populateResults(kVerifiedBuckets());
        m_statusLabel->setText(tr("显示热门仓库（%1 个）").arg(kVerifiedBuckets().size()));
        return;
    }

    m_searching = true;
    m_statusLabel->setText(tr("正在搜索..."));
    m_searchBtn->setEnabled(false);

    // GitHub search API
    QUrl url("https://api.github.com/search/repositories");
    QUrlQuery query;
    query.addQueryItem("q", buildQuery());
    query.addQueryItem("sort", m_sortCombo->currentData().toString());
    query.addQueryItem("order", "desc");
    query.addQueryItem("per_page", "50");
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "gScoop/0.1");
    req.setRawHeader("Accept", "application/vnd.github+json");
    m_nam->get(req);
}

void ExploreBucketDialog::onSortChanged(int) {
    if (!m_searching) onSearch();
}

void ExploreBucketDialog::onReplyFinished(QNetworkReply* reply) {
    m_searching = false;
    m_searchBtn->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText(tr("搜索失败：%1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonObject obj = doc.object();
    const QJsonArray items = obj.value("items").toArray();

    QVector<SearchableBucket> buckets;
    const int minStars = m_minStarsSpin->value();
    for (const auto& v : items) {
        const QJsonObject o = v.toObject();
        SearchableBucket b;
        b.full_name = o.value("full_name").toString();
        b.name = b.full_name.section('/', -1);
        b.description = o.value("description").toString();
        b.url = o.value("html_url").toString();
        b.stars = o.value("stargazers_count").toInt();
        b.forks = o.value("forks_count").toInt();
        b.last_updated = o.value("updated_at").toString().left(10);
        // apps 数量：无法直接从 API 拿到，留 0（可用 manifest 数替代）
        b.apps = 0;
        if (b.stars < minStars) continue;
        // 隐藏中文：描述含 CJK 字符
        if (m_hideChineseCheck->isChecked()) {
            static const QRegularExpression cjk("[\u4e00-\u9fa5]");
            if (b.description.contains(cjk)) continue;
        }
        buckets.append(b);
    }

    m_results = buckets;
    populateResults(buckets);
    m_statusLabel->setText(tr("找到 %1 个仓库").arg(buckets.size()));
}

void ExploreBucketDialog::populateResults(const QVector<SearchableBucket>& buckets) {
    m_resultList->clear();
    const bool dark = ThemeManager::instance().isDark();
    for (const auto& b : buckets) {
        QString text = QString("%1  (%2)")
                           .arg(b.full_name)
                           .arg(b.is_verified ? tr("✓ 已验证") : "");
        if (b.stars > 0) text += QString("  ⭐ %1").arg(b.stars);
        if (b.apps > 0) text += QString("  📦 %1").arg(b.apps);
        if (!b.last_updated.isEmpty()) text += QString("  %1").arg(b.last_updated);
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, b.url);
        item->setToolTip(b.description);
        if (b.is_verified) {
            item->setForeground(QColor(Theme::success(dark)));
        }
        m_resultList->addItem(item);
    }
    m_results = buckets;
    if (!buckets.isEmpty()) m_resultList->setCurrentRow(0);
    m_addBtn->setEnabled(!buckets.isEmpty());
}

void ExploreBucketDialog::onAddClicked() {
    QListWidgetItem* item = m_resultList->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一个仓库。"));
        return;
    }
    const QString url = item->data(Qt::UserRole).toString();
    const QString name = item->text().section(' ', 0, 0).section('/', -1);
    if (url.isEmpty()) return;
    m_service->addBucket(name, url);
    QMessageBox::information(this, tr("添加"), tr("已开始添加 bucket \"%1\"。").arg(name));
}
