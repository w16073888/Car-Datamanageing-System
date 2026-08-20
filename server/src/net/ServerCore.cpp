#include "ServerCore.h"
#include "JsonProtocol.h"
#include "../auth/AuthManager.h"
#include "../cmd/CommandDispatcher.h"

#include <QHostAddress>
#include <QJsonValue>
#include <QDebug>

ServerCore::ServerCore(QObject *parent)
    : QObject(parent)
{
}

bool ServerCore::listen(quint16 port, QString *err)
{
    if (!m_server.listen(QHostAddress::Any, port)) {
        if (err)
            *err = m_server.errorString();
        return false;
    }
    connect(&m_server, &QTcpServer::newConnection, this, &ServerCore::onNewConnection);
    return true;
}

void ServerCore::stop()
{
    m_server.close();
    for (QTcpSocket *sock : m_buffers.keys()) {
        sock->disconnectFromHost();
        sock->deleteLater();
    }
    m_buffers.clear();
    m_tokenBySocket.clear();
}

void ServerCore::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *sock = m_server.nextPendingConnection();
        connect(sock, &QTcpSocket::readyRead, this, &ServerCore::onReadyRead);
        connect(sock, &QTcpSocket::disconnected, this, &ServerCore::onDisconnected);
        m_buffers.insert(sock, QByteArray());
    }
}

void ServerCore::onReadyRead()
{
    auto *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock || !m_buffers.contains(sock))
        return;
    m_buffers[sock].append(sock->readAll());
    processBuffer(sock);
}

void ServerCore::processBuffer(QTcpSocket *sock)
{
    QByteArray &buf = m_buffers[sock];
    for (;;) {
        QJsonObject req;
        QString err;
        if (!JsonProtocol::tryParseFrame(buf, req, err)) {
            if (!err.isEmpty()) {
                // 协议错误 → 回错误帧并断开
                QJsonObject resp;
                resp["id"] = -1;
                resp["ok"] = false;
                resp["error"] = "协议错误: " + err;
                sock->write(JsonProtocol::makeFrame(resp));
                sock->flush();
                sock->disconnectFromHost();
                return;
            }
            return; // 等待更多数据
        }
        QJsonObject inner = handleRequest(sock, req);
        inner["id"] = req.value("id").toInt(-1);
        sock->write(JsonProtocol::makeFrame(inner));
    }
}

QJsonObject ServerCore::handleRequest(QTcpSocket *sock, const QJsonObject &req)
{
    const QString cmd = req.value("cmd").toString();
    const QString token = req.value("token").toString();
    const QJsonObject params = req.value("params").toObject();

    // auth.logout：无需分发器，直接用 token 注销会话
    if (cmd == "auth.logout") {
        AuthManager::instance().logout(token);
        m_tokenBySocket.remove(sock);
        return { { "ok", true }, { "data", QJsonObject() } };
    }

    QJsonObject resp = CommandDispatcher::instance().dispatch(cmd, params, token);

    // 登录成功时关联 socket→token，便于断开时登出
    if (cmd == "auth.login" && resp.value("ok").toBool()) {
        const QString tok = resp.value("data").toObject().value("token").toString();
        if (!tok.isEmpty())
            m_tokenBySocket[sock] = tok;
    }

    return resp;
}

void ServerCore::onDisconnected()
{
    auto *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock)
        return;
    const QString token = m_tokenBySocket.take(sock);
    if (!token.isEmpty())
        AuthManager::instance().logout(token);
    m_buffers.remove(sock);
    sock->deleteLater();
}
