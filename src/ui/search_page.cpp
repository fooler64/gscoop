#include "ui/search_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QFrame>
#include <QColor>

#include "core/scoop_service.h"
#include "ui/package_info_dialog.h"
#include "ui/theme.h"

SearchPage::SearchPage(ScoopService* service, QWidget* parent)
    : QWidget(parent), m_service(service) {
    setupUi();

    connect(m_service, &ScoopService::searchResultsReady,
            this, &SearchPage::onResultsReady);
    connect(m_service, &ScoopService::installedPackagesLoaded,
            this, [this](QVector<InstalledPackage>) { refreshInstalledState(); });
    connect(m_service, &ScoopService::opFinished,
            this, &SearchPage::onOpFinished);
}

void SearchPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 20);
    layout->setSpacing(16);

    // ---- 标题 ----
    auto* title = new QLabel(tr("搜索包"), this);
    title->setObjectName("sectionTitle");
    layout->addWidget(title);

    // ---- 搜索栏（现代化大搜索框）----
    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(10);

    auto* searchWrap = new QFrame(this);
    searchWrap->setObjectName("card");
    auto* searchWrapLayout = new QHBoxLayout(searchWrap);
    searchWrapLayout->setContentsMargins(6, 6, 6, 6);
    searchWrapLayout->setSpacing(8);

    m_searchEdit = new QLineEdit(searchWrap);
    m_searchEdit->setPlaceholderText(tr("搜索包名、描述、二进制..."));
    m_searchEdit->setMinimumHeight(42);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFont(QFont(m_searchEdit->font().family(), 11));
    searchWrapLayout->addWidget(m_searchEdit, 1);

    m_searchBtn = new QPushButton(tr("搜索"), searchWrap);
    m_searchBtn->setProperty("primaryBtn", true);
    m_searchBtn->setMinimumHeight(42);
    m_searchBtn->setMinimumWidth(96);
    searchWrapLayout->addWidget(m_searchBtn);

    searchRow->addWidget(searchWrap, 1);
    layout->addLayout(searchRow);

    // ---- 结果 tabs ----
    m_tabs = new QTabWidget(this);
    m_packageTable = new QTableWidget(this);
    m_binaryTable = new QTableWidget(this);
    setupTable(m_packageTable);
    setupTable(m_binaryTable);
    m_tabs->addTab(m_packageTable, tr("包 (0)"));
    m_tabs->addTab(m_binaryTable, tr("二进制 (0)"));
    layout->addWidget(m_tabs, 1);

    // ---- 状态栏 ----
    m_statusLabel = new QLabel(tr("输入关键词开始搜索"), this);
    m_statusLabel->setObjectName("statusLabel");
    layout->addWidget(m_statusLabel);

    // 信号
    connect(m_searchBtn, &QPushButton::clicked, this, &SearchPage::doSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchPage::doSearch);
}

void SearchPage::setupTable(QTableWidget* table) {
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({tr("名称"), tr("版本"), tr("Bucket"), tr("描述"), tr("状态")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setMouseTracking(true);

    connect(table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem*) {
        showPackageInfo();
    });
    connect(table, &QTableWidget::customContextMenuRequested, this,
            [this, table](const QPoint& pos) {
        QModelIndex idx = table->indexAt(pos);
        if (!idx.isValid()) return;
        table->selectRow(idx.row());
        auto* menu = new QMenu(this);
        menu->addAction(tr("查看信息"), this, [this]() { showPackageInfo(); });
        menu->addAction(tr("安装"), this, [this]() { onInstallClicked(); });
        menu->addAction(tr("卸载"), this, [this]() { onUninstallClicked(); });
        menu->exec(table->viewport()->mapToGlobal(pos));
    });
}

void SearchPage::doSearch() {
    const QString q = m_searchEdit->text().trimmed();
    if (q.isEmpty()) return;
    m_currentQuery = q;
    m_statusLabel->setText(tr("正在搜索 \"%1\"...").arg(q));
    m_service->searchPackages(q);
}

void SearchPage::onResultsReady(QVector<ScoopPackage> packages, bool isCold) {
    Q_UNUSED(isCold);
    m_packageResults.clear();
    m_binaryResults.clear();
    for (const ScoopPackage& pkg : packages) {
        if (pkg.match_source == MatchSource::Binary) {
            m_binaryResults.append(pkg);
        } else {
            m_packageResults.append(pkg);
        }
    }
    populateTable(m_packageTable, m_packageResults);
    populateTable(m_binaryTable, m_binaryResults);
    m_tabs->setTabText(0, tr("包 (%1)").arg(m_packageResults.size()));
    m_tabs->setTabText(1, tr("二进制 (%1)").arg(m_binaryResults.size()));
    m_statusLabel->setText(tr("找到 %1 个结果").arg(m_packageResults.size() + m_binaryResults.size()));
}

void SearchPage::populateTable(QTableWidget* table, const QVector<ScoopPackage>& packages) {
    table->setRowCount(packages.size());
    for (int i = 0; i < packages.size(); ++i) {
        const ScoopPackage& pkg = packages[i];
        auto* nameItem = new QTableWidgetItem(pkg.name);
        nameItem->setData(Qt::UserRole, pkg.name);
        QFont nameFont = nameItem->font();
        nameFont.setBold(true);
        nameItem->setFont(nameFont);
        table->setItem(i, 0, nameItem);
        table->setItem(i, 1, new QTableWidgetItem(pkg.version));
        table->setItem(i, 2, new QTableWidgetItem(pkg.source));
        table->setItem(i, 3, new QTableWidgetItem(pkg.info.left(100)));
        auto* statusItem = new QTableWidgetItem(pkg.is_installed ? tr("已安装") : tr("未安装"));
        if (pkg.is_installed) {
            statusItem->setForeground(QColor("#22c55e"));
            statusItem->setFont([&]() {
                QFont f = statusItem->font();
                f.setBold(true);
                return f;
            }());
        }
        table->setItem(i, 4, statusItem);
    }
    table->clearSelection();
}

void SearchPage::refreshInstalledState() {
    for (int t = 0; t < 2; ++t) {
        QTableWidget* table = (t == 0) ? m_packageTable : m_binaryTable;
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* item = table->item(row, 0);
            if (!item) continue;
            const QString name = item->data(Qt::UserRole).toString();
            const QString appsDir = m_service->scoopAppsDir();
            const bool inst = QFileInfo(appsDir + "/" + name).isDir();
            QTableWidgetItem* status = table->item(row, 4);
            if (status) {
                status->setText(inst ? tr("已安装") : tr("未安装"));
                if (inst) {
                    status->setForeground(QColor("#22c55e"));
                }
            }
        }
    }
}

void SearchPage::showPackageInfo() {
    QTableWidget* table = qobject_cast<QTableWidget*>(m_tabs->currentWidget());
    if (!table) return;
    int row = table->currentRow();
    if (row < 0) return;
    QTableWidgetItem* item = table->item(row, 0);
    if (!item) return;
    const QString name = item->data(Qt::UserRole).toString();

    PackageInfoDialog dlg(m_service, name, this);
    dlg.exec();
}

void SearchPage::onInstallClicked() {
    QTableWidget* table = qobject_cast<QTableWidget*>(m_tabs->currentWidget());
    if (!table) return;
    int row = table->currentRow();
    if (row < 0) return;
    QTableWidgetItem* item = table->item(row, 0);
    if (!item) return;
    const QString name = item->data(Qt::UserRole).toString();

    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("安装"), tr("确定要安装 \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) {
        m_service->installPackage(name);
    }
}

void SearchPage::onUninstallClicked() {
    QTableWidget* table = qobject_cast<QTableWidget*>(m_tabs->currentWidget());
    if (!table) return;
    int row = table->currentRow();
    if (row < 0) return;
    QTableWidgetItem* item = table->item(row, 0);
    if (!item) return;
    const QString name = item->data(Qt::UserRole).toString();

    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("卸载"), tr("确定要卸载 \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) {
        m_service->uninstallPackage(name);
    }
}

void SearchPage::onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error) {
    if (!success) {
        QMessageBox::warning(this, tr("操作失败"),
                             tr("操作 %1 失败：%2").arg(package, error));
    } else {
        m_statusLabel->setText(tr("操作完成：%1").arg(package));
    }
    refreshInstalledState();
    Q_UNUSED(type);
}

void SearchPage::onPageShown() {
    if (!m_loaded) {
        m_loaded = true;
        m_service->scanInstalledPackages();
    }
}
