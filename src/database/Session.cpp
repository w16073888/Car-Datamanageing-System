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
    if (!m_loggedIn)
        return false;

    // 经理：全开放
    if (m_position == "经理")
        return true;

    // 员工管理：仅经理可访问，其余职位一律关闭
    if (menuPath == "员工管理")
        return false;

    // 库管：关闭 前台业务 / 服务跟踪
    if (m_position == "库管"
        && (menuPath == "前台业务" || menuPath == "服务跟踪"))
        return false;

    // 前台 / 客服：关闭 库房管理
    if ((m_position == "前台" || m_position == "客服")
        && menuPath == "库房管理")
        return false;

    return true;
}

bool Session::canDeleteDataTable(const QString &tableName) const
{
    if (!m_loggedIn)
        return false;

    // 经理：全部表可删除
    if (m_position == "经理")
        return true;

    // 回访记录表：前台/客服可删除（删除 = 清除该工单的回访信息）
    if (tableName == "回访记录")
        return (m_position == "前台" || m_position == "客服");

    // 前台：仅工单表可删除（onDelete 中仍校验“仅已派工”状态）
    if (m_position == "前台")
        return tableName == "t_workorder";

    // 库管：仅备件表可删除
    if (m_position == "库管")
        return tableName == "t_parts";

    // 客服：无其它删除权限；未知职位默认按最严格处理
    return false;
}
