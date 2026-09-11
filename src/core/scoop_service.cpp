#include "core/scoop_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QThread>
#include <QTimer>
#include <QRegularExpression>
#include <cstdio>
#include <algorithm>

// ---------------------------------------------------------------------------
// 构造与探测
// ---------------------------------------------------------------------------
ScoopService::ScoopService(QObject* parent)
    : QObject(parent), m_process(new QProcess(this)) {

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &ScoopService::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &ScoopService::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ScoopService::onProcessFinished);
}

QString ScoopService::findScoopExecutable() {
    // 1. PATH 中的 scoop（shim）
    const QStringList searchPaths = qEnvironmentVariable("PATH").split(';');
    for (const QString& dir : searchPaths) {
        QFileInfo fi(dir + "/scoop.cmd");
        if (fi.exists()) return fi.absoluteFilePath();
        QFileInfo fi2(dir + "/scoop.ps1");
        if (fi2.exists()) return fi2.absoluteFilePath();
        QFileInfo fi3(dir + "/scoop");
        if (fi3.exists() && fi3.isExecutable()) return fi3.absoluteFilePath();
    }
    // 2. 默认安装位置
    const QString home = QDir::homePath();
    QFileInfo defaultShim(home + "/scoop/shims/scoop.cmd");
    if (defaultShim.exists()) return defaultShim.absoluteFilePath();
    QFileInfo defaultShim2(home + "/scoop/shims/scoop");
    if (defaultShim2.exists()) return defaultShim2.absoluteFilePath();
    return QString();
}

QString ScoopService::resolveAppsDir() {
    const QString home = QDir::homePath();
    QFileInfo apps(home + "/scoop/apps");
    if (apps.exists()) return apps.absoluteFilePath();
    return QString();
}

QString ScoopService::bucketsRootDir() const {
    const QString home = QDir::homePath();
    QFileInfo b(home + "/scoop/buckets");
    if (b.exists()) return b.absoluteFilePath();
    return QString();
}

QStringList ScoopService::findBucketDirs(const QString& appsDir) {
    Q_UNUSED(appsDir);
    QStringList result;
    const QString home = QDir::homePath();
    QDir bucketsRoot(home + "/scoop/buckets");
    if (!bucketsRoot.exists()) return result;

    // 每个 bucket 仓库：~/scoop/buckets/<name>/bucket/*.json
    const QStringList entries = bucketsRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QDir bucketManifestDir(home + "/scoop/buckets/" + entry + "/bucket");
        const QStringList jsonFiles = bucketManifestDir.entryList(QStringList() << "*.json",
                                                                  QDir::Files);
        if (!jsonFiles.isEmpty()) {
            result << bucketManifestDir.absolutePath();
        }
    }
    return result;
}

QStringList ScoopService::discoverBucketDirs() {
    return findBucketDirs(m_appsDir);
}

void ScoopService::probeEnvironment() {
    m_scoopPath = findScoopExecutable();
    m_appsDir = resolveAppsDir();
    if (!m_appsDir.isEmpty()) {
        m_bucketDirs = discoverBucketDirs();
    }
    emit scoopDetected(!m_scoopPath.isEmpty());
}

// ---------------------------------------------------------------------------
// 后台扫描实现（静态）
// ---------------------------------------------------------------------------
QVector<InstalledPackage> ScoopService::scanInstalledImpl(
        const QString& appsDir, const QStringList& bucketDirs) {
    QVector<InstalledPackage> result;
    if (appsDir.isEmpty()) return result;

    QDir dir(appsDir);
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    const QStringList knownBuckets = {"main", "extras", "versions", "nirsoft",
                                      "sysinternals", "php", "nerd-fonts", "nonportable",
                                      "java", "games", "extras-cn", "cluttered-bucket"};

    for (const QString& entry : entries) {
        // 跳过 bucket 目录本身
        if (knownBuckets.contains(entry)) continue;
        bool isBucket = false;
        for (const QString& bd : bucketDirs) {
            if (QFileInfo(bd).fileName() == entry) { isBucket = true; break; }
        }
        if (isBucket) continue;

        const QString pkgPath = appsDir + "/" + entry;
        QDir pkgDir(pkgPath);

        // 找版本目录：优先 current 链接指向的目录，否则找最新
        QString versionDir;
        QString version;
        QFileInfo currentLink(pkgPath + "/current");
        if (currentLink.exists()) {
            QString target = currentLink.symLinkTarget();
            if (target.isEmpty()) target = currentLink.canonicalFilePath();
            if (QFileInfo(target).isDir()) {
                versionDir = target;
                version = QFileInfo(target).fileName();
            }
        }
        if (versionDir.isEmpty()) {
            // 找含 install.json/manifest.json 的最新版本目录
            QStringList dirs = pkgDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            qint64 newestMtime = 0;
            for (const QString& d : dirs) {
                if (d.compare("current", Qt::CaseInsensitive) == 0) continue;
                QFileInfo vfi(pkgPath + "/" + d);
                QFileInfo ij(pkgPath + "/" + d + "/install.json");
                QFileInfo mj(pkgPath + "/" + d + "/manifest.json");
                if (!ij.exists() && !mj.exists()) continue;
                qint64 mtime = ij.exists() ? ij.lastModified().toMSecsSinceEpoch()
                                           : mj.lastModified().toMSecsSinceEpoch();
                if (mtime > newestMtime) {
                    newestMtime = mtime;
                    versionDir = vfi.absoluteFilePath();
                    version = d;
                }
            }
        }
        if (versionDir.isEmpty()) continue;

        InstalledPackage ip;
        ip.pkg.name = entry;
        ip.pkg.is_installed = true;
        ip.pkg.version = version;
        ip.install_path = versionDir;

        // install.json
        QFile ifile(versionDir + "/install.json");
        if (ifile.open(QIODevice::ReadOnly)) {
            const QJsonObject obj = QJsonDocument::fromJson(ifile.readAll()).object();
            ip.pkg.source = obj.value("bucket").toString();
            if (ip.pkg.source.isEmpty() && obj.value("version").isString()) {
                ip.pkg.version = obj.value("version").toString();
            }
            ifile.close();
        }
        // manifest.json
        QFile mfile(versionDir + "/manifest.json");
        if (mfile.open(QIODevice::ReadOnly)) {
            const QJsonObject obj = QJsonDocument::fromJson(mfile.readAll()).object();
            ip.description = obj.value("description").toString();
            ip.pkg.info = ip.description;
            ip.pkg.version = obj.value("version").toString(ip.pkg.version);
            ip.is_deprecated = obj.contains("deprecated");
            mfile.close();
        }
        // 修改时间
        QFileInfo dirInfo(versionDir);
        ip.pkg.updated = dirInfo.lastModified().toString("yyyy-MM-dd HH:mm");

        result.append(ip);
    }

    // 按名称排序
    std::sort(result.begin(), result.end(),
              [](const InstalledPackage& a, const InstalledPackage& b) {
                  return a.pkg.name.compare(b.pkg.name, Qt::CaseInsensitive) < 0;
              });
    return result;
}

QVector<ScoopPackage> ScoopService::searchImpl(const QString& query,
                                               const QStringList& bucketDirs,
                                               const QString& appsDir) {
    QVector<ScoopPackage> result;
    const QString q = query.trimmed().toLower();
    if (q.isEmpty()) return result;

    QSet<QString> seen;
    const QString normalizedQuery = q;

    for (const QString& bucketDir : bucketDirs) {
        // bucketDir = ~/scoop/buckets/<name>/bucket → 取上级目录名
        const QString bucketName = QFileInfo(QFileInfo(bucketDir).path()).fileName();
        QDir dir(bucketDir);
        const QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
        for (const QString& file : jsonFiles) {
            if (file.endsWith(".schema.json", Qt::CaseInsensitive)) continue;
            const QString pkgName = file.left(file.length() - 5); // strip .json
            // 名称匹配（子串，不区分大小写）
            if (!pkgName.toLower().contains(normalizedQuery)) {
                // 尝试 bin 匹配（读 manifest）
                continue;
            }
            if (seen.contains(pkgName)) continue;
            seen.insert(pkgName);

            ScoopPackage pkg;
            pkg.name = pkgName;
            pkg.source = bucketName;
            pkg.match_source = MatchSource::Name;

            // 读取版本
            QFile f(bucketDir + "/" + file);
            if (f.open(QIODevice::ReadOnly)) {
                const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
                pkg.version = obj.value("version").toString();
                pkg.info = obj.value("description").toString();
                f.close();
            }
            // 是否已安装
            pkg.is_installed = QFileInfo(appsDir + "/" + pkgName).isDir();
            result.append(pkg);
        }
    }
    return result;
}

QVector<BucketInfo> ScoopService::fetchBucketsImpl(const QString& scoopPath,
                                                   const QString& appsDir) {
    QVector<BucketInfo> result;
    Q_UNUSED(scoopPath);

    // 从 ~/scoop/buckets/<name>/bucket 目录读
    QStringList dirs = findBucketDirs(appsDir);
    for (const QString& d : dirs) {
        BucketInfo b;
        // d = ~/scoop/buckets/<name>/bucket → 上级目录 = bucket 仓库根
        const QString repoRoot = QFileInfo(d).path();
        b.name = QFileInfo(repoRoot).fileName();
        b.path = repoRoot;
        QDir dir(d);
        b.manifest_count = dir.entryList(QStringList() << "*.json", QDir::Files).size();
        b.is_git_repo = QFileInfo(repoRoot + "/.git").isDir();

        // git remote url
        QFile config(repoRoot + "/.git/config");
        if (config.open(QIODevice::ReadOnly)) {
            const QString content = QString::fromUtf8(config.readAll());
            QRegularExpression re("url\\s*=\\s*(.+)$", QRegularExpression::MultilineOption);
            QRegularExpressionMatch m = re.match(content);
            if (m.hasMatch()) b.git_url = m.captured(1).trimmed();
            config.close();
        }
        // 最后更新时间 = .git/FETCH_HEAD 或目录 mtime
        QFileInfo fh(repoRoot + "/.git/FETCH_HEAD");
        if (fh.exists()) {
            b.last_updated = fh.lastModified().toString("yyyy-MM-dd HH:mm");
        } else {
            b.last_updated = QFileInfo(repoRoot).lastModified().toString("yyyy-MM-dd HH:mm");
        }
        result.append(b);
    }
    return result;
}

// ---------------------------------------------------------------------------
// 异步入口
// ---------------------------------------------------------------------------
void ScoopService::scanInstalledPackages() {
    const QString appsDir = m_appsDir;
    const QStringList bucketDirs = m_bucketDirs;
    if (m_scanWatcher) { delete m_scanWatcher; }
    m_scanWatcher = new QFutureWatcher<QVector<InstalledPackage>>(this);
    connect(m_scanWatcher, &QFutureWatcher<QVector<InstalledPackage>>::finished,
            this, &ScoopService::onScanFinished);
    m_scanWatcher->setFuture(QtConcurrent::run(scanInstalledImpl, appsDir, bucketDirs));
}

void ScoopService::searchPackages(const QString& query) {
    const QStringList bucketDirs = m_bucketDirs;
    const QString appsDir = m_appsDir;
    if (m_searchWatcher) { delete m_searchWatcher; }
    m_searchWatcher = new QFutureWatcher<QVector<ScoopPackage>>(this);
    connect(m_searchWatcher, &QFutureWatcher<QVector<ScoopPackage>>::finished,
            this, [this]() {
        if (!m_searchWatcher) return;
        emit searchResultsReady(m_searchWatcher->result(), true);
    });
    m_searchWatcher->setFuture(QtConcurrent::run(searchImpl, query, bucketDirs, appsDir));
}

void ScoopService::fetchBuckets() {
    const QString scoopPath = m_scoopPath;
    const QString appsDir = m_appsDir;
    if (m_bucketWatcher) { delete m_bucketWatcher; }
    m_bucketWatcher = new QFutureWatcher<QVector<BucketInfo>>(this);
    connect(m_bucketWatcher, &QFutureWatcher<QVector<BucketInfo>>::finished,
            this, [this]() {
        if (!m_bucketWatcher) return;
        emit bucketsLoaded(m_bucketWatcher->result());
    });
    m_bucketWatcher->setFuture(QtConcurrent::run(fetchBucketsImpl, scoopPath, appsDir));
}

void ScoopService::onScanFinished() {
    if (!m_scanWatcher) return;
    emit installedPackagesLoaded(m_scanWatcher->result());
}

// ---------------------------------------------------------------------------
// scoop CLI 操作
// ---------------------------------------------------------------------------
void ScoopService::runScoop(const QStringList& args, ScoopOpType op, const QString& package) {
    if (m_process->state() != QProcess::NotRunning) {
        emit errorOccurred(tr("已有操作在进行中，请等待完成。"));
        return;
    }
    if (m_scoopPath.isEmpty()) {
        emit errorOccurred(tr("未找到 scoop，请先安装 Scoop。"));
        return;
    }
    m_currentOp = op;
    m_currentPackage = package;
    emit opStarted(op, package);

    // scoop.cmd 需要通过 cmd 执行；直接用 QProcess 启动 .cmd
    QStringList cmdArgs;
    if (m_scoopPath.endsWith(".cmd") || m_scoopPath.endsWith(".bat")) {
        cmdArgs << "/d" << "/c" << m_scoopPath << args;
        m_process->setProgram("cmd.exe");
    } else {
        m_process->setProgram(m_scoopPath);
        cmdArgs = args;
    }
    m_process->setArguments(cmdArgs);
    m_process->setWorkingDirectory(QDir::homePath());
    m_process->start();
}

void ScoopService::installPackage(const QString& name) {
    runScoop({"install", name}, ScoopOpType::Install, name);
}

void ScoopService::uninstallPackage(const QString& name) {
    runScoop({"uninstall", name}, ScoopOpType::Uninstall, name);
}

void ScoopService::updatePackage(const QString& name) {
    runScoop({"update", name}, ScoopOpType::Update, name);
}

void ScoopService::updateAllPackages() {
    runScoop({"update", "*"}, ScoopOpType::UpdateAll, "*");
}

void ScoopService::holdPackage(const QString& name, bool hold) {
    runScoop({hold ? "hold" : "unhold", name}, hold ? ScoopOpType::Hold : ScoopOpType::Unhold, name);
}

void ScoopService::addBucket(const QString& name, const QString& url) {
    QStringList args{"bucket", "add", name};
    if (!url.isEmpty()) args << url;
    runScoop(args, ScoopOpType::BucketAdd, name);
}

void ScoopService::removeBucket(const QString& name) {
    runScoop({"bucket", "rm", name}, ScoopOpType::BucketRemove, name);
}

void ScoopService::cleanupApps() {
    runScoop({"cleanup"}, ScoopOpType::Cleanup, QString());
}

void ScoopService::cleanupCache() {
    runScoop({"cache", "rm", "*"}, ScoopOpType::CacheRm, QString());
}

void ScoopService::onProcessOutput() {
    const QByteArray data = m_process->readAllStandardOutput();
    const QByteArray errData = m_process->readAllStandardError();
    QString output = QString::fromLocal8Bit(data);
    if (!errData.isEmpty()) output += QString::fromLocal8Bit(errData);

    ScoopOpProgress p;
    p.type = m_currentOp;
    p.package = m_currentPackage;
    p.output = output;
    p.stage = output.trimmed();
    emit opProgress(p);
}

void ScoopService::onProcessFinished(int exitCode) {
    ScoopOpType op = m_currentOp;
    QString pkg = m_currentPackage;
    m_currentOp = ScoopOpType::None;
    m_currentPackage.clear();

    const bool success = (exitCode == 0);
    QString err;
    if (!success) {
        err = m_process->readAllStandardError();
        if (err.isEmpty()) err = tr("scoop 退出码 %1").arg(exitCode);
    }
    emit opFinished(op, pkg, success, err);

    // 操作完成后刷新数据
    scanInstalledPackages();
    fetchBuckets();
}
