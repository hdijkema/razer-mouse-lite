#pragma once
#include "mousebackend.h"
#include <QFutureWatcher>
#include <QMainWindow>
#include <QSystemTrayIcon>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;
class QMenu;

class RazerMouseWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit RazerMouseWindow(QWidget *parent = nullptr, bool preview = false);
    ~RazerMouseWindow() override;
    void reveal();
    static QIcon mouseIcon(const QColor &color);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    void buildUi();
    void refreshDevices();
    void rebuildProfiles();
    void rebuildTray();
    void loadEditor(int index);
    bool saveEditor();
    bool persist();
    void activateProfile(int index);
    void updateColorButton();
    void updateCapabilities();
    ProfileStore store;
    QList<MouseDevice> devices;
    QFutureWatcher<QList<MouseDevice>> scan;
    QFutureWatcher<ApplyResult> application;
    QSystemTrayIcon *tray = nullptr;
    QMenu *trayMenu = nullptr;
    QTimer *deviceTimer = nullptr;
    QTimer *trayClickTimer = nullptr;
    QListWidget *profileList = nullptr;
    QComboBox *deviceBox = nullptr;
    QLineEdit *nameEdit = nullptr;
    QSpinBox *dpiEdit = nullptr;
    QSlider *dpiSlider = nullptr;
    QSlider *speedSlider = nullptr;
    QLabel *speedLabel = nullptr;
    QComboBox *pollBox = nullptr;
    QCheckBox *accelerationBox = nullptr;
    QCheckBox *pointerBox = nullptr;
    QComboBox *lightingBox = nullptr;
    QPushButton *colorButton = nullptr;
    QSlider *brightnessSlider = nullptr;
    QLabel *brightnessLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *deviceLabel = nullptr;
    QLabel *activeLabel = nullptr;
    QWidget *editor = nullptr;
    QPushButton *applyButton = nullptr;
    QPushButton *refreshButton = nullptr;
    QPushButton *addButton = nullptr;
    QPushButton *deleteButton = nullptr;
    QColor editedColor;
    int editing = -1;
    bool readOnly = false;
    bool preview = false;
    bool restorePending = false;
    QString applyingId;
};


