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
