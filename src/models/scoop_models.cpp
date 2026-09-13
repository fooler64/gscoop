// SPDX-License-Identifier: LGPL-3.0-or-later
#include "models/scoop_models.h"
#include <QJsonValue>

namespace ScoopJson {

PackageManifest parseManifest(const QJsonObject& obj) {
    PackageManifest m;
    m.version = obj.value("version").toString();
    m.description = obj.value("description").toString();
    m.deprecated = obj.value("deprecated").toString();
    m.homepage = obj.value("homepage").toString();
    m.license = obj.value("license").toString();

    // bin 字段：可以是字符串、数组、对象
    const QJsonValue binVal = obj.value("bin");
    if (binVal.isString()) {
        m.bin << binVal.toString();
    } else if (binVal.isArray()) {
        const QJsonArray arr = binVal.toArray();
        for (const QJsonValue& entry : arr) {
            if (entry.isString()) {
                m.bin << entry.toString();
            } else if (entry.isObject()) {
                // { "name": "target" } 形式的映射
                const QJsonObject o = entry.toObject();
                for (auto it = o.begin(); it != o.end(); ++it) {
                    m.bin << it.key();
                    if (it.value().isString()) {
                        m.bin << it.value().toString();
                    }
                }
            }
        }
    } else if (binVal.isObject()) {
        const QJsonObject o = binVal.toObject();
        for (auto it = o.begin(); it != o.end(); ++it) {
            m.bin << it.key();
            if (it.value().isString()) {
                m.bin << it.value().toString();
            }
        }
    }
    return m;
}

InstallManifest parseInstallManifest(const QJsonObject& obj) {
    InstallManifest m;
    m.bucket = obj.value("bucket").toString();
    m.version = obj.value("version").toString();
    const QJsonValue linked = obj.value("linkedApps");
    if (linked.isArray()) {
        for (const QJsonValue& v : linked.toArray()) {
            if (v.isString()) m.linkedApps << v.toString();
        }
    }
    return m;
}

BucketInfo parseBucket(const QString& name, const QJsonObject& obj) {
    BucketInfo b;
    b.name = name;
    b.path = obj.value("Path").toString();
    b.is_git_repo = obj.value("IsGitRepo").toBool();
    b.git_url = obj.value("RemoteURL").toString();
    b.git_branch = obj.value("Branch").toString();
    b.last_updated = obj.value("LastUpdated").toString();
    return b;
}

} // namespace ScoopJson
