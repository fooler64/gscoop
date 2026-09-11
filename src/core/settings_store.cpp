#include "core/settings_store.h"
#include <QStandardPaths>
#include <QDir>

SettingsStore& SettingsStore::instance() {
    static SettingsStore store;
    return store;
}

SettingsStore::SettingsStore(QObject* parent)
    : QObject(parent),
      m_qsettings(QSettings::IniFormat, QSettings::UserScope,
                  "gscoop", "gscoop") {
    load();
}

void SettingsStore::load() {
    m_settings.theme = m_qsettings.value("ui/theme", "system").toString();
    m_settings.language = m_qsettings.value("ui/language", "zh-CN").toString();
    m_settings.defaultLaunchPage = m_qsettings.value("ui/launchPage", "search").toString();
    m_settings.autoUpdateCheck = m_qsettings.value("ui/autoUpdateCheck", true).toBool();
    m_settings.minimizeToTray = m_qsettings.value("tray/minimizeToTray", false).toBool();
    m_settings.closeToTray = m_qsettings.value("tray/closeToTray", false).toBool();
    m_settings.showUpdateBanner = m_qsettings.value("ui/showUpdateBanner", true).toBool();
    m_settings.proxyUrl = m_qsettings.value("net/proxyUrl", "").toString();
    m_settings.useProxy = m_qsettings.value("net/useProxy", false).toBool();
    m_settings.rememberWindowSize = m_qsettings.value("window/rememberSize", true).toBool();
    m_settings.startMinimized = m_qsettings.value("window/startMinimized", false).toBool();
}

void SettingsStore::save() {
    m_qsettings.setValue("ui/theme", m_settings.theme);
    m_qsettings.setValue("ui/language", m_settings.language);
    m_qsettings.setValue("ui/launchPage", m_settings.defaultLaunchPage);
    m_qsettings.setValue("ui/autoUpdateCheck", m_settings.autoUpdateCheck);
    m_qsettings.setValue("tray/minimizeToTray", m_settings.minimizeToTray);
    m_qsettings.setValue("tray/closeToTray", m_settings.closeToTray);
    m_qsettings.setValue("ui/showUpdateBanner", m_settings.showUpdateBanner);
    m_qsettings.setValue("net/proxyUrl", m_settings.proxyUrl);
    m_qsettings.setValue("net/useProxy", m_settings.useProxy);
    m_qsettings.setValue("window/rememberSize", m_settings.rememberWindowSize);
    m_qsettings.setValue("window/startMinimized", m_settings.startMinimized);
    m_qsettings.sync();
}

void SettingsStore::setTheme(const QString& theme) {
    if (m_settings.theme == theme) return;
    m_settings.theme = theme;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setLanguage(const QString& lang) {
    if (m_settings.language == lang) return;
    m_settings.language = lang;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setLaunchPage(const QString& page) {
    if (m_settings.defaultLaunchPage == page) return;
    m_settings.defaultLaunchPage = page;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setAutoUpdateCheck(bool on) {
    m_settings.autoUpdateCheck = on;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setMinimizeToTray(bool on) {
    m_settings.minimizeToTray = on;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setCloseToTray(bool on) {
    m_settings.closeToTray = on;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setShowUpdateBanner(bool on) {
    m_settings.showUpdateBanner = on;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setProxy(const QString& url, bool use) {
    m_settings.proxyUrl = url;
    m_settings.useProxy = use;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setRememberWindowSize(bool on) {
    m_settings.rememberWindowSize = on;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::setStartMinimized(bool on) {
    m_settings.startMinimized = on;
    save();
    emit settingsChanged(m_settings);
}
