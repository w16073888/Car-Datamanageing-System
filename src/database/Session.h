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
    bool hasPermission(const QString &menuPath) const;

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
