#include "selftest.h"
#include "mousebackend.h"
#include "razerprotocol.h"
#include "profile.h"
#include "hiddescriptor.h"
#include "desktopentry.h"
#include "translations.h"
#include "devicedatabase.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QTranslator>
#include <QXmlStreamReader>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
static void check(bool condition, const char *description)
{
    if (!condition) throw std::runtime_error(description);
}

static QStringList placeholders(const QString &value)
{
    QStringList result;
    auto matches = QRegularExpression("%[1-9][0-9]*|%n").globalMatch(value);
    while (matches.hasNext()) result.append(matches.next().captured());
    result.sort();
    return result;
}

static void checkTranslations(const QString &directory)
{
    QFile manifestFile(":/data/messages.json");
    check(manifestFile.open(QIODevice::ReadOnly), "Translation manifest missing");
    const auto manifest = QJsonDocument::fromJson(manifestFile.readAll()).array();
    check(manifest.size() >= 86, "Incomplete translation manifest");
    QSet<QString> manifestKeys;
    for (const auto &entry : manifest) {
        const auto message = entry.toObject();
        manifestKeys.insert(message.value("context").toString() + QChar(4) + message.value("source").toString());
    }
    for (const auto &language : Translations::languages()) {
        QTranslator catalogue;
        check(catalogue.load(":/i18n/mouse-lite_" + language + ".qm"), "Compiled translation catalogue missing");
        for (const auto &entry : manifest) {
            const auto message = entry.toObject();
            const auto context = message.value("context").toString().toUtf8();
            const auto source = message.value("source").toString();
            const auto translated = catalogue.translate(context.constData(), source.toUtf8().constData());
            if (translated.isEmpty() || placeholders(source) != placeholders(translated))
                throw std::runtime_error((language + ": invalid or missing translation: " + source).toStdString());
            check(!source.startsWith(' ') || translated.startsWith(' '), "Translation lost leading spacing");
            check(!source.endsWith(' ') || translated.endsWith(' '), "Translation lost trailing spacing");
        }
        if (directory.isEmpty()) continue;
        QFile file(directory + "/mouse-lite_" + language + ".ts");
        check(file.open(QIODevice::ReadOnly), "Translation source file missing");
        QXmlStreamReader xml(&file);
        QString context;
        QSet<QString> sourceKeys;
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            if (xml.name() == QLatin1String("name")) context = xml.readElementText();
            if (xml.name() != QLatin1String("message")) continue;
            QString source, translated;
            bool unfinished = false;
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("source")) source = xml.readElementText();
                else if (xml.name() == QLatin1String("translation")) {
                    unfinished = !xml.attributes().value("type").isEmpty();
                    translated = xml.readElementText();
                } else xml.skipCurrentElement();
            }
            check(!unfinished && !translated.isEmpty() && placeholders(source) == placeholders(translated), "Unfinished translation source or broken placeholder");
            sourceKeys.insert(context + QChar(4) + source);
        }
        check(!xml.hasError() && sourceKeys == manifestKeys, "Translation source and manifest differ; update all catalogues");
    }
    check(Translations::resolveLanguage("system", "no_NO") == "nb", "Norwegian locale alias");
    check(Translations::resolveLanguage("system", "de_CH") == "de", "Regional language fallback");
    check(Translations::resolveLanguage("pt-BR", "nl_NL") == "en", "Unknown language fallback");
    check(Translations::resolveLanguage("sv", "en_US") == "sv", "Explicit language preference");
}

int selfTest(const QString &catalogueDirectory)
{
    QTextStream output(stdout);
    try {
        checkTranslations(catalogueDirectory);
        const auto bundledModels = DeviceDatabase::bundled();
        const auto databaseBytes = QJsonDocument(bundledModels).toJson(QJsonDocument::Compact);
        for (const auto &model : bundledModels) {
            if (!DeviceDatabase::validate(QJsonDocument(QJsonArray{model}).toJson(QJsonDocument::Compact)))
                throw std::runtime_error(("Invalid bundled device definition: " + model.toObject().value("name").toString()).toStdString());
        }
        check(DeviceDatabase::validate(databaseBytes), "Bundled device definitions fail validation");
        check(!DeviceDatabase::validate("[]") && !DeviceDatabase::validate("{broken"), "Empty or malformed device definitions accepted");
        check(!DeviceDatabase::validate(QByteArray(DeviceDatabase::maximumBytes + 1, ' ')), "Oversized device definitions accepted");
        auto duplicateModels = bundledModels;
        duplicateModels.append(bundledModels.first());
        check(!DeviceDatabase::validate(QJsonDocument(duplicateModels).toJson()), "Duplicate USB product IDs accepted");
        auto badModel = bundledModels.first().toObject();
        badModel["dpiTx"] = 256;
        check(!DeviceDatabase::validate(QJsonDocument(QJsonArray{badModel}).toJson()), "Out-of-range protocol byte accepted");
        badModel = bundledModels.first().toObject();
        badModel["newProtocol"] = true;
        check(!DeviceDatabase::validate(QJsonDocument(QJsonArray{badModel}).toJson()), "Unknown model format accepted");
        badModel = bundledModels.first().toObject();
        const QJsonObject unknownStep{{"op", "build"}, {"fn", "unknown_protocol_function"}, {"args", QJsonArray{}}};
        const QJsonObject badZone{{"name", "logo"}, {"color", QJsonArray{unknownStep}}, {"off", QJsonArray{}}, {"brightness", QJsonArray{}}};
        badModel["zones"] = QJsonArray{badZone};
        check(!DeviceDatabase::validate(QJsonDocument(QJsonArray{badModel}).toJson()), "Unknown lighting protocol accepted");
        QTemporaryDir databaseDirectory;
        check(databaseDirectory.isValid(), "Temporary device database directory");
        const auto databasePath = databaseDirectory.filePath("definitions.json");
        QString databaseError;
        check(DeviceDatabase::install(databaseBytes, databasePath, &databaseError), "Atomic device database install failed");
        QFile savedDatabase(databasePath);
        check(savedDatabase.open(QIODevice::ReadOnly), "Saved device database missing");
        const auto savedBytes = savedDatabase.readAll();
        savedDatabase.close();
        check(!DeviceDatabase::install("[broken", databasePath, &databaseError), "Invalid database replaced saved definitions");
        check(savedDatabase.open(QIODevice::ReadOnly) && savedDatabase.readAll() == savedBytes, "Rejected update changed saved definitions");
        savedDatabase.close();
        check(DeviceDatabase::load(databasePath) && DeviceDatabase::models() == bundledModels, "Device database reload failed");
        auto expandedModels = bundledModels;
        auto syntheticModel = bundledModels.first().toObject();
        syntheticModel["pid"] = 65534;
        syntheticModel["name"] = "Synthetic database test device";
        expandedModels.append(syntheticModel);
        check(DeviceDatabase::install(QJsonDocument(expandedModels).toJson(QJsonDocument::Compact), databasePath, &databaseError)
            && DeviceDatabase::models() == expandedModels, "Validated new device did not become available");
        check(DeviceDatabase::load(databasePath) && DeviceDatabase::models() == expandedModels, "Updated devices were not retained on reload");
        check(DeviceDatabase::install(databaseBytes, databasePath, &databaseError), "Could not restore bundled test definitions");
        check(!DeviceDatabase::install(databaseBytes, databaseDirectory.path(), &databaseError), "Database install to directory succeeded");
        auto wrongVersion = QJsonDocument::fromJson(savedBytes).object();
        wrongVersion["appVersion"] = "incompatible-version";
        check(savedDatabase.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot write incompatible cache fixture");
        savedDatabase.write(QJsonDocument(wrongVersion).toJson()); savedDatabase.close();
        check(!DeviceDatabase::load(databasePath), "Cache from another app version accepted");
        check(HidDescriptor::featureReportLength(QByteArray::fromHex("7508955ab102")) == 91, "Unnumbered HID feature report");
        check(HidDescriptor::featureReportLength(QByteArray::fromHex("7508952db102b102")) == 91, "Accumulated HID feature fields");
        check(HidDescriptor::featureReportLength(QByteArray::fromHex("7508955aa485019501b102b4b102")) == 91, "HID global push/pop");
        check(HidDescriptor::featureReportLength(QByteArray::fromHex("85017508955ab102")) == 0, "Numbered HID report rejected");
        check(HidDescriptor::featureReportLength(QByteArray::fromHex("750895")) == 0, "Truncated HID descriptor rejected");
        check(HidDescriptor::featureReportLength(QByteArray::fromHex("77ffffffff97ffffffffb102")) == 0, "Oversized HID descriptor rejected");
        const auto desktop = desktopAutostartEntry("/home/user/My Mouse/50%/mouse-lite");
        check(desktop.contains("Exec=\"/home/user/My Mouse/50%%/mouse-lite\" --tray\n"), "XDG autostart quoting and percent escaping");
        check(desktopAutostartEntry("/tmp/a\nb").contains("a\\nb"), "Desktop entry newline escaping");
        check(!Profile::fromJson({{"windowsSettings", false}}).pointerSettings, "Legacy Windows profile migration");
        check(Profile::fromJson({{"pointerSettings", true}, {"windowsSettings", false}}).pointerSettings, "Portable profile key precedence");
        const auto models = RazerProtocol::models();
        check(models.size() >= 100, "Embedded model database missing");
        QJsonObject cobra;
        int commands = 0;
        for (const auto &item : models) {
            const auto model = item.toObject();
            if (model.value("pid").toInt() == 0x00a3) cobra = model;
            const auto dpi = RazerProtocol::dpiReport(model, 800);
            check(dpi.size() == 90, "DPI report length");
            for (const auto &zoneValue : model.value("zones").toArray()) {
                const auto zone = zoneValue.toObject();
                for (const auto &effect : {"color", "off", "brightness"}) {
                    const auto steps = zone.value(effect).toArray();
                    const auto packets = RazerProtocol::lightingReports(steps, QColor("#123456"), 100);
                    if (steps.isEmpty()) continue;
                    check(!packets.isEmpty(), "Lighting recipe generated no packets");
                    for (const auto &packet : packets) {
                        auto response = packet;
                        response[0] = 2;
                        check(RazerProtocol::validResponse(packet, response), "Packet checksum");
                        response[10] = char(response[10] ^ 1);
                        check(!RazerProtocol::validResponse(packet, response), "Corrupt response accepted");
                        ++commands;
                    }
                }
            }
        }
        check(!cobra.isEmpty(), "Razer Cobra missing");
        check(cobra.value("maxDpi").toInt() == 8500, "Cobra DPI maximum");
        const auto dpi = RazerProtocol::dpiReport(cobra, 1850);
        check(dpi.left(13).toHex() == "00ff00000007040501073a073a", "Cobra DPI golden packet");
        const auto zone = cobra.value("zones").toArray().first().toObject();
        const auto color = RazerProtocol::lightingReports(zone.value("color").toArray(), QColor("#123456"), 100).first();
        check(color.left(17).toHex() == "001f000000090f02010401000001123456", "Cobra static color golden packet");
        const auto brightness = RazerProtocol::lightingReports(zone.value("brightness").toArray(), QColor(), 100).first();
        check(brightness.mid(5, 6).toHex() == "030f040104ff", "Cobra brightness golden packet");
        bool invalidRejected = false;
        try { RazerProtocol::dpiReport(cobra, 8501); } catch (...) { invalidRejected = true; }
        check(invalidRejected, "Out-of-range DPI accepted");
        auto mismatched = dpi;
        mismatched[6] = 7;
        RazerProtocol::checksum(mismatched);
        check(!RazerProtocol::validResponse(dpi, mismatched), "Mismatched command accepted");
        Profile original;
        original.id = "test"; original.name = "Precisie"; original.dpi = 1850;
        original.pollRate = 500; original.color = QColor("#123456");
        check(Profile::fromJson(original.toJson()).toJson() == original.toJson(), "Profile roundtrip");
        const auto bounded = Profile::fromJson({{"dpi", -500}, {"speed", 80}, {"brightness", 999}, {"color", "invalid"}});
        check(bounded.dpi == 100 && bounded.speed == 20 && bounded.brightness == 100 && bounded.color.isValid(), "Profile validation");
        QTemporaryDir directory;
        check(directory.isValid(), "Temporary directory");
        const auto path = directory.filePath("profiles.json");
        ProfileStore store(path);
        store.profiles.append(original); store.activeId = original.id; store.restoreOnStart = true;
        check(store.save(), "Atomic profile save");
        ProfileStore loaded(path);
        check(loaded.load() && loaded.profiles.size() == 1 && loaded.activeId == original.id && loaded.restoreOnStart, "Profile store load");
        QFile broken(path);
        check(broken.open(QIODevice::WriteOnly | QIODevice::Truncate), "Corrupt test file");
        broken.write("{broken"); broken.close();
        ProfileStore corrupted(path);
        check(!corrupted.load(), "Corrupted profile file silently overwritten");
        const auto devices = MouseBackend::enumerate();
        output << "PASS: " << models.size() << " models, " << commands << " lighting packets, Cobra golden packets, CRC/response checks, profile migration and atomic persistence.\n";
        output << "PASS: 8 complete translation catalogues, placeholders, locale fallback, HID descriptor parsing and XDG autostart serialization.\n";
        output << "Detected supported USB mice: " << devices.size() << "\n";
        for (const auto &device : devices) output << "  " << device.name << "\n";
        return 0;
    } catch (const std::exception &error) {
        output << "FAIL: " << error.what() << "\n";
        return 1;
    }
}


