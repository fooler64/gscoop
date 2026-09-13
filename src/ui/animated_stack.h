// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QStackedWidget>
#include <QVariantAnimation>

// 高性能页面切换动画（纯 QPainter 手绘，无 QGraphicsOpacityEffect）：
// 新页面从右侧轻微滑入 + 透明度渐变，旧页面直接切换隐藏
// 轻量：只对当前帧重绘一次，适合卡片密集页面
class AnimatedStackedWidget : public QStackedWidget {
    Q_OBJECT
public:
    explicit AnimatedStackedWidget(QWidget* parent = nullptr);

    void setCurrentIndex(int index, bool animate = true);
    void setCurrentWidget(QWidget* widget, bool animate = true);
    int currentIndex() const { return QStackedWidget::currentIndex(); }

    // 动画参数
    void setDuration(int ms) { m_duration = ms; }
    int duration() const { return m_duration; }

    // 动画进度（0~1，paintEvent 用）
    double progress() const { return m_progress; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_duration = 180;
    double m_progress = 1.0;      // 1=完成
    bool m_animRunning = false;
    QWidget* m_oldWidget = nullptr;
    QWidget* m_newWidget = nullptr;
};
