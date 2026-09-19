// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QStackedWidget>
#include <QVariantAnimation>
#include <QPixmap>

// 页面切换动画（双不透明快照互补滑动，零重叠）：
// 旧页滑出、新页滑入，两页各占一半区域、互补拼接，不会出现半透明叠加/重影。
// - 方向感知：切到后面的页 → 新页从右(下)滑入；切回前面的页 → 从左(上)滑入
// - 轴向可配：主页面用水平滑动；设置子标签（竖直列表）用垂直滑动
// - 快照用 QPixmap（每帧仅位图搬运），卡片密集页面也流畅
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

    // 滑动轴向：Horizontal（主页面）/ Vertical（设置子标签）
    void setSlideAxis(Qt::Orientation axis) { m_axis = axis; }
    Qt::Orientation slideAxis() const { return m_axis; }

    // 滑动距离比例（0.6~1.0；越小越轻快）
    void setSlideExtent(double ratio) { m_extent = qBound(0.5, ratio, 1.0); }
    double slideExtent() const { return m_extent; }

    double progress() const { return m_progress; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void stopAnimation(bool commit);
    void drawTransition(QPainter& painter) const;

    int m_duration = 220;
    double m_progress = 1.0;
    bool m_animRunning = false;
    bool m_forward = true;
    Qt::Orientation m_axis = Qt::Horizontal;
    double m_extent = 1.0;
    int m_targetIndex = -1;
    QWidget* m_oldWidget = nullptr;
    QWidget* m_newWidget = nullptr;
    QPixmap m_oldSnapshot;
    QPixmap m_newSnapshot;
    QVariantAnimation* m_anim = nullptr;
};
