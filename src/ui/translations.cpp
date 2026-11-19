#include "translations.h"
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>

QStringList Translations::languages() { return {"en", "nl", "de", "da", "nb", "sv", "fr", "es"}; }
QString Translations::languageName(const QString &language)
{
    const QStringList names{"English", "Nederlands", "Deutsch", "Dansk", "Norsk (bokmål)", "Svenska", "Français", "Español"};
    const auto index = languages().indexOf(language);
    return index < 0 ? QString() : names.at(index);
}
QString Translations::selectedLanguage() { return QSettings().value("ui/language", "system").toString(); }
QString Translations::resolveLanguage(const QString &requested, const QString &systemLocale)
{
    QString language = requested == "system" || requested.isEmpty() ? systemLocale : requested;
    language = language.toLower().replace('-', '_').section('_', 0, 0);
    if (language == "no") language = "nb";
    return languages().contains(language) ? language : "en";
}
bool Translations::load(const QString &overrideLanguage)
{
    const QString language = resolveLanguage(overrideLanguage.isEmpty() ? selectedLanguage() : overrideLanguage, QLocale::system().name());
    if (!translator.load(":/i18n/mouse-lite_" + language + ".qm")) return false;
    QCoreApplication::installTranslator(&translator);
    if (qtTranslator.load("qtbase_" + language, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(&qtTranslator);
    return true;
}
bool Translations::saveLanguage(const QString &language)
{
    if (language != "system" && !languages().contains(language)) return false;
    QSettings settings;
    settings.setValue("ui/language", language);
    settings.sync();
    return settings.status() == QSettings::NoError;
}
