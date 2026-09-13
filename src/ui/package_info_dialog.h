// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QDialog>

class QLabel;
class QVBoxLayout;
class QPushButton;
class QScrollArea;
class QMouseEvent;
class ScoopService;

// 包信息弹窗（复刻原版 PackageInfoModal）
// 显示包名、版本、bucket、描述、主页、license、依赖、URL 等完整信息
class PackageInfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit PackageInfoDialog(ScoopService* service, const QString& packageName,
                               QWidget* parent = nullptr);

private slots:
    void onInstall();
    void onUninstall();
    void onScanVirusTotal();

protected:
    // 无边框窗口拖动
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void fetchInfo();
    QString findManifest();

    ScoopService* m_service;
    QString m_name;
    bool m_dragging = false;
    QPoint m_dragOffset;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QLabel* m_sourceLabel = nullptr;
    QLabel* m_descLabel = nullptr;
    QLabel* m_homepageLabel = nullptr;
    QLabel* m_licenseLabel = nullptr;
    QLabel* m_dependsLabel = nullptr;
    QLabel* m_urlLabel = nullptr;
    QLabel* m_authorLabel = nullptr;
    QLabel* m_notesLabel = nullptr;
    QPushButton* m_installBtn = nullptr;
    QPushButton* m_uninstallBtn = nullptr;
    QPushButton* m_vtBtn = nullptr;
    QScrollArea* m_scroll = nullptr;
};
