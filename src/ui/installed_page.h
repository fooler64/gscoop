// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QFrame>
#include <QSet>
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

    // 多选模式（批量操作）
    void setSelectable(bool on);
    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }
    void setSelectionTint(bool on) { m_selectionTint = on; update(); }

signals:
    void holdRequested(const QString& name);
    void uninstallRequested(const QString& name);
    void cardClicked(const QString& name);
    void checkToggled(const QString& name, bool checked);

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
    QRect m_checkRect;        // 多选复选框区域
    bool m_lockHover = false;
    bool m_trashHover = false;
    bool m_selectable = false;   // 是否处于多选模式
    bool m_checked = false;
    bool m_selectionTint = false; // 过滤命中的淡色底
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
    void onToggleSelectMode();
    void onSelectAll();
    void onBatchUpdate();
    void onBatchUninstall();
    void onBatchHold();
    void onCardCheckToggled(const QString& name, bool checked);

private:
    void setupUi();
    void rebuildCards();
    void applyFilter();
    void doHold(const QString& name);
    void doUninstall(const QString& name);
    void updateSelectionUi();
    QStringList checkedNames() const;

    ScoopService* m_service;
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QPushButton* m_updateAllBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QLabel* m_countLabel = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_cardsHost = nullptr;
    QGridLayout* m_cardsLayout = nullptr;

    // 批量操作栏
    QPushButton* m_selectModeBtn = nullptr;
    QWidget* m_batchBar = nullptr;
    QLabel* m_selectedLabel = nullptr;
    QPushButton* m_selectAllBtn = nullptr;
    QPushButton* m_batchUpdateBtn = nullptr;
    QPushButton* m_batchHoldBtn = nullptr;
    QPushButton* m_batchUninstallBtn = nullptr;

    QVector<InstalledPackage> m_packages;
    QSet<QString> m_checked;      // 已勾选包名
    QString m_filter;
    QString m_searchText;
    bool m_selectMode = false;
    bool m_loaded = false;
};
