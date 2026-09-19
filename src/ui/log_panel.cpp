// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/log_panel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QPainter>
#include <QPen>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QTextCursor>
#include <QDateTime>
#include <QScrollBar>

#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

LogPanel::LogPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("logPanel");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAutoFillBackground(true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- 标题行 ----
    m_header = new QFrame(this);
    m_header->setFixedHeight(34);
    m_header->setCursor(Qt::PointingHandCursor);
    auto* hl = new QHBoxLayout(m_header);
    hl->setContentsMargins(12, 0, 8, 0);
    hl->setSpacing(8);

    m_statusIcon = new QLabel(m_header);
    m_statusIcon->setFixedSize(16, 16);
    hl->addWidget(m_statusIcon);

    m_titleLabel = new QLabel(tr("操作日志"), m_header);
    QFont tf = m_titleLabel->font();
    tf.setBold(true);
    m_titleLabel->setFont(tf);
    hl->addWidget(m_titleLabel);

    m_stageLabel = new QLabel(m_header);
    m_stageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    hl->addWidget(m_stageLabel, 1);

    m_progress = new QProgressBar(m_header);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    m_progress->setFixedWidth(180);
    m_progress->setFixedHeight(16);
    m_progress->setVisible(false);
    hl->addWidget(m_progress);

    m_clearBtn = new QPushButton(m_header);
    m_clearBtn->setIcon(IconPainter::trash(Theme::textSub(false), 14));
    m_clearBtn->setFlat(true);
    m_clearBtn->setFixedSize(24, 24);
    m_clearBtn->setToolTip(tr("清空日志"));
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    hl->addWidget(m_clearBtn);

    m_toggleBtn = new QPushButton(m_header);
    m_toggleBtn->setIcon(IconPainter::upArrow(Theme::textSub(false), 14));
    m_toggleBtn->setFlat(true);
    m_toggleBtn->setFixedSize(24, 24);
    m_toggleBtn->setToolTip(tr("展开/收起日志"));
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    hl->addWidget(m_toggleBtn);

    root->addWidget(m_header);

    // ---- 日志正文 ----
    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_logView->setMinimumHeight(120);
    m_logView->setMaximumHeight(240);
    m_logView->setVisible(false);
    root->addWidget(m_logView);

    connect(m_clearBtn, &QPushButton::clicked, this, &LogPanel::clearLog);
    connect(m_toggleBtn, &QPushButton::clicked, this, [this]() { setExpanded(!m_expanded); });

    refreshTheme();
}

void LogPanel::refreshTheme() {
    const bool dark = ThemeManager::instance().isDark();

    QPalette hp = m_header->palette();
    hp.setColor(QPalette::Window, Theme::surface2(dark));
    m_header->setPalette(hp);
    m_header->setAutoFillBackground(true);

    m_stageLabel->setStyleSheet(QString("color:%1;").arg(Theme::textSub(dark).name()));
    m_titleLabel->setStyleSheet(QString("color:%1;").arg(Theme::text(dark).name()));

    QPalette lp = m_logView->palette();
    lp.setColor(QPalette::Base, Theme::surface(dark));
    lp.setColor(QPalette::Text, Theme::text(dark));
    m_logView->setPalette(lp);

    m_toggleBtn->setIcon(IconPainter::upArrow(Theme::textSub(dark), 14));
    m_clearBtn->setIcon(IconPainter::trash(Theme::textSub(dark), 14));
    update();
}

void LogPanel::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    const bool dark = ThemeManager::instance().isDark();
    // 顶部一条分隔线
    p.setPen(QPen(Theme::border(dark), 1));
    p.drawLine(0, 0, width(), 0);
}

void LogPanel::mousePressEvent(QMouseEvent* event) {
    // 点击标题行区域 → 折叠/展开
    if (event->pos().y() <= m_header->height()) {
        setExpanded(!m_expanded);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void LogPanel::setExpanded(bool expanded) {
    m_expanded = expanded;
    m_logView->setVisible(expanded);
    const bool dark = ThemeManager::instance().isDark();
    m_toggleBtn->setIcon(expanded ? IconPainter::downArrow(Theme::textSub(dark), 14)
                                  : IconPainter::upArrow(Theme::textSub(dark), 14));
    if (expanded) {
        // 展开时滚到底部
        m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
    }
    updateGeometry();
}

void LogPanel::clearLog() {
    m_logView->clear();
    m_pendingOutput.clear();
}

void LogPanel::beginOperation(const QString& title, const QString& package) {
    m_running = true;
    m_failed = false;
    m_titleLabel->setText(title.isEmpty() ? tr("操作日志") : title);
    m_stageLabel->setText(package);
    m_progress->setVisible(true);
    m_progress->setRange(0, 0);   // 不确定进度
    m_progress->setValue(0);
    const bool dark = ThemeManager::instance().isDark();
    m_statusIcon->setPixmap(IconPainter::refresh(Theme::accent(dark), 16).pixmap(16, 16));

    // 新操作：追加分隔头
    if (!m_logView->document()->isEmpty()) {
        m_logView->appendPlainText(QString());
    }
    m_logView->appendPlainText(QString("=== %1 [%2] ===").arg(
        QDateTime::currentDateTime().toString("HH:mm:ss"),
        title.isEmpty() ? package : title));
    setExpanded(true);
}

void LogPanel::appendRaw(const QString& text) {
    if (text.isEmpty()) return;
    QTextCursor cur = m_logView->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertText(text);
    m_logView->setTextCursor(cur);
    // 自动滚到底部
    m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
}

void LogPanel::setProgress(int percent) {
    if (percent < 0) {
        m_progress->setRange(0, 0);
    } else {
        m_progress->setRange(0, 100);
        m_progress->setValue(percent);
    }
}

void LogPanel::setStage(const QString& stage) {
    m_stageLabel->setText(stage.simplified().left(120));
}

void LogPanel::finishOperation(bool success, const QString& error) {
    m_running = false;
    m_failed = !success;
    const bool dark = ThemeManager::instance().isDark();
    m_progress->setVisible(false);
    if (success) {
        m_statusIcon->setPixmap(IconPainter::check(Theme::success(dark), 16).pixmap(16, 16));
        m_stageLabel->setText(tr("完成"));
        m_logView->appendPlainText(tr("--- 操作完成 ---"));
    } else {
        m_statusIcon->setPixmap(IconPainter::close(Theme::danger(dark), 16).pixmap(16, 16));
        m_stageLabel->setText(tr("失败：%1").arg(error.left(120)));
        m_logView->appendPlainText(tr("--- 操作失败：%1 ---").arg(error));
    }
    m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
}
