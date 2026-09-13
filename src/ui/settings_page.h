#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QLabel;
class QGroupBox;
class ScoopService;

// 设置页（复刻原版 SettingsPage 的核心项）
class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

private slots:
    void onSettingsChanged();

private:
    void setupUi();
    QGroupBox* makeGroupBox(const QString& title, QWidget* parent);

    ScoopService* m_service;
    QComboBox* m_themeCombo = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QComboBox* m_launchPageCombo = nullptr;
    QCheckBox* m_autoUpdateCheck = nullptr;
    QCheckBox* m_minimizeToTray = nullptr;
    QCheckBox* m_closeToTray = nullptr;
    QCheckBox* m_showUpdateBanner = nullptr;
    QLineEdit* m_proxyEdit = nullptr;
    QCheckBox* m_useProxy = nullptr;
    QLabel* m_scoopStatusLabel = nullptr;
};
