#include "ui/package_info_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

#include "core/scoop_service.h"

PackageInfoDialog::PackageInfoDialog(ScoopService* service, const QString& packageName,
                                     QWidget* parent)
    : QDialog(parent), m_service(service), m_name(packageName) {
    setWindowTitle(tr("包信息 - %1").arg(packageName));
    setMinimumWidth(480);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    m_nameLabel = new QLabel(packageName, this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setPointSize(nameFont.pointSize() + 4);
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    layout->addWidget(m_nameLabel);

    auto* form = new QFormLayout;
    m_versionLabel = new QLabel("-", this);
    m_sourceLabel = new QLabel("-", this);
    m_descLabel = new QLabel("-", this);
    m_descLabel->setWordWrap(true);
    m_homepageLabel = new QLabel("-", this);
    m_homepageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_homepageLabel->setOpenExternalLinks(true);
    m_licenseLabel = new QLabel("-", this);

    form->addRow(tr("版本:"), m_versionLabel);
    form->addRow(tr("Bucket:"), m_sourceLabel);
    form->addRow(tr("描述:"), m_descLabel);
    form->addRow(tr("主页:"), m_homepageLabel);
    form->addRow(tr("许可证:"), m_licenseLabel);
    layout->addLayout(form);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_installBtn = new QPushButton(tr("安装"), this);
    m_uninstallBtn = new QPushButton(tr("卸载"), this);
    m_uninstallBtn->setEnabled(false);
    btnRow->addWidget(m_uninstallBtn);
    btnRow->addWidget(m_installBtn);
    layout->addLayout(btnRow);

    connect(m_installBtn, &QPushButton::clicked, this, &PackageInfoDialog::onInstall);
    connect(m_uninstallBtn, &QPushButton::clicked, this, &PackageInfoDialog::onUninstall);

    fetchInfo();
}

void PackageInfoDialog::fetchInfo() {
    // 从已安装目录或 bucket manifest 读取信息
    const QString appsDir = m_service->scoopAppsDir();
    QString manifestPath;

    // 1. 已安装：<apps>/<name>/current/manifest.json
    QFileInfo instManifest(appsDir + "/" + m_name + "/current/manifest.json");
    if (instManifest.exists()) {
        manifestPath = instManifest.absoluteFilePath();
        m_uninstallBtn->setEnabled(true);
    }

    // 2. 未安装：搜索 bucket 目录
    if (manifestPath.isEmpty()) {
        const QString home = QDir::homePath();
        QDir bucketsDir(home + "/scoop/apps");
        const QStringList entries = bucketsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& bucket : entries) {
            QFileInfo bm(home + "/scoop/apps/" + bucket + "/" + m_name + ".json");
            if (bm.exists()) {
                manifestPath = bm.absoluteFilePath();
                m_sourceLabel->setText(bucket);
                break;
            }
        }
    }

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
    const QString homepage = obj.value("homepage").toString();
    if (!homepage.isEmpty()) {
        m_homepageLabel->setText(QString("<a href=\"%1\">%1</a>").arg(homepage));
    }
    // deprecated
    if (obj.contains("deprecated")) {
        m_descLabel->setText(tr("[已废弃] ") + m_descLabel->text());
    }
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
