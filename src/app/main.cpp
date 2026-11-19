#include "razermousewindow.h"
#include "selftest.h"
#include "translations.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <stdexcept>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("MouseLite");
    app.setApplicationName("MouseLite");
    app.setApplicationDisplayName("Razer Mouse Lite");
    app.setApplicationVersion(MOUSE_LITE_VERSION);
    app.setStyle("Fusion");
    const auto arguments = app.arguments();
    Translations translations;
    const int languageArgument = arguments.indexOf("--language");
    if (!translations.load(languageArgument < 0 ? QString() : arguments.value(languageArgument + 1))) {
        QTextStream(stderr) << "Failed to load the embedded translation catalogue.\n";
        return 1;
    }
    if (arguments.contains("--self-test")) return selfTest(arguments.value(arguments.indexOf("--catalogues") >= 0 ? arguments.indexOf("--catalogues") + 1 : -1));
    if (arguments.contains("--probe")) {
        for (const auto &device : MouseBackend::enumerate()) {
            if (device.pid != 0x00a3) continue;
            const auto result = MouseBackend::readCobraStatus(device);
            QTextStream(stdout) << result.message << "\n";
            return result.success ? 0 : 1;
        }
        QTextStream(stdout) << "No wired Razer Cobra found.\n";
        return 1;
    }
    const bool smoke = arguments.contains("--smoke-test");
    app.setQuitOnLastWindowClosed(false);
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!smoke && !QDir().mkpath(directory)) return 1;
    QLockFile lock(directory + "/instance.lock");
    lock.setStaleLockTime(0);
    const QString serverName = "MouseLite-" + QString::number(qHash(directory), 16);
    if (!smoke && !lock.tryLock(1000)) {
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (!socket.waitForConnected(1500)) return 1;
        socket.write("show"); socket.waitForBytesWritten(1000);
        return 0;
    }
    QLocalServer server;
    if (!smoke) {
        QLocalServer::removeServer(serverName);
        server.setSocketOptions(QLocalServer::UserAccessOption);
        if (!server.listen(serverName)) return 1;
    }
    app.setStyleSheet(R"(
        QWidget { background: #151b20; color: #e3eaf0; font-family: 'Segoe UI'; font-size: 12px; }
        QMainWindow { background: #151b20; }
        QLabel#title { font-size: 28px; font-weight: 650; }
        QLabel#muted { color: #98a8b4; font-size: 11px; }
        QLabel#sectionLabel { color: #98a8b4; font-size: 11px; font-weight: 600; }
        QLabel#activeBadge { background: #203b34; color: #7ce4bb; border-radius: 12px; padding: 7px 12px; }
        QGroupBox { border: 1px solid #34414b; border-radius: 8px; margin-top: 10px; padding: 12px 12px 10px; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; color: #bed0dd; }
        QPushButton, QLineEdit, QComboBox, QSpinBox { background: #222d35; border: 1px solid #3b4a56; border-radius: 5px; padding: 7px 10px; min-height: 17px; }
        QPushButton:hover { background: #2d3d47; border-color: #6a8c9b; }
        QPushButton:disabled, QComboBox:disabled { color: #64717b; }
        QPushButton#primary { background: #55d6a0; color: #10271d; border: 0; font-weight: 650; padding: 8px 15px; }
        QPushButton#primary:hover { background: #81e6bd; }
        QPushButton#primary:disabled { background: #304d42; color: #8ba69a; }
        QLineEdit:focus, QSpinBox:focus { border-color: #55d6a0; }
        QComboBox QAbstractItemView { background: #222d35; selection-background-color: #36594c; }
        QListWidget { background: transparent; border: 0; outline: 0; }
        QListWidget::item { background: #1d272f; border: 1px solid #33414c; border-radius: 7px; padding: 6px; }
        QListWidget::item:selected { color: #e3eaf0; background: #263d35; border-color: #55d6a0; }
        QListWidget::item:hover { border-color: #6e9887; }
        QSlider::groove:horizontal { height: 5px; background: #37464f; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #55d6a0; border-radius: 2px; }
        QSlider::handle:horizontal { background: #d9f8eb; width: 14px; margin: -5px 0; border-radius: 7px; }
        QCheckBox { spacing: 7px; }
        QLabel#status { color: #bbd3c7; border-top: 1px solid #34414b; padding-top: 8px; }
        QLabel#status[error="true"] { color: #ffb1a5; }
        QMenu { background: #202a32; border: 1px solid #44545f; padding: 5px; }
        QMenu::item { padding: 7px 24px; }
        QMenu::item:selected { background: #36594c; }
        QScrollBar:vertical { width: 8px; background: #151b20; }
        QScrollBar::handle:vertical { background: #40515b; border-radius: 4px; min-height: 25px; }
    )");
    RazerMouseWindow window(nullptr, smoke);
    QObject::connect(&server, &QLocalServer::newConnection, &window, [&] {
        while (server.hasPendingConnections()) {
            auto socket = server.nextPendingConnection();
            window.reveal();
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            socket->disconnectFromServer();
        }
    });
    if (smoke) {
        window.show();
        QTimer::singleShot(1000, &window, [&] {
            const int index = arguments.indexOf("--smoke-test");
            const QString image = arguments.value(index + 1);
            const bool saved = !image.isEmpty() && window.grab().save(image);
            app.exit(saved ? 0 : 1);
        });
    } else if (!arguments.contains("--tray") || !QSystemTrayIcon::isSystemTrayAvailable()) window.show();
    return app.exec();
}

