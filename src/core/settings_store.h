#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <QStringList>

// 应用设置存储（对应原版 settings）
// 主题/语言/启动页/窗口行为等
struct AppSettings {
    QString theme = "system";        // system | light | dark
    QString language = "zh-CN";      // 界面语言
    QString defaultLaunchPage = "search"; // search | installed | buckets | settings | doctor
    bool autoUpdateCheck = true;
    bool minimizeToTray = false;
    bool closeToTray = false;
    bool showUpdateBanner = true;
    QString proxyUrl;                // 可选代理
    bool useProxy = false;
    QString virusTotalApiKey;        // VirusTotal API key（可选）
    // 窗口
    bool rememberWindowSize = true;
    bool startMinimized = false;
};

class SettingsStore : public QObject {
    Q_OBJECT
public:
    static SettingsStore& instance();

    AppSettings settings() const { return m_settings; }

    void setTheme(const QString& theme);
    void setLanguage(const QString& lang);
    void setLaunchPage(const QString& page);
    void setAutoUpdateCheck(bool on);
    void setMinimizeToTray(bool on);
    void setCloseToTray(bool on);
    void setShowUpdateBanner(bool on);
    void setProxy(const QString& url, bool use);
    void setVirusTotalApiKey(const QString& key);
    void setRememberWindowSize(bool on);
    void setStartMinimized(bool on);

signals:
    void settingsChanged(const AppSettings& settings);

private:
    explicit SettingsStore(QObject* parent = nullptr);
    void load();
    void save();

    AppSettings m_settings;
    QSettings m_qsettings;
};
