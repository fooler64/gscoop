// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/animated_stack.h"

#include <QPainter>
#include <QWidget>
#include <QLayout>
#include <QPalette>

// 抓取页面快照：先用容器背景色填充（标签页自身透明，背景由父级提供），
// 否则快照为透明图，滑动时会透出下层造成"重叠/重影"
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
    setDuration(220);
}

void AnimatedStackedWidget::stopAnimation(bool commit) {
    if (m_anim) {
        m_anim->stop();
        m_anim->deleteLater();
        m_anim = nullptr;
    }
    // 恢复子页可见性（动画期间被隐藏）
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
    m_oldSnapshot = QPixmap();
    m_newSnapshot = QPixmap();
}

void AnimatedStackedWidget::setCurrentIndex(int index, bool animate) {
    if (index < 0 || index >= count()) return;

    // 动画进行中再次切换：先把上一段动画收尾（提交目标页），再开始新的一段
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

    // 方向感知：向后 → 从右/下滑入；向前 → 从左/上滑入
    m_forward = (index > QStackedWidget::currentIndex());

    // 新页从未显示过：先强制布局/几何，再抓快照，否则快照为空或错位
    newW->ensurePolished();
    newW->setGeometry(rect());
    if (auto* lay = newW->layout()) lay->activate();
    newW->update();

    // 双不透明快照（互补拼接滑动 → 零重叠）
    const QColor bg = palette().color(QPalette::Window);
    m_oldSnapshot = grabOpaque(oldW, bg);
    m_newSnapshot = grabOpaque(newW, bg);

    // 动画期间隐藏两个真实子页，只显示快照
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
    // OutQuint：起步轻快、末端平稳收束
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
    if (!m_animRunning || m_oldSnapshot.isNull() || m_newSnapshot.isNull()) {
        QStackedWidget::paintEvent(event);
        return;
    }
    QPainter painter(this);
    drawTransition(painter);
}

// 过渡帧绘制（paintEvent 与调试抓帧共用）
void AnimatedStackedWidget::drawTransition(QPainter& painter) const {
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), palette().color(QPalette::Window));

    const double p = m_progress;
    const int dir = m_forward ? 1 : -1;
    const double span = m_extent;

    if (m_axis == Qt::Horizontal) {
        const int w = width();
        const int travel = int(w * span * p);
        // 旧页滑出：p=0 在 0，p=1 在 -dir*w
        painter.drawPixmap(-dir * travel, 0, m_oldSnapshot);
        // 新页滑入：p=0 在 dir*w（屏外），p=1 在 0
        painter.drawPixmap(dir * (int(w * span) - travel), 0, m_newSnapshot);
    } else {
        const int h = height();
        const int travel = int(h * span * p);
        painter.drawPixmap(0, -dir * travel, m_oldSnapshot);
        painter.drawPixmap(0, dir * (int(h * span) - travel), m_newSnapshot);
    }
}
