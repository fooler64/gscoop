// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QList>

class QPushButton;
class QVBoxLayout;

// VSCode 风格活动栏（左侧窄图标栏）
// 甘雨冰蓝配色，代码绘制
class ActivityBar : public QWidget {
    Q_OBJECT
public:
    explicit ActivityBar(QWidget* parent = nullptr);

    // 添加一个活动项（返回按钮供外部连接）
    QPushButton* addItem(const QString& icon, const QString& tooltip);

    void setActive(int index);
    int activeIndex() const { return m_active; }
    void setDark(bool dark);

signals:
    void itemClicked(int index);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVBoxLayout* m_layout = nullptr;
    QList<QPushButton*> m_buttons;
    int m_active = 0;
    bool m_dark = true;
};
