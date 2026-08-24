#include "AppSettings.h"

#include <QSettings>
#include <QCoreApplication>

static QString settingsFilePath()
{
    return QCoreApplication::applicationDirPath() + "/client_settings.ini";
}

bool AppSettings::simpleMode()
{
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    return s.value("frontdesk/simpleMode", true).toBool();   // 默认简洁模式；未显式设置过则返回 true
}

void AppSettings::setSimpleMode(bool on)
{
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    s.setValue("frontdesk/simpleMode", on);
    s.sync();
}

QString AppSettings::officeAddress()
{
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    return s.value("frontdesk/officeAddress",
                   QString::fromUtf8("成都市双流区航都大街二段370号")).toString();
}

void AppSettings::setOfficeAddress(const QString &addr)
{
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    s.setValue("frontdesk/officeAddress", addr);
    s.sync();
}

QString AppSettings::servicePhone()
{
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    return s.value("frontdesk/servicePhone").toString();
}

void AppSettings::setServicePhone(const QString &phone)
{
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    s.setValue("frontdesk/servicePhone", phone);
    s.sync();
}
