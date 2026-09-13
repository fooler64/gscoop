// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QFrame>
#include "models/scoop_models.h"
#include "core/scoop_service.h"

class QPushButton;
class QLabel;
class QVBoxLayout;
class QScrollArea;
class QPaintEvent;
class ScoopService;

// Doctor 检查项卡片
class DoctorItemCard : public QFrame {
    Q_OBJECT
public:
    DoctorItemCard(const DoctorCheckItem& item, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    DoctorCheckItem m_item;
};

// Doctor 页：环境自检 + 自动清理
class DoctorPage : public QWidget {
    Q_OBJECT
public:
    explicit DoctorPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

private slots:
    void onRunCheck();
    void onCleanupApps();
    void onCleanupCache();

private:
    void setupUi();
    void populate(const QVector<DoctorCheckItem>& items);

    ScoopService* m_service;
    QPushButton* m_runBtn = nullptr;
    QPushButton* m_cleanupAppsBtn = nullptr;
    QPushButton* m_cleanupCacheBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_host = nullptr;
    QVBoxLayout* m_hostLayout = nullptr;

    bool m_loaded = false;
};
