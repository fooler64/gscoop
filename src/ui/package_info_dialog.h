#pragma once

#include <QDialog>

class QLabel;
class QVBoxLayout;
class QPushButton;
class ScoopService;

// 包信息弹窗（复刻原版 PackageInfoModal）
// 显示包名、版本、bucket、描述、主页、license，以及安装/卸载操作
class PackageInfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit PackageInfoDialog(ScoopService* service, const QString& packageName,
                               QWidget* parent = nullptr);

private slots:
    void onInstall();
    void onUninstall();

private:
    void fetchInfo();

    ScoopService* m_service;
    QString m_name;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QLabel* m_sourceLabel = nullptr;
    QLabel* m_descLabel = nullptr;
    QLabel* m_homepageLabel = nullptr;
    QLabel* m_licenseLabel = nullptr;
    QPushButton* m_installBtn = nullptr;
    QPushButton* m_uninstallBtn = nullptr;
};
