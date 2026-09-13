#pragma once

#include <QWidget>
#include "models/scoop_models.h"
#include "core/scoop_service.h"

class QLineEdit;
class QPushButton;
class QTabWidget;
class QTableWidget;
class QLabel;
class QComboBox;
class QVBoxLayout;
class ScoopService;
class PackageInfoDialog;

// 搜索页（复刻原版 SearchPage，现代化 UI）
class SearchPage : public QWidget {
    Q_OBJECT
public:
    explicit SearchPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

private slots:
    void doSearch();
    void onResultsReady(QVector<ScoopPackage> packages, bool isCold);
    void onInstallClicked();
    void onUninstallClicked();
    void showPackageInfo();
    void onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error);

private:
    void setupUi();
    void setupTable(QTableWidget* table);
    void populateTable(QTableWidget* table, const QVector<ScoopPackage>& packages);
    void refreshInstalledState();

    ScoopService* m_service;
    QLineEdit* m_searchEdit = nullptr;
    QTabWidget* m_tabs = nullptr;
    QTableWidget* m_packageTable = nullptr;
    QTableWidget* m_binaryTable = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector<ScoopPackage> m_packageResults;
    QVector<ScoopPackage> m_binaryResults;
    QString m_currentQuery;
    bool m_loaded = false;
};
