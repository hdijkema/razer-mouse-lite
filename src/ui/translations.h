#pragma once
#include <QTranslator>
#include <QStringList>

class Translations
{
public:
    bool load(const QString &overrideLanguage = QString());
    static QStringList languages();
    static QString languageName(const QString &language);
    static QString selectedLanguage();
    static QString resolveLanguage(const QString &requested, const QString &systemLocale);
    static bool saveLanguage(const QString &language);
private:
    QTranslator translator;
    QTranslator qtTranslator;
};
