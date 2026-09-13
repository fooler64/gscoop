// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QProxyStyle>

// 现代化控件样式（QProxyStyle，非 QSS）
// 统一美化 QPushButton/QLineEdit/QComboBox/QCheckBox/QSpinBox：
//   - 圆角背景 + 细边框
//   - hover 高亮、按下加深
//   - 颜色全部从 ThemeManager 读取（JSON 主题驱动）
class ModernStyle : public QProxyStyle {
    Q_OBJECT
public:
    explicit ModernStyle(QStyle* base = nullptr);

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                       QPainter* painter, const QWidget* widget) const override;
    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                            QPainter* painter, const QWidget* widget) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
                         SubControl sc, const QWidget* widget) const override;

private:
    QColor themeColor(const char* key) const;
    QColor blended(const QColor& a, const QColor& b, double t) const;
    void drawButton(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    void drawLineEdit(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    void drawComboBox(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    void drawCheckBox(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
    void drawSpinBox(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
};
