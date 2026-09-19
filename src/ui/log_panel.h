// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QFrame>

class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QVBoxLayout;
class QMouseEvent;

// 底部操作日志面板（参考 rscoop 的 OperationModal / 底部日志）
// - 顶部一行：状态图标 + 标题 + 进度条 + 折叠按钮 + 清空按钮
// - 展开后显示实时 scoop 输出（可复制、自动滚动）
class LogPanel : public QFrame {
    Q_OBJECT
public:
    explicit LogPanel(QWidget* parent = nullptr);

    void beginOperation(const QString& title, const QString& package);
    void appendRaw(const QString& text);            // 原样追加输出
    void setProgress(int percent);                  // -1 = 不确定
    void setStage(const QString& stage);            // 当前阶段文字
    void finishOperation(bool success, const QString& error);

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);
    void clearLog();
    void refreshTheme();   // 主题切换后刷新配色

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

    QFrame* m_header = nullptr;
    QLabel* m_statusIcon = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_stageLabel = nullptr;
    QProgressBar* m_progress = nullptr;
    QPushButton* m_toggleBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QPlainTextEdit* m_logView = nullptr;

    bool m_expanded = false;
    bool m_running = false;
    bool m_failed = false;
    QString m_pendingOutput;
};
