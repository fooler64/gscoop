#include "ui/installed_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QMenu>
#include <QColor>

#include "core/scoop_service.h"
#include "ui/theme.h"

InstalledPage::InstalledPage(ScoopService* service, QWidget* parent)
    : QWidget(parent), m_service(service) {
    setupUi();

    connect(m_service, &ScoopService::installedPackagesLoaded,
            this, &InstalledPage::onPackagesLoaded);
    connect(m_service, &ScoopService::opFinished,
            this, &InstalledPage::onOpFinished);
}

void InstalledPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 20);
    layout->setSpacing(16);

    // ---- 标题 ----
    auto* title = new QLabel(tr("已安装"), this);
    title->setObjectName("sectionTitle");
    layout->addWidget(title);

    // ---- 工具栏 ----
    auto* toolbar = new QHBoxLayout;
    m_countLabel = new QLabel(tr("已安装包：-"), this);
    m_countLabel->setObjectName("mutedLabel");
    toolbar->addWidget(m_countLabel);
    toolbar->addStretch();

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem(tr("全部"), "all");
    m_filterCombo->addItem(tr("可更新"), "outdated");
    m_filterCombo->addItem(tr("已 Hold"), "held");
    m_filterCombo->addItem(tr("异常"), "failed");
    toolbar->addWidget(m_filterCombo);

    m_refreshBtn = new QPushButton(tr("刷新"), this);
    toolbar->addWidget(m_refreshBtn);

    m_updateAllBtn = new QPushButton(tr("更新全部"), this);
    m_updateAllBtn->setProperty("primaryBtn", true);
    toolbar->addWidget(m_updateAllBtn);
    layout->addLayout(toolbar);

    // ---- 表格 ----
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("名称"), tr("版本"), tr("Bucket"), tr("更新时间"), tr("状态")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    layout->addWidget(m_table, 1);

    // 信号
    connect(m_refreshBtn, &QPushButton::clicked, this, &InstalledPage::onRefreshClicked);
    connect(m_updateAllBtn, &QPushButton::clicked, this, &InstalledPage::onUpdateAllClicked);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { populateTable(m_packages); });
    connect(m_table, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) {
        QModelIndex idx = m_table->indexAt(pos);
        if (!idx.isValid()) return;
        m_table->selectRow(idx.row());
        auto* menu = new QMenu(this);
        menu->addAction(tr("更新"), this, [this]() { onUpdateClicked(); });
        menu->addAction(tr("卸载"), this, [this]() { onUninstallClicked(); });
        menu->addAction(tr("Hold"), this, [this]() { onHoldClicked(); });
        menu->exec(m_table->viewport()->mapToGlobal(pos));
    });
}

void InstalledPage::onPackagesLoaded(QVector<InstalledPackage> packages) {
    m_packages = packages;
    populateTable(packages);
    m_countLabel->setText(tr("已安装包：%1").arg(packages.size()));
}

void InstalledPage::populateTable(const QVector<InstalledPackage>& packages) {
    const QString filter = m_filterCombo ? m_filterCombo->currentData().toString() : "all";

    QVector<InstalledPackage> shown;
    for (const InstalledPackage& ip : packages) {
        if (filter == "outdated" && !ip.is_outdated) continue;
        if (filter == "held" && !ip.is_held) continue;
        if (filter == "failed" && !ip.is_failed) continue;
        shown.append(ip);
    }

    m_table->setRowCount(shown.size());
    for (int i = 0; i < shown.size(); ++i) {
        const InstalledPackage& ip = shown[i];
        auto* nameItem = new QTableWidgetItem(ip.pkg.name);
        nameItem->setData(Qt::UserRole, ip.pkg.name);
        m_table->setItem(i, 0, nameItem);

        QString versionText = ip.pkg.version;
        if (ip.is_outdated) {
            versionText += tr(" → %1").arg(ip.update_version);
        }
        m_table->setItem(i, 1, new QTableWidgetItem(versionText));
        m_table->setItem(i, 2, new QTableWidgetItem(ip.pkg.source));
        m_table->setItem(i, 3, new QTableWidgetItem(ip.pkg.updated));

        QStringList status;
        if (ip.is_held) status << tr("Hold");
        if (ip.is_outdated) status << tr("可更新");
        if (ip.is_failed) status << tr("异常");
        if (ip.is_deprecated) status << tr("已废弃");
        auto* statusItem = new QTableWidgetItem(status.join(" "));
        if (ip.is_outdated) statusItem->setForeground(Theme::outdated(false));
        else if (ip.is_failed) statusItem->setForeground(Theme::failed(false));
        else if (ip.is_held) statusItem->setForeground(Theme::held(false));
        m_table->setItem(i, 4, statusItem);
    }
    m_table->clearSelection();
}

QString currentPackage(QTableWidget* table) {
    int row = table->currentRow();
    if (row < 0) return QString();
    QTableWidgetItem* item = table->item(row, 0);
    if (!item) return QString();
    return item->data(Qt::UserRole).toString();
}

void InstalledPage::onUpdateClicked() {
    const QString name = currentPackage(m_table);
    if (name.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一个包。"));
        return;
    }
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("更新"), tr("确定要更新 \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) m_service->updatePackage(name);
}

void InstalledPage::onUninstallClicked() {
    const QString name = currentPackage(m_table);
    if (name.isEmpty()) return;
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("卸载"), tr("确定要卸载 \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) m_service->uninstallPackage(name);
}

void InstalledPage::onHoldClicked() {
    const QString name = currentPackage(m_table);
    if (name.isEmpty()) return;
    m_service->holdPackage(name, true);
}

void InstalledPage::onUpdateAllClicked() {
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("更新全部"), tr("确定要更新所有已安装包吗？"));
    if (res == QMessageBox::Yes) m_service->updateAllPackages();
}

void InstalledPage::onRefreshClicked() {
    m_service->scanInstalledPackages();
}

void InstalledPage::onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error) {
    if (!success && type != ScoopOpType::None) {
        QMessageBox::warning(this, tr("操作失败"), tr("操作失败：%1").arg(error));
    }
    Q_UNUSED(package);
}

void InstalledPage::onPageShown() {
    if (!m_loaded) {
        m_loaded = true;
        m_service->scanInstalledPackages();
    } else {
        m_service->scanInstalledPackages();
    }
}
