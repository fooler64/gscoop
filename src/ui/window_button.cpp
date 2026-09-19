// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/window_button.h"

#include <QPainter>
#include <QPainterPath>
#include <QEnterEvent>

#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

WindowButton::WindowButton(Type type, QWidget* parent)
    : QAbstractButton(parent), m_type(type) {
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
    if (type == Close) setToolTip(tr("关闭"));
    else if (type == Minimize) setToolTip(tr("最小化"));
    else setToolTip(tr("最大化"));
}

QSize WindowButton::sizeHint() const {
    return QSize(44, 32);
}

void WindowButton::setMaximized(bool maximized) {
    if (m_maximized == maximized) return;
    m_maximized = maximized;
    setToolTip(maximized ? tr("还原") : tr("最大化"));
    update();
}

void WindowButton::animateHover(bool hovered) {
    if (m_hoverAnim) {
        m_hoverAnim->stop();
        m_hoverAnim->deleteLater();
        m_hoverAnim = nullptr;
    }
    auto* anim = new QVariantAnimation(this);
    m_hoverAnim = anim;
    anim->setDuration(120);
    anim->setStartValue(m_hoverProgress);
    anim->setEndValue(hovered ? 1.0 : 0.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_hoverProgress = v.toDouble();
        update();
    });
    connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
    anim->start();
}

void WindowButton::enterEvent(QEnterEvent* event) {
    m_hovered = true;
    animateHover(true);
    QAbstractButton::enterEvent(event);
}

void WindowButton::leaveEvent(QEvent* event) {
    m_hovered = false;
    m_pressed = false;
    animateHover(false);
    QAbstractButton::leaveEvent(event);
}

void WindowButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const bool dark = ThemeManager::instance().isDark();
    const bool isClose = (m_type == Close);
    const double hp = m_hoverProgress;
    const bool down = isDown();

    // ---- 背景：hover 时淡入圆角底色（关闭=红，其余=冰蓝高亮）----
    if (hp > 0.001) {
        QColor bg = isClose ? Theme::danger(dark) : Theme::highlight(dark);
        if (down) bg = Theme::blend(bg, Qt::black, 0.15);
        bg.setAlphaF(bg.alphaF() * hp);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(rect().adjusted(3, 3, -3, -3), 7, 7);
    }

    // ---- 图标：颜色随 hover 过渡（关闭 hover 变白，对比清晰）----
    QColor iconColor;
    if (isClose) {
        iconColor = Theme::blend(Theme::textSub(dark), Qt::white, hp);
    } else {
        iconColor = Theme::blend(Theme::textSub(dark), Theme::text(dark), hp);
    }

    const int iconSize = 15;
    const QRect iconRect((width() - iconSize) / 2, (height() - iconSize) / 2,
                         iconSize, iconSize);

    QIcon icon;
    switch (m_type) {
    case Minimize:
        icon = IconPainter::minimize(iconColor, iconSize);
        break;
    case MaximizeRestore:
        icon = m_maximized ? IconPainter::restore(iconColor, iconSize)
                           : IconPainter::maximize(iconColor, iconSize);
        break;
    case Close:
        icon = IconPainter::close(iconColor, iconSize);
        break;
    }
    p.drawPixmap(iconRect, icon.pixmap(iconSize, iconSize));
}
