// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/animated_stack.h"

#include <QPainter>
#include <QWidget>
#include <QLayout>

// 抓取页面快照：先用容器背景色填充（标签页自身透明，背景由父级提供），
// 否则快照为透明图，滑动时内容会消失
static QPixmap grabOpaque(QWidget* w, const QColor& bg) {
    const qreal dpr = w->devicePixelRatioF();
    QPixmap pm(w->size() * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(bg);
    QPainter p(&pm);
    w->render(&p, QPoint(0, 0), QRegion(),
              QWidget::DrawWindowBackground | QWidget::DrawChildren);
    p.end();
    return pm;
}

AnimatedStackedWidget::AnimatedStackedWidget(QWidget* parent)
    : QStackedWidget(parent) {
    setDuration(200);
}

void AnimatedStackedWidget::stopAnimation(bool commit) {
    if (m_anim) {
        m_anim->stop();
        m_anim->deleteLater();
        m_anim = nullptr;
    }
    // 恢复子页可见性（动画期间被 hide）
    if (m_newWidget) m_newWidget->show();
    if (m_oldWidget && m_oldWidget != m_newWidget) m_oldWidget->show();
    if (commit && m_targetIndex >= 0) {
        QStackedWidget::setCurrentIndex(m_targetIndex);
    }
    m_animRunning = false;
    m_progress = 1.0;
    m_targetIndex = -1;
    m_oldWidget = nullptr;
    m_newWidget = nullptr;
    m_newSnapshot = QPixmap();
}

void AnimatedStackedWidget::setCurrentIndex(int index, bool animate) {
    if (index < 0 || index >= count()) return;

    // 动画进行中再次切换：立即结束上一段
    if (m_animRunning) stopAnimation(true);

    if (index == QStackedWidget::currentIndex()) {
        if (animate) update();
        return;
    }
    if (!animate) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }

    QWidget* oldW = currentWidget();
    QWidget* newW = widget(index);
    if (!oldW || !newW || oldW == newW) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }

    // 新页从未显示过：先强制布局/几何，再抓快照，否则快照空或错位
    newW->ensurePolished();
    newW->setGeometry(rect());
    if (auto* lay = newW->layout()) lay->activate();
    newW->update();

    const QColor bg = palette().color(QPalette::Window);
    m_newSnapshot = grabOpaque(newW, bg);

    // 动画期间隐藏两个真实子页：只有一张快照参与绘制 → 物理上不可能重叠/重影
    oldW->hide();
    newW->hide();

    m_oldWidget = oldW;
    m_newWidget = newW;
    m_animRunning = true;
    m_progress = 0.0;
    m_targetIndex = index;

    auto* anim = new QVariantAnimation(this);
    m_anim = anim;
    anim->setDuration(m_duration);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    // OutQuint：起步快、末端平稳，观感顺滑
    anim->setEasingCurve(QEasingCurve::OutQuint);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_progress = v.toDouble();
        update();
    });
    connect(anim, &QVariantAnimation::finished, this, [this]() {
        const int target = m_targetIndex;
        QWidget* newW2 = m_newWidget;
        stopAnimation(false);
        if (target >= 0) {
            if (newW2) newW2->show();
            QStackedWidget::setCurrentIndex(target);
            emit currentChanged(target);
        }
    });
    anim->start();
}

void AnimatedStackedWidget::setCurrentWidget(QWidget* widget, bool animate) {
    setCurrentIndex(indexOf(widget), animate);
}

void AnimatedStackedWidget::paintEvent(QPaintEvent* event) {
    if (!m_animRunning || m_newSnapshot.isNull()) {
        QStackedWidget::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 铺满容器背景，避免边缘露出
    painter.fillRect(rect(), palette().color(QPalette::Window));

    // 新页快照从右侧滑入到 0（OutQuint 缓动由动画提供）
    const double p = m_progress;
    const int offset = int((1.0 - p) * width() * 0.35);   // 滑入距离 35% 宽，轻快
    painter.drawPixmap(offset, 0, m_newSnapshot);
}
