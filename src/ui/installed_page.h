#pragma once

#include <QWidget>
#include "models/scoop_models.h"
#include "core/scoop_service.h"

class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;
class ScoopService;

// 已安装页（复刻原版 InstalledPage）
// 列表显示已安装包，支持 更新/卸载/hold/更新全部
class InstalledPage : public QWidget {
    Q_OBJECT
public:
    explicit InstalledPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

private slots:
    void onPackagesLoaded(QVector<InstalledPackage> packages);
    void onUpdateClicked();
    void onUninstallClicked();
    void onHoldClicked();
    void onUpdateAllClicked();
    void onRefreshClicked();
    void onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error);

private:
    void setupUi();
    void populateTable(const QVector<InstalledPackage>& packages);

    ScoopService* m_service;
    QTableWidget* m_table = nullptr;
    QPushButton* m_updateAllBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QLabel* m_countLabel = nullptr;
    QComboBox* m_filterCombo = nullptr;

    QVector<InstalledPackage> m_packages;
    bool m_loaded = false;
};
