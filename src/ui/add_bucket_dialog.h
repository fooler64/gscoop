#pragma once

#include <QDialog>

class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QLabel;
class ScoopService;

// 添加 bucket 弹窗（单个）
// 右上角 X 关闭，底部"添加"按钮；右下角"添加多个"按钮 → 打开批量添加
class AddBucketDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddBucketDialog(ScoopService* service, QWidget* parent = nullptr);

signals:
    void addMultipleRequested();   // 用户点了"添加多个 buckets"

private slots:
    void onAdd();

private:
    ScoopService* m_service;
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_urlEdit = nullptr;
    QLabel* m_hintLabel = nullptr;
};
