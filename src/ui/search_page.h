#pragma once

#include <QWidget>
#include <QFrame>
#include "models/scoop_models.h"
#include "core/scoop_service.h"

class QLineEdit;
class QPushButton;
class QTabWidget;
class QLabel;
class QComboBox;
class QVBoxLayout;
class QScrollArea;
class QGridLayout;
class QMouseEvent;
class QPaintEvent;
class QEnterEvent;
class QContextMenuEvent;
class ScoopService;
class PackageInfoDialog;

// 搜索结果卡片：圆角无边框，显示名称/版本/bucket/描述/状态
// 点击=打开包信息，右键=安装/卸载菜单
class SearchResultCard : public QFrame {
    Q_OBJECT
public:
    SearchResultCard(const ScoopPackage& pkg, QWidget* parent = nullptr);

    QString packageName() const { return m_pkg.name; }
    void setInstalled(bool installed);

signals:
    void clicked(const QString& name);
    void installRequested(const QString& name);
    void uninstallRequested(const QString& name);
    void infoRequested(const QString& name);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    ScoopPackage m_pkg;
    bool m_hover = false;
};

// 搜索页：卡片式搜索结果 + rscoop 风格搜索栏
class SearchPage : public QWidget {
    Q_OBJECT
public:
    explicit SearchPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

private slots:
    void doSearch();
    void onResultsReady(QVector<ScoopPackage> packages, bool isCold);
    void onInstallClicked(const QString& name);
    void onUninstallClicked(const QString& name);
    void showPackageInfo(const QString& name);
    void onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error);

private:
    void setupUi();
    QWidget* makeResultsPage(QVBoxLayout** outLayout);
    void populateCards(QVBoxLayout* layout, const QVector<ScoopPackage>& packages);
    void refreshInstalledState();

    ScoopService* m_service;
    QLineEdit* m_searchEdit = nullptr;
    QTabWidget* m_tabs = nullptr;
    QVBoxLayout* m_packageLayout = nullptr;
    QVBoxLayout* m_binaryLayout = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector<ScoopPackage> m_packageResults;
    QVector<ScoopPackage> m_binaryResults;
    QString m_currentQuery;
    bool m_loaded = false;
};
