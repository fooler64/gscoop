#pragma once

#include <QObject>
#include <QColor>
#include <QMap>
#include <QStringList>
#include <QVector>
#include <QPalette>

// 主题管理器：从 JSON 文件加载主题色，替代硬编码
// 用法：ThemeManager::instance().color("surface")
// 主题文件位于 <app>/themes/*.json
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    // 主题列表（扫描 themes 目录）
    struct ThemeInfo {
        QString id;
        QString name;
        bool dark = true;
    };
    QVector<ThemeInfo> availableThemes() const;

    // 加载指定主题（id 或文件名）
    bool loadTheme(const QString& id);

    // 当前主题信息
    QString currentId() const { return m_id; }
    QString currentName() const { return m_name; }
    bool isDark() const { return m_dark; }

    // 取色（找不到返回 fallback 黑色）
    QColor color(const QString& key) const;
    QColor color(const QString& key, const QColor& fallback) const;
    // border 色（供 appPalette 使用）
    QColor borderColor() const;

    // 生成全局 QPalette
    QPalette appPalette() const;
    // 应用到 QApplication
    void apply();

    // 主题文件目录
    static QString themesDir();

signals:
    void themeChanged();

private:
    explicit ThemeManager(QObject* parent = nullptr);
    void loadDefaults();
    bool loadThemeFromJson(const QByteArray& data, const QString& id);

    QString m_id = "ganyu";
    QString m_name = "甘雨·蓝白冰系";
    bool m_dark = false;
    QMap<QString, QColor> m_colors;
};
