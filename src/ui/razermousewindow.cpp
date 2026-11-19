#include "razermousewindow.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <QtConcurrent>
#include "platform.h"
#include "translations.h"

QIcon RazerMouseWindow::mouseIcon(const QColor &color)
{
    QPixmap image(64, 64);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 4));
    p.setBrush(QColor("#182127"));
    p.drawRoundedRect(QRectF(15, 7, 34, 50), 16, 16);
    p.drawLine(QPointF(32, 8), QPointF(32, 28));
    p.drawLine(QPointF(16, 29), QPointF(48, 29));
    p.setPen(QPen(color, 5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(32, 18), QPointF(32, 22));
    return QIcon(image);
}

RazerMouseWindow::RazerMouseWindow(QWidget *parent, bool preview) : QMainWindow(parent), preview(preview)
{
    QString error;
    if (!store.load(&error)) readOnly = true;
    restorePending = !preview && !readOnly && store.restoreOnStart && !store.activeId.isEmpty();
    setWindowTitle("Razer Mouse Lite");
    setWindowIcon(QIcon(":/assets/razer-synapse.ico"));
    resize(1000, 850);
    setMinimumSize(900, 700);
    buildUi();
    tray = new QSystemTrayIcon(windowIcon(), this);
    trayMenu = new QMenu(this);
    tray->setContextMenu(trayMenu);
    trayClickTimer = new QTimer(this);
    trayClickTimer->setSingleShot(true);
    connect(trayClickTimer, &QTimer::timeout, this, [this] {
        if (readOnly || store.profiles.isEmpty() || application.isRunning() || scan.isRunning()) return;
        int index = -1;
        for (int i = 0; i < store.profiles.size(); ++i) if (store.profiles[i].id == store.activeId) index = i;
        const int next = (index + 1) % int(store.profiles.size());
        profileList->setCurrentRow(next);
        activateProfile(next);
    });
    connect(tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) trayClickTimer->start(QApplication::doubleClickInterval());
        if (reason == QSystemTrayIcon::DoubleClick) { trayClickTimer->stop(); reveal(); }
    });
    connect(&scan, &QFutureWatcher<QList<MouseDevice>>::finished, this, [this] {
        bool wasConnected = false;
        for (const auto &device : devices) if (device.path == store.devicePath) wasConnected = true;
        devices = scan.result();
        const QString previous = store.devicePath;
        const QSignalBlocker blocker(deviceBox);
        deviceBox->clear();
        int selected = -1;
        for (int i = 0; i < devices.size(); ++i) {
            deviceBox->addItem(devices[i].name + QString(" · USB %1").arg(devices[i].pid, 4, 16, QChar('0')), devices[i].path);
            if (devices[i].path == previous) selected = i;
        }
        if (selected < 0 && !devices.isEmpty()) selected = 0;
        deviceBox->setCurrentIndex(selected);
        if (devices.isEmpty()) deviceBox->addItem(tr("No supported USB mouse found"));
        refreshButton->setEnabled(true);
        if (selected >= 0) store.devicePath = devices[selected].path;
        updateCapabilities();
        const bool reconnect = !this->preview && store.restoreOnStart && !wasConnected && selected >= 0 && !store.activeId.isEmpty();
        if ((restorePending || reconnect) && selected >= 0) {
            restorePending = false;
            for (int i = 0; i < store.profiles.size(); ++i)
                if (store.profiles[i].id == store.activeId) { activateProfile(i); break; }
        }
    });
    connect(&application, &QFutureWatcher<ApplyResult>::finished, this, [this] {
        const auto result = application.result();
        editor->setEnabled(!readOnly);
        profileList->setEnabled(!readOnly);
        addButton->setEnabled(!readOnly);
        deleteButton->setEnabled(!readOnly);
        deviceBox->setEnabled(true);
        refreshButton->setEnabled(true);
        statusLabel->setText(result.message);
        statusLabel->setProperty("error", !result.success);
        statusLabel->style()->unpolish(statusLabel);
        statusLabel->style()->polish(statusLabel);
        if (result.success) {
            store.activeId = applyingId;
            persist();
            rebuildTray();
            rebuildProfiles();
        } else {
            store.activeId.clear();
            persist();
            rebuildTray();
            rebuildProfiles();
        }
        updateCapabilities();
    });
    deviceTimer = new QTimer(this);
    deviceTimer->setSingleShot(true);
    connect(deviceTimer, &QTimer::timeout, this, &RazerMouseWindow::refreshDevices);
    Platform::watchDevices(this, [this] { deviceTimer->start(800); });
    rebuildProfiles();
    rebuildTray();
    tray->show();
    if (readOnly) {
        statusLabel->setText(error);
        editor->setEnabled(false);
        profileList->setEnabled(false);
        addButton->setEnabled(false);
        deleteButton->setEnabled(false);
    }
    QTimer::singleShot(0, this, &RazerMouseWindow::refreshDevices);
}

RazerMouseWindow::~RazerMouseWindow()
{
    scan.waitForFinished();
    application.waitForFinished();
}

void RazerMouseWindow::buildUi()
{
    auto central = new QWidget;
    setCentralWidget(central);
    auto root = new QVBoxLayout(central);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(14);
    auto header = new QHBoxLayout;
    auto title = new QLabel("Razer Mouse Lite");
    title->setObjectName("title");
    header->addWidget(title);
    header->addStretch();
    activeLabel = new QLabel(tr("No profile applied yet"));
    activeLabel->setObjectName("activeBadge");
    header->addWidget(activeLabel);
    root->addLayout(header);
    auto subtitle = new QLabel(tr("Your mouse. Your settings. Done."));
    subtitle->setObjectName("muted");
    root->addWidget(subtitle);
    auto deviceRow = new QHBoxLayout;
    deviceBox = new QComboBox;
    deviceBox->setMinimumWidth(350);
    deviceRow->addWidget(deviceBox, 1);
    refreshButton = new QPushButton(tr("Refresh"));
    deviceRow->addWidget(refreshButton);
    root->addLayout(deviceRow);
    deviceLabel = new QLabel(tr("Looking for USB mice…"));
    deviceLabel->setObjectName("muted");
    deviceLabel->setWordWrap(true);
    root->addWidget(deviceLabel);
    connect(refreshButton, &QPushButton::clicked, this, &RazerMouseWindow::refreshDevices);
    connect(deviceBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0 && index < devices.size()) { store.devicePath = devices[index].path; persist(); }
        updateCapabilities();
    });
    auto body = new QHBoxLayout;
    body->setSpacing(18);
    auto sidebar = new QVBoxLayout;
    auto profileTitle = new QLabel(tr("PROFILES"));
    profileTitle->setObjectName("sectionLabel");
    sidebar->addWidget(profileTitle);
    profileList = new QListWidget;
    profileList->setFixedWidth(210);
    profileList->setSpacing(5);
    sidebar->addWidget(profileList, 1);
    auto hint = new QLabel(tr("Click a profile to apply\nit immediately."));
    hint->setObjectName("muted");
    sidebar->addWidget(hint);
    auto actions = new QHBoxLayout;
    addButton = new QPushButton(tr("+ New"));
    deleteButton = new QPushButton(tr("Delete"));
    actions->addWidget(addButton); actions->addWidget(deleteButton);
    sidebar->addLayout(actions);
    body->addLayout(sidebar);
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    editor = new QWidget;
    scroll->setWidget(editor);
    auto fields = new QVBoxLayout(editor);
    fields->setContentsMargins(0, 0, 8, 0);
    fields->setSpacing(10);
    auto nameRow = new QFormLayout;
    nameEdit = new QLineEdit;
    nameEdit->setMaxLength(80);
    nameRow->addRow(tr("Profile name"), nameEdit);
    fields->addLayout(nameRow);
    auto sensor = new QGroupBox(tr("Sensor"));
    auto sensorForm = new QFormLayout(sensor);
    auto dpiRow = new QHBoxLayout;
    dpiSlider = new QSlider(Qt::Horizontal);
    dpiSlider->setRange(100, 8500);
    dpiSlider->setSingleStep(50);
    dpiEdit = new QSpinBox;
    dpiEdit->setRange(100, 45000);
    dpiEdit->setSingleStep(50);
    dpiEdit->setSuffix(" DPI");
    dpiEdit->setMinimumWidth(130);
    dpiRow->addWidget(dpiSlider, 1); dpiRow->addWidget(dpiEdit);
    sensorForm->addRow(tr("Sensitivity"), dpiRow);
    connect(dpiEdit, &QSpinBox::valueChanged, dpiSlider, &QSlider::setValue);
    connect(dpiSlider, &QSlider::valueChanged, dpiEdit, &QSpinBox::setValue);
    pollBox = new QComboBox;
    pollBox->addItem(tr("Keep current"), 0);
    for (int rate : {125, 500, 1000}) pollBox->addItem(QString::number(rate) + " Hz", rate);
    sensorForm->addRow(tr("Polling rate"), pollBox);
    fields->addWidget(sensor);
    auto pointerGroup = new QGroupBox(tr("Desktop pointer"));
    auto pointerForm = new QFormLayout(pointerGroup);
    pointerBox = new QCheckBox(tr("Include desktop pointer settings in this profile"));
    pointerForm->addRow(pointerBox);
    auto speedRow = new QHBoxLayout;
    speedSlider = new QSlider(Qt::Horizontal); speedSlider->setRange(1, 20);
    speedLabel = new QLabel; speedLabel->setMinimumWidth(55);
    speedRow->addWidget(speedSlider, 1); speedRow->addWidget(speedLabel);
    pointerForm->addRow(tr("Speed"), speedRow);
    accelerationBox = new QCheckBox(tr("Enable pointer acceleration"));
    pointerForm->addRow(accelerationBox);
    auto pointerHint = new QLabel(Platform::pointerSettingsDescription());
    pointerHint->setObjectName("muted"); pointerHint->setWordWrap(true);
    pointerForm->addRow(pointerHint);
    connect(speedSlider, &QSlider::valueChanged, this, [this](int value) { speedLabel->setText(QString::number(value) + " / 20"); });
    connect(pointerBox, &QCheckBox::toggled, this, [this](bool value) { speedSlider->setEnabled(value); accelerationBox->setEnabled(value); });
    fields->addWidget(pointerGroup);
    auto lighting = new QGroupBox(tr("Lighting"));
    auto lightForm = new QFormLayout(lighting);
    lightingBox = new QComboBox;
    lightingBox->addItem(tr("Static color"), "static");
    lightingBox->addItem(tr("Off"), "off");
    lightingBox->addItem(tr("Keep current"), "keep");
    colorButton = new QPushButton;
    auto lightRow = new QHBoxLayout;
    lightRow->addWidget(lightingBox, 1); lightRow->addWidget(colorButton);
    lightForm->addRow(tr("Effect / color"), lightRow);
    auto brightnessRow = new QHBoxLayout;
    brightnessSlider = new QSlider(Qt::Horizontal); brightnessSlider->setRange(0, 100);
    brightnessLabel = new QLabel; brightnessLabel->setMinimumWidth(55);
    brightnessRow->addWidget(brightnessSlider, 1); brightnessRow->addWidget(brightnessLabel);
    lightForm->addRow(tr("Brightness"), brightnessRow);
    connect(brightnessSlider, &QSlider::valueChanged, this, [this](int value) { brightnessLabel->setText(QString::number(value) + "%"); });
    connect(lightingBox, &QComboBox::currentIndexChanged, this, [this] { updateCapabilities(); });
    connect(colorButton, &QPushButton::clicked, this, [this] {
        const auto color = QColorDialog::getColor(editedColor, this, tr("Profile color"));
        if (color.isValid()) { editedColor = color; updateColorButton(); }
    });
    fields->addWidget(lighting);
    auto saveRow = new QHBoxLayout;
    auto save = new QPushButton(tr("Save"));
    applyButton = new QPushButton(tr("Save and apply"));
    applyButton->setObjectName("primary");
    saveRow->addWidget(save); saveRow->addStretch(); saveRow->addWidget(applyButton);
    fields->addLayout(saveRow);
    auto saveHint = new QLabel(tr("Save changes before switching profiles."));
    saveHint->setWordWrap(true); saveHint->setObjectName("muted");
    fields->addWidget(saveHint);
    fields->addStretch();
    body->addWidget(scroll, 1);
    root->addLayout(body, 1);
    connect(profileList, &QListWidget::currentRowChanged, this, &RazerMouseWindow::loadEditor);
    connect(profileList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) { activateProfile(profileList->row(item)); });
    connect(save, &QPushButton::clicked, this, [this] { if (saveEditor()) statusLabel->setText(tr("Profile saved. Click the profile to apply it.")); });
    connect(applyButton, &QPushButton::clicked, this, [this] { if (saveEditor()) activateProfile(editing); });
    connect(addButton, &QPushButton::clicked, this, [this] {
        Profile p;
        if (editing >= 0 && editing < store.profiles.size()) p = store.profiles[editing];
        p.id = QUuid::createUuid().toString(QUuid::WithoutBraces); p.name = tr("New profile");
        store.profiles.append(p);
        if (persist()) { rebuildProfiles(); profileList->setCurrentRow(int(store.profiles.size()) - 1); rebuildTray(); nameEdit->setFocus(); nameEdit->selectAll(); }
    });
    connect(deleteButton, &QPushButton::clicked, this, [this] {
        if (editing < 0 || store.profiles.size() <= 1) return;
        QMessageBox confirmation(QMessageBox::Question, tr("Delete profile"), tr("Delete profile '%1'?").arg(store.profiles[editing].name), QMessageBox::NoButton, this);
        auto remove = confirmation.addButton(tr("Delete"), QMessageBox::DestructiveRole);
        auto cancel = confirmation.addButton(tr("Cancel"), QMessageBox::RejectRole);
        confirmation.setDefaultButton(cancel);
        confirmation.exec();
        if (confirmation.clickedButton() != remove) return;
        if (store.profiles[editing].id == store.activeId) store.activeId.clear();
        store.profiles.removeAt(editing); editing = -1;
        persist(); rebuildProfiles(); rebuildTray();
    });
    auto startupRow = new QHBoxLayout;
    auto startup = new QCheckBox(tr("Start at login"));
    startup->setChecked(Platform::startupEnabled());
    auto restore = new QCheckBox(tr("Restore active profile on startup / reconnect"));
    restore->setChecked(store.restoreOnStart);
    restore->setEnabled(!readOnly);
    startupRow->addWidget(startup); startupRow->addStretch(); startupRow->addWidget(restore);
    root->addLayout(startupRow);
    auto languageRow = new QHBoxLayout;
    languageRow->addWidget(new QLabel(tr("Language (restart required)")));
    auto languageBox = new QComboBox;
    languageBox->addItem(tr("System language"), "system");
    for (const auto &language : Translations::languages()) languageBox->addItem(Translations::languageName(language), language);
    languageBox->setCurrentIndex(qMax(0, languageBox->findData(Translations::selectedLanguage())));
    languageRow->addWidget(languageBox);
    languageRow->addStretch();
    root->addLayout(languageRow);
    connect(languageBox, &QComboBox::currentIndexChanged, this, [this, languageBox] {
        if (Translations::saveLanguage(languageBox->currentData().toString()))
            statusLabel->setText(tr("Language saved. Restart Razer Mouse Lite to apply it."));
        else statusLabel->setText(tr("Could not save the language preference."));
    });
    connect(startup, &QCheckBox::toggled, this, [this, startup](bool enabled) {
        QString error;
        if (!Platform::setStartupEnabled(enabled, &error)) {
            const QSignalBlocker blocker(startup); startup->setChecked(!enabled); statusLabel->setText(error);
        }
    });
    connect(restore, &QCheckBox::toggled, this, [this](bool enabled) { store.restoreOnStart = enabled; persist(); });
    statusLabel = new QLabel(tr("Choose a profile or edit its settings."));
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(38);
    statusLabel->setObjectName("status");
    root->addWidget(statusLabel);
}

void RazerMouseWindow::rebuildProfiles()
{
    const int selected = store.profiles.isEmpty() ? -1 : qBound(0, editing, int(store.profiles.size()) - 1);
    const QSignalBlocker blocker(profileList);
    profileList->clear();
    for (const auto &profile : store.profiles) {
        const bool active = store.activeId == profile.id;
        auto item = new QListWidgetItem(mouseIcon(profile.color), profile.name + (active ? "  •" : "") + "\n" + QString::number(profile.dpi) + " DPI", profileList);
        item->setSizeHint(QSize(190, 68));
    }
    profileList->setCurrentRow(selected);
    loadEditor(selected);
    deleteButton->setEnabled(store.profiles.size() > 1 && !readOnly);
}

void RazerMouseWindow::loadEditor(int index)
{
    if (index < 0 || index >= store.profiles.size()) return;
    editing = index;
    const auto &profile = store.profiles[index];
    nameEdit->setText(profile.name);
    // The model limit is shown by the slider; keep an incompatible saved DPI visible for validation.
    { const QSignalBlocker blocker(dpiSlider); dpiEdit->setValue(profile.dpi); }
    speedSlider->setValue(profile.speed);
    speedLabel->setText(QString::number(profile.speed) + " / 20");
    accelerationBox->setChecked(profile.acceleration);
    pointerBox->setChecked(profile.pointerSettings && Platform::pointerSettingsSupported());
    pointerBox->setEnabled(Platform::pointerSettingsSupported());
    speedSlider->setEnabled(pointerBox->isChecked());
    accelerationBox->setEnabled(pointerBox->isChecked());
    pollBox->setCurrentIndex(pollBox->findData(profile.pollRate));
    lightingBox->setCurrentIndex(lightingBox->findData(profile.lighting));
    brightnessSlider->setValue(profile.brightness);
    brightnessLabel->setText(QString::number(profile.brightness) + "%");
    editedColor = profile.color;
    updateColorButton();
    updateCapabilities();
}

void RazerMouseWindow::updateColorButton()
{
    colorButton->setText(editedColor.name().toUpper());
    colorButton->setStyleSheet("QPushButton { border: 2px solid " + editedColor.name() + "; padding: 6px 16px; }");
}

bool RazerMouseWindow::saveEditor()
{
    if (readOnly || editing < 0 || editing >= store.profiles.size()) return false;
    if (nameEdit->text().trimmed().isEmpty()) { statusLabel->setText(tr("Enter a profile name.")); return false; }
    auto &profile = store.profiles[editing];
    profile.name = nameEdit->text().trimmed(); profile.dpi = dpiEdit->value();
    profile.pollRate = pollBox->currentData().toInt();
    profile.speed = speedSlider->value(); profile.acceleration = accelerationBox->isChecked();
    if (Platform::pointerSettingsSupported()) profile.pointerSettings = pointerBox->isChecked();
    profile.lighting = lightingBox->currentData().toString();
    profile.color = editedColor; profile.brightness = brightnessSlider->value();
    // Editing an active profile does not apply its new settings yet.
    if (store.activeId == profile.id) store.activeId.clear();
    if (!persist()) return false;
    rebuildProfiles(); rebuildTray(); return true;
}

bool RazerMouseWindow::persist()
{
    if (readOnly) return false;
    QString error;
    if (store.save(&error)) return true;
    statusLabel->setText(tr("Could not save: %1").arg(error));
    return false;
}

void RazerMouseWindow::activateProfile(int index)
{
    if (preview || readOnly || index < 0 || index >= store.profiles.size() || application.isRunning() || scan.isRunning()) return;
    const int selected = deviceBox->currentIndex();
    if (selected < 0 || selected >= devices.size()) {
        statusLabel->setText(tr("No supported mouse connected. Connect the mouse using USB and click Refresh."));
        return;
    }
    const auto device = devices[selected];
    auto profile = store.profiles[index];
    if (!Platform::pointerSettingsSupported()) profile.pointerSettings = false;
    applyingId = profile.id;
    editor->setEnabled(false); profileList->setEnabled(false); deviceBox->setEnabled(false);
    refreshButton->setEnabled(false); addButton->setEnabled(false); deleteButton->setEnabled(false);
    statusLabel->setText(tr("Applying profile '%1'…").arg(profile.name));
    application.setFuture(QtConcurrent::run([device, profile] { return MouseBackend::apply(device, profile); }));
}

void RazerMouseWindow::rebuildTray()
{
    if (!tray) return;
    trayMenu->clear();
    QString activeName = tr("No profile applied yet");
    for (int i = 0; i < store.profiles.size(); ++i) {
        const auto &profile = store.profiles[i];
        auto action = trayMenu->addAction(mouseIcon(profile.color), profile.name + " · " + QString::number(profile.dpi) + " DPI");
        action->setCheckable(true); action->setChecked(store.activeId == profile.id);
        action->setEnabled(!readOnly);
        if (store.activeId == profile.id) activeName = profile.name;
        connect(action, &QAction::triggered, this, [this, i] { profileList->setCurrentRow(i); activateProfile(i); });
    }
    activeLabel->setText(activeName);
    tray->setToolTip(tr("Razer Mouse Lite · %1\nClick: next profile · Double-click: settings").arg(activeName));
    trayMenu->addSeparator();
    connect(trayMenu->addAction(tr("Open settings")), &QAction::triggered, this, &RazerMouseWindow::reveal);
    connect(trayMenu->addAction(tr("Quit")), &QAction::triggered, qApp, &QApplication::quit);
}

void RazerMouseWindow::refreshDevices()
{
    if (scan.isRunning() || application.isRunning()) { deviceTimer->start(800); return; }
    refreshButton->setEnabled(false);
    applyButton->setEnabled(false);
    scan.setFuture(QtConcurrent::run(&MouseBackend::enumerate));
}

void RazerMouseWindow::updateCapabilities()
{
    const int selected = deviceBox->currentIndex();
    const bool connected = selected >= 0 && selected < devices.size();
    if (connected) {
        const auto model = devices[selected].model;
        const int maximum = model.value("maxDpi").toInt(8500);
        const QSignalBlocker blocker(dpiSlider);
        dpiSlider->setMaximum(maximum);
        pollBox->setEnabled(model.value("pollTx").toInt(-1) >= 0);
        deviceLabel->setText(tr("Detected automatically · 100–%1 DPI · ").arg(maximum)
            + (model.value("zones").toArray().isEmpty() ? tr("No RGB control for this model") : tr("DPI and lighting over USB")));
    } else deviceLabel->setText(tr("Connect your Razer mouse using USB. You can save profiles in advance."));
    const bool color = lightingBox->currentData().toString() == "static";
    colorButton->setEnabled(color); brightnessSlider->setEnabled(color);
    applyButton->setEnabled(connected && !readOnly && !application.isRunning() && !scan.isRunning());
}

void RazerMouseWindow::reveal()
{
    showNormal(); raise(); activateWindow();
}

void RazerMouseWindow::closeEvent(QCloseEvent *event)
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) { hide(); event->ignore(); }
    else { event->accept(); qApp->quit(); }
}



