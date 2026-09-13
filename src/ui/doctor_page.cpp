#include "ui/doctor_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>
#include <QPen>
#include <QMessageBox>
#include <QFontMetrics>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QLayoutItem>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

// ==================== DoctorItemCard ====================
DoctorItemCard::DoctorItemCard(const DoctorCheckItem& item, QWidget* parent)
    : QFrame(parent), m_item(item) {
    setMinimumHeight(56);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void DoctorItemCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const bool dark = ThemeManager::instance().isDark();
    // 状态色：通过=success，警告=warn，失败=danger
    QColor stateColor;
    if (m_item.passed) stateColor = Theme::success(dark);
    else if (m_item.warning) stateColor = Theme::warn(dark);
    else stateColor = Theme::danger(dark);

    // 卡片背景
    p.setPen(QPen(Theme::border(dark), 1));
    p.setBrush(Theme::surface2(dark));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);

    // 左侧状态指示条
    p.setPen(Qt::NoPen);
    p.setBrush(stateColor);
    p.drawRoundedRect(QRect(2, 6, 4, height() - 12), 2, 2);

    // 状态图标（对勾/警告/叉）
    const QRect iconRect(16, (height() - 20) / 2, 20, 20);
    QIcon icon;
    if (m_item.passed) icon = IconPainter::check(stateColor, 20);
    else if (m_item.warning) icon = IconPainter::warning(stateColor, 20);
    else icon = IconPainter::close(stateColor, 20);
    p.drawPixmap(iconRect, icon.pixmap(20, 20));

    // 标题
    QFont titleFont = font();
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(Theme::text(dark));
    const QRect titleRect(46, 8, width() - 60, 20);
    p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_item.title);

    // 详情（版本号等）
    if (!m_item.detail.isEmpty()) {
        QFont subFont = font();
        subFont.setPointSizeF(subFont.pointSizeF() - 0.5);
        p.setFont(subFont);
        p.setPen(Theme::textSub(dark));
        const QRect detailRect(46, 30, width() - 60, 16);
        p.drawText(detailRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QFontMetrics(subFont).elidedText(m_item.detail, Qt::ElideRight, detailRect.width()));
    }

    // 状态文字（右侧）
    QFont stateFont = font();
    stateFont.setPointSizeF(stateFont.pointSizeF() - 0.5);
    stateFont.setBold(true);
    p.setFont(stateFont);
    p.setPen(stateColor);
    const QString stateText = m_item.passed ? tr("通过") : (m_item.warning ? tr("警告") : tr("失败"));
    p.drawText(QRect(width() - 70, 0, 56, height()), Qt::AlignRight | Qt::AlignVCenter, stateText);
}

// ==================== DoctorPage ====================
DoctorPage::DoctorPage(ScoopService* service, QWidget* parent)
    : QWidget(parent), m_service(service) {
    setupUi();
}

void DoctorPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 20);
    layout->setSpacing(14);

    // 标题
    auto* title = new QLabel(tr("环境自检 (Doctor)"), this);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    // 工具行
    auto* toolRow = new QHBoxLayout;
    toolRow->setSpacing(10);

    m_runBtn = new QPushButton(this);
    m_runBtn->setIcon(IconPainter::bug(Theme::text(false), 16));
    m_runBtn->setText(tr("运行自检"));
    m_runBtn->setToolTip(tr("检查 git、7zip、bucket、Windows 设置等"));
    m_runBtn->setMinimumHeight(34);
    toolRow->addWidget(m_runBtn);

    m_cleanupAppsBtn = new QPushButton(this);
    m_cleanupAppsBtn->setIcon(IconPainter::broom(Theme::text(false), 16));
    m_cleanupAppsBtn->setText(tr("清理旧版本"));
    m_cleanupAppsBtn->setToolTip(tr("scoop cleanup：清理已安装应用的旧版本"));
    m_cleanupAppsBtn->setMinimumHeight(34);
    toolRow->addWidget(m_cleanupAppsBtn);

    m_cleanupCacheBtn = new QPushButton(this);
    m_cleanupCacheBtn->setIcon(IconPainter::trash(Theme::text(false), 16));
    m_cleanupCacheBtn->setText(tr("清理缓存"));
    m_cleanupCacheBtn->setToolTip(tr("scoop cache rm *：清空下载缓存"));
    m_cleanupCacheBtn->setMinimumHeight(34);
    toolRow->addWidget(m_cleanupCacheBtn);

    toolRow->addStretch();
    layout->addLayout(toolRow);

    // 状态
    m_statusLabel = new QLabel(tr("点击\"运行自检\"开始检查环境"), this);
    m_statusLabel->setStyleSheet(QString("color:%1;").arg(Theme::textSub(false).name()));
    layout->addWidget(m_statusLabel);

    // 结果列表（滚动）
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_host = new QWidget(m_scroll);
    m_hostLayout = new QVBoxLayout(m_host);
    m_hostLayout->setContentsMargins(0, 0, 0, 0);
    m_hostLayout->setSpacing(8);
    m_hostLayout->setAlignment(Qt::AlignTop);
    m_scroll->setWidget(m_host);
    layout->addWidget(m_scroll, 1);

    connect(m_runBtn, &QPushButton::clicked, this, &DoctorPage::onRunCheck);
    connect(m_cleanupAppsBtn, &QPushButton::clicked, this, &DoctorPage::onCleanupApps);
    connect(m_cleanupCacheBtn, &QPushButton::clicked, this, &DoctorPage::onCleanupCache);
}

void DoctorPage::onRunCheck() {
    m_statusLabel->setText(tr("正在检查..."));
    m_runBtn->setEnabled(false);

    // 在后台线程执行（避免阻塞 UI）
    QFuture<QVector<DoctorCheckItem>> future = QtConcurrent::run([this]() {
        return m_service->runDoctor();
    });
    auto* watcher = new QFutureWatcher<QVector<DoctorCheckItem>>(this);
    connect(watcher, &QFutureWatcher<QVector<DoctorCheckItem>>::finished, this, [this, watcher]() {
        populate(watcher->result());
        m_statusLabel->setText(tr("自检完成"));
        m_runBtn->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void DoctorPage::populate(const QVector<DoctorCheckItem>& items) {
    // 清空旧卡片
    while (QLayoutItem* item = m_hostLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    int passed = 0;
    for (const auto& item : items) {
        if (item.passed) ++passed;
        m_hostLayout->addWidget(new DoctorItemCard(item, m_host));
    }
    m_hostLayout->addStretch();
    m_statusLabel->setText(tr("自检完成：%1/%2 通过").arg(passed).arg(items.size()));
}

void DoctorPage::onCleanupApps() {
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("清理旧版本"), tr("确定要运行 scoop cleanup 清理所有应用的旧版本吗？"));
    if (res == QMessageBox::Yes) {
        m_service->cleanupApps();
        m_statusLabel->setText(tr("正在清理旧版本..."));
    }
}

void DoctorPage::onCleanupCache() {
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("清理缓存"), tr("确定要清空 scoop 下载缓存吗？"));
    if (res == QMessageBox::Yes) {
        m_service->cleanupCache();
        m_statusLabel->setText(tr("正在清理缓存..."));
    }
}

void DoctorPage::onPageShown() {
    // 首次进入自动运行自检
    if (!m_loaded) {
        m_loaded = true;
        onRunCheck();
    }
}
