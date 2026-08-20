#include "RemoteClient.h"
#include "JsonProtocol.h"

#include <QTcpSocket>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonValue>
#include <QDebug>

RemoteClient& RemoteClient::instance()
{
    static RemoteClient inst;
    return inst;
}

RemoteClient::RemoteClient(QObject *parent)
    : QObject(parent)
{
}

bool RemoteClient::connectToServer(const QString &host, quint16 port, int timeoutMs)
{
    disconnect();

    m_sock = new QTcpSocket(this);
    connect(m_sock, &QTcpSocket::readyRead, this, &RemoteClient::onReadyRead);
    connect(m_sock, &QTcpSocket::disconnected, this, &RemoteClient::onDisconnected);

    m_sock->connectToHost(host, port);
    if (!m_sock->waitForConnected(timeoutMs)) {
        m_lastError = QString("连接服务器 %1:%2 失败: %3")
                          .arg(host).arg(port).arg(m_sock->errorString());
        qWarning() << "[RemoteClient]" << m_lastError;
        disconnect();
        return false;
    }
    m_sock->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    m_lastError.clear();
    m_buffer.clear();
    m_pending.clear();
    m_responses.clear();
    m_manuallyDisconnecting = false;
    qInfo() << "[RemoteClient] 已连接服务器:" << host << ":" << port;
    return true;
}

void RemoteClient::disconnect()
{
    m_manuallyDisconnecting = true;
    if (m_sock) {
        if (m_sock->state() == QAbstractSocket::ConnectedState)
            m_sock->disconnectFromHost();
        m_sock->deleteLater();
        m_sock = nullptr;
    }
    // 中断所有 pending
    for (QEventLoop *loop : m_pending.values()) {
        if (loop)
            loop->quit();
    }
    m_pending.clear();
    m_responses.clear();
    m_buffer.clear();
}

bool RemoteClient::isConnected() const
{
    return m_sock && m_sock->state() == QAbstractSocket::ConnectedState;
}

QString RemoteClient::lastError() const
{
    return m_lastError;
}

void RemoteClient::setToken(const QString &token)
{
    m_token = token;
}

QString RemoteClient::token() const
{
    return m_token;
}

QJsonObject RemoteClient::call(const QString &cmd, const QJsonObject &params, int timeoutMs)
{
    if (!isConnected()) {
        m_lastError = "未连接到服务器";
        return { { "ok", false }, { "error", m_lastError } };
    }

    QJsonObject req;
    req["id"] = m_nextId++;
    req["cmd"] = cmd;
    req["params"] = params;
    if (!m_token.isEmpty())
        req["token"] = m_token;

    const int id = req["id"].toInt();
    QEventLoop loop;
    m_pending[id] = &loop;

    m_sock->write(JsonProtocol::makeFrame(req));
    m_sock->flush();

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();   // onReadyRead 收到匹配响应时 quit
    timer.stop();

    m_pending.remove(id);
    const QJsonObject resp = m_responses.take(id);

    if (resp.isEmpty()) {
        m_lastError = isConnected() ? "请求超时，服务器无响应" : "连接已断开";
        return { { "ok", false }, { "error", m_lastError } };
    }

    if (!resp.value("ok").toBool()) {
        m_lastError = resp.value("error").toString();
        return resp;
    }
    m_lastError.clear();
    return resp;
}

void RemoteClient::onReadyRead()
{
    if (!m_sock)
        return;
    m_buffer.append(m_sock->readAll());

    for (;;) {
        QJsonObject resp;
        QString err;
        if (!JsonProtocol::tryParseFrame(m_buffer, resp, err)) {
            if (!err.isEmpty()) {
                m_lastError = err;
                disconnect();
                return;
            }
            return;   // 等待更多数据
        }
        const int id = resp.value("id").toInt(-1);
        if (id >= 0 && m_pending.contains(id)) {
            m_responses[id] = resp;
            m_pending[id]->quit();
        }
        // 非 pending 的帧（超时后到达）直接丢弃
    }
}

void RemoteClient::onDisconnected()
{
    if (!m_manuallyDisconnecting && isConnected() == false) {
        emit connectionLost();
    }
    // 连接中断：让所有 pending 立即返回失败
    for (QEventLoop *loop : m_pending.values()) {
        if (loop)
            loop->quit();
    }
}
