// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/installed_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollArea>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QFontMetrics>
#include <QMouseEvent>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"
#include "ui/package_info_dialog.h"

// ==================== InstalledCard ====================
InstalledCard::InstalledCard(const InstalledPackage& pkg, QWidget* parent)
    : QFrame(parent), m_pkg(pkg) {
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(78);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(pkg.pkg.name);
}

void InstalledCard::setPackage(const InstalledPackage& pkg) {
    m_pkg = pkg;
    rebuildText();
    update();
}

void InstalledCard::setSelectable(bool on) {
    m_selectable = on;
    if (!on) m_checked = false;
    update();
}

void InstalledCard::setChecked(bool checked) {
    if (m_checked == checked) return;
    m_checked = checked;
    update();
}

void InstalledCard::rebuildText() {
    // 文本由 paintEvent 绘制，这里只需要触发重绘
    update();
}

void InstalledCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const bool dark = ThemeManager::instance().isDark();
    const QColor bgBase = m_pkg.is_held ? Theme::surface3(dark)
        : (m_pkg.is_outdated ? Theme::highlight(dark) : Theme::surface2(dark));
    QColor bg = m_hover ? Theme::blend(bgBase, Theme::accent(dark), 0.06) : bgBase;
    if (m_pressed) bg = Theme::blend(bg, Theme::accent(dark), 0.12);

    // 多选模式：勾选时用 accent 边框强调
    QColor borderC = m_pkg.is_outdated ? Theme::accent(dark) : Theme::border(dark);
    if (m_selectable && m_checked) borderC = Theme::accent(dark);
    p.setPen(QPen(borderC, m_selectable && m_checked ? 2 : 1));
    p.setBrush(bg);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);

    const qreal h = height();
    const qreal w = width();

    // ---- 多选复选框（右上角）----
    if (m_selectable) {
        const int cs = 18;
        m_checkRect = QRect(int(w) - cs - 10, 8, cs, cs);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(m_checked ? Theme::accent(dark) : Theme::border(dark), 1.5));
        p.setBrush(m_checked ? Theme::accent(dark) : Theme::surface(dark));
        p.drawRoundedRect(m_checkRect, 4, 4);
        if (m_checked) {
            p.setPen(QPen(Theme::surface(dark), 2, Qt::SolidLine, Qt::RoundCap));
            const qreal x = m_checkRect.left() + cs * 0.24;
            const qreal y = m_checkRect.top() + cs * 0.52;
            p.drawLine(QPointF(x, y), QPointF(x + cs * 0.2, y + cs * 0.22));
            p.drawLine(QPointF(x + cs * 0.2, y + cs * 0.22),
                       QPointF(x + cs * 0.56, y - cs * 0.22));
        }
    }

    // ---- 左侧按钮区（锁 + 删除）----
    const int iconSize = 18;
    const int btnY = 10;
    // 删除图标（最左）
    m_trashRect = QRect(12, btnY, iconSize, iconSize);
    QColor trashColor = m_trashHover ? Theme::danger(dark) : Theme::textSub(dark);
    p.drawPixmap(m_trashRect, IconPainter::trash(trashColor, iconSize).pixmap(iconSize, iconSize));

    // 锁图标（删除右侧）
    m_lockRect = QRect(12 + iconSize + 6, btnY, iconSize, iconSize);
    QColor lockColor = m_pkg.is_held
        ? (m_lockHover ? Theme::danger(dark) : Theme::warn(dark))
        : (m_lockHover ? Theme::accent(dark) : Theme::textSub(dark));
    QIcon lockIcon = m_pkg.is_held ? IconPainter::lock(lockColor, iconSize)
                                   : IconPainter::lockOpen(lockColor, iconSize);
    p.drawPixmap(m_lockRect, lockIcon.pixmap(iconSize, iconSize));

    // ---- 名称 + 版本 ----
    QFont nameFont = font();
    nameFont.setPointSizeF(nameFont.pointSizeF() + 0.5);
    nameFont.setBold(true);
    p.setFont(nameFont);
    p.setPen(Theme::text(dark));
    const QRect nameRect(12, 38, int(w) - 90, 22);
    p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(nameFont).elidedText(m_pkg.pkg.name, Qt::ElideRight, nameRect.width()));

    // ---- bucket + 版本 ----
    QFont subFont = font();
    subFont.setPointSizeF(subFont.pointSizeF() - 1.0);
    p.setFont(subFont);
    p.setPen(Theme::textSub(dark));
    // 版本号完整绘制（不截断）。用单点 drawText(pos, text) 重载，
    // 规避 QRect 重载在 Qt6 下对"数字.数字"串的字符整形 bug（点号被渲染成空格）
    p.drawText(QPointF(12, 66), m_pkg.pkg.version);
    if (!m_pkg.pkg.source.isEmpty()) {
        const QRect srcRect(12 + 128, 56, int(w) - 12 - 128 - 12, 16);
        p.drawText(srcRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QFontMetrics(subFont).elidedText(m_pkg.pkg.source, Qt::ElideRight, srcRect.width()));
    }

    // ---- 右上角：可更新徽标 ----
    if (m_pkg.is_outdated) {
        const int bw = 64, bh = 18;
        m_badgeRect = QRect(int(w) - bw - 10, 8, bw, bh);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::accent(dark));
        p.drawRoundedRect(m_badgeRect, 9, 9);
        p.setPen(Theme::surface(dark));
        QFont badgeFont = font();
        badgeFont.setPointSizeF(badgeFont.pointSizeF() - 1.5);
        badgeFont.setBold(true);
        p.setFont(badgeFont);
        p.drawText(m_badgeRect, Qt::AlignCenter, tr("可更新"));
    } else if (m_pkg.is_held) {
        const int bw = 64, bh = 18;
        m_badgeRect = QRect(int(w) - bw - 10, 8, bw, bh);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::warn(dark));
        p.drawRoundedRect(m_badgeRect, 9, 9);
        p.setPen(Theme::surface(dark));
        QFont badgeFont = font();
        badgeFont.setPointSizeF(badgeFont.pointSizeF() - 1.5);
        badgeFont.setBold(true);
        p.setFont(badgeFont);
        p.drawText(m_badgeRect, Qt::AlignCenter, tr("已锁定"));
    } else if (m_pkg.is_failed) {
        const int bw = 64, bh = 18;
        m_badgeRect = QRect(int(w) - bw - 10, 8, bw, bh);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::danger(dark));
        p.drawRoundedRect(m_badgeRect, 9, 9);
        p.setPen(Theme::surface(dark));
        QFont badgeFont = font();
        badgeFont.setPointSizeF(badgeFont.pointSizeF() - 1.5);
        badgeFont.setBold(true);
        p.setFont(badgeFont);
        p.drawText(m_badgeRect, Qt::AlignCenter, tr("异常"));
    }
}

void InstalledCard::mousePressEvent(QMouseEvent* event) {
    const QPoint pos = event->pos();

    // 多选模式：点卡片任意处 = 切换勾选（复选框优先）
    if (m_selectable) {
        if (m_checkRect.adjusted(-8, -8, 8, 8).contains(pos) || rect().contains(pos)) {
            m_checked = !m_checked;
            update();
            emit checkToggled(m_pkg.pkg.name, m_checked);
            return;
        }
    }

    // 扩大点击热区（图标周围加 padding，方便点击）
    const QRect lockHit = m_lockRect.adjusted(-6, -6, 6, 6);
    const QRect trashHit = m_trashRect.adjusted(-6, -6, 6, 6);
    if (lockHit.contains(pos)) {
        emit holdRequested(m_pkg.pkg.name);
        return;
    }
    if (trashHit.contains(pos)) {
        emit uninstallRequested(m_pkg.pkg.name);
        return;
    }
    m_pressed = true;
    update();
    QFrame::mousePressEvent(event);
}

void InstalledCard::mouseReleaseEvent(QMouseEvent* event) {
    if (m_pressed) {
        m_pressed = false;
        update();
        if (rect().contains(event->pos())) {
            emit cardClicked(m_pkg.pkg.name);
        }
    }
    QFrame::mouseReleaseEvent(event);
}

void InstalledCard::enterEvent(QEnterEvent* event) {
    m_hover = true;
    update();
    QFrame::enterEvent(event);
}

void InstalledCard::leaveEvent(QEvent* event) {
    m_hover = false;
    m_lockHover = false;
    m_trashHover = false;
    update();
    QFrame::leaveEvent(event);
}

// ==================== InstalledPage ====================
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
    layout->setContentsMargins(28, 24, 28, 20);
    layout->setSpacing(14);

    // ---- 标题 ----
    auto* title = new QLabel(tr("已安装"), this);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    // ---- 工具栏：搜索框 + 筛选 + 刷新 + 更新全部 ----
    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(10);

    // 搜索框（带放大镜图标）
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索已安装软件..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumHeight(34);
    m_searchEdit->addAction(IconPainter::search(Theme::textSub(false), 16),
                            QLineEdit::LeadingPosition);
    toolbar->addWidget(m_searchEdit, 1);

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem(tr("全部"), "all");
    m_filterCombo->addItem(tr("可更新"), "outdated");
    m_filterCombo->addItem(tr("已锁定"), "held");
    m_filterCombo->addItem(tr("异常"), "failed");
    m_filterCombo->setMinimumWidth(110);
    toolbar->addWidget(m_filterCombo);

    m_refreshBtn = new QPushButton(this);
    m_refreshBtn->setIcon(IconPainter::refresh(Theme::text(false), 16));
    m_refreshBtn->setText(tr("刷新"));
    m_refreshBtn->setToolTip(tr("刷新已安装列表"));
    m_refreshBtn->setMinimumHeight(34);
    toolbar->addWidget(m_refreshBtn);

    m_updateAllBtn = new QPushButton(this);
    m_updateAllBtn->setIcon(IconPainter::updateAll(Theme::text(false), 16));
    m_updateAllBtn->setText(tr("更新全部"));
    m_updateAllBtn->setToolTip(tr("更新所有可更新的软件"));
    m_updateAllBtn->setMinimumHeight(34);
    toolbar->addWidget(m_updateAllBtn);

    // 批量选择开关
    m_selectModeBtn = new QPushButton(this);
    m_selectModeBtn->setText(tr("批量选择"));
    m_selectModeBtn->setToolTip(tr("进入多选模式，批量更新/卸载/锁定"));
    m_selectModeBtn->setCheckable(true);
    m_selectModeBtn->setMinimumHeight(34);
    toolbar->addWidget(m_selectModeBtn);

    layout->addLayout(toolbar);

    // ---- 批量操作栏（仅多选模式显示）----
    m_batchBar = new QWidget(this);
    auto* batchLayout = new QHBoxLayout(m_batchBar);
    batchLayout->setContentsMargins(0, 0, 0, 0);
    batchLayout->setSpacing(8);
    m_selectedLabel = new QLabel(tr("已选 0 项"), m_batchBar);
    batchLayout->addWidget(m_selectedLabel);
    batchLayout->addStretch();
    m_selectAllBtn = new QPushButton(tr("全选"), m_batchBar);
    batchLayout->addWidget(m_selectAllBtn);
    m_batchUpdateBtn = new QPushButton(tr("批量更新"), m_batchBar);
    m_batchUpdateBtn->setProperty("primary", true);
    batchLayout->addWidget(m_batchUpdateBtn);
    m_batchHoldBtn = new QPushButton(tr("批量锁定"), m_batchBar);
    batchLayout->addWidget(m_batchHoldBtn);
    m_batchUninstallBtn = new QPushButton(tr("批量卸载"), m_batchBar);
    batchLayout->addWidget(m_batchUninstallBtn);
    m_batchBar->setVisible(false);
    layout->addWidget(m_batchBar);

    connect(m_selectModeBtn, &QPushButton::toggled, this, &InstalledPage::onToggleSelectMode);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &InstalledPage::onSelectAll);
    connect(m_batchUpdateBtn, &QPushButton::clicked, this, &InstalledPage::onBatchUpdate);
    connect(m_batchHoldBtn, &QPushButton::clicked, this, &InstalledPage::onBatchHold);
    connect(m_batchUninstallBtn, &QPushButton::clicked, this, &InstalledPage::onBatchUninstall);

    // ---- 计数 ----
    m_countLabel = new QLabel(tr("已安装包：-"), this);
    m_countLabel->setObjectName("mutedLabel");
    layout->addWidget(m_countLabel);

    // ---- 卡片网格（滚动区域）----
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_cardsHost = new QWidget(m_scroll);
    m_cardsLayout = new QGridLayout(m_cardsHost);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(10);
    m_cardsLayout->setAlignment(Qt::AlignTop);
    m_scroll->setWidget(m_cardsHost);
    layout->addWidget(m_scroll, 1);

    // 信号
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& t) {
        m_searchText = t.trimmed();
        rebuildCards();
    });
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InstalledPage::onFilterChanged);
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() {
        m_service->scanInstalledPackages();
    });
    connect(m_updateAllBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton res = QMessageBox::question(
            this, tr("更新全部"), tr("确定要更新所有已安装包吗？"));
        if (res == QMessageBox::Yes) m_service->updateAllPackages();
    });
}

void InstalledPage::onFilterChanged(int) {
    m_filter = m_filterCombo->currentData().toString();
    rebuildCards();
}

void InstalledPage::onPackagesLoaded(QVector<InstalledPackage> packages) {
    m_packages = packages;
    m_countLabel->setText(tr("已安装包：%1").arg(packages.size()));
    rebuildCards();
}

void InstalledPage::rebuildCards() {
    if (!m_cardsLayout) return;
    // 清空旧卡片
    while (QLayoutItem* item = m_cardsLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    // 过滤
    QVector<InstalledPackage> shown;
    for (const auto& p : m_packages) {
        // 搜索过滤
        if (!m_searchText.isEmpty()) {
            if (!p.pkg.name.contains(m_searchText, Qt::CaseInsensitive) &&
                !p.description.contains(m_searchText, Qt::CaseInsensitive)) {
                continue;
            }
        }
        // 状态过滤
        if (m_filter == "outdated" && !p.is_outdated) continue;
        if (m_filter == "held" && !p.is_held) continue;
        if (m_filter == "failed" && !p.is_failed) continue;
        shown.append(p);
    }

    const int cols = qMax(1, width() / 340);
    for (int i = 0; i < shown.size(); ++i) {
        auto* card = new InstalledCard(shown[i], m_cardsHost);
        connect(card, &InstalledCard::holdRequested, this, &InstalledPage::doHold);
        connect(card, &InstalledCard::uninstallRequested, this, &InstalledPage::doUninstall);
        connect(card, &InstalledCard::cardClicked, this, [this](const QString& name) {
            if (m_selectMode) return;   // 多选模式下不弹详情
            for (const auto& p : m_packages) {
                if (p.pkg.name == name) {
                    // 打开包信息弹窗
                    PackageInfoDialog dlg(m_service, p.pkg.name, this);
                    dlg.exec();
                    break;
                }
            }
        });
        // 多选模式：显示复选框 + 恢复勾选状态
        card->setSelectable(m_selectMode);
        card->setChecked(m_checked.contains(shown[i].pkg.name));
        connect(card, &InstalledCard::checkToggled,
                this, &InstalledPage::onCardCheckToggled);
        m_cardsLayout->addWidget(card, i / cols, i % cols);
    }

    if (shown.isEmpty()) {
        auto* empty = new QLabel(tr("没有匹配的已安装软件"), m_cardsHost);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QString("color:%1;font-size:14px;padding:40px;")
                                 .arg(Theme::textSub(false).name()));
        m_cardsLayout->addWidget(empty, 0, 0, 1, cols);
    }

    m_cardsLayout->setColumnStretch(cols - 1, 1);
    m_cardsHost->updateGeometry();
}

void InstalledPage::doHold(const QString& name) {
    // 当前状态：如果已 held 则解除，否则 hold
    bool currentlyHeld = false;
    for (const auto& p : m_packages) {
        if (p.pkg.name == name) { currentlyHeld = p.is_held; break; }
    }
    m_service->holdPackage(name, !currentlyHeld);
}

void InstalledPage::doUninstall(const QString& name) {
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("卸载"), tr("确定要卸载 \"%1\" 吗？").arg(name));
    if (res == QMessageBox::Yes) m_service->uninstallPackage(name);
}

// ---------------------------------------------------------------------------
// 批量操作
// ---------------------------------------------------------------------------
void InstalledPage::onToggleSelectMode() {
    m_selectMode = m_selectModeBtn->isChecked();
    m_batchBar->setVisible(m_selectMode);
    if (!m_selectMode) m_checked.clear();
    rebuildCards();
    updateSelectionUi();
}

void InstalledPage::onSelectAll() {
    if (!m_selectMode) return;
    // 全选/全不选（作用于当前过滤后可见的项）
    bool allChecked = true;
    for (const auto& p : m_packages) {
        if (m_checked.contains(p.pkg.name)) continue;
        if (m_filter == "outdated" && !p.is_outdated) continue;
        if (m_filter == "held" && !p.is_held) continue;
        if (m_filter == "failed" && !p.is_failed) continue;
        if (!m_searchText.isEmpty()
            && !p.pkg.name.contains(m_searchText, Qt::CaseInsensitive)
            && !p.description.contains(m_searchText, Qt::CaseInsensitive)) continue;
        allChecked = false;
        break;
    }
    for (const auto& p : m_packages) {
        if (m_filter == "outdated" && !p.is_outdated) continue;
        if (m_filter == "held" && !p.is_held) continue;
        if (m_filter == "failed" && !p.is_failed) continue;
        if (!m_searchText.isEmpty()
            && !p.pkg.name.contains(m_searchText, Qt::CaseInsensitive)
            && !p.description.contains(m_searchText, Qt::CaseInsensitive)) continue;
        if (allChecked) m_checked.remove(p.pkg.name);
        else m_checked.insert(p.pkg.name);
    }
    // 同步所有卡片
    for (int i = 0; i < m_cardsLayout->count(); ++i) {
        if (auto* card = qobject_cast<InstalledCard*>(m_cardsLayout->itemAt(i)->widget())) {
            card->setChecked(m_checked.contains(card->package().pkg.name));
        }
    }
    updateSelectionUi();
}

void InstalledPage::onCardCheckToggled(const QString& name, bool checked) {
    if (checked) m_checked.insert(name);
    else m_checked.remove(name);
    updateSelectionUi();
}

void InstalledPage::updateSelectionUi() {
    m_selectedLabel->setText(tr("已选 %1 项").arg(m_checked.size()));
    const bool has = !m_checked.isEmpty();
    m_batchUpdateBtn->setEnabled(has);
    m_batchHoldBtn->setEnabled(has);
    m_batchUninstallBtn->setEnabled(has);
}

QStringList InstalledPage::checkedNames() const {
    QStringList names;
    for (const auto& p : m_packages) {
        if (m_checked.contains(p.pkg.name)) names << p.pkg.name;
    }
    return names;
}

void InstalledPage::onBatchUpdate() {
    const QStringList names = checkedNames();
    if (names.isEmpty()) return;
    if (QMessageBox::question(this, tr("批量更新"),
            tr("确定要更新选中的 %1 个软件吗？").arg(names.size()))
        != QMessageBox::Yes) return;
    m_service->updatePackages(names);
    if (m_countLabel) m_countLabel->setText(tr("批量更新进行中…"));
}

void InstalledPage::onBatchUninstall() {
    const QStringList names = checkedNames();
    if (names.isEmpty()) return;
    if (QMessageBox::question(this, tr("批量卸载"),
            tr("确定要卸载选中的 %1 个软件吗？此操作不可撤销。").arg(names.size()))
        != QMessageBox::Yes) return;
    m_service->uninstallPackages(names);
    if (m_countLabel) m_countLabel->setText(tr("批量卸载进行中…"));
}

void InstalledPage::onBatchHold() {
    const QStringList names = checkedNames();
    if (names.isEmpty()) return;
    m_service->holdPackages(names, true);
    if (m_countLabel) m_countLabel->setText(tr("批量锁定进行中…"));
}

void InstalledPage::onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error) {
    if (!success && type != ScoopOpType::None) {
        QMessageBox::warning(this, tr("操作失败"), tr("操作失败：%1").arg(error));
    }
    Q_UNUSED(package);
    // 操作完成后刷新列表
    if (success) m_service->scanInstalledPackages();
}

void InstalledPage::onPageShown() {
    if (!m_loaded) {
        m_loaded = true;
        m_service->scanInstalledPackages();
    } else {
        m_service->scanInstalledPackages();
    }
}
