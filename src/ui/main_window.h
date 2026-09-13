#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QList>

class QPushButton;
class QLabel;
class QVBoxLayout;
class QFrame;
class ScoopService;
class SearchPage;
class InstalledPage;
class BucketPage;
class SettingsPage;
class AnimatedStackedWidget;
class ActivityBar;

// 主窗口：VSCode 风格（左侧活动栏 + 顶部标题栏 + 内容区）
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ScoopService* service, QWidget* parent = nullptr);

public slots:
    void navigateTo(int pageIndex, bool animate = true);   // 0=search 1=installed 2=bucket 3=settings

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onScoopDetected(bool installed);
    void onSettingsChanged();

private:
    void setupUi();
    void setupTray();
    void applyTheme(const QString& theme);

    ScoopService* m_service;
    AnimatedStackedWidget* m_stack = nullptr;
    QList<QPushButton*> m_navButtons;
    ActivityBar* m_activityBar = nullptr;
    QFrame* m_titleBar = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    bool m_exiting = false;
    bool m_centered = false;
    bool m_dark = true;
    int m_currentPage = 0;

    SearchPage* m_searchPage = nullptr;
    InstalledPage* m_installedPage = nullptr;
    BucketPage* m_bucketPage = nullptr;
    SettingsPage* m_settingsPage = nullptr;
};
