#include "ui/modern_style.h"

#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionButton>
#include <QStyleOptionFrame>
#include <QStyleOptionComboBox>
#include <QStyleOptionSpinBox>
#include <QApplication>
#include <QLineEdit>
#include <QComboBox>

#include "core/theme_manager.h"

ModernStyle::ModernStyle(QStyle* base)
    : QProxyStyle(base) {}

QColor ModernStyle::themeColor(const char* key) const {
    return ThemeManager::instance().color(key);
}

QColor ModernStyle::blended(const QColor& a, const QColor& b, double t) const {
    return QColor(int(a.red() + (b.red() - a.red()) * t),
                  int(a.green() + (b.green() - a.green()) * t),
                  int(a.blue() + (b.blue() - a.blue()) * t));
}

void ModernStyle::drawButton(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    const auto* btn = qstyleoption_cast<const QStyleOptionButton*>(option);
    if (!btn) { QProxyStyle::drawControl(CE_PushButton, option, painter, widget); return; }

    const bool dark = ThemeManager::instance().isDark();
    const bool enabled = btn->state & State_Enabled;
    const bool hovered = btn->state & State_MouseOver;
    const bool pressed = btn->state & State_Sunken;
    const bool primary = widget && widget->property("primary").toBool();

    QRectF r = option->rect.adjusted(1, 1, -1, -1);
    QColor bg;
    QColor border;

    if (!enabled) {
        bg = themeColor("surface3");
        border = themeColor("border");
        border.setAlpha(120);
    } else if (primary) {
        bg = themeColor("accent");
        border = bg;
        if (hovered) bg = blended(bg, Qt::white, 0.12);
        if (pressed) bg = blended(bg, Qt::black, 0.15);
    } else {
        bg = themeColor("surface2");
        border = themeColor("border");
        if (hovered) bg = blended(bg, themeColor("accentDim"), 0.25);
        if (pressed) bg = blended(bg, themeColor("accentDim"), 0.45);
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(border, 1));
    painter->setBrush(bg);
    painter->drawRoundedRect(r, 6, 6);

    // 文字/图标由基类画，但我们需要在圆角内画
    // 基类会画文字，我们只需确保背景不遮文字
    QStyleOptionButton copy = *btn;
    copy.rect = option->rect;
    copy.palette.setColor(QPalette::ButtonText,
                          primary ? themeColor("surface") : themeColor("text"));
    if (!enabled) {
        copy.palette.setColor(QPalette::ButtonText, themeColor("textSub"));
    }
    // 画文字（用基类，但关掉背景绘制避免重画方形）
    QProxyStyle::drawControl(CE_PushButtonLabel, &copy, painter, widget);
}

void ModernStyle::drawLineEdit(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    const auto* frame = qstyleoption_cast<const QStyleOptionFrame*>(option);
    if (!frame) { QProxyStyle::drawPrimitive(PE_PanelLineEdit, option, painter, widget); return; }

    const bool dark = ThemeManager::instance().isDark();
    const bool enabled = option->state & State_Enabled;
    const bool focus = option->state & State_HasFocus;
    const bool hovered = option->state & State_MouseOver;

    QRectF r = option->rect.adjusted(1, 1, -1, -1);
    QColor bg = enabled ? themeColor("surface") : themeColor("surface2");
    QColor border = themeColor("border");
    if (focus) border = themeColor("accent");
    else if (hovered) border = blended(border, themeColor("accent"), 0.4);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(border, focus ? 1.6 : 1));
    painter->setBrush(bg);
    painter->drawRoundedRect(r, 6, 6);
}

void ModernStyle::drawComboBox(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    const auto* cb = qstyleoption_cast<const QStyleOptionComboBox*>(option);
    if (!cb) {
        const auto* oc = qstyleoption_cast<const QStyleOptionComplex*>(option);
        if (oc) { QProxyStyle::drawComplexControl(CC_ComboBox, oc, painter, widget); }
        return;
    }

    const bool dark = ThemeManager::instance().isDark();
    const bool enabled = option->state & State_Enabled;
    const bool hovered = option->state & State_MouseOver;

    QRectF r = option->rect.adjusted(1, 1, -1, -1);
    QColor bg = enabled ? themeColor("surface") : themeColor("surface2");
    QColor border = themeColor("border");
    if (hovered) border = blended(border, themeColor("accent"), 0.4);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(border, 1));
    painter->setBrush(bg);
    painter->drawRoundedRect(r, 6, 6);

    // 下拉箭头
    QStyleOptionComboBox copy = *cb;
    copy.rect = option->rect;
    QRect arrowRect = subControlRect(CC_ComboBox, &copy, SC_ComboBoxArrow, widget);
    const QColor arrowColor = enabled ? themeColor("textSub") : themeColor("border");
    QPolygonF tri;
    tri << QPointF(arrowRect.center().x() - 5, arrowRect.center().y() - 2)
        << QPointF(arrowRect.center().x() + 5, arrowRect.center().y() - 2)
        << QPointF(arrowRect.center().x(), arrowRect.center().y() + 4);
    painter->setPen(Qt::NoPen);
    painter->setBrush(arrowColor);
    painter->drawPolygon(tri);
}

void ModernStyle::drawCheckBox(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    const auto* cb = qstyleoption_cast<const QStyleOptionButton*>(option);
    if (!cb) { QProxyStyle::drawPrimitive(PE_IndicatorCheckBox, option, painter, widget); return; }

    const bool dark = ThemeManager::instance().isDark();
    const bool checked = cb->state & State_On;
    const bool hovered = cb->state & State_MouseOver;
    const bool enabled = cb->state & State_Enabled;

    const QRectF r = QRectF(option->rect).adjusted(1, 1, -1, -1);
    QColor bg = checked ? themeColor("accent") : themeColor("surface");
    QColor border = checked ? themeColor("accent") : themeColor("border");
    if (hovered && !checked) border = blended(border, themeColor("accent"), 0.4);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(border, 1.2));
    painter->setBrush(bg);
    painter->drawRoundedRect(r, 3, 3);

    if (checked) {
        // 对勾
        painter->setPen(QPen(themeColor("surface"), 1.8, Qt::SolidLine, Qt::RoundCap));
        const qreal x = r.left() + r.width() * 0.22;
        const qreal y = r.top() + r.height() * 0.5;
        painter->drawLine(QPointF(x, y), QPointF(x + r.width() * 0.22, y + r.height() * 0.25));
        painter->drawLine(QPointF(x + r.width() * 0.22, y + r.height() * 0.25),
                          QPointF(x + r.width() * 0.62, y - r.height() * 0.3));
    }
}

void ModernStyle::drawSpinBox(const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    const auto* sb = qstyleoption_cast<const QStyleOptionSpinBox*>(option);
    if (!sb) {
        const auto* oc = qstyleoption_cast<const QStyleOptionComplex*>(option);
        if (oc) { QProxyStyle::drawComplexControl(CC_SpinBox, oc, painter, widget); }
        return;
    }

    const bool dark = ThemeManager::instance().isDark();
    const bool enabled = option->state & State_Enabled;
    const bool focus = option->state & State_HasFocus;

    QRectF r = option->rect.adjusted(1, 1, -1, -1);
    QColor bg = enabled ? themeColor("surface") : themeColor("surface2");
    QColor border = focus ? themeColor("accent") : themeColor("border");

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(border, focus ? 1.6 : 1));
    painter->setBrush(bg);
    painter->drawRoundedRect(r, 6, 6);
}

void ModernStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                QPainter* painter, const QWidget* widget) const {
    switch (element) {
    case PE_PanelLineEdit:
        drawLineEdit(option, painter, widget);
        return;
    case PE_IndicatorCheckBox:
        drawCheckBox(option, painter, widget);
        return;
    case PE_Frame:
        drawLineEdit(option, painter, widget);
        return;
    default:
        break;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void ModernStyle::drawControl(ControlElement element, const QStyleOption* option,
                              QPainter* painter, const QWidget* widget) const {
    switch (element) {
    case CE_PushButton:
        drawButton(option, painter, widget);
        return;
    default:
        break;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void ModernStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                                     QPainter* painter, const QWidget* widget) const {
    switch (control) {
    case CC_ComboBox:
        drawComboBox(option, painter, widget);
        return;
    case CC_SpinBox:
        drawSpinBox(option, painter, widget);
        return;
    default:
        break;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

QRect ModernStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
                                  SubControl sc, const QWidget* widget) const {
    // 组合框箭头区域稍微内缩，避免贴近边框
    if (cc == CC_ComboBox && sc == SC_ComboBoxArrow) {
        QRect base = QProxyStyle::subControlRect(cc, opt, sc, widget);
        base.adjust(-4, 4, -8, -4);
        return base;
    }
    return QProxyStyle::subControlRect(cc, opt, sc, widget);
}
