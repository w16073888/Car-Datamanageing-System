#include "Session.h"
#include <QDebug>

Session::Session(QObject *parent)
    : QObject(parent)
{
}

Session::~Session()
{
}

Session& Session::instance()
{
    static Session inst;
    return inst;
}

void Session::login(int id, const QString &employeeId, const QString &name,
                     const QString &position)
{
    m_userId = id;
    m_employeeId = employeeId;
    m_userName = name;
    m_position = position;
    m_loggedIn = true;
    qDebug() << "[Session] 用户登录:" << name << "| 工号:" << employeeId << "| 职位:" << position;
}

void Session::logout()
{
    m_userId = 0;
    m_employeeId.clear();
    m_userName.clear();
    m_position.clear();
    m_loggedIn = false;
    qDebug() << "[Session] 用户已退出";
}

bool Session::isLoggedIn() const
{
    return m_loggedIn;
}

int Session::userId() const
{
    return m_userId;
}

QString Session::employeeId() const
{
    return m_employeeId;
}

QString Session::userName() const
{
    return m_userName;
}

QString Session::position() const
{
    return m_position;
}

bool Session::hasPermission(const QString &menuPath) const
{
    Q_UNUSED(menuPath)
    // 按需求: 暂时所有职位拥有全部权限
    return m_loggedIn;
}
