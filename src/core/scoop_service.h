// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QVector>
#include <QFutureWatcher>
#include "models/scoop_models.h"

// 运行中的 scoop 操作类型
enum class ScoopOpType {
    None,
    Install,
    Uninstall,
    Update,
    Hold,
    Unhold,
    Status,
    BucketAdd,
    BucketRemove,
    Cleanup,
    CacheRm,
    UpdateAll,
    VirusTotal,
    ExportConfig,
    ImportConfig,
    BucketUpdateAll
};

// 操作进度信息（对应原版 OperationModal）
struct ScoopOpProgress {
    ScoopOpType type = ScoopOpType::None;
    QString package;
    QString stage;       // 当前阶段描述
    int percent = -1;    // -1 = 不确定
    QString output;      // 实时输出
    bool finished = false;
    bool success = false;
    QString error;
};

// Scoop 服务：封装与 scoop CLI 的交互
class ScoopService : public QObject {
    Q_OBJECT
public:
    // 批量队列项：{操作类型, 命令行参数, 包名}
    struct QueueItem {
        ScoopOpType op;
        QStringList args;
        QString package;
    };

    explicit ScoopService(QObject* parent = nullptr);

    // 环境探测
    QString scoopPath() const { return m_scoopPath; }
    QString scoopAppsDir() const { return m_appsDir; }
    bool isScoopInstalled() const { return !m_scoopPath.isEmpty(); }

    // 启动时探测 scoop 环境（异步）
    void probeEnvironment();

    // 已安装包扫描（异步，后台线程）
    void scanInstalledPackages();
    // 搜索（异步）
    void searchPackages(const QString& query);
    // Bucket 列表
    void fetchBuckets();

    // 操作（调用 scoop CLI）
    void installPackage(const QString& name);
    void uninstallPackage(const QString& name);
    void updatePackage(const QString& name);
    void updateAllPackages();
    void holdPackage(const QString& name, bool hold);
    void addBucket(const QString& name, const QString& url);
    // 批量添加多个 bucket（name, url 对），scoop bucket add name1 url1 name2 url2 ...
    void addBuckets(const QVector<QPair<QString, QString>>& buckets);
    void removeBucket(const QString& name);
    void cleanupApps();
    void cleanupCache();
    // Doctor 环境自检（同步执行系统命令，返回检查项）
    QVector<DoctorCheckItem> runDoctor();
    // VirusTotal 查毒（调用 scoop virustotal <package>）
    void scanVirusTotal(const QString& package);

    // ---- 批量操作 ----
    void updatePackages(const QStringList& names);      // 逐个更新（自动排队）
    void uninstallPackages(const QStringList& names);
    void holdPackages(const QStringList& names, bool hold);

    // ---- 导出 / 导入配置 ----
    void exportConfig(const QString& filePath);         // scoop export > file
    void importConfig(const QString& filePath);         // scoop import file

    // ---- bucket 批量刷新（scoop update） ----
    void updateAllBuckets();

    // ---- 安装指定版本 ----
    void installPackageVersion(const QString& name, const QString& version);

    // ---- 包信息增强 ----
    // 已安装包占用空间（字节；未安装返回 -1）
    qint64 installedSize(const QString& name) const;
    // 可用版本列表（读 bucket manifest 的 version + 历史版本目录）
    QStringList availableVersions(const QString& name) const;

    // 队列状态（批量操作进度）
    int queueTotal() const { return m_queueTotal; }
    int queueIndex() const { return m_queueIndex; }
    bool isQueueRunning() const { return !m_queue.isEmpty(); }

    // 当前操作状态
    bool isBusy() const { return m_currentOp != ScoopOpType::None; }
    ScoopOpType currentOp() const { return m_currentOp; }

signals:
    void scoopDetected(bool installed);
    void installedPackagesLoaded(QVector<InstalledPackage> packages);
    void searchResultsReady(QVector<ScoopPackage> packages, bool isCold);
    void bucketsLoaded(QVector<BucketInfo> buckets);
    void opStarted(ScoopOpType type, const QString& package);
    void opProgress(const ScoopOpProgress& progress);
    void opFinished(ScoopOpType type, const QString& package, bool success, const QString& error);
    void errorOccurred(const QString& message);
    void queueProgress(int index, int total, const QString& currentPackage);
    void queueFinished(int succeeded, int failed);

private slots:
    void onProcessOutput();
    void onProcessFinished(int exitCode);
    void onScanFinished();

private:
    QString m_scoopPath;
    QString m_appsDir;
    QStringList m_bucketDirs;
    QProcess* m_process = nullptr;
    ScoopOpType m_currentOp = ScoopOpType::None;
    QString m_currentPackage;

    QFutureWatcher<QVector<InstalledPackage>>* m_scanWatcher = nullptr;
    QFutureWatcher<QVector<ScoopPackage>>* m_searchWatcher = nullptr;
    QFutureWatcher<QVector<BucketInfo>>* m_bucketWatcher = nullptr;

    // 批量操作队列
    QVector<QueueItem> m_queue;
    int m_queueTotal = 0;
    int m_queueIndex = 0;
    int m_queueFailed = 0;

    // 辅助
    void enqueue(const QueueItem& item);
    void runNextQueued();

    QString findScoopExecutable();
    QString resolveAppsDir();
    QString bucketsRootDir() const;
    QStringList discoverBucketDirs();
    void runScoop(const QStringList& args, ScoopOpType op, const QString& package);
    void parseScoopStatusOutput(const QString& output);

    // 后台扫描函数（静态，供 QtConcurrent 调用）
    static QVector<InstalledPackage> scanInstalledImpl(const QString& appsDir,
                                                       const QStringList& bucketDirs);
    static QVector<ScoopPackage> searchImpl(const QString& query,
                                            const QStringList& bucketDirs,
                                            const QString& appsDir);
    static QVector<BucketInfo> fetchBucketsImpl(const QString& scoopPath,
                                                const QString& appsDir);
    static QStringList findBucketDirs(const QString& appsDir);
};
