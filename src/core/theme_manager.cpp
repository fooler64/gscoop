#include "core/theme_manager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

// 内置默认主题（ganyu 甘雨蓝白冰系）—— 当 JSON 缺失时的兜底
static const QMap<QString, QColor>& defaultGanyu() {
    static QMap<QString, QColor> c = {
        {"bg", QColor("#e8f4fa")},
        {"surface", QColor("#ffffff")},
        {"surface2", QColor("#f2f9fd")},
        {"surface3", QColor("#dceef7")},
        {"border", QColor("#c4dfec")},
        {"text", QColor("#1c3a4d")},
        {"textSub", QColor("#5f8299")},
        {"accent", QColor("#3d8bb5")},
        {"accentDim", QColor("#bcdcea")},
        {"active", QColor("#e3f2f9")},
        {"highlight", QColor("#d9ecf6")},
        {"gold", QColor("#c9a227")},
        {"danger", QColor("#c94f4f")},
        {"warn", QColor("#c1843a")},
        {"success", QColor("#3d9a7a")},
        {"activityBarBg", QColor("#dceef7")},
        {"activityBarIcon", QColor("#5f8299")},
        {"activityBarActive", QColor("#3d8bb5")},
        {"titleBarBg", QColor("#e8f4fa")},
        {"sideBarBg", QColor("#e3f1f8")},
    };
    return c;
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager mgr;
    return mgr;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent) {
    loadDefaults();
}

void ThemeManager::loadDefaults() {
    m_id = "ganyu";
    m_name = "甘雨·蓝白冰系";
    m_dark = false;
    m_colors = defaultGanyu();
}

QString ThemeManager::themesDir() {
    // 优先应用目录下 themes/，其次源码目录
    const QString appDir = QCoreApplication::applicationDirPath();
    QDir d1(appDir + "/themes");
    if (d1.exists()) return d1.absolutePath();
    QDir d2(QDir::currentPath() + "/themes");
    if (d2.exists()) return d2.absolutePath();
    return appDir + "/themes";
}

QVector<ThemeManager::ThemeInfo> ThemeManager::availableThemes() const {
    QVector<ThemeInfo> list;
    QDir dir(themesDir());
    const QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString& f : files) {
        QFile file(dir.filePath(f));
        if (!file.open(QIODevice::ReadOnly)) continue;
        const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        ThemeInfo info;
        info.id = obj.value("id").toString(QFileInfo(f).completeBaseName());
        info.name = obj.value("name").toString(info.id);
        info.dark = obj.value("dark").toBool(true);
        list.append(info);
        file.close();
    }
    return list;
}

bool ThemeManager::loadTheme(const QString& id) {
    // 尝试 themes/<id>.json
    const QString path = themesDir() + "/" + id + ".json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        // 尝试完整路径
        QFile f2(id);
        if (!f2.open(QIODevice::ReadOnly)) {
            loadDefaults();
            return false;
        }
        return loadThemeFromJson(f2.readAll(), id);
    }
    const bool ok = loadThemeFromJson(file.readAll(), id);
    file.close();
    return ok;
}

bool ThemeManager::loadThemeFromJson(const QByteArray& data, const QString& id) {
    const QJsonObject obj = QJsonDocument::fromJson(data).object();
    if (obj.isEmpty()) return false;

    m_id = obj.value("id").toString(id);
    m_name = obj.value("name").toString(m_id);
    m_dark = obj.value("dark").toBool(true);

    m_colors.clear();
    const QJsonObject colors = obj.value("colors").toObject();
    for (auto it = colors.begin(); it != colors.end(); ++it) {
        m_colors.insert(it.key(), QColor(it.value().toString()));
    }
    // 兜底：缺的 key 用默认甘雨色
    const auto defs = defaultGanyu();
    for (auto it = defs.begin(); it != defs.end(); ++it) {
        if (!m_colors.contains(it.key())) {
            m_colors.insert(it.key(), it.value());
        }
    }
    emit themeChanged();
    return true;
}

QColor ThemeManager::color(const QString& key) const {
    return m_colors.value(key, QColor(0, 0, 0));
}

QColor ThemeManager::color(const QString& key, const QColor& fallback) const {
    return m_colors.value(key, fallback);
}

QPalette ThemeManager::appPalette() const {
    QPalette p;
    const QColor bgC = color("bg");
    const QColor surfaceC = color("surface");
    const QColor surface2C = color("surface2");
    const QColor textC = color("text");
    const QColor textSubC = color("textSub");
    const QColor accentC = color("accent");
    const QColor accentDimC = color("accentDim");

    p.setColor(QPalette::Window, bgC);
    p.setColor(QPalette::WindowText, textC);
    p.setColor(QPalette::Base, surfaceC);
    p.setColor(QPalette::AlternateBase, surface2C);
    p.setColor(QPalette::Text, textC);
    p.setColor(QPalette::Button, surfaceC);
    p.setColor(QPalette::ButtonText, textC);
    p.setColor(QPalette::BrightText, textC);
    p.setColor(QPalette::Highlight, accentDimC);
    p.setColor(QPalette::HighlightedText, textC);
    p.setColor(QPalette::ToolTipBase, surface2C);
    p.setColor(QPalette::ToolTipText, textC);
    p.setColor(QPalette::PlaceholderText, textSubC);
    p.setColor(QPalette::Link, accentC);
    p.setColor(QPalette::Disabled, QPalette::WindowText, textSubC);
    p.setColor(QPalette::Disabled, QPalette::Text, textSubC);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, textSubC);
    p.setColor(QPalette::Disabled, QPalette::Highlight, surface2C);
    return p;
}

void ThemeManager::apply() {
    QApplication* app = qApp;
    if (!app) return;
    app->setPalette(appPalette());
    emit themeChanged();
}
