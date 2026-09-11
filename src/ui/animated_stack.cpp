#include "ui/animated_stack.h"

#include <QPainter>
#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <QTimer>

AnimatedStackedWidget::AnimatedStackedWidget(QWidget* parent)
    : QStackedWidget(parent) {
    setDuration(220);
}

void AnimatedStackedWidget::setCurrentIndex(int index, bool animate) {
    if (index == currentIndex() && !animate) {
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

    m_animRunning = true;

    // 淡入淡出
    auto* effect = new QGraphicsOpacityEffect(m_newWidget);
    m_newWidget->setGraphicsEffect(effect);
    effect->setOpacity(0.0);

    auto* fadeIn = new QPropertyAnimation(effect, "opacity", this);
    fadeIn->setDuration(m_duration);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    auto* fadeOut = new QPropertyAnimation(m_oldWidget, "windowOpacity", this);
    fadeOut->setDuration(m_duration);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::OutCubic);

    auto* group = new QParallelAnimationGroup(this);
    group->addAnimation(fadeIn);
    group->addAnimation(fadeOut);

    connect(group, &QParallelAnimationGroup::finished, this, [this, group, index]() {
        QStackedWidget::setCurrentIndex(indexOf(m_newWidget));
        if (m_oldWidget) {
            m_oldWidget->setGraphicsEffect(nullptr);
            m_oldWidget->setWindowOpacity(1.0);
        }
        if (m_newWidget) {
            m_newWidget->setGraphicsEffect(nullptr);
        }
        m_animRunning = false;
        group->deleteLater();
        emit currentChanged(index);
    });

    // 先切到新页（透明），再播动画
    QStackedWidget::setCurrentIndex(index);
    m_newWidget->setWindowOpacity(1.0);
    group->start();
}

void AnimatedStackedWidget::setCurrentWidget(QWidget* widget, bool animate) {
    setCurrentIndex(indexOf(widget), animate);
}

void AnimatedStackedWidget::paintEvent(QPaintEvent* event) {
    QStackedWidget::paintEvent(event);
}
