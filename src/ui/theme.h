#pragma once

#include <QColor>
#include "core/theme_manager.h"

// 主题取色：转发到 ThemeManager（JSON 驱动，无硬编码）
// 用法：Theme::color("surface")
namespace Theme {

// 语义化取色（内部映射到 ThemeManager keys）
inline QColor bg(bool)          { return ThemeManager::instance().color("bg"); }
inline QColor surface(bool)     { return ThemeManager::instance().color("surface"); }
inline QColor surface2(bool)    { return ThemeManager::instance().color("surface2"); }
inline QColor surface3(bool)    { return ThemeManager::instance().color("surface3"); }
inline QColor border(bool)      { return ThemeManager::instance().color("border"); }
inline QColor text(bool)        { return ThemeManager::instance().color("text"); }
inline QColor textSub(bool)     { return ThemeManager::instance().color("textSub"); }
inline QColor accent(bool)      { return ThemeManager::instance().color("accent"); }
inline QColor accentDim(bool)   { return ThemeManager::instance().color("accentDim"); }
inline QColor active(bool)      { return ThemeManager::instance().color("active"); }
inline QColor highlight(bool)   { return ThemeManager::instance().color("highlight"); }
inline QColor gold(bool)        { return ThemeManager::instance().color("gold"); }
inline QColor danger(bool)      { return ThemeManager::instance().color("danger"); }
inline QColor warn(bool)        { return ThemeManager::instance().color("warn"); }
inline QColor success(bool)     { return ThemeManager::instance().color("success"); }

// 状态色
inline QColor outdated(bool)    { return ThemeManager::instance().color("warn"); }
inline QColor installed(bool)   { return ThemeManager::instance().color("success"); }
inline QColor failed(bool)      { return ThemeManager::instance().color("danger"); }
inline QColor held(bool)        { return ThemeManager::instance().color("accent"); }

// VSCode 组件色
inline QColor activityBarBg(bool)    { return ThemeManager::instance().color("activityBarBg"); }
inline QColor activityBarIcon(bool)  { return ThemeManager::instance().color("activityBarIcon"); }
inline QColor activityBarActive(bool){ return ThemeManager::instance().color("activityBarActive"); }
inline QColor titleBarBg(bool)       { return ThemeManager::instance().color("titleBarBg"); }
inline QColor sideBarBg(bool)        { return ThemeManager::instance().color("sideBarBg"); }

// 应用
inline void applyToApplication(bool) { ThemeManager::instance().apply(); }

// 工具
QColor blend(const QColor& a, const QColor& b, double t);

} // namespace Theme
