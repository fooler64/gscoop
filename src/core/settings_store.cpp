// SPDX-License-Identifier: LGPL-3.0-or-later
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
    m_settings.theme = m_qsettings.value("ui/theme", "ganyu").toString();
    // 旧版本存了 system/dark 的迁移到 ganyu
    if (m_settings.theme == "system" || m_settings.theme == "dark") {
        m_settings.theme = "ganyu";
        m_qsettings.setValue("ui/theme", "ganyu");
    }
    m_settings.language = m_qsettings.value("ui/language", "zh-CN").toString();
    m_settings.defaultLaunchPage = m_qsettings.value("ui/launchPage", "search").toString();
    m_settings.autoUpdateCheck = m_qsettings.value("ui/autoUpdateCheck", true).toBool();
    m_settings.minimizeToTray = m_qsettings.value("tray/minimizeToTray", false).toBool();
    m_settings.closeToTray = m_qsettings.value("tray/closeToTray", false).toBool();
    m_settings.showUpdateBanner = m_qsettings.value("ui/showUpdateBanner", true).toBool();
    m_settings.proxyUrl = m_qsettings.value("net/proxyUrl", "").toString();
    m_settings.useProxy = m_qsettings.value("net/useProxy", false).toBool();
    m_settings.virusTotalApiKey = m_qsettings.value("security/vtApiKey", "").toString();
    m_settings.rememberWindowSize = m_qsettings.value("window/rememberSize", true).toBool();
    m_settings.startMinimized = m_qsettings.value("window/startMinimized", false).toBool();
    m_settings.autoBucketUpdate = m_qsettings.value("bucket/autoUpdate", false).toBool();
    m_settings.bucketUpdateHours = m_qsettings.value("bucket/updateHours", 24).toInt();
    m_settings.searchHistory = m_qsettings.value("search/history").toStringList();
    m_settings.favorites = m_qsettings.value("search/favorites").toStringList();
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
    m_qsettings.setValue("security/vtApiKey", m_settings.virusTotalApiKey);
    m_qsettings.setValue("window/rememberSize", m_settings.rememberWindowSize);
    m_qsettings.setValue("window/startMinimized", m_settings.startMinimized);
    m_qsettings.setValue("bucket/autoUpdate", m_settings.autoBucketUpdate);
    m_qsettings.setValue("bucket/updateHours", m_settings.bucketUpdateHours);
    m_qsettings.setValue("search/history", m_settings.searchHistory);
    m_qsettings.setValue("search/favorites", m_settings.favorites);
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

void SettingsStore::setVirusTotalApiKey(const QString& key) {
    m_settings.virusTotalApiKey = key;
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

void SettingsStore::setAutoBucketUpdate(bool on, int hours) {
    m_settings.autoBucketUpdate = on;
    m_settings.bucketUpdateHours = hours;
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::addSearchHistory(const QString& term) {
    const QString t = term.trimmed();
    if (t.isEmpty()) return;
    m_settings.searchHistory.removeAll(t);
    m_settings.searchHistory.prepend(t);
    while (m_settings.searchHistory.size() > 20) m_settings.searchHistory.removeLast();
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::clearSearchHistory() {
    m_settings.searchHistory.clear();
    save();
    emit settingsChanged(m_settings);
}

void SettingsStore::toggleFavorite(const QString& name) {
    if (m_settings.favorites.contains(name)) m_settings.favorites.removeAll(name);
    else m_settings.favorites.prepend(name);
    save();
    emit settingsChanged(m_settings);
}

bool SettingsStore::isFavorite(const QString& name) const {
    return m_settings.favorites.contains(name);
}
