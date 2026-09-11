#pragma once

#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

// 现代化页面切换动画：
// 新页面从右侧滑入 + 淡入，旧页面淡出
// 用法与 QStackedWidget 相同，setCurrentIndex 会自动播放动画
class AnimatedStackedWidget : public QStackedWidget {
    Q_OBJECT
    Q_PROPERTY(double fadeValue READ fadeValue WRITE setFadeValue)
public:
    explicit AnimatedStackedWidget(QWidget* parent = nullptr);

    void setCurrentIndex(int index, bool animate = true);
    void setCurrentWidget(QWidget* widget, bool animate = true);
    int currentIndex() const { return QStackedWidget::currentIndex(); }

    // 动画参数
    void setDuration(int ms) { m_duration = ms; }
    int duration() const { return m_duration; }

    double fadeValue() const { return m_fadeValue; }
    void setFadeValue(double v) { m_fadeValue = v; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_duration = 220;
    double m_fadeValue = 1.0;
    bool m_animRunning = false;
    QWidget* m_oldWidget = nullptr;
    QWidget* m_newWidget = nullptr;
};
