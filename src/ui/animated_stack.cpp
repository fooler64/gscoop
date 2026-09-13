// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/animated_stack.h"

#include <QPainter>
#include <QWidget>
#include <QLayout>
#include <QEvent>
#include <QDebug>

AnimatedStackedWidget::AnimatedStackedWidget(QWidget* parent)
    : QStackedWidget(parent) {
    setDuration(180);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void AnimatedStackedWidget::setCurrentIndex(int index, bool animate) {
    if (index < 0 || index >= count()) return;
    if (index == currentIndex()) {
        if (!animate) return;
        // 同页重入：直接重绘
        update();
        return;
    }
    if (!animate || m_animRunning) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }

    m_oldWidget = currentWidget();
    m_newWidget = widget(index);
    if (!m_oldWidget || !m_newWidget || m_oldWidget == m_newWidget) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }

    // 关键：新页从未显示过，render 前必须强制布局/设置几何，
    // 否则动画期间 render 出空白或错位内容
    m_newWidget->ensurePolished();
    m_newWidget->setGeometry(rect());
    if (auto* lay = m_newWidget->layout()) {
        lay->activate();
    }
    m_newWidget->update();

    m_animRunning = true;
    m_progress = 0.0;

    // 用 QVariantAnimation 驱动进度（纯数值动画，无效果对象）
    auto* anim = new QVariantAnimation(this);
    anim->setDuration(m_duration);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_progress = v.toDouble();
        update();   // 触发 paintEvent 手绘
    });
    connect(anim, &QVariantAnimation::finished, this, [this, anim, index]() {
        m_animRunning = false;
        m_progress = 1.0;
        QStackedWidget::setCurrentIndex(index);
        m_oldWidget = nullptr;
        m_newWidget = nullptr;
        anim->deleteLater();
        emit currentChanged(index);
    });
    anim->start();
}

void AnimatedStackedWidget::setCurrentWidget(QWidget* widget, bool animate) {
    setCurrentIndex(indexOf(widget), animate);
}

void AnimatedStackedWidget::paintEvent(QPaintEvent* event) {
    if (!m_animRunning || !m_oldWidget || !m_newWidget) {
        QStackedWidget::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 1. 底层：旧页面（完整绘制）
    m_oldWidget->render(&painter, QPoint(0, 0),
                        QRegion(m_oldWidget->rect()));

    // 2. 顶层：新页面淡入 + 轻微右滑
    const double p = m_progress;
    painter.save();
    // 先铺不透明背景，避免新页透明处透出旧页造成闪烁
    painter.fillRect(rect(), m_newWidget->palette().window());
    painter.setOpacity(p);
    // 从右 24px 滑入到 0
    const int offset = int((1.0 - p) * 24.0);
    painter.translate(offset, 0);
    m_newWidget->render(&painter, QPoint(0, 0),
                        QRegion(m_newWidget->rect()));
    painter.restore();
}
