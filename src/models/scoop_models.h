#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>
#include <QVector>

// 匹配来源（原版 MatchSource）
enum class MatchSource {
    Name,
    Binary,
    None
};

// 已安装包的状态（原版 AppStatusInfo 相关）
struct InstalledStatus {
    bool is_outdated = false;
    bool is_failed = false;
    bool is_held = false;
    bool is_deprecated = false;
    bool is_removed = false;
};

// Scoop 包（原版 ScoopPackage）
struct ScoopPackage {
    QString name;
    QString version;
    QString source;          // bucket 名
    QString updated;         // 安装/更新时间
    bool is_installed = false;
    QString info;            // 包描述
    MatchSource match_source = MatchSource::None;
    bool is_versioned_install = false;

    bool operator==(const ScoopPackage& other) const {
        return name == other.name;
    }
};

// Bucket 信息（原版 BucketInfo）
struct BucketInfo {
    QString name;
    QString path;
    int manifest_count = 0;
    bool is_git_repo = false;
    QString git_url;
    QString git_branch;
    QString last_updated;
};

// 包 manifest（原版 PackageManifest）
struct PackageManifest {
    QString description;
    QString version;
    QString deprecated;      // deprecated 提示，空 = 未废弃
    QString homepage;
    QString license;
    QStringList bin;         // bin 字段（用于搜索二进制匹配）
};

// 安装 manifest（原版 InstallManifest）
struct InstallManifest {
    QString bucket;
    QString version;
    QStringList linkedApps;
};

// 搜索结果（原版 SearchResult）
struct SearchResult {
    QVector<ScoopPackage> packages;
    bool is_cold = false;
};

// 完整已安装包信息（installed 页用）
struct InstalledPackage {
    ScoopPackage pkg;
    bool is_outdated = false;
    bool is_failed = false;
    bool is_held = false;
    bool is_deprecated = false;
    bool is_removed = false;
    QString install_path;
    QString update_version;   // 最新版本（若可获取）
    QString description;
};

Q_DECLARE_METATYPE(ScoopPackage)
Q_DECLARE_METATYPE(InstalledPackage)
Q_DECLARE_METATYPE(BucketInfo)
Q_DECLARE_METATYPE(SearchResult)

// JSON 解析辅助
namespace ScoopJson {
    // 从 manifest JSON 解析包描述/bin
    PackageManifest parseManifest(const QJsonObject& obj);

    // 从 install.json 解析安装信息
    InstallManifest parseInstallManifest(const QJsonObject& obj);

    // 从 bucket JSON（scoop bucket list 输出）解析
    BucketInfo parseBucket(const QString& name, const QJsonObject& obj);
}
