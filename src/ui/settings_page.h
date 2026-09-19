// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QFrame>
#include "models/scoop_models.h"
#include "core/scoop_service.h"

class QVariantAnimation;

class QComboBox;
class QCheckBox;
class QLineEdit;
class QLabel;
class QGroupBox;
class QListWidget;
class QVBoxLayout;
class QScrollArea;
class QPushButton;
class QPaintEvent;
class QMouseEvent;
class AnimatedStackedWidget;
class ScoopService;

// 设置页左侧标签按钮（自绘：圆角、选中高亮，带颜色过渡动画）
class SettingsTabButton : public QFrame {
    Q_OBJECT
public:
    SettingsTabButton(const QString& text, const QString& icon, QWidget* parent = nullptr);
    // 选中状态（平滑过渡）
    void setActive(bool active);

signals:
    void clicked(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_text;
    QString m_icon;
    double m_activeProgress = 0.0;   // 0=未选中 1=选中（动画插值）
    QVariantAnimation* m_activeAnim = nullptr;
};

// 设置页（现代化：左侧标签导航 + 右侧内容区）
// 标签：自动化 / 管理（含环境自检+清理） / 安全（VirusTotal） / 窗口 / 托盘 / 关于
class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

private slots:
    void onSettingsChanged();
    void onTabChanged(int index);
    void onRunDoctor();
    void onCleanupApps();
    void onCleanupCache();

private:
    void setupUi();
    void buildAutomationTab(QWidget* page);
    void buildManagementTab(QWidget* page);
    void buildSecurityTab(QWidget* page);
    void buildWindowTab(QWidget* page);
    void buildTrayTab(QWidget* page);
    void buildAboutTab(QWidget* page);
    QGroupBox* makeGroupBox(const QString& title, QWidget* parent);
    QWidget* makeTabPage();
    void populateDoctorResults(const QVector<DoctorCheckItem>& items);

    ScoopService* m_service;
    QListWidget* m_tabList = nullptr;
    AnimatedStackedWidget* m_stack = nullptr;

    // 自动化
    QCheckBox* m_autoUpdateCheck = nullptr;
    QCheckBox* m_showUpdateBanner = nullptr;
    // 管理
    QPushButton* m_runDoctorBtn = nullptr;
    QPushButton* m_cleanupAppsBtn = nullptr;
    QPushButton* m_cleanupCacheBtn = nullptr;
    QLabel* m_doctorStatusLabel = nullptr;
    QVBoxLayout* m_doctorResultsLayout = nullptr;
    QWidget* m_doctorResultsHost = nullptr;
    // 安全
    QLineEdit* m_vtApiKeyEdit = nullptr;
    QCheckBox* m_useProxy = nullptr;
    QLineEdit* m_proxyEdit = nullptr;
    // 窗口
    QComboBox* m_themeCombo = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QComboBox* m_launchPageCombo = nullptr;
    // 托盘
    QCheckBox* m_minimizeToTray = nullptr;
    QCheckBox* m_closeToTray = nullptr;
    // 关于
    QLabel* m_scoopStatusLabel = nullptr;
};
