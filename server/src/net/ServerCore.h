#ifndef SERVERCORE_H
#define SERVERCORE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>

// ============================================================
// 服务端监听核心（单线程事件循环）
//   每个 socket 一个累积缓冲；收到完整帧后分发到 CommandDispatcher。
//   登录成功后记录 socket→token 映射，断开时自动登出会话。
// ============================================================
class ServerCore : public QObject
{
    Q_OBJECT
public:
    explicit ServerCore(QObject *parent = nullptr);

    bool listen(quint16 port, QString *err = nullptr);
    void stop();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void processBuffer(QTcpSocket *sock);
    QJsonObject handleRequest(QTcpSocket *sock, const QJsonObject &req);

    QTcpServer m_server;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QHash<QTcpSocket*, QString> m_tokenBySocket;
};

#endif // SERVERCORE_H
