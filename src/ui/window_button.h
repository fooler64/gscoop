// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>

// 自绘窗口控制按钮（最小化 / 最大化-还原 / 关闭）
// - 圆角 hover 背景 + 120ms 颜色过渡动画
// - 关闭按钮 hover 变红底白叉（对比清晰）
// - 按下加深反馈，主题色全部取自 ThemeManager
class WindowButton : public QAbstractButton {
    Q_OBJECT
public:
    enum Type { Minimize, MaximizeRestore, Close };

    explicit WindowButton(Type type, QWidget* parent = nullptr);

    void setMaximized(bool maximized);   // 切换 最大化/还原 图标
    bool isMaximized() const { return m_maximized; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void animateHover(bool hovered);

    Type m_type;
    bool m_maximized = false;
    bool m_hovered = false;
    bool m_pressed = false;
    double m_hoverProgress = 0.0;   // 0~1 平滑过渡
    QVariantAnimation* m_hoverAnim = nullptr;
};
