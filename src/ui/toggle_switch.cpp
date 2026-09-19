// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/toggle_switch.h"

#include <QPainter>
#include <QPainterPath>
#include <QEnterEvent>

#include "core/theme_manager.h"
#include "ui/theme.h"

ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QAbstractButton(parent) {
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setFixedSize(sizeHint());

    // 点击切换 → 播放动画
    connect(this, &QAbstractButton::toggled, this, [this](bool on) { animateTo(on); });
}

QSize ToggleSwitch::sizeHint() const {
    return QSize(44, 24);
}

void ToggleSwitch::animateTo(bool on) {
    // 复用动画对象，避免野指针（line 参考 WindowButton 的教训）
    if (!m_anim) {
        m_anim = new QVariantAnimation(this);
        m_anim->setDuration(140);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
            m_progress = v.toDouble();
            update();
        });
    }
    m_anim->stop();
    m_anim->setStartValue(m_progress);
    m_anim->setEndValue(on ? 1.0 : 0.0);
    m_anim->start();
}

void ToggleSwitch::enterEvent(QEnterEvent* event) {
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ToggleSwitch::leaveEvent(QEvent* event) {
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ToggleSwitch::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const bool dark = ThemeManager::instance().isDark();
    const double t = m_progress;

    // 轨道颜色：关=border（hover 稍亮），开=accent
    QColor off = Theme::border(dark);
    if (m_hover && t < 0.5) off = Theme::textSub(dark);
    const QColor track = Theme::blend(off, Theme::accent(dark), t);

    const QRectF trackRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(Qt::NoPen);
    p.setBrush(track);
    p.drawRoundedRect(trackRect, trackRect.height() / 2, trackRect.height() / 2);

    // 圆形滑块：从左侧滑到右侧
    const qreal knobD = trackRect.height() - 4;
    const qreal x0 = trackRect.left() + 2;
    const qreal x1 = trackRect.right() - knobD - 2;
    const qreal kx = x0 + (x1 - x0) * t;
    const QRectF knob(kx, trackRect.top() + 2, knobD, knobD);

    // 滑块阴影（轻微）+ 白色圆
    p.setBrush(QColor(0, 0, 0, 28));
    p.drawEllipse(knob.translated(0, 1));
    p.setBrush(Qt::white);
    p.drawEllipse(knob);
}
