#ifndef JSONPROTOCOL_H
#define JSONPROTOCOL_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

// ============================================================
// 通信协议（JSON over TCP）
//   帧格式: [4字节大端长度][UTF-8 JSON]  长度上限 16MB
//   请求:  { "id": int, "token": "...", "cmd": "...", "params": {...} }
//   响应:  { "id": int, "ok": bool, "data": {...} | "error": "..." }
// ============================================================
class JsonProtocol
{
public:
    static const qint32 MAX_FRAME = 16 * 1024 * 1024;

    // 构造一帧
    static QByteArray makeFrame(const QJsonObject &obj)
    {
        QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        QByteArray frame;
        const int n = payload.size();
        frame.append(char((n >> 24) & 0xFF));
        frame.append(char((n >> 16) & 0xFF));
        frame.append(char((n >> 8) & 0xFF));
        frame.append(char(n & 0xFF));
        frame.append(payload);
        return frame;
    }

    // 从累积缓冲中尝试解析一帧。
    //   返回 true 表示成功取出一帧（out 被填充）；
    //   返回 false 时若 err 非空表示协议错误（应断开），否则等待更多数据。
    static bool tryParseFrame(QByteArray &buffer, QJsonObject &out, QString &err)
    {
        err.clear();
        while (buffer.size() >= 4) {
            const qint32 len = (uchar(buffer[0]) << 24) | (uchar(buffer[1]) << 16)
                             | (uchar(buffer[2]) << 8) | uchar(buffer[3]);
            if (len < 0 || len > MAX_FRAME) {
                err = QString("非法帧长度: %1").arg(len);
                buffer.clear();
                return false;
            }
            if (buffer.size() < 4 + len)
                return false; // 数据不足，等待更多

            const QByteArray payload = buffer.mid(4, len);
            buffer.remove(0, 4 + len);

            QJsonParseError perr;
            const QJsonDocument doc = QJsonDocument::fromJson(payload, &perr);
            if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
                err = QString("JSON 解析失败: %1").arg(perr.errorString());
                return false;
            }
            out = doc.object();
            return true;
        }
        return false;
    }
};

#endif // JSONPROTOCOL_H
