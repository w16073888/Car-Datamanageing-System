#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <QString>
#include <QSet>

class Session : public QObject
{
    Q_OBJECT

public:
    static Session& instance();

    void login(int id, const QString &employeeId, const QString &name,
               const QString &position);
    void logout();
    bool isLoggedIn() const;

    // 获取当前用户信息
    int userId() const;
    QString employeeId() const;
    QString userName() const;
    QString position() const;

    // 权限校验：判断当前职位是否有权访问指定菜单路径
    //   menuPath 取值：员工管理 / 前台业务 / 库房管理 / 服务跟踪 / 财务管理 / 报表查询 …
    bool hasPermission(const QString &menuPath) const;

    // 数据管理页：指定数据表是否允许执行“删除选中行”
    //   经理=全部表；前台=仅工单表；库管=仅备件表；客服=全部关闭
    bool canDeleteDataTable(const QString &tableName) const;

private:
    explicit Session(QObject *parent = nullptr);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    int m_userId = 0;
    QString m_employeeId;
    QString m_userName;
    QString m_position;
    bool m_loggedIn = false;
};

#endif // SESSION_H
