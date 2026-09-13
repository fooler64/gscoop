// SPDX-License-Identifier: LGPL-3.0-or-later
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
class QMouseEvent;

// 主窗口：VSCode 风格（左侧活动栏 + 顶部标题栏 + 内容区）
// 无边框：自绘标题栏 + 自绘最小化/最大化/关闭按钮
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ScoopService* service, QWidget* parent = nullptr);

public slots:
    void navigateTo(int pageIndex, bool animate = true);   // 0=search 1=installed 2=bucket 3=settings

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void changeEvent(QEvent* event) override;
    // 无边框窗口拖动（标题栏区域）
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    // 双击标题栏最大化/还原
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onScoopDetected(bool installed);
    void onSettingsChanged();
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();

private:
    void setupUi();
    void setupTray();
    void applyTheme(const QString& theme);
    void updateMaximizeIcon();

    ScoopService* m_service;
    AnimatedStackedWidget* m_stack = nullptr;
    QList<QPushButton*> m_navButtons;
    ActivityBar* m_activityBar = nullptr;
    QFrame* m_titleBar = nullptr;
    QPushButton* m_minBtn = nullptr;
    QPushButton* m_maxBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    bool m_exiting = false;
    bool m_centered = false;
    bool m_dark = true;
    bool m_dragging = false;
    bool m_maximized = false;
    QPoint m_dragOffset;
    QRect m_restoreGeometry;
    int m_currentPage = 0;

    SearchPage* m_searchPage = nullptr;
    InstalledPage* m_installedPage = nullptr;
    BucketPage* m_bucketPage = nullptr;
    SettingsPage* m_settingsPage = nullptr;
};
