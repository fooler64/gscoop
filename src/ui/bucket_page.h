// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QFrame>
#include <QSet>
#include "models/scoop_models.h"
#include "core/scoop_service.h"

class QListWidget;
class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QGridLayout;
class QVBoxLayout;
class QScrollArea;
class QMouseEvent;
class QPaintEvent;
class QEnterEvent;
class ScoopService;

// 预置常用 bucket
struct PresetBucket {
    QString name;
    QString url;       // GitHub 原始地址
    QString desc;
};

// 常用 bucket 卡片：自绘背景/边框/hover，内部用 QLabel 渲染富文本
// （QPushButton 的 rich text 渲染不可靠，会显示字面 HTML 标签）
class BucketCard : public QFrame {
    Q_OBJECT
public:
    BucketCard(const QString& name, const QString& url, const QString& desc,
               bool installed, QWidget* parent = nullptr);

signals:
    void clicked(const QString& name, const QString& url);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_name;
    QString m_url;
    bool m_installed = false;
    bool m_hover = false;
};

// 已安装 bucket 卡片：圆角无边框，显示名称 + manifest 数，hover 高亮
// 点击主体 → 查看介绍；点右上角删除图标 → 删除
class InstalledBucketCard : public QFrame {
    Q_OBJECT
public:
    InstalledBucketCard(const BucketInfo& bucket, QWidget* parent = nullptr);

signals:
    void infoRequested(const QString& name);     // 点击卡片主体 → 查看介绍
    void removeRequested(const QString& name);   // 点击删除图标 → 删除

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    BucketInfo m_bucket;
    bool m_hover = false;
    QRect m_trashRect;   // 删除图标区域（点击检测）
};

// Bucket 页：预置常用 buckets 网格 + 一键添加 + 国内镜像切换
class BucketPage : public QWidget {
    Q_OBJECT
public:
    explicit BucketPage(ScoopService* service, QWidget* parent = nullptr);
    void onPageShown();

    // 常用 bucket 预设
    static const QVector<PresetBucket>& presets();

private slots:
    void onBucketsLoaded(QVector<BucketInfo> buckets);
    void onAddPreset(const QString& name, const QString& url);
    void onRemoveBucket();
    void onRefreshClicked();
    void onOpFinished(ScoopOpType type, const QString& package, bool success, const QString& error);
    void onMirrorChanged(int idx);
    void onAddBucketClicked();
    void onExploreClicked();

private:
    void setupUi();
    void populateInstalledList(const QVector<BucketInfo>& buckets);
    void rebuildPresetGrid();
    QString mirrorUrl(const QString& githubUrl) const;

    ScoopService* m_service;
    // 已安装 buckets（卡片网格）
    QScrollArea* m_installedScroll = nullptr;
    QWidget* m_installedHost = nullptr;
    QGridLayout* m_installedLayout = nullptr;
    QLabel* m_countLabel = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_exploreBtn = nullptr;
    // 预置网格
    QWidget* m_presetGrid = nullptr;
    QGridLayout* m_presetLayout = nullptr;
    // 镜像选择
    QComboBox* m_mirrorCombo = nullptr;

    QVector<BucketInfo> m_buckets;
    QSet<QString> m_installedNames;
    QString m_mirror;   // 当前镜像前缀
    QString m_pendingRemove;  // 待删除 bucket 名（点击卡片）
    bool m_loaded = false;
};
