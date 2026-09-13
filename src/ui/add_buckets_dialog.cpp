// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/add_buckets_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"

AddBucketsDialog::AddBucketsDialog(ScoopService* service, QWidget* parent)
    : QDialog(parent), m_service(service) {
    setWindowTitle(tr("批量添加 Buckets"));
    setModal(true);
    setMinimumSize(520, 400);

    const bool dark = ThemeManager::instance().isDark();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    auto* title = new QLabel(tr("批量添加 Buckets"), this);
    QFont tf = title->font();
    tf.setPointSize(14);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    m_hintLabel = new QLabel(
        tr("每行一个 bucket，格式：\n"
           "  仓库URL                     （自动推断名称）\n"
           "  名称 仓库URL                 （指定名称）\n\n"
           "示例：\n"
           "https://github.com/ScoopInstaller/Extras\n"
           "games https://github.com/Calinou/scoop-games"),
        this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::textSub(dark).name()));
    layout->addWidget(m_hintLabel);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setPlaceholderText(tr("https://github.com/owner/repo1\nhttps://github.com/owner/repo2"));
    layout->addWidget(m_textEdit, 1);

    // 底部按钮行
    auto* btnRow = new QHBoxLayout;
    auto* exampleBtn = new QPushButton(tr("填入示例"), this);
    btnRow->addWidget(exampleBtn);
    btnRow->addStretch();

    auto* cancelBtn = new QPushButton(tr("取消"), this);
    btnRow->addWidget(cancelBtn);

    auto* addAllBtn = new QPushButton(tr("全部添加"), this);
    addAllBtn->setProperty("primary", true);
    btnRow->addWidget(addAllBtn);

    layout->addLayout(btnRow);

    connect(exampleBtn, &QPushButton::clicked, this, &AddBucketsDialog::onExample);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(addAllBtn, &QPushButton::clicked, this, &AddBucketsDialog::onAddAll);
}

void AddBucketsDialog::onExample() {
    m_textEdit->setPlainText(
        "https://github.com/ScoopInstaller/Versions\n"
        "games https://github.com/Calinou/scoop-games\n"
        "https://github.com/matthewjberger/scoop-nerd-fonts");
}

void AddBucketsDialog::onAddAll() {
    const QString text = m_textEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请输入要添加的 bucket 列表。"));
        return;
    }

    QVector<QPair<QString, QString>> buckets;
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        if (parts.size() >= 2 && !parts[1].contains("://") && !parts[1].startsWith("git@")) {
            // 名称 + URL
            buckets.append({parts[0], parts[1]});
        } else {
            // 只有 URL，推断名称
            QString url = parts[0];
            QString u = url;
            while (u.endsWith('/')) u.chop(1);
            if (u.endsWith(".git")) u.chop(4);
            const int slash = u.lastIndexOf('/');
            QString name = (slash >= 0) ? u.mid(slash + 1) : u;
            if (name.isEmpty()) name = "custom";
            buckets.append({name, url});
        }
    }

    if (buckets.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("没有可添加的 bucket。"));
        return;
    }

    m_service->addBuckets(buckets);
    accept();
}
