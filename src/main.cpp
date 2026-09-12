#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include <QSettings>

#include "core/scoop_service.h"
#include "core/settings_store.h"
#include "ui/main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("gscoop");
    app.setApplicationDisplayName("gScoop");
    app.setOrganizationName("gscoop");
    app.setApplicationVersion("0.1.0");
    app.setQuitOnLastWindowClosed(true);

    // 强制 Fusion 风格：保证 QPalette 完全生效（甘雨深色主题）
    app.setStyle(QStyleFactory::create("Fusion"));

    // 全局字体（中文友好）
    QFont font("Microsoft YaHei", 9);
    app.setFont(font);

    // 服务与主窗口
    ScoopService service;
    MainWindow window(&service);
    window.show();

    return app.exec();
}
