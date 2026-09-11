#include "ui/theme.h"

#include <QApplication>
#include <QPalette>
#include <QWidget>
#include <QColor>

namespace Theme {

QColor blend(const QColor& a, const QColor& b, double t) {
    return QColor(
        int(a.red() + (b.red() - a.red()) * t),
        int(a.green() + (b.green() - a.green()) * t),
        int(a.blue() + (b.blue() - a.blue()) * t));
}

QColor activityBarBg(bool dark) {
    return dark ? QColor("#0a1015") : QColor("#e8eef2");
}

QColor activityBarIcon(bool dark) {
    return dark ? QColor("#7f97a5") : QColor("#5d7484");
}

QColor activityBarActive(bool dark) {
    return dark ? QColor("#7cc4dd") : QColor("#2f6f8f");
}

void applyWidgetPalette(QWidget* w, bool dark) {
    QPalette p = w->palette();
    if (dark) {
        p.setColor(QPalette::Window, Ganyu::darkBg());
        p.setColor(QPalette::WindowText, Ganyu::darkText());
        p.setColor(QPalette::Base, Ganyu::darkSurface());
        p.setColor(QPalette::AlternateBase, Ganyu::darkSurface2());
        p.setColor(QPalette::Text, Ganyu::darkText());
        p.setColor(QPalette::Button, Ganyu::darkSurface());
        p.setColor(QPalette::ButtonText, Ganyu::darkText());
        p.setColor(QPalette::Highlight, Ganyu::darkAccentDim());
        p.setColor(QPalette::HighlightedText, Ganyu::darkText());
        p.setColor(QPalette::ToolTipBase, Ganyu::darkSurface2());
        p.setColor(QPalette::ToolTipText, Ganyu::darkText());
        p.setColor(QPalette::PlaceholderText, Ganyu::darkTextSub());
        p.setColor(QPalette::Disabled, QPalette::Text, Ganyu::darkTextSub());
        p.setColor(QPalette::Disabled, QPalette::ButtonText, Ganyu::darkTextSub());
    } else {
        p.setColor(QPalette::Window, Ganyu::lightBg());
        p.setColor(QPalette::WindowText, Ganyu::lightText());
        p.setColor(QPalette::Base, Ganyu::lightSurface());
        p.setColor(QPalette::AlternateBase, Ganyu::lightSurface2());
        p.setColor(QPalette::Text, Ganyu::lightText());
        p.setColor(QPalette::Button, Ganyu::lightSurface());
        p.setColor(QPalette::ButtonText, Ganyu::lightText());
        p.setColor(QPalette::Highlight, Ganyu::lightAccentDim());
        p.setColor(QPalette::HighlightedText, Ganyu::lightText());
        p.setColor(QPalette::ToolTipBase, Ganyu::lightSurface());
        p.setColor(QPalette::ToolTipText, Ganyu::lightText());
        p.setColor(QPalette::PlaceholderText, Ganyu::lightTextSub());
        p.setColor(QPalette::Disabled, QPalette::Text, Ganyu::lightTextSub());
        p.setColor(QPalette::Disabled, QPalette::ButtonText, Ganyu::lightTextSub());
    }
    w->setPalette(p);
}

void applyToApplication(bool dark) {
    QApplication* app = qApp;
    if (!app) return;

    QPalette p = app->palette();
    if (dark) {
        p.setColor(QPalette::Window, Ganyu::darkBg());
        p.setColor(QPalette::WindowText, Ganyu::darkText());
        p.setColor(QPalette::Base, Ganyu::darkSurface());
        p.setColor(QPalette::AlternateBase, Ganyu::darkSurface2());
        p.setColor(QPalette::Text, Ganyu::darkText());
        p.setColor(QPalette::Button, Ganyu::darkSurface());
        p.setColor(QPalette::ButtonText, Ganyu::darkText());
        p.setColor(QPalette::Highlight, Ganyu::darkAccentDim());
        p.setColor(QPalette::HighlightedText, Ganyu::darkText());
        p.setColor(QPalette::ToolTipBase, Ganyu::darkSurface2());
        p.setColor(QPalette::ToolTipText, Ganyu::darkText());
        p.setColor(QPalette::PlaceholderText, Ganyu::darkTextSub());
        p.setColor(QPalette::Link, Ganyu::darkAccent());
        p.setColor(QPalette::Disabled, QPalette::WindowText, Ganyu::darkTextSub());
        p.setColor(QPalette::Disabled, QPalette::Text, Ganyu::darkTextSub());
        p.setColor(QPalette::Disabled, QPalette::ButtonText, Ganyu::darkTextSub());
    } else {
        p.setColor(QPalette::Window, Ganyu::lightBg());
        p.setColor(QPalette::WindowText, Ganyu::lightText());
        p.setColor(QPalette::Base, Ganyu::lightSurface());
        p.setColor(QPalette::AlternateBase, Ganyu::lightSurface2());
        p.setColor(QPalette::Text, Ganyu::lightText());
        p.setColor(QPalette::Button, Ganyu::lightSurface());
        p.setColor(QPalette::ButtonText, Ganyu::lightText());
        p.setColor(QPalette::Highlight, Ganyu::lightAccentDim());
        p.setColor(QPalette::HighlightedText, Ganyu::lightText());
        p.setColor(QPalette::ToolTipBase, Ganyu::lightSurface());
        p.setColor(QPalette::ToolTipText, Ganyu::lightText());
        p.setColor(QPalette::PlaceholderText, Ganyu::lightTextSub());
        p.setColor(QPalette::Link, Ganyu::lightAccent());
        p.setColor(QPalette::Disabled, QPalette::WindowText, Ganyu::lightTextSub());
        p.setColor(QPalette::Disabled, QPalette::Text, Ganyu::lightTextSub());
        p.setColor(QPalette::Disabled, QPalette::ButtonText, Ganyu::lightTextSub());
    }
    app->setPalette(p);
}

} // namespace Theme
