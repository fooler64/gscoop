// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QDialog>
#include "models/scoop_models.h"

class QLabel;
class QVBoxLayout;
class QPushButton;
class QScrollArea;
class QMouseEvent;
class ScoopService;

// Bucket 信息弹窗（仿 rscoop BucketInfoModal）
// 显示名称、类型、URL、manifests 数、分支、更新时间、路径 + 删除按钮
class BucketInfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit BucketInfoDialog(ScoopService* service, const BucketInfo& bucket,
                              QWidget* parent = nullptr);

signals:
    void removeRequested(const QString& name);

protected:
    // 无边框窗口拖动
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    ScoopService* m_service;
    BucketInfo m_bucket;
    bool m_dragging = false;
    QPoint m_dragOffset;
};
