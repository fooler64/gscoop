// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/activity_bar.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QFont>

#include "ui/theme.h"

ActivityBar::ActivityBar(QWidget* parent)
    : QWidget(parent) {
    setFixedWidth(56);
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 10, 0, 10);
    m_layout->setSpacing(6);
    m_layout->addStretch(1);
}

QPushButton* ActivityBar::addItem(const QString& icon, const QString& tooltip) {
    auto* btn = new QPushButton(this);
    btn->setText(icon);   // emoji 图标
    btn->setToolTip(tooltip);
    btn->setFixedSize(56, 44);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setFlat(true);
    btn->setFont(QFont("Segoe UI Emoji", 14));

    // 插入到 stretch 之前
    m_layout->insertWidget(m_layout->count() - 1, btn);

    const int idx = m_buttons.size();
    m_buttons.append(btn);
    connect(btn, &QPushButton::clicked, this, [this, idx]() {
        setActive(idx);
        emit itemClicked(idx);
    });
    return btn;
}

void ActivityBar::setActive(int index) {
    m_active = index;
    for (int i = 0; i < m_buttons.size(); ++i) {
        m_buttons[i]->setChecked(i == index);
    }
    update();
}

void ActivityBar::setDark(bool dark) {
    m_dark = dark;
    update();
    for (QPushButton* btn : m_buttons) {
        btn->update();
    }
}

void ActivityBar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), Theme::activityBarBg(m_dark));

    for (int i = 0; i < m_buttons.size(); ++i) {
        QPushButton* btn = m_buttons[i];
        const QRect r = btn->geometry();
        const bool active = (i == m_active);

        if (active) {
            // 左侧指示条
            p.fillRect(r.left(), r.top(), 3, r.height(),
                       Theme::activityBarActive(m_dark));
            // 选中背景
            p.fillRect(r, Theme::blend(Theme::activityBarBg(m_dark),
                                       Theme::activityBarActive(m_dark), 0.18));
        } else if (btn->underMouse()) {
            p.fillRect(r, Theme::blend(Theme::activityBarBg(m_dark),
                                       Theme::highlight(m_dark), 0.35));
        }

        // 图标颜色（通过按钮前景色实现）
        QColor iconColor = active
            ? Theme::activityBarActive(m_dark)
            : Theme::activityBarIcon(m_dark);
        QPalette pal = btn->palette();
        pal.setColor(QPalette::ButtonText, iconColor);
        btn->setPalette(pal);
    }
}
