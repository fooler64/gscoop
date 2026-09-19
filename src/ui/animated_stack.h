// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QStackedWidget>
#include <QVariantAnimation>
#include <QPixmap>

// 高性能页面切换动画（单快照滑入，零重叠）：
// 只绘制新页面快照从右滑入 + 容器底色兜底，旧页直接隐藏，
// 因此不存在两页叠加/重影。快照用 QPixmap（每帧仅位图搬运），流畅。
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

    // 动画进度（0~1）
    double progress() const { return m_progress; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void stopAnimation(bool commit);

    int m_duration = 200;
    double m_progress = 1.0;      // 1=完成
    bool m_animRunning = false;
    int m_targetIndex = -1;
    QWidget* m_oldWidget = nullptr;
    QWidget* m_newWidget = nullptr;
    QPixmap m_newSnapshot;
    QVariantAnimation* m_anim = nullptr;
};
