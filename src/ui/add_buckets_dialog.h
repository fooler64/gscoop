#pragma once

#include <QDialog>

class QPlainTextEdit;
class QPushButton;
class QLabel;
class QLineEdit;
class ScoopService;

// 批量添加 bucket 弹窗
// 每行一个 bucket：格式 "name url" 或 "url"（自动推断 name）
class AddBucketsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddBucketsDialog(ScoopService* service, QWidget* parent = nullptr);

private slots:
    void onAddAll();
    void onExample();

private:
    ScoopService* m_service;
    QPlainTextEdit* m_textEdit = nullptr;
    QLabel* m_hintLabel = nullptr;
};
