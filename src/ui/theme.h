#pragma once

#include <QPalette>
#include <QColor>

// gScoop 主题：甘雨（Ganyu）冰蓝配色，VSCode 风格
// 完全代码驱动（QPalette + 自定义绘制），不使用 QSS

namespace Theme {

// 甘雨冰蓝配色（Ganyu ice-blue palette）
struct Ganyu {
    // 深色
    static QColor darkBg()        { return QColor("#101820"); }  // 深冰蓝背景
    static QColor darkSurface()   { return QColor("#16222d"); }  // 卡片/面板
    static QColor darkSurface2()  { return QColor("#1b2936"); }  // 面板2/hover
    static QColor darkBorder()    { return QColor("#243442"); }  // 边框
    static QColor darkText()      { return QColor("#e8f4f8"); }  // 主文字（淡蓝白）
    static QColor darkTextSub()   { return QColor("#8fa7b5"); }  // 次要文字
    static QColor darkAccent()    { return QColor("#7cc4dd"); }  // 甘雨冰蓝主色
    static QColor darkAccentDim() { return QColor("#3d6e85"); }  // 主色暗版（选中背景）
    static QColor darkActive()    { return QColor("#0e2530"); }  // 活动项背景
    static QColor darkHighlight() { return QColor("#2a4a5c"); }  // hover 背景
    static QColor darkDanger()    { return QColor("#e06c75"); }
    static QColor darkWarn()      { return QColor("#d19a66"); }
    static QColor darkSuccess()   { return QColor("#98c379"); }

    // 浅色
    static QColor lightBg()       { return QColor("#f2f7fa"); }
    static QColor lightSurface()  { return QColor("#ffffff"); }
    static QColor lightSurface2() { return QColor("#eef4f8"); }
    static QColor lightBorder()   { return QColor("#d5e3ec"); }
    static QColor lightText()     { return QColor("#1c2a35"); }
    static QColor lightTextSub()  { return QColor("#5d7484"); }
    static QColor lightAccent()   { return QColor("#3a8fb7"); }
    static QColor lightAccentDim(){ return QColor("#cfe8f2"); }
    static QColor lightActive()   { return QColor("#dceef5"); }
    static QColor lightHighlight(){ return QColor("#e3f1f7"); }
    static QColor lightDanger()   { return QColor("#d1453f"); }
    static QColor lightWarn()     { return QColor("#c1843a"); }
    static QColor lightSuccess()  { return QColor("#3d9a5f"); }
};

// 应用主题到 QApplication（纯代码，无 QSS）
void applyToApplication(bool dark);

// 设置控件的调色板（对单个 widget）
void applyWidgetPalette(QWidget* w, bool dark);

// 主窗口活动栏配色
QColor activityBarBg(bool dark);    // VSCode 活动栏
QColor activityBarIcon(bool dark);
QColor activityBarActive(bool dark);

// 工具函数
QColor blend(const QColor& a, const QColor& b, double t);

} // namespace Theme
