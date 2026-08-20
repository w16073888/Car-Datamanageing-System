#ifndef REMOTECLIENT_H
#define REMOTECLIENT_H

#include <QObject>
#include <QJsonObject>
#include <QHash>
#include <QByteArray>

class QTcpSocket;
class QEventLoop;

// ============================================================
// 客户端远程调用入口（单例）
//   - JSON/TCP 同步调用（内部 QEventLoop + 超时；局域网毫秒级延迟）
//   - 登录成功后持有 token，自动附带在每个请求上
//   - 支持嵌套调用（重入安全：pending 按 id 派发）
// ============================================================
class RemoteClient : public QObject
{
    Q_OBJECT
public:
    static RemoteClient& instance();

    bool connectToServer(const QString &host, quint16 port, int timeoutMs = 5000);
    void disconnect();
    bool isConnected() const;
    QString lastError() const;

    void setToken(const QString &token);
    QString token() const;

    // 同步调用。返回响应对象 {ok, data|error}；失败时 lastError() 给出原因。
    QJsonObject call(const QString &cmd, const QJsonObject &params, int timeoutMs = 15000);

signals:
    void connectionLost();

private:
    explicit RemoteClient(QObject *parent = nullptr);
    void onReadyRead();
    void onDisconnected();

    QTcpSocket *m_sock = nullptr;
    QByteArray m_buffer;
    int m_nextId = 1;
    QString m_lastError;
    QString m_token;
    bool m_manuallyDisconnecting = false;
    QHash<int, QEventLoop*> m_pending;
    QHash<int, QJsonObject> m_responses;
};

#endif // REMOTECLIENT_H
