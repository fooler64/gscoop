#include "ui/search_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QFrame>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QScrollArea>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QFontMetrics>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/package_info_dialog.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

// ==================== SearchResultCard ====================
SearchResultCard::SearchResultCard(const ScoopPackage& pkg, QWidget* parent)
    : QFrame(parent), m_pkg(pkg) {
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(pkg.info);
    // 高度按简介自适应：无描述 64px，有描述按行数增加
    int descLines = 0;
    if (!pkg.info.isEmpty()) {
        // 估算行数：约 48 个字符一行
        descLines = qMax(1, int(pkg.info.length() / 44.0));
    }
    setMinimumHeight(64 + descLines * 18);
    setMaximumHeight(64 + descLines * 18);
}

void SearchResultCard::setInstalled(bool installed) {
    m_pkg.is_installed = installed;
    update();
}

void SearchResultCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_pkg.name);
    }
    QFrame::mousePressEvent(event);
}

void SearchResultCard::contextMenuEvent(QContextMenuEvent* event) {
    auto* menu = new QMenu(this);
    menu->addAction(tr("查看信息"), this, [this]() { emit infoRequested(m_pkg.name); });
    menu->addAction(tr("安装"), this, [this]() { emit installRequested(m_pkg.name); });
    menu->addAction(tr("卸载"), this, [this]() { emit uninstallRequested(m_pkg.name); });
    menu->exec(event->globalPos());
    delete menu;
}

void SearchResultCard::enterEvent(QEnterEvent* event) {
    m_hover = true;
    update();
    QFrame::enterEvent(event);
}

void SearchResultCard::leaveEvent(QEvent* event) {
    m_hover = false;
    update();
    QFrame::leaveEvent(event);
}

void SearchResultCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const bool dark = ThemeManager::instance().isDark();

    // 圆角卡片：bg=surface3（比页面背景深一档，卡片可见且结果区背景与页面同色），hover 时 blend accent
    QColor bg = Theme::surface3(dark);
    if (m_hover) bg = Theme::blend(bg, Theme::accent(dark), 0.1);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);

    const qreal w = width();
    const qreal h = height();
    const int rightMargin = 90;   // 给右侧下载箭头留空间

    // 状态徽标（右上角）
    if (m_pkg.is_installed) {
        const int bw = 52, bh = 18;
        const QRect badge(int(w) - bw - 12, 8, bw, bh);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::success(dark));
        p.drawRoundedRect(badge, 9, 9);
        p.setPen(Theme::surface(dark));
        QFont bf = font();
        bf.setPointSizeF(bf.pointSizeF() - 1.5);
        bf.setBold(true);
        p.setFont(bf);
        p.drawText(badge, Qt::AlignCenter, tr("已安装"));
    } else {
        // 未安装：显示下载箭头（右侧）
        const int iconSize = 22;
        const QRect iconRect(int(w) - iconSize - 14, (int(h) - iconSize) / 2, iconSize, iconSize);
        const QColor iconColor = m_hover ? Theme::accent(dark) : Theme::textSub(dark);
        p.drawPixmap(iconRect, IconPainter::download(iconColor, iconSize).pixmap(iconSize, iconSize));
    }

    // 名称（加粗）
    QFont nameFont = font();
    nameFont.setPointSizeF(nameFont.pointSizeF() + 0.5);
    nameFont.setBold(true);
    p.setFont(nameFont);
    p.setPen(Theme::text(dark));
    const QRect nameRect(14, 8, int(w) - rightMargin - 10, 22);
    p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(nameFont).elidedText(m_pkg.name, Qt::ElideRight, nameRect.width()));

    // 版本 + bucket
    QFont subFont = font();
    subFont.setPointSizeF(subFont.pointSizeF() - 1.0);
    p.setFont(subFont);
    p.setPen(Theme::textSub(dark));
    const QString sub = QString("%1 · %2").arg(m_pkg.version, m_pkg.source);
    const QRect subRect(14, 30, int(w) - rightMargin - 10, 16);
    p.drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(subFont).elidedText(sub, Qt::ElideRight, subRect.width()));

    // 描述（最多 2 行，按卡片高度）
    if (!m_pkg.info.isEmpty()) {
        QFont descFont = font();
        descFont.setPointSizeF(descFont.pointSizeF() - 1.0);
        p.setFont(descFont);
        p.setPen(Theme::textSub(dark));
        const QRect descRect(14, 48, int(w) - rightMargin - 10, int(h) - 52);
        // 手动换行绘制（最多 2 行）
        const QStringList words = m_pkg.info.split(' ', Qt::SkipEmptyParts);
        QString line1, line2;
        for (const QString& word : words) {
            const QString test = line1.isEmpty() ? word : line1 + " " + word;
            if (QFontMetrics(descFont).horizontalAdvance(test) < descRect.width()) {
                line1 = test;
            } else {
                line2 = (line2.isEmpty() ? word : line2 + " " + word);
            }
        }
        p.drawText(QRect(descRect.left(), descRect.top(), descRect.width(), 16),
                   Qt::AlignLeft | Qt::AlignVCenter, line1);
        if (!line2.isEmpty()) {
            p.drawText(QRect(descRect.left(), descRect.top() + 18, descRect.width(), 16),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QFontMetrics(descFont).elidedText(line2, Qt::ElideRight, descRect.width()));
        }
    }
}

// ==================== SearchPage ====================
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

    // ---- 搜索栏（仿 rscoop：左侧放大镜图标，无边框底色，与结果区背景对调）----
    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(10);

    auto* searchWrap = new QFrame(this);
    searchWrap->setObjectName("card");
    auto* searchWrapLayout = new QHBoxLayout(searchWrap);
    searchWrapLayout->setContentsMargins(14, 8, 14, 8);
    searchWrapLayout->setSpacing(10);
    searchWrap->setAutoFillBackground(true);

    // 搜索容器背景：与页面统一（surface2），不做突出区分（结果卡片才用深色）
    const bool dark = ThemeManager::instance().isDark();
    {
        QPalette sp = searchWrap->palette();
        sp.setColor(QPalette::Window, Theme::surface2(dark));
        searchWrap->setPalette(sp);
    }

    // 左侧放大镜图标（自绘，浅色）
    auto* searchIcon = new QLabel(searchWrap);
    searchIcon->setPixmap(IconPainter::search(Theme::textSub(dark), 18).pixmap(18, 18));
    searchWrapLayout->addWidget(searchIcon);

    m_searchEdit = new QLineEdit(searchWrap);
    m_searchEdit->setPlaceholderText(tr("搜索包名、描述、二进制..."));
    m_searchEdit->setMinimumHeight(38);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFont(QFont(m_searchEdit->font().family(), 11));
    m_searchEdit->setFrame(false);
    m_searchEdit->setStyleSheet(QString("background:transparent;"));
    searchWrapLayout->addWidget(m_searchEdit, 1);

    searchRow->addWidget(searchWrap, 1);
    layout->addLayout(searchRow);

    // ---- 结果 tabs（扁平无边框）----
    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);

    // 包 tab：卡片容器（背景透明，与页面同色）
    auto* packagePage = new QWidget(this);
    packagePage->setAutoFillBackground(false);
    auto* packageScroll = new QScrollArea(packagePage);
    packageScroll->setWidgetResizable(true);
    packageScroll->setFrameShape(QFrame::NoFrame);
    auto* packageHost = new QWidget(packageScroll);
    m_packageLayout = new QVBoxLayout(packageHost);
    m_packageLayout->setContentsMargins(0, 4, 0, 4);
    m_packageLayout->setSpacing(8);
    m_packageLayout->setAlignment(Qt::AlignTop);
    packageScroll->setWidget(packageHost);
    auto* packageOuter = new QVBoxLayout(packagePage);
    packageOuter->setContentsMargins(0, 0, 0, 0);
    packageOuter->addWidget(packageScroll);

    // 二进制 tab：同结构
    auto* binaryPage = new QWidget(this);
    binaryPage->setAutoFillBackground(false);
    auto* binaryScroll = new QScrollArea(binaryPage);
    binaryScroll->setWidgetResizable(true);
    binaryScroll->setFrameShape(QFrame::NoFrame);
    auto* binaryHost = new QWidget(binaryScroll);
    m_binaryLayout = new QVBoxLayout(binaryHost);
    m_binaryLayout->setContentsMargins(0, 4, 0, 4);
    m_binaryLayout->setSpacing(8);
    m_binaryLayout->setAlignment(Qt::AlignTop);
    binaryScroll->setWidget(binaryHost);
    auto* binaryOuter = new QVBoxLayout(binaryPage);
    binaryOuter->setContentsMargins(0, 0, 0, 0);
    binaryOuter->addWidget(binaryScroll);

    m_tabs->addTab(packagePage, tr("包 (0)"));
    m_tabs->addTab(binaryPage, tr("二进制 (0)"));
    layout->addWidget(m_tabs, 1);

    // ---- 状态栏 ----
    m_statusLabel = new QLabel(tr("输入关键词开始搜索"), this);
    m_statusLabel->setObjectName("statusLabel");
    layout->addWidget(m_statusLabel);

    // 信号
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchPage::doSearch);
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
    populateCards(m_packageLayout, m_packageResults);
    populateCards(m_binaryLayout, m_binaryResults);
    m_tabs->setTabText(0, tr("包 (%1)").arg(m_packageResults.size()));
    m_tabs->setTabText(1, tr("二进制 (%1)").arg(m_binaryResults.size()));
    m_statusLabel->setText(tr("找到 %1 个结果").arg(m_packageResults.size() + m_binaryResults.size()));
}

void SearchPage::populateCards(QVBoxLayout* layout, const QVector<ScoopPackage>& packages) {
    if (!layout) return;
    // 清空旧卡片
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    if (packages.isEmpty()) {
        auto* empty = new QLabel(tr("无结果"), this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QString("color:%1;padding:30px;")
                                 .arg(Theme::textSub(false).name()));
        layout->addWidget(empty);
        return;
    }
    for (const auto& pkg : packages) {
        auto* card = new SearchResultCard(pkg, this);
        connect(card, &SearchResultCard::clicked, this, [this](const QString& name) {
            showPackageInfo(name);
        });
        connect(card, &SearchResultCard::infoRequested, this, &SearchPage::showPackageInfo);
        connect(card, &SearchResultCard::installRequested, this, &SearchPage::onInstallClicked);
        connect(card, &SearchResultCard::uninstallRequested, this, &SearchPage::onUninstallClicked);
        layout->addWidget(card);
    }
}

void SearchPage::refreshInstalledState() {
    // 更新所有卡片的已安装状态
    const QString appsDir = m_service->scoopAppsDir();
    auto updateLayout = [this, &appsDir](QVBoxLayout* layout) {
        if (!layout) return;
        for (int i = 0; i < layout->count(); ++i) {
            if (auto* card = qobject_cast<SearchResultCard*>(layout->itemAt(i)->widget())) {
                const bool inst = QFileInfo(appsDir + "/" + card->packageName()).isDir();
                card->setInstalled(inst);
            }
        }
    };
    updateLayout(m_packageLayout);
    updateLayout(m_binaryLayout);
}

void SearchPage::showPackageInfo(const QString& name) {
    PackageInfoDialog dlg(m_service, name, this);
    dlg.exec();
}

void SearchPage::onInstallClicked(const QString& name) {
    const QString appsDir = m_service->scoopAppsDir();
    if (QFileInfo(appsDir + "/" + name).isDir()) {
        QMessageBox::information(this, tr("已安装"), tr("\"%1\" 已安装。").arg(name));
        return;
    }
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("安装"), tr("确定要安装 \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) {
        m_service->installPackage(name);
    }
}

void SearchPage::onUninstallClicked(const QString& name) {
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
