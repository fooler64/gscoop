// SPDX-License-Identifier: LGPL-3.0-or-later
#include "core/scoop_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QDirIterator>
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

        // install.json（含 bucket 与 hold 状态）
        QFile ifile(versionDir + "/install.json");
        if (ifile.open(QIODevice::ReadOnly)) {
            const QJsonObject obj = QJsonDocument::fromJson(ifile.readAll()).object();
            ip.pkg.source = obj.value("bucket").toString();
            if (ip.pkg.source.isEmpty() && obj.value("version").isString()) {
                ip.pkg.version = obj.value("version").toString();
            }
            // hold 状态（scoop core.ps1: $status.hold = ($install_info.hold -eq $true)）
            ip.is_held = obj.value("hold").toBool(false);
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

    // 相关性排序：完全匹配 > 前缀匹配 > 子串匹配；同级内名称短者优先，再按字母序
    std::sort(result.begin(), result.end(),
              [&q](const ScoopPackage& a, const ScoopPackage& b) {
        const QString na = a.name.toLower();
        const QString nb = b.name.toLower();
        auto rank = [&q](const QString& n) {
            if (n == q) return 0;          // 完全匹配
            if (n.startsWith(q)) return 1; // 前缀匹配
            return 2;                      // 子串匹配
        };
        const int ra = rank(na);
        const int rb = rank(nb);
        if (ra != rb) return ra < rb;
        if (na.length() != nb.length()) return na.length() < nb.length();
        return na.compare(nb) < 0;
    });
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

void ScoopService::addBuckets(const QVector<QPair<QString, QString>>& buckets) {
    if (buckets.isEmpty()) return;
    QStringList args{"bucket", "add"};
    for (const auto& b : buckets) {
        args << b.first;
        if (!b.second.isEmpty()) args << b.second;
    }
    runScoop(args, ScoopOpType::BucketAdd, tr("批量添加 %1 个 bucket").arg(buckets.size()));
}

void ScoopService::removeBucket(const QString& name) {
    runScoop({"bucket", "rm", name}, ScoopOpType::BucketRemove, name);
}

void ScoopService::cleanupApps() {
    // cleanup 也走队列：避免与批量操作冲突
    enqueue({ScoopOpType::Cleanup, {"cleanup"}, QString()});
}

// ---------------------------------------------------------------------------
// 批量操作队列
// ---------------------------------------------------------------------------
void ScoopService::enqueue(const QueueItem& item) {
    const bool wasEmpty = m_queue.isEmpty();
    m_queue.append(item);
    if (wasEmpty) {
        m_queueTotal = m_queue.size();
        m_queueIndex = 0;
        m_queueFailed = 0;
    } else {
        m_queueTotal = m_queueIndex + m_queue.size();
    }
    if (wasEmpty) runNextQueued();
}

void ScoopService::runNextQueued() {
    if (m_queue.isEmpty()) {
        const int total = m_queueTotal;
        const int failed = m_queueFailed;
        m_queueTotal = 0;
        m_queueIndex = 0;
        m_queueFailed = 0;
        emit queueFinished(total - failed, failed);
        return;
    }
    const QueueItem item = m_queue.takeFirst();
    emit queueProgress(m_queueIndex, m_queueTotal, item.package);
    runScoop(item.args, item.op, item.package);
}

void ScoopService::updatePackages(const QStringList& names) {
    if (names.isEmpty()) return;
    for (const QString& n : names) {
        enqueue({ScoopOpType::Update, {"update", n}, n});
    }
}

void ScoopService::installPackages(const QStringList& names) {
    if (names.isEmpty()) return;
    for (const QString& n : names) {
        enqueue({ScoopOpType::Install, {"install", n}, n});
    }
}

void ScoopService::uninstallPackages(const QStringList& names) {
    if (names.isEmpty()) return;
    for (const QString& n : names) {
        enqueue({ScoopOpType::Uninstall, {"uninstall", n}, n});
    }
}

void ScoopService::holdPackages(const QStringList& names, bool hold) {
    if (names.isEmpty()) return;
    for (const QString& n : names) {
        enqueue({hold ? ScoopOpType::Hold : ScoopOpType::Unhold,
                 {hold ? "hold" : "unhold", n}, n});
    }
}

// ---------------------------------------------------------------------------
// 导出 / 导入配置
// ---------------------------------------------------------------------------
void ScoopService::exportConfig(const QString& filePath) {
    // 直接写文件：scoop export 输出 JSON 到 stdout
    if (m_scoopPath.isEmpty()) {
        emit errorOccurred(tr("未找到 scoop，请先安装 Scoop。"));
        return;
    }
    QProcess proc;
    QStringList args{"export"};
    if (m_scoopPath.endsWith(".cmd") || m_scoopPath.endsWith(".bat")) {
        proc.setProgram("cmd.exe");
        proc.setArguments({"/d", "/c", m_scoopPath, "export"});
    } else {
        proc.setProgram(m_scoopPath);
        proc.setArguments(args);
    }
    proc.setWorkingDirectory(QDir::homePath());
    proc.start();
    if (!proc.waitForFinished(30000)) {
        emit errorOccurred(tr("导出超时"));
        return;
    }
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) {
        emit errorOccurred(tr("无法写入文件：%1").arg(filePath));
        return;
    }
    f.write(proc.readAllStandardOutput());
    f.close();
    emit opFinished(ScoopOpType::ExportConfig, filePath, true, QString());
}

void ScoopService::importConfig(const QString& filePath) {
    enqueue({ScoopOpType::ImportConfig, {"import", filePath}, filePath});
}

void ScoopService::updateAllBuckets() {
    enqueue({ScoopOpType::BucketUpdateAll, {"update"}, tr("全部 bucket")});
}

void ScoopService::installPackageVersion(const QString& name, const QString& version) {
    enqueue({ScoopOpType::Install, {"install", name + "@" + version},
             name + "@" + version});
}

// ---------------------------------------------------------------------------
// 包信息增强
// ---------------------------------------------------------------------------
// 递归计算目录大小
static qint64 dirSize(const QString& path) {
    qint64 total = 0;
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

qint64 ScoopService::installedSize(const QString& name) const {
    const QString pkgDir = m_appsDir + "/" + name;
    if (!QDir(pkgDir).exists()) return -1;
    return dirSize(pkgDir);
}

QStringList ScoopService::availableVersions(const QString& name) const {
    QStringList versions;
    // 1. 从本地 bucket manifest 读当前版本
    const QString home = QDir::homePath();
    QDir bucketsRoot(home + "/scoop/buckets");
    const QStringList buckets = bucketsRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& b : buckets) {
        QFile mf(home + "/scoop/buckets/" + b + "/bucket/" + name + ".json");
        if (mf.exists() && mf.open(QIODevice::ReadOnly)) {
            const QJsonObject o = QJsonDocument::fromJson(mf.readAll()).object();
            const QString v = o.value("version").toString();
            if (!v.isEmpty() && !versions.contains(v)) versions << v;
            mf.close();
        }
    }
    // 2. 已安装的本地版本目录
    QDir pkgDir(m_appsDir + "/" + name);
    const QStringList localDirs = pkgDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& d : localDirs) {
        if (d.compare("current", Qt::CaseInsensitive) == 0) continue;
        if (!versions.contains(d)) versions << d;
    }
    return versions;
}

// Doctor 环境自检：检查 git/7zip/main bucket/开发者模式/长路径/NTFS 等
QVector<DoctorCheckItem> ScoopService::runDoctor() {
    QVector<DoctorCheckItem> items;

    auto addItem = [&items](const QString& title, bool passed, const QString& detail = QString(),
                            const QString& desc = QString(), bool warning = false) {
        DoctorCheckItem item;
        item.title = title;
        item.passed = passed;
        item.warning = warning;
        item.detail = detail;
        item.description = desc;
        items.append(item);
    };

    // 1. Git
    QProcess gitProc;
    gitProc.start("git", {"--version"});
    gitProc.waitForFinished(8000);
    const QString gitOut = QString::fromLocal8Bit(gitProc.readAllStandardOutput()).trimmed();
    addItem(tr("Git"), !gitOut.isEmpty(), gitOut,
            gitOut.isEmpty() ? tr("请安装 git：scoop install git") : QString());

    // 2. Scoop 本体
    addItem(tr("Scoop"), isScoopInstalled(), m_scoopPath,
            isScoopInstalled() ? QString() : tr("未检测到 Scoop，请先安装"));

    // 3. 7zip（解压依赖）
    bool has7zip = false;
    QString sevenZipPath;
    const QStringList candidates = {
        m_appsDir + "/7zip/current/7z.exe",
        m_appsDir + "/7zip/7z.exe",
    };
    for (const QString& c : candidates) {
        if (QFile::exists(c)) { has7zip = true; sevenZipPath = c; break; }
    }
    addItem(tr("7zip"), has7zip,
            has7zip ? sevenZipPath : QString(),
            has7zip ? QString() : tr("缺少 7zip，可能影响解压：scoop install 7zip"));

    // 4. main bucket
    const QString mainBucket = bucketsRootDir() + "/main";
    const bool hasMain = QDir(mainBucket).exists();
    addItem(tr("Main bucket"), hasMain,
            hasMain ? mainBucket : QString(),
            hasMain ? QString() : tr("缺少 main bucket：scoop bucket add main"));

    // 5. Windows 长路径支持（注册表 LongPathsEnabled）
    {
        QProcess reg;
        reg.start("reg", {"query", "HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem",
                          "/v", "LongPathsEnabled"});
        reg.waitForFinished(6000);
        const QString out = QString::fromLocal8Bit(reg.readAllStandardOutput());
        const bool enabled = out.contains("0x1");
        addItem(tr("Windows 长路径支持"), enabled,
                enabled ? tr("已启用") : tr("未启用"),
                enabled ? QString() : tr("建议启用长路径支持以处理深层路径（regedit → LongPathsEnabled=1）"),
                true);  // 警告级别
    }

    // 6. Scoop 在 NTFS 上（非 FAT）
    {
        // 检查 scoop 目录所在盘的文件系统
        QProcess fs;
        fs.start("wmic", {"logicaldisk", "where", "name='" + m_scoopPath.left(2) + "'",
                          "get", "filesystem"});
        fs.waitForFinished(6000);
        const QString out = QString::fromLocal8Bit(fs.readAllStandardOutput()).toUpper();
        const bool ntfs = out.contains("NTFS");
        addItem(tr("Scoop 磁盘格式"), ntfs,
                ntfs ? tr("NTFS") : out.simplified(),
                ntfs ? QString() : tr("Scoop 应安装在 NTFS 磁盘上（FAT32 不支持符号链接）"),
                true);
    }

    // 7. 开发者模式（用于符号链接）
    {
        QProcess reg;
        reg.start("reg", {"query", "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
                          "/v", "AllowDevelopmentWithoutDevLicense"});
        reg.waitForFinished(6000);
        const QString out = QString::fromLocal8Bit(reg.readAllStandardOutput());
        const bool devMode = out.contains("0x1");
        addItem(tr("开发者模式"), devMode,
                devMode ? tr("已启用") : tr("未启用"),
                devMode ? QString() : tr("开发者模式有助于 Scoop 使用符号链接（可选）"),
                true);
    }

    return items;
}

void ScoopService::cleanupCache() {
    enqueue({ScoopOpType::CacheRm, {"cache", "rm", "*"}, QString()});
}

void ScoopService::scanVirusTotal(const QString& package) {
    runScoop({"virustotal", package}, ScoopOpType::VirusTotal, package);
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

    // 批量队列：失败计数 + 继续下一个
    if (!m_queue.isEmpty() || m_queueTotal > 0) {
        if (!success) ++m_queueFailed;
        ++m_queueIndex;
        if (!m_queue.isEmpty()) {
            runNextQueued();
            return;   // 队列未完，延迟到最后统一刷新
        }
        // 队列跑完：汇总
        scanInstalledPackages();
        fetchBuckets();
        return;
    }

    // 单次操作：刷新数据
    scanInstalledPackages();
    fetchBuckets();
}
