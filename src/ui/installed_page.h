// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QFrame>
#include "models/scoop_models.h"
#include "core/scoop_service.h"

class QPushButton;
class QLabel;
class QComboBox;
class QLineEdit;
class QGridLayout;
class QVBoxLayout;
class QScrollArea;
class QMouseEvent;
class QPaintEvent;
class QEnterEvent;
class ScoopService;

// 已安装包卡片：自绘圆角卡片
// 布局：左上 [锁][删除] 名称+版本+bucket
//       右上 [可更新徽标]
class InstalledCard : public QFrame {
    Q_OBJECT
public:
    InstalledCard(const InstalledPackage& pkg, QWidget* parent = nullptr);

    void setPackage(const InstalledPackage& pkg);
    const InstalledPackage& package() const { return m_pkg; }

signals:
    void holdRequested(const QString& name);
    void uninstallRequested(const QString& name);
    void cardClicked(const QString& name);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void rebuildText();

    InstalledPackage m_pkg;
    bool m_hover = false;
    bool m_pressed = false;
    // 按钮区域（用于命中测试）
    QRect m_lockRect;
    QRect m_trashRect;
    QRect m_badgeRect;
    bool m_lockHover = false;
    bool m_trashHover = false;
};

// 已安装页（卡片式）
class InstalledPage : public QWidget {
    Q_OBJECT
public:
    explicit InstalledPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

private slots:
    void onPackagesLoaded(QVector<InstalledPackage> packages);
    void onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error);
    void onFilterChanged(int idx);

private:
    void setupUi();
    void rebuildCards();
    void applyFilter();
    void doHold(const QString& name);
    void doUninstall(const QString& name);

    ScoopService* m_service;
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QPushButton* m_updateAllBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QLabel* m_countLabel = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_cardsHost = nullptr;
    QGridLayout* m_cardsLayout = nullptr;

    QVector<InstalledPackage> m_packages;
    QString m_filter;
    QString m_searchText;
    bool m_loaded = false;
};
