// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QDialog>
#include <QVector>

class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QListWidget;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class ScoopService;

// 搜索结果项（对应 rscoop SearchableBucket）
struct SearchableBucket {
    QString name;
    QString full_name;      // owner/repo
    QString description;
    QString url;
    int stars = 0;
    int forks = 0;
    int apps = 0;           // manifest 数量（如果可获取）
    QString last_updated;
    bool is_verified = false;
};

// 探索新仓库弹窗：搜索 GitHub 上的 scoop bucket 仓库
// 独立弹窗，支持排序/过滤/添加
class ExploreBucketDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExploreBucketDialog(ScoopService* service, QWidget* parent = nullptr);

private slots:
    void onSearch();
    void onSortChanged(int idx);
    void onAddClicked();
    void onReplyFinished(QNetworkReply* reply);

private:
    void setupUi();
    void populateResults(const QVector<SearchableBucket>& buckets);
    QString buildQuery() const;

    ScoopService* m_service;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_searchBtn = nullptr;
    QComboBox* m_sortCombo = nullptr;
    QSpinBox* m_minStarsSpin = nullptr;
    QCheckBox* m_hideChineseCheck = nullptr;
    QListWidget* m_resultList = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_addBtn = nullptr;
    QNetworkAccessManager* m_nam = nullptr;

    QVector<SearchableBucket> m_results;
    bool m_searching = false;
};
