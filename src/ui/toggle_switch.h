// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>

// 现代化开关（Toggle Switch）：替代 QCheckBox 的滑块样式
// - 自绘胶囊轨道 + 圆形滑块，开/关有 140ms 平滑滑动动画
// - 开启色取主题 accent，关闭色取 border；主题切换自动跟随
class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void animateTo(bool on);

    double m_progress = 0.0;   // 0=关 1=开
    bool m_hover = false;
    QVariantAnimation* m_anim = nullptr;
};
