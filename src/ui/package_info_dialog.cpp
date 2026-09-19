// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/package_info_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QMouseEvent>

#include "core/scoop_service.h"
#include "core/theme_manager.h"
#include "ui/theme.h"
#include "ui/icon_painter.h"

PackageInfoDialog::PackageInfoDialog(ScoopService* service, const QString& packageName,
                                     QWidget* parent)
    : QDialog(parent), m_service(service), m_name(packageName) {
    setWindowTitle(tr("包信息 - %1").arg(packageName));
    resize(540, 580);
    // 去掉原生标题栏（顶部已有自绘标题栏含关闭按钮），避免双 ×
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    const bool dark = ThemeManager::instance().isDark();

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ---- 顶部标题栏（accentDim 底，统一色调）----
    auto* header = new QFrame(this);
    header->setFixedHeight(64);
    header->setAutoFillBackground(true);
    {
        QPalette hp = header->palette();
        hp.setColor(QPalette::Window, Theme::accentDim(dark));
        header->setPalette(hp);
    }
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);
    headerLayout->setSpacing(12);

    // 包图标（箱子/下载图标）
    auto* iconLbl = new QLabel(header);
    iconLbl->setPixmap(IconPainter::download(Theme::text(dark), 24).pixmap(24, 24));
    headerLayout->addWidget(iconLbl);

    m_nameLabel = new QLabel(packageName, header);
    QFont nameFont = m_nameLabel->font();
    nameFont.setPointSize(nameFont.pointSize() + 3);
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    m_nameLabel->setStyleSheet(QString("color:%1;").arg(Theme::text(dark).name()));
    headerLayout->addWidget(m_nameLabel);

    headerLayout->addStretch();

    // 关闭按钮
    auto* closeBtn = new QPushButton(header);
    closeBtn->setIcon(IconPainter::close(Theme::text(dark), 16));
    closeBtn->setFlat(true);
    closeBtn->setToolTip(tr("关闭"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerLayout->addWidget(closeBtn);

    outer->addWidget(header);

    // ---- 中间内容区 ----
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll, 1);

    auto* container = new QWidget(scroll);
    scroll->setWidget(container);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(28, 24, 28, 20);
    layout->setSpacing(12);

    auto* form = new QFormLayout;
    form->setSpacing(10);
    m_versionLabel = new QLabel("-", container);
    m_sourceLabel = new QLabel("-", container);
    m_descLabel = new QLabel("-", container);
    m_descLabel->setWordWrap(true);
    m_descLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_homepageLabel = new QLabel("-", container);
    m_homepageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_homepageLabel->setOpenExternalLinks(true);
    m_licenseLabel = new QLabel("-", container);
    m_dependsLabel = new QLabel("-", container);
    m_dependsLabel->setWordWrap(true);
    m_urlLabel = new QLabel("-", container);
    m_urlLabel->setWordWrap(true);
    m_urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_authorLabel = new QLabel("-", container);
    m_notesLabel = new QLabel("-", container);
    m_notesLabel->setWordWrap(true);
    m_notesLabel->setStyleSheet(QString("color:%1;").arg(Theme::warn(dark).name()));
    // 安装大小 / 可用版本（增强信息）
    m_sizeLabel = new QLabel("-", container);
    m_versionsLabel = new QLabel("-", container);
    m_versionsLabel->setWordWrap(true);
    m_versionsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    form->addRow(tr("版本:"), m_versionLabel);
    form->addRow(tr("Bucket:"), m_sourceLabel);
    form->addRow(tr("描述:"), m_descLabel);
    form->addRow(tr("依赖:"), m_dependsLabel);
    form->addRow(tr("主页:"), m_homepageLabel);
    form->addRow(tr("许可证:"), m_licenseLabel);
    form->addRow(tr("下载:"), m_urlLabel);
    form->addRow(tr("作者:"), m_authorLabel);
    form->addRow(tr("大小:"), m_sizeLabel);
    form->addRow(tr("可用版本:"), m_versionsLabel);
    form->addRow(tr("备注:"), m_notesLabel);
    layout->addLayout(form);

    // ---- 底部操作栏（surface 统一色调 + 顶部分隔线）----
    auto* footer = new QFrame(this);
    footer->setFixedHeight(56);
    footer->setAutoFillBackground(true);
    {
        QPalette fp = footer->palette();
        fp.setColor(QPalette::Window, Theme::surface(dark));
        footer->setPalette(fp);
    }
    auto* btnRow = new QHBoxLayout(footer);
    btnRow->setContentsMargins(20, 8, 20, 8);
    btnRow->setSpacing(10);
    btnRow->addStretch();
    m_vtBtn = new QPushButton(tr("VirusTotal 查毒"), footer);
    m_vtBtn->setToolTip(tr("使用 VirusTotal 扫描此软件（需要 scoop-virustotal 扩展）"));
    m_vtBtn->setEnabled(false);  // 只有已安装的能扫描（需要本地文件 hash）
    m_uninstallBtn = new QPushButton(tr("卸载"), footer);
    m_uninstallBtn->setEnabled(false);
    m_installBtn = new QPushButton(tr("安装"), footer);
    m_installBtn->setProperty("primary", true);
    btnRow->addWidget(m_vtBtn);
    btnRow->addWidget(m_uninstallBtn);
    btnRow->addWidget(m_installBtn);
    outer->addWidget(footer);

    connect(m_installBtn, &QPushButton::clicked, this, &PackageInfoDialog::onInstall);
    connect(m_uninstallBtn, &QPushButton::clicked, this, &PackageInfoDialog::onUninstall);
    connect(m_vtBtn, &QPushButton::clicked, this, &PackageInfoDialog::onScanVirusTotal);

    fetchInfo();
}

// 无边框窗口拖动（仅从标题栏区域拖动）
void PackageInfoDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->pos().y() <= 64) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void PackageInfoDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void PackageInfoDialog::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

QString PackageInfoDialog::findManifest() {
    // 1. 已安装：<apps>/<name>/current/manifest.json
    const QString appsDir = m_service->scoopAppsDir();
    QFileInfo instManifest(appsDir + "/" + m_name + "/current/manifest.json");
    if (instManifest.exists()) {
        m_uninstallBtn->setEnabled(true);
        m_vtBtn->setEnabled(true);
        return instManifest.absoluteFilePath();
    }
    // 2. 搜索 bucket：~/scoop/buckets/<bucket>/bucket/<name>.json
    const QString home = QDir::homePath();
    QDir bucketsRoot(home + "/scoop/buckets");
    const QStringList buckets = bucketsRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& bucket : buckets) {
        QFileInfo bm(home + "/scoop/buckets/" + bucket + "/bucket/" + m_name + ".json");
        if (bm.exists()) {
            m_sourceLabel->setText(bucket);
            return bm.absoluteFilePath();
        }
        // 某些 bucket 结构：<bucket>/<name>.json（旧式）
        QFileInfo bm2(home + "/scoop/buckets/" + bucket + "/" + m_name + ".json");
        if (bm2.exists()) {
            m_sourceLabel->setText(bucket);
            return bm2.absoluteFilePath();
        }
    }
    return QString();
}

void PackageInfoDialog::fetchInfo() {
    const bool dark = ThemeManager::instance().isDark();
    const QString manifestPath = findManifest();
    if (manifestPath.isEmpty()) {
        m_descLabel->setText(tr("（未找到 manifest）"));
        return;
    }

    QFile f(manifestPath);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();

    m_versionLabel->setText(obj.value("version").toString("-"));
    m_descLabel->setText(obj.value("description").toString("-"));
    m_licenseLabel->setText(obj.value("license").toString("-"));
    m_authorLabel->setText(obj.value("author").toString("-"));

    const QString homepage = obj.value("homepage").toString();
    if (!homepage.isEmpty()) {
        m_homepageLabel->setText(QString("<a href=\"%1\">%1</a>").arg(homepage));
    }

    // 依赖：depends 字段（字符串或数组）
    const QJsonValue dependsVal = obj.value("depends");
    if (dependsVal.isArray()) {
        QStringList deps;
        for (const auto& v : dependsVal.toArray()) deps << v.toString();
        m_dependsLabel->setText(deps.isEmpty() ? "-" : deps.join(", "));
    } else if (dependsVal.isString()) {
        m_dependsLabel->setText(dependsVal.toString());
    } else {
        m_dependsLabel->setText("-");
    }

    // 下载 URL：url / url64
    QString url = obj.value("url64").toString();
    if (url.isEmpty()) url = obj.value("url").toString();
    m_urlLabel->setText(url.isEmpty() ? "-" : url);

    // 备注：notes
    const QJsonValue notesVal = obj.value("notes");
    if (notesVal.isString()) {
        m_notesLabel->setText(notesVal.toString());
    } else if (notesVal.isArray()) {
        QStringList notes;
        for (const auto& v : notesVal.toArray()) notes << v.toString();
        m_notesLabel->setText(notes.join("\n"));
    } else {
        m_notesLabel->hide();
    }

    // deprecated
    if (obj.contains("deprecated")) {
        m_descLabel->setText(tr("[已废弃] ") + m_descLabel->text());
        m_descLabel->setStyleSheet(QString("color:%1;").arg(Theme::danger(dark).name()));
    }

    // ---- 安装大小 ----
    const qint64 bytes = m_service->installedSize(m_name);
    if (bytes < 0) {
        m_sizeLabel->setText(tr("未安装"));
    } else if (bytes < 1024 * 1024) {
        m_sizeLabel->setText(tr("%1 KB").arg(bytes / 1024.0, 0, 'f', 1));
    } else if (bytes < qint64(1024) * 1024 * 1024) {
        m_sizeLabel->setText(tr("%1 MB").arg(bytes / 1024.0 / 1024.0, 0, 'f', 1));
    } else {
        m_sizeLabel->setText(tr("%1 GB").arg(bytes / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2));
    }

    // ---- 可用版本 ----
    const QStringList vers = m_service->availableVersions(m_name);
    m_versionsLabel->setText(vers.isEmpty() ? "-" : vers.join("  "));
}

void PackageInfoDialog::onInstall() {
    const bool installed = QFileInfo(m_service->scoopAppsDir() + "/" + m_name).isDir();
    if (installed) {
        QMessageBox::information(this, tr("已安装"), tr("\"%1\" 已安装。").arg(m_name));
        return;
    }
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("安装"), tr("确定要安装 \"%1\" 吗？").arg(m_name));
    if (res == QMessageBox::Yes) {
        m_service->installPackage(m_name);
        accept();
    }
}

void PackageInfoDialog::onUninstall() {
    QMessageBox::StandardButton res = QMessageBox::question(
        this, tr("卸载"), tr("确定要卸载 \"%1\" 吗？").arg(m_name));
    if (res == QMessageBox::Yes) {
        m_service->uninstallPackage(m_name);
        accept();
    }
}

void PackageInfoDialog::onScanVirusTotal() {
    QMessageBox::information(this, tr("VirusTotal 查毒"),
        tr("将调用 scoop virustotal %1 进行扫描。\n"
           "需要已安装 scoop-virustotal 扩展并配置 API key。\n"
           "扫描可能需要较长时间，请在弹出的操作进度中查看结果。")
            .arg(m_name));
    m_service->scanVirusTotal(m_name);
}
