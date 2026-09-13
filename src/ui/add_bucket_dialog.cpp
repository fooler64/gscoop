// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/add_bucket_dialog.h"
#include "ui/add_buckets_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

AddBucketDialog::AddBucketDialog(ScoopService* service, QWidget* parent)
    : QDialog(parent), m_service(service) {
    setWindowTitle(tr("添加 Bucket"));
    setModal(true);
    setMinimumWidth(440);

    const bool dark = ThemeManager::instance().isDark();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    // 标题
    auto* title = new QLabel(tr("添加 Bucket"), this);
    QFont tf = title->font();
    tf.setPointSize(14);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    auto* hint = new QLabel(tr("填写要添加的 bucket 仓库信息，或从预置列表选择。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QString("color:%1;").arg(Theme::textSub(dark).name()));
    layout->addWidget(hint);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("bucket 名称（可选，留空自动从 URL 推断）"));
    form->addRow(tr("名称:"), m_nameEdit);

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("https://github.com/owner/repo"));
    form->addRow(tr("URL:"), m_urlEdit);

    layout->addLayout(form);

    // 右下角按钮行
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();

    auto* multiBtn = new QPushButton(tr("添加多个 Buckets..."), this);
    multiBtn->setToolTip(tr("打开批量添加界面，一次添加多个仓库"));
    btnRow->addWidget(multiBtn);

    auto* cancelBtn = new QPushButton(tr("取消"), this);
    btnRow->addWidget(cancelBtn);

    auto* addBtn = new QPushButton(tr("添加"), this);
    addBtn->setProperty("primary", true);
    addBtn->setIcon(IconPainter::plus(Theme::text(dark), 16));
    btnRow->addWidget(addBtn);

    layout->addLayout(btnRow);

    // 信号
    connect(multiBtn, &QPushButton::clicked, this, [this]() {
        AddBucketsDialog dlg(m_service, this);
        dlg.exec();
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(addBtn, &QPushButton::clicked, this, &AddBucketDialog::onAdd);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &AddBucketDialog::onAdd);
}

void AddBucketDialog::onAdd() {
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请输入 bucket 的 URL。"));
        return;
    }
    // 名称：手动填的优先，否则从 URL 推断
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        // 取 URL 最后一段作为名称
        QString u = url;
        while (u.endsWith('/')) u.chop(1);
        if (u.endsWith(".git")) u.chop(4);
        const int slash = u.lastIndexOf('/');
        name = (slash >= 0) ? u.mid(slash + 1) : u;
        if (name.isEmpty()) name = "custom";
    }
    m_service->addBucket(name, url);
    accept();
}
