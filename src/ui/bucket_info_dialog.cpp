// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/bucket_info_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QMouseEvent>
#include <QMessageBox>
#include <QUrl>
#include <QDesktopServices>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

BucketInfoDialog::BucketInfoDialog(ScoopService* service, const BucketInfo& bucket,
                                   QWidget* parent)
    : QDialog(parent), m_service(service), m_bucket(bucket) {
    setWindowTitle(tr("Bucket 信息 - %1").arg(bucket.name));
    resize(460, 480);
    // 无边框窗口（自绘标题栏）
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    const bool dark = ThemeManager::instance().isDark();

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ---- 顶部标题栏 ----
    auto* header = new QFrame(this);
    header->setFixedHeight(60);
    header->setAutoFillBackground(true);
    {
        QPalette hp = header->palette();
        hp.setColor(QPalette::Window, Theme::accentDim(dark));
        header->setPalette(hp);
    }
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 0, 18, 0);
    headerLayout->setSpacing(10);

    auto* iconLbl = new QLabel(header);
    iconLbl->setPixmap(IconPainter::folder(Theme::text(dark), 22).pixmap(22, 22));
    headerLayout->addWidget(iconLbl);

    auto* titleLbl = new QLabel(bucket.name, header);
    QFont tf = titleLbl->font();
    tf.setPointSize(tf.pointSize() + 2);
    tf.setBold(true);
    titleLbl->setFont(tf);
    titleLbl->setStyleSheet(QString("color:%1;").arg(Theme::text(dark).name()));
    headerLayout->addWidget(titleLbl);

    headerLayout->addStretch();

    auto* closeBtn = new QPushButton(header);
    closeBtn->setIcon(IconPainter::close(Theme::text(dark), 15));
    closeBtn->setFlat(true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerLayout->addWidget(closeBtn);

    outer->addWidget(header);

    // ---- 内容区 ----
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll, 1);

    auto* container = new QWidget(scroll);
    scroll->setWidget(container);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(12);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    auto* nameLbl = new QLabel(bucket.name, container);
    form->addRow(tr("名称:"), nameLbl);

    auto* typeLbl = new QLabel(bucket.is_git_repo ? tr("Git 仓库") : tr("本地目录"), container);
    typeLbl->setStyleSheet(QString("color:%1;").arg(
        bucket.is_git_repo ? Theme::success(dark).name() : Theme::textSub(dark).name()));
    form->addRow(tr("类型:"), typeLbl);

    if (!bucket.git_url.isEmpty()) {
        auto* urlLbl = new QLabel(tr("<a href=\"%1\">%1</a>").arg(bucket.git_url), container);
        urlLbl->setOpenExternalLinks(true);
        urlLbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
        urlLbl->setWordWrap(true);
        form->addRow(tr("仓库:"), urlLbl);
    }

    auto* countLbl = new QLabel(tr("%1 个软件清单").arg(bucket.manifest_count), container);
    countLbl->setStyleSheet(QString("color:%1;font-weight:bold;").arg(Theme::accent(dark).name()));
    form->addRow(tr("Manifests:"), countLbl);

    if (!bucket.git_branch.isEmpty()) {
        form->addRow(tr("分支:"), new QLabel(bucket.git_branch, container));
    }
    if (!bucket.last_updated.isEmpty()) {
        form->addRow(tr("更新时间:"), new QLabel(bucket.last_updated, container));
    }
    if (!bucket.path.isEmpty()) {
        auto* pathLbl = new QLabel(bucket.path, container);
        pathLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        pathLbl->setWordWrap(true);
        pathLbl->setStyleSheet(QString("color:%1;").arg(Theme::textSub(dark).name()));
        form->addRow(tr("路径:"), pathLbl);
    }

    layout->addLayout(form);

    auto* descHint = new QLabel(
        tr("该 bucket 包含 %1 个软件包，可通过搜索页搜索安装其中的软件。")
            .arg(bucket.manifest_count), container);
    descHint->setWordWrap(true);
    descHint->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::textSub(dark).name()));
    layout->addWidget(descHint);

    layout->addStretch();

    // ---- 底部操作栏 ----
    auto* footer = new QFrame(this);
    footer->setFixedHeight(52);
    footer->setAutoFillBackground(true);
    {
        QPalette fp = footer->palette();
        fp.setColor(QPalette::Window, Theme::surface(dark));
        footer->setPalette(fp);
    }
    auto* btnRow = new QHBoxLayout(footer);
    btnRow->setContentsMargins(18, 6, 18, 6);
    btnRow->setSpacing(8);

    if (!bucket.git_url.isEmpty()) {
        auto* openBtn = new QPushButton(tr("打开仓库"), footer);
        openBtn->setIcon(IconPainter::globe(Theme::text(false), 15));
        openBtn->setCursor(Qt::PointingHandCursor);
        connect(openBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl(m_bucket.git_url));
        });
        btnRow->addWidget(openBtn);
    }

    btnRow->addStretch();

    auto* removeBtn = new QPushButton(tr("删除 Bucket"), footer);
    removeBtn->setIcon(IconPainter::trash(Theme::danger(dark), 15));
    removeBtn->setCursor(Qt::PointingHandCursor);
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton res = QMessageBox::question(
            this, tr("删除 Bucket"), tr("确定要删除 bucket \"%1\" 吗？").arg(m_bucket.name));
        if (res == QMessageBox::Yes) {
            emit removeRequested(m_bucket.name);
            accept();
        }
    });
    btnRow->addWidget(removeBtn);

    outer->addWidget(footer);
}

// 无边框窗口拖动
void BucketInfoDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->pos().y() <= 60) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void BucketInfoDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void BucketInfoDialog::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}
