// devtools_server.cc
#include "devtools_server.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QWidget>

#include "utils/logger.hpp"

#include "config/config.hpp"

namespace rui {

namespace {

QString normalizePathForCompare(const QString& path) {
    QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(path));
    while (normalized.size() > 3 && normalized.endsWith('/')) {
        normalized.chop(1);
    }
    return normalized.toCaseFolded();
}

bool isPathUnderWorkpath(const QString& workpath, const QString& path) {
    const QString base = normalizePathForCompare(workpath);
    const QString target = normalizePathForCompare(path);
    return target == base || target.startsWith(base + "/");
}

QString resolveConfigPath(const QString& workpath, const QString& path) {
    QString resolved;
    if (QDir::isAbsolutePath(path)) {
        resolved = QDir::cleanPath(path);
    } else {
        resolved = QDir::cleanPath(workpath + QDir::separator() + path);
    }

    if (!isPathUnderWorkpath(workpath, resolved)) {
        return {};
    }
    return resolved;
}

bool isHtmlFile(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "html" || suffix == "htm";
}

} // namespace

static Logger::ptr logger = LOG_NAME("DevTools");

static ConfigVar<std::string>::ptr gWorkpath =
    ConfigVar<std::string>::Lookup("workpath", {}, "Application work path");
static ConfigVar<std::string>::ptr gFrontendPath =
    ConfigVar<std::string>::Lookup("devtools.frontend", "frontend.html", "DevTools frontend HTML path");

void DevToolsServerManager::start(quint16 port) {
    if (server_) {
        return;
    }

    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, &DevToolsServerManager::onNewConnection);

    if (!server_->listen(QHostAddress::LocalHost, port)) {
        ERROR(logger) << "Failed to listen on port " << port
                      << ": " << server_->errorString();
        delete server_;
        server_ = nullptr;
        return;
    }

    INFO(logger) << "http://localhost:" << port;
    INFO(logger) << "WebSocket endpoint: ws://localhost:" << port << "/ws";
}

void DevToolsServerManager::stop() {
    // 关闭所有连接
    for (auto& [socket, state] : connections_) {
        socket->close();
        socket->deleteLater();
    }
    connections_.clear();

    if (server_) {
        server_->close();
        delete server_;
        server_ = nullptr;
    }
}

bool DevToolsServerManager::isRunning() const {
    return server_ && server_->isListening();
}

quint16 DevToolsServerManager::port() const {
    return server_ ? server_->serverPort() : 0;
}

void DevToolsServerManager::broadcast(const QJsonObject& message) {
    QByteArray json = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray frame = encodeWebSocketFrame(json);

    for (auto& [socket, state] : connections_) {
        if (state.isWebSocket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(frame);
        }
    }
}

void DevToolsServerManager::registerMethod(const QString& method, RpcHandler handler) {
    handlers_[method] = std::move(handler);
}

void DevToolsServerManager::setRootWidget(QWidget* widget) {
    rootWidget_ = widget;
}

QWidget* DevToolsServerManager::rootWidget() const {
    return rootWidget_;
}

void DevToolsServerManager::onNewConnection() {
    while (QTcpSocket* socket = server_->nextPendingConnection()) {
        connections_[socket] = ConnState{};
        connect(socket, &QTcpSocket::readyRead, this, &DevToolsServerManager::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &DevToolsServerManager::onDisconnected);
        INFO(logger) << "New connection from "
                      << socket->peerAddress().toString()
                      << ":" << socket->peerPort();
    }
}

void DevToolsServerManager::onReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    auto& state = connections_[socket];

    if (state.isWebSocket) {
        // WebSocket 模式：累积帧数据
        QByteArray chunk = socket->readAll();
        DEBUG(logger) << "WS read " << chunk.size() << " bytes, buffer was "
                  << state.wsReadBuffer.size() << " bytes";
        state.wsReadBuffer.append(chunk);
        int frameCount = 0;
        while (state.wsReadBuffer.size() >= 2) {
            // 解析 WebSocket 帧
            const uint8_t* data = reinterpret_cast<const uint8_t*>(state.wsReadBuffer.constData());
            int pos = 0;

            uint8_t byte0 = data[pos++];
            bool fin = byte0 & 0x80;
            uint8_t opcode = byte0 & 0x0F;

            uint8_t byte1 = data[pos++];
            bool masked = byte1 & 0x80;
            uint64_t payloadLen = byte1 & 0x7F;

            if (payloadLen == 126) {
                if (state.wsReadBuffer.size() < 4) {
                    return;
                }
                payloadLen = (data[pos] << 8) | data[pos + 1];
                pos += 2;
            } else if (payloadLen == 127) {
                if (state.wsReadBuffer.size() < 10) {
                    return;
                }
                payloadLen = 0;
                for (int i = 0; i < 8; i++) {
                    payloadLen = (payloadLen << 8) | data[pos + i];
                }
                pos += 8;
            }

            uint8_t maskKey[4] = {0, 0, 0, 0};
            if (masked) {
                if (state.wsReadBuffer.size() < pos + 4) {
                    return;
                }
                memcpy(maskKey, data + pos, 4);
                pos += 4;
            }

            if (state.wsReadBuffer.size() < pos + static_cast<int>(payloadLen)) {
                return;
            }

            // 提取并解码 payload
            QByteArray payload;
            payload.resize(static_cast<int>(payloadLen));
            for (uint64_t i = 0; i < payloadLen; i++) {
                payload[static_cast<int>(i)] = data[pos + i] ^ maskKey[i % 4];
            }
            pos += static_cast<int>(payloadLen);

            // 移除已处理的数据
            state.wsReadBuffer.remove(0, pos);
            frameCount++;

            // 根据 opcode 处理不同帧类型
            if (opcode == 0x8) {  // Close frame
                DEBUG(logger) << "WS close frame received";
                uint16_t closeCode = 0;
                if (payload.size() >= 2) {
                    closeCode = (static_cast<uint8_t>(payload[0]) << 8) | static_cast<uint8_t>(payload[1]);
                }
                DEBUG(logger) << "Close code: " << closeCode;
                // 回应 close 帧
                QByteArray closeResp;
                closeResp.append(static_cast<char>(0x88)); // FIN + close opcode
                closeResp.append(static_cast<char>(0x00)); // no mask, 0 length
                socket->write(closeResp);
                socket->flush();
                socket->close();
            } else if (opcode == 0x9) {  // Ping frame
                DEBUG(logger) << "WS ping, sending pong";
                QByteArray pong;
                pong.append(static_cast<char>(0x8A)); // FIN + pong opcode
                pong.append(static_cast<char>(payload.size()));
                pong.append(payload);
                socket->write(pong);
                socket->flush();
            } else if (opcode == 0x1 && fin) {  // Text frame (complete)
                handleWebSocketFrame(socket, payload);
            } else if (opcode == 0x1 && !fin) {
                WARN(logger) << "WS fragmented text (not supported)";
            } else {
                WARN(logger) << "WS unhandled opcode: " << (int)opcode;
            }
        }
        if (frameCount > 0) {
            DEBUG(logger) << "Processed " << frameCount << " WS frame(s), remaining buffer: "
                      << state.wsReadBuffer.size() << " bytes";
        }
    } else {
        // HTTP 模式：读取请求头
        state.httpBuffer.append(socket->readAll());
        if (state.httpBuffer.contains("\r\n\r\n")) {
            handleHttpRequest(socket, state.httpBuffer);
            state.httpBuffer.clear();
        }
    }
}

void DevToolsServerManager::onDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        connections_.erase(socket);
        socket->deleteLater();
    }
}

void DevToolsServerManager::handleHttpRequest(QTcpSocket* socket, const QByteArray& request) {
    QString req = QString::fromUtf8(request);
    QStringList lines = req.split("\r\n");

    if (lines.isEmpty()) {
        return;
    }

    // 解析请求行: GET /path HTTP/1.1
    QStringList parts = lines[0].split(' ');
    if (parts.size() < 2) {
        return;
    }

    QString method = parts[0];
    QString path = parts[1];

    DEBUG(logger) << "HTTP " << method << " " << path;

    if (method == "GET" && path == "/ws") {
        DEBUG(logger) << "WebSocket upgrade request";
        if (tryWebSocketUpgrade(socket, request)) {
            connections_[socket].isWebSocket = true;
            INFO(logger) << "WebSocket upgrade SUCCESS";
        } else {
            WARN(logger) << "WebSocket upgrade FAILED";
        }
    } else if (method == "GET" && (path == "/" || path == "/index.html")) {
        serveFrontend(socket);
    } else {
        sendHttpResponse(socket, 404, "Not Found", "text/plain");
    }
}

void DevToolsServerManager::serveFrontend(QTcpSocket* socket) {
    QByteArray html;
    QString path = resolveConfigPath(
        QString::fromStdString(gWorkpath->get()),
        QString::fromStdString(gFrontendPath->get())
    );
    if (path.isEmpty()) {
        WARN(logger) << "Ignoring devtools.frontend path outside workpath: "
                     << QString::fromStdString(gFrontendPath->get());
    } else if (!isHtmlFile(path)) {
        WARN(logger) << "Ignoring non-html devtools.frontend path: " << path;
    } else {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            html = file.readAll();
        }
    }
    if (html.isEmpty()) {
        html = "<html><body><h1>frontend.html not found</h1>"
               "<p>Check &lt;frontend&gt; path in config.xml</p></body></html>";
    }
    sendHttpResponse(socket, 200, html);
}

void DevToolsServerManager::sendHttpResponse(QTcpSocket* socket, int code, const QByteArray& body, const QByteArray& contentType) {
    QByteArray response;
    response += "HTTP/1.1 " + QByteArray::number(code) + " OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "\r\n";
    response += body;
    socket->write(response);
    socket->flush();
}

bool DevToolsServerManager::tryWebSocketUpgrade(QTcpSocket* socket, const QByteArray& request) {
    QString req = QString::fromUtf8(request);
    QByteArray key;

    for (const QString& line : req.split("\r\n")) {
        if (line.startsWith("Sec-WebSocket-Key:", Qt::CaseInsensitive)) {
            key = line.mid(line.indexOf(':') + 1).trimmed().toUtf8();
            break;
        }
    }

    if (key.isEmpty()) {
        sendHttpResponse(socket, 400, "Missing Sec-WebSocket-Key", "text/plain");
        return false;
    }

    QByteArray acceptKey = computeAcceptKey(key);

    QByteArray response;
    response += "HTTP/1.1 101 Switching Protocols\r\n";
    response += "Upgrade: websocket\r\n";
    response += "Connection: Upgrade\r\n";
    response += "Sec-WebSocket-Accept: " + acceptKey + "\r\n";
    response += "\r\n";

    socket->write(response);
    socket->flush();
    return true;
}

QByteArray DevToolsServerManager::computeAcceptKey(const QByteArray& key) {
    QByteArray magic = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    return QCryptographicHash::hash(magic, QCryptographicHash::Sha1).toBase64();
}

void DevToolsServerManager::handleWebSocketFrame(QTcpSocket* socket, const QByteArray& payload) {
    DEBUG(logger) << "WS frame payload(" << payload.size() << "): "
                  << QString::fromUtf8(payload.left(200));

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError) {
        ERROR(logger) << "JSON parse error: " << err.errorString();
        return;
    }

    QJsonObject request = doc.object();
    QJsonObject response = handleRpcCall(request);

    // 响应包含请求 id
    if (request.contains("id")) {
        response["id"] = request["id"];
    }

    QByteArray respJson = QJsonDocument(response).toJson(QJsonDocument::Compact);
    sendWebSocketFrame(socket, respJson);
}

void DevToolsServerManager::sendWebSocketFrame(QTcpSocket* socket, const QByteArray& payload) {
    socket->write(encodeWebSocketFrame(payload));
    socket->flush();
}

QByteArray DevToolsServerManager::encodeWebSocketFrame(const QByteArray& payload) {
    QByteArray frame;
    frame.append(static_cast<char>(0x81)); // FIN + text opcode

    if (payload.size() < 126) {
        frame.append(static_cast<char>(payload.size()));
    } else if (payload.size() <= 0xFFFF) {
        frame.append(static_cast<char>(126));
        frame.append(static_cast<char>((payload.size() >> 8) & 0xFF));
        frame.append(static_cast<char>(payload.size() & 0xFF));
    } else {
        frame.append(static_cast<char>(127));
        for (int i = 7; i >= 0; i--) {
            frame.append(static_cast<char>((payload.size() >> (i * 8)) & 0xFF));
        }
    }

    frame.append(payload);
    return frame;
}

QJsonObject DevToolsServerManager::handleRpcCall(const QJsonObject& request) {
    QString method = request["method"].toString();
    QJsonObject params = request["params"].toObject();

    DEBUG(logger) << "RPC: " << method
                  << " (registered handlers: " << handlers_.size() << ")";

    auto it = handlers_.find(method);
    if (it != handlers_.end()) {
        QJsonObject result = it->second(params);
        QStringList keys = result.keys();
        DEBUG(logger) << "RPC result keys: " << keys.join(" ");
        return result;
    }

    WARN(logger) << "Unknown method: " << method;
    QJsonObject error;
    error["error"] = "Unknown method: " + method;
    return error;
}


} // namespace rui
