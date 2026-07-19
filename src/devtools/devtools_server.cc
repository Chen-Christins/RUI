// devtools_server.cc
#include "devtools_server.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QWidget>

#include <iostream>

namespace rui {

// ── 单例 ──────────────────────────────────────────────
DevToolsServer& DevToolsServer::instance() {
    static DevToolsServer s;
    return s;
}

// ── 启动 / 停止 ──────────────────────────────────────
void DevToolsServer::start(quint16 port) {
    if (server_) return;

    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, &DevToolsServer::onNewConnection);

    if (!server_->listen(QHostAddress::LocalHost, port)) {
        std::cerr << "[DevTools] Failed to listen on port " << port
                  << ": " << server_->errorString().toStdString() << std::endl;
        delete server_;
        server_ = nullptr;
        return;
    }

    std::cout << "[DevTools] http://localhost:" << port << std::endl;
    std::cout << "[DevTools] WebSocket endpoint: ws://localhost:" << port << "/ws" << std::endl;
}

void DevToolsServer::stop() {
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

bool DevToolsServer::isRunning() const {
    return server_ && server_->isListening();
}

quint16 DevToolsServer::port() const {
    return server_ ? server_->serverPort() : 0;
}

// ── 广播 ────────────────────────────────────────────
void DevToolsServer::broadcast(const QJsonObject& message) {
    QByteArray json = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray frame = encodeWebSocketFrame(json);

    for (auto& [socket, state] : connections_) {
        if (state.isWebSocket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(frame);
        }
    }
}

// ── RPC 注册 ─────────────────────────────────────────
void DevToolsServer::registerMethod(const QString& method, RpcHandler handler) {
    handlers_[method] = std::move(handler);
}

// ── 新连接 ───────────────────────────────────────────
void DevToolsServer::onNewConnection() {
    while (QTcpSocket* socket = server_->nextPendingConnection()) {
        connections_[socket] = ConnState{};
        connect(socket, &QTcpSocket::readyRead, this, &DevToolsServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &DevToolsServer::onDisconnected);
        std::cout << "[DevTools] New connection from "
                  << socket->peerAddress().toString().toStdString()
                  << ":" << socket->peerPort() << std::endl;
    }
}

// ── 数据就绪 ─────────────────────────────────────────
void DevToolsServer::onReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    auto& state = connections_[socket];

    if (state.isWebSocket) {
        // WebSocket 模式：累积帧数据
        QByteArray chunk = socket->readAll();
        std::cout << "[DevTools] WS read " << chunk.size() << " bytes, buffer was "
                  << state.wsReadBuffer.size() << " bytes" << std::endl;
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
                if (state.wsReadBuffer.size() < 4) return;
                payloadLen = (data[pos] << 8) | data[pos + 1];
                pos += 2;
            } else if (payloadLen == 127) {
                if (state.wsReadBuffer.size() < 10) return;
                payloadLen = 0;
                for (int i = 0; i < 8; i++) {
                    payloadLen = (payloadLen << 8) | data[pos + i];
                }
                pos += 8;
            }

            uint8_t maskKey[4] = {0, 0, 0, 0};
            if (masked) {
                if (state.wsReadBuffer.size() < pos + 4) return;
                memcpy(maskKey, data + pos, 4);
                pos += 4;
            }

            if (state.wsReadBuffer.size() < pos + static_cast<int>(payloadLen)) return;

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
                std::cout << "[DevTools] WS close frame received" << std::endl;
                uint16_t closeCode = 0;
                if (payload.size() >= 2) {
                    closeCode = (static_cast<uint8_t>(payload[0]) << 8) | static_cast<uint8_t>(payload[1]);
                }
                std::cout << "[DevTools] Close code: " << closeCode << std::endl;
                // 回应 close 帧
                QByteArray closeResp;
                closeResp.append(static_cast<char>(0x88)); // FIN + close opcode
                closeResp.append(static_cast<char>(0x00)); // no mask, 0 length
                socket->write(closeResp);
                socket->flush();
                socket->close();
            } else if (opcode == 0x9) {  // Ping frame
                std::cout << "[DevTools] WS ping, sending pong" << std::endl;
                QByteArray pong;
                pong.append(static_cast<char>(0x8A)); // FIN + pong opcode
                pong.append(static_cast<char>(payload.size()));
                pong.append(payload);
                socket->write(pong);
                socket->flush();
            } else if (opcode == 0x1 && fin) {  // Text frame (complete)
                handleWebSocketFrame(socket, payload);
            } else if (opcode == 0x1 && !fin) {
                std::cout << "[DevTools] WS fragmented text (not supported)" << std::endl;
            } else {
                std::cout << "[DevTools] WS unhandled opcode: " << (int)opcode << std::endl;
            }
        }
        if (frameCount > 0) {
            std::cout << "[DevTools] Processed " << frameCount << " WS frame(s), remaining buffer: "
                      << state.wsReadBuffer.size() << " bytes" << std::endl;
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

void DevToolsServer::onDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        connections_.erase(socket);
        socket->deleteLater();
    }
}

// ── HTTP 处理 ────────────────────────────────────────
void DevToolsServer::handleHttpRequest(QTcpSocket* socket, const QByteArray& request) {
    QString req = QString::fromUtf8(request);
    QStringList lines = req.split("\r\n");

    if (lines.isEmpty()) return;

    // 解析请求行: GET /path HTTP/1.1
    QStringList parts = lines[0].split(' ');
    if (parts.size() < 2) return;

    QString method = parts[0];
    QString path = parts[1];

    std::cout << "[DevTools] HTTP " << method.toStdString()
              << " " << path.toStdString() << std::endl;

    if (method == "GET" && path == "/ws") {
        std::cout << "[DevTools] WebSocket upgrade request" << std::endl;
        if (tryWebSocketUpgrade(socket, request)) {
            connections_[socket].isWebSocket = true;
            std::cout << "[DevTools] WebSocket upgrade SUCCESS" << std::endl;
        } else {
            std::cout << "[DevTools] WebSocket upgrade FAILED" << std::endl;
        }
    } else if (method == "GET" && (path == "/" || path == "/index.html")) {
        serveFrontend(socket);
    } else {
        sendHttpResponse(socket, 404, "Not Found", "text/plain");
    }
}

void DevToolsServer::serveFrontend(QTcpSocket* socket) {
    sendHttpResponse(socket, 200, QByteArray(frontendHtml()));
}

void DevToolsServer::sendHttpResponse(QTcpSocket* socket, int code, const QByteArray& body,
                                       const QByteArray& contentType) {
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

// ── WebSocket 升级 ───────────────────────────────────
bool DevToolsServer::tryWebSocketUpgrade(QTcpSocket* socket, const QByteArray& request) {
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

QByteArray DevToolsServer::computeAcceptKey(const QByteArray& key) {
    QByteArray magic = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    return QCryptographicHash::hash(magic, QCryptographicHash::Sha1).toBase64();
}

// ── WebSocket 帧处理 ─────────────────────────────────
void DevToolsServer::handleWebSocketFrame(QTcpSocket* socket, const QByteArray& payload) {
    std::cout << "[DevTools] WS frame payload(" << payload.size() << "): "
              << std::string(payload.constData(), std::min(payload.size(), qsizetype(200)))
              << std::endl;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError) {
        std::cout << "[DevTools] JSON parse error: " << err.errorString().toStdString() << std::endl;
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

void DevToolsServer::sendWebSocketFrame(QTcpSocket* socket, const QByteArray& payload) {
    socket->write(encodeWebSocketFrame(payload));
    socket->flush();
}

QByteArray DevToolsServer::encodeWebSocketFrame(const QByteArray& payload) {
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

// ── JSON-RPC 分发 ────────────────────────────────────
QJsonObject DevToolsServer::handleRpcCall(const QJsonObject& request) {
    QString method = request["method"].toString();
    QJsonObject params = request["params"].toObject();

    std::cout << "[DevTools] RPC: " << method.toStdString()
              << " (registered handlers: " << handlers_.size() << ")" << std::endl;

    auto it = handlers_.find(method);
    if (it != handlers_.end()) {
        QJsonObject result = it->second(params);
        std::cout << "[DevTools] RPC result keys: ";
        for (const auto& key : result.keys()) {
            std::cout << key.toStdString() << " ";
        }
        std::cout << std::endl;
        return result;
    }

    std::cout << "[DevTools] Unknown method: " << method.toStdString() << std::endl;
    QJsonObject error;
    error["error"] = "Unknown method: " + method;
    return error;
}

// ── 前端 HTML（嵌入为字符串常量）─────────────────────
const char* DevToolsServer::frontendHtml() {
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>RUI DevTools</title>
<style>
:root {
    --bg: #1e1e2e;
    --panel: #2b2b3d;
    --border: #3e3e56;
    --text: #cdd6f4;
    --text-secondary: #a6adc8;
    --accent: #89b4fa;
    --green: #a6e3a1;
    --orange: #fab387;
    --red: #f38ba8;
    --blue: #89b4fa;
    --font: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    --mono: 'SF Mono', 'Fira Code', 'Cascadia Code', monospace;
}
:root.light {
    --bg: #ffffff;
    --panel: #f5f5f5;
    --border: #e0e0e0;
    --text: #1e1e2e;
    --text-secondary: #6e6e7a;
    --accent: #2962ff;
    --green: #2e7d32;
    --orange: #e65100;
    --red: #c62828;
    --blue: #2962ff;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
    font-family: var(--font);
    background: var(--bg);
    color: var(--text);
    height: 100vh;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}
/* ── Toolbar ── */
#toolbar {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 8px 16px;
    background: var(--panel);
    border-bottom: 1px solid var(--border);
    flex-shrink: 0;
}
#toolbar h1 {
    font-size: 14px;
    font-weight: 600;
    color: var(--accent);
    margin-right: auto;
}
#toolbar .dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: var(--green);
}
#toolbar .dot.disconnected { background: var(--red); }
#toolbar button {
    padding: 4px 12px;
    border: 1px solid var(--border);
    border-radius: 4px;
    background: var(--bg);
    color: var(--text);
    font-size: 12px;
    cursor: pointer;
}
#toolbar button:hover { border-color: var(--accent); }
#toolbar button.active { background: var(--accent); color: var(--bg); border-color: var(--accent); }

/* ── Main Layout ── */
#main {
    display: flex;
    flex: 1;
    overflow: hidden;
}
.panel {
    border-right: 1px solid var(--border);
    overflow: auto;
}
.panel-header {
    padding: 8px 12px;
    font-size: 11px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: var(--text-secondary);
    background: var(--panel);
    border-bottom: 1px solid var(--border);
    position: sticky;
    top: 0;
    z-index: 1;
}

/* ── Tree Panel ── */
#tree-panel { width: 280px; flex-shrink: 0; }
.tree { padding: 4px 0; }
.tree-node { cursor: pointer; user-select: none; }
.tree-node-content {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 4px 8px;
    font-size: 12px;
    font-family: var(--mono);
    border-left: 2px solid transparent;
    border-radius: 0 3px 3px 0;
    margin: 1px 4px;
    transition: background 0.1s, border-color 0.1s;
}
.tree-node-content:hover {
    background: rgba(137,180,250,0.12);
    border-left: 2px solid rgba(137,180,250,0.4);
}
.tree-node-content.selected {
    background: rgba(137,180,250,0.18);
    border-left: 2px solid var(--accent);
}
.tree-node-content:active { background: rgba(137,180,250,0.25); }
.tree-toggle { width: 14px; font-size: 10px; color: var(--text-secondary); flex-shrink: 0; }
.tree-icon { font-size: 12px; flex-shrink: 0; }
.tree-label { white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.tree-type { font-size: 10px; color: var(--text-secondary); margin-left: auto; flex-shrink: 0; }
.tree-children { padding-left: 16px; }
.tree-children.collapsed { display: none; }
.w-type-Column .tree-icon, .w-type-Row .tree-icon { color: var(--orange); }
.w-type-QLabel .tree-icon { color: var(--blue); }
.w-type-QPushButton .tree-icon { color: var(--green); }

/* ── Detail Panel ── */
#detail-panel { flex: 1; }
.detail-grid {
    display: grid;
    grid-template-columns: 120px 1fr;
    gap: 0;
    font-size: 12px;
}
.detail-grid .key {
    padding: 4px 12px;
    color: var(--text-secondary);
    font-family: var(--mono);
    font-size: 11px;
    border-bottom: 1px solid rgba(255,255,255,0.03);
}
.detail-grid .val {
    padding: 4px 12px;
    font-family: var(--mono);
    border-bottom: 1px solid rgba(255,255,255,0.03);
}
.tab-btn {
    padding: 2px 10px;
    border: none;
    background: transparent;
    color: var(--text-secondary);
    font-size: 11px;
    cursor: pointer;
    border-radius: 3px;
    font-family: var(--font);
}
.tab-btn.active { background: var(--accent); color: var(--bg); }
.tab-btn:hover:not(.active) { color: var(--text); }
.json-view {
    font-family: var(--mono);
    font-size: 11px;
    white-space: pre;
    overflow: auto;
    max-height: calc(100vh - 120px);
    color: var(--text);
    background: rgba(0,0,0,0.2);
    border-radius: 4px;
    padding: 10px;
    line-height: 1.5;
}

/* ── State Panel ── */
#state-panel { width: 320px; flex-shrink: 0; border-right: none; }
.state-row {
    display: flex;
    align-items: center;
    padding: 6px 12px;
    font-size: 12px;
    border-bottom: 1px solid rgba(255,255,255,0.03);
    font-family: var(--mono);
}
.state-tag { color: var(--accent); min-width: 100px; }
.state-val { color: var(--green); margin-left: auto; }
.state-val.updated { animation: flash 0.4s; }
@keyframes flash {
    0%, 100% { background: transparent; }
    50% { background: rgba(166,227,161,0.2); }
}
.empty-state {
    padding: 24px;
    text-align: center;
    color: var(--text-secondary);
    font-size: 13px;
}
</style>
</head>
<body>

<div id="toolbar">
    <div id="status-dot" class="dot disconnected"></div>
    <h1>RUI DevTools</h1>
    <span id="status-text" style="font-size:11px;color:var(--text-secondary)">Connecting...</span>
    <button id="btn-theme" onclick="toggleTheme()" title="Toggle light/dark mode">☀</button>
    <button id="btn-overlay" onclick="toggleOverlay()">Layout</button>
    <button id="btn-refresh" onclick="refreshTree()">Refresh</button>
</div>

<div id="main">
    <!-- Component Tree -->
    <div id="tree-panel" class="panel">
        <div class="panel-header">Component Tree <span id="tree-status" style="font-weight:400;font-size:10px;color:var(--text-secondary)"></span></div>
        <div id="tree" class="tree"></div>
    </div>

    <!-- Detail -->
    <div id="detail-panel" class="panel">
        <div class="panel-header">
            <span style="display:inline-flex;gap:0;">
                <button id="tab-props" class="tab-btn active" onclick="switchTab('props')">Props</button>
                <button id="tab-json" class="tab-btn" onclick="switchTab('json')">JSON</button>
            </span>
        </div>
        <div id="detail-props" class="detail-grid">
            <div class="empty-state">👈 Click a component in the tree on the left</div>
        </div>
        <div id="detail-json" style="display:none;padding:8px;position:relative;">
            <button id="btn-copy-json" onclick="copyJson()" style="position:absolute;top:8px;right:16px;z-index:10;padding:2px 8px;font-size:10px;border:1px solid var(--border);border-radius:3px;background:var(--panel);color:var(--text);cursor:pointer;">Copy</button>
            <div id="detail-json-content">
                <div class="empty-state">👈 Click a component in the tree on the left</div>
            </div>
        </div>
    </div>

    <!-- Preview -->
    <div id="preview-panel" class="panel" style="width:360px;flex-shrink:0;border-right:none;">
        <div class="panel-header">App Preview <button onclick="refreshPreview()" style="float:right;font-size:10px;padding:1px 6px;border:1px solid var(--border);border-radius:3px;background:var(--bg);color:var(--text);cursor:pointer;">↻</button></div>
        <div id="preview" style="padding:12px;text-align:center;position:relative;display:inline-block;">
            <div class="empty-state">Loading preview...</div>
        </div>
    </div>

    <!-- State Viewer -->
    <div id="state-panel" class="panel">
        <div class="panel-header">States</div>
        <div id="states">
            <div class="empty-state">No states registered</div>
        </div>
    </div>
</div>

<script>
const WS_URL = `ws://${location.hostname}:${location.port}/ws`;
let ws = null;
let reqId = 0;
const callbacks = {};
let overlayEnabled = false;
let sendCount = 0;
let recvCount = 0;

function updateCounters() {
    document.getElementById('tree-status').textContent = '(' + sendCount + 's/' + recvCount + 'r)';
}

function setStatus(text, dotClass) {
    const el = document.getElementById('status-text');
    const dot = document.getElementById('status-dot');
    if (el) el.textContent = text;
    if (dot) { dot.className = 'dot'; if (dotClass) dot.classList.add(dotClass); }
}

// ── WebSocket ──
function connect() {
    setStatus('Connecting...', 'disconnected');
    try {
        ws = new WebSocket(WS_URL);
    } catch(e) {
        setStatus('Failed: ' + e.message, 'disconnected');
        document.getElementById('tree').innerHTML = '<div class="empty-state">❌ ' + e.message + '</div>';
        return;
    }
    ws.onopen = () => {
        setStatus('Connected', '');
        console.log('[DevTools] onopen, readyState=' + ws.readyState);
        // 先 ping 测试连通性，再加载数据
        pingThenLoad();
    };
    ws.onclose = (ev) => {
        setStatus('Closed code=' + ev.code + ' — retry in 2s', 'disconnected');
        console.log('[DevTools] onclose code=' + ev.code + ' reason=' + ev.reason);
        setTimeout(connect, 2000);
    };
    ws.onerror = (e) => {
        console.error('[DevTools] onerror', e);
    };
    ws.onmessage = (e) => {
        recvCount++;
        updateCounters();
        const msg = JSON.parse(e.data);
        if (msg.id !== undefined && callbacks[msg.id]) {
            if (msg.error) {
                console.warn('[DevTools] RPC error:', msg.id, msg.error);
                if (callbacks[msg.id].err) callbacks[msg.id].err(msg.error);
            } else {
                if (callbacks[msg.id].ok) callbacks[msg.id].ok(msg);
            }
            delete callbacks[msg.id];
        }
        if (msg.event === 'stateChange') {
            updateStateUI(msg.data.tag, msg.data.value);
        }
    };
}

function pingThenLoad() {
    rpc('getAllStates', {}, () => {
        console.log('[DevTools] ping OK, loading tree');
        loadTree();
        loadStates();
        refreshPreview();
    }, () => {
        console.warn('[DevTools] ping failed, retrying in 200ms');
        setTimeout(pingThenLoad, 200);
    });
}

// ── RPC ──
function rpc(method, params, okCb, errCb) {
    const id = ++reqId;
    callbacks[id] = { ok: okCb, err: errCb };
    const req = JSON.stringify({ id, method, params: params || {} });
    try {
        if (!ws) throw new Error('ws is null');
        if (ws.readyState !== WebSocket.OPEN) throw new Error('readyState=' + ws.readyState);
        console.log('[DevTools] SEND #' + id + ' ' + method);
        ws.send(req);
        sendCount++;
        updateCounters();
    } catch(e) {
        console.warn('[DevTools] SEND FAIL #' + id + ' ' + method + ': ' + e.message);
        if (errCb) errCb(e.message);
        else console.warn('[DevTools] No errCb for #' + id);
        delete callbacks[id];
    }
}

// ── Tree + Geometry Index ──
let widgetIndex = {};  // ptr → {type, name, x, y, w, h}
let rawTree = null;    // 完整的组件树 JSON（用于提取子树）

function buildIndex(node, parentX, parentY, isRoot) {
    parentX = parentX || 0;
    parentY = parentY || 0;
    if (!node || !node.ptr) return;
    // 根 widget 的屏幕坐标归零（预览图从窗口左上角开始）
    const localX = isRoot ? 0 : (node.x || 0);
    const localY = isRoot ? 0 : (node.y || 0);
    const absX = localX + parentX;
    const absY = localY + parentY;
    widgetIndex[node.ptr] = {
        type: node.type || '?',
        name: node.name || node.type || '?',
        x: absX, y: absY,
        w: node.width || 0, h: node.height || 0,
    };
    if (node.children) {
        for (const c of node.children) buildIndex(c, absX, absY, false);
    }
}

// 在 rawTree 中查找 ptr 对应的子树节点
function findSubtree(node, ptr) {
    if (!node) return null;
    if (node.ptr === ptr) return node;
    if (node.children) {
        for (const c of node.children) {
            const found = findSubtree(c, ptr);
            if (found) return found;
        }
    }
    return null;
}

function loadTree() {
    const treeEl = document.getElementById('tree');
    treeEl.innerHTML = '<div class="empty-state">⏳ Loading component tree...</div>';
    rpc('getComponentTree', {}, (resp) => {
        console.log('[DevTools] getComponentTree OK, keys:', Object.keys(resp));
        if (!resp || !resp.tree || !resp.tree.type) {
            treeEl.innerHTML = '<div class="empty-state">⚠ Empty tree. resp keys: ' + (resp ? Object.keys(resp).join(',') : 'null') + '</div>';
            return;
        }
        rawTree = resp.tree;
        widgetIndex = {};
        buildIndex(resp.tree, 0, 0, true);
        treeEl.innerHTML = renderTree(resp.tree);
        const first = treeEl.querySelector('.tree-node-content');
        if (first) selectNode(first, first.getAttribute('data-ptr'));
    }, (err) => {
        treeEl.innerHTML = '<div class="empty-state">⚠ RPC failed: ' + err + '</div>';
    });
}

// ── Tab switching ──
function switchTab(tab) {
    document.getElementById('tab-props').classList.toggle('active', tab === 'props');
    document.getElementById('tab-json').classList.toggle('active', tab === 'json');
    document.getElementById('detail-props').style.display = tab === 'props' ? '' : 'none';
    document.getElementById('detail-json').style.display = tab === 'json' ? '' : 'none';
}

function loadStates() {
    rpc('getAllStates', {}, (resp) => {
        renderStates(resp.states || {});
    });
    // Poll states periodically
    clearInterval(window._statePoll);
    window._statePoll = setInterval(() => rpc('getAllStates', {}, (resp) => {
        renderStates(resp.states || {});
    }), 3000);
}

function renderTree(node) {
    if (!node) return '';
    const hasChildren = node.children && node.children.length > 0;
    const typeClass = 'w-type-' + (node.type || '').replace('rui::', '');
    let html = '<div class="tree-node ' + typeClass + '">';
    html += '<div class="tree-node-content" data-ptr="' + (node.ptr || '') + '">';
    html += '<span class="tree-toggle">' + (hasChildren ? '▼' : '') + '</span>';
    html += '<span class="tree-icon">' + getIcon(node.type) + '</span>';
    html += '<span class="tree-label">' + (node.name || node.type) + '</span>';
    html += '<span class="tree-type">' + (node.type || '') + '</span>';
    html += '</div>';
    if (hasChildren) {
        html += '<div class="tree-children">';
        for (const child of node.children) html += renderTree(child);
        html += '</div>';
    }
    html += '</div>';
    return html;
}

function getIcon(type) {
    if (!type) return '⬡';
    if (type.includes('Label') || type === 'Text') return '📝';
    if (type.includes('Button')) return '🔘';
    if (type.includes('Column')) return '📋';
    if (type.includes('Row')) return '📊';
    return '⬡';
}

// ── Event delegation for tree clicks ──
document.getElementById('tree').addEventListener('click', (e) => {
    // Toggle collapse
    if (e.target.classList.contains('tree-toggle')) {
        const children = e.target.closest('.tree-node').querySelector(':scope > .tree-children');
        if (children) {
            children.classList.toggle('collapsed');
            e.target.textContent = children.classList.contains('collapsed') ? '▶' : '▼';
        }
        return;
    }
    // Select node
    const content = e.target.closest('.tree-node-content');
    if (content) {
        selectNode(content, content.getAttribute('data-ptr'));
    }
});

let selectedPtr = null;

function selectNode(el, ptr) {
    selectedPtr = ptr;
    document.querySelectorAll('.tree-node-content.selected').forEach(n => n.classList.remove('selected'));
    el.classList.add('selected');
    // 加载属性面板
    rpc('getWidgetDetail', {ptr}, (resp) => { renderDetail(resp); });
    // 加载 JSON 面板
    renderJson(ptr);
    // 在 web 预览图上绘制高亮
    if (ptr) drawHighlight(ptr);
}

function renderDetail(detail) {
    let html = '';
    if (!detail || !detail.properties) {
        html = '<div class="empty-state">👈 Click a component in the tree on the left</div>';
    } else {
        for (const [key, val] of Object.entries(detail.properties)) {
            html += '<div class="key">' + key + '</div>';
            html += '<div class="val">' + val + '</div>';
        }
    }
    document.getElementById('detail-props').innerHTML = html;
}

function renderJson(ptr) {
    const el = document.getElementById('detail-json-content');
    if (!ptr || !rawTree) {
        el.innerHTML = '<div class="empty-state">👈 Click a component</div>';
        return;
    }
    const subtree = findSubtree(rawTree, ptr);
    if (!subtree) {
        el.innerHTML = '<div class="empty-state">Not found</div>';
        return;
    }
    const jsonText = JSON.stringify(subtree, null, 2);
    el.innerHTML = '<pre class="json-view">' + escapeHtml(jsonText) + '</pre>';
}

function escapeHtml(text) {
    return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function copyJson() {
    if (!selectedPtr || !rawTree) return;
    const subtree = findSubtree(rawTree, selectedPtr);
    if (!subtree) return;
    const text = JSON.stringify(subtree, null, 2);
    navigator.clipboard.writeText(text).then(() => {
        const btn = document.getElementById('btn-copy-json');
        btn.textContent = 'Copied!';
        setTimeout(() => { btn.textContent = 'Copy'; }, 1500);
    }).catch(() => {
        // fallback for older browsers
        const ta = document.createElement('textarea');
        ta.value = text;
        ta.style.position = 'fixed'; ta.style.opacity = '0';
        document.body.appendChild(ta);
        ta.select();
        document.execCommand('copy');
        document.body.removeChild(ta);
        const btn = document.getElementById('btn-copy-json');
        btn.textContent = 'Copied!';
        setTimeout(() => { btn.textContent = 'Copy'; }, 1500);
    });
}

function renderStates(states) {
    const el = document.getElementById('states');
    const entries = Object.entries(states);
    if (entries.length === 0) {
        el.innerHTML = '<div class="empty-state">No states registered</div>';
        return;
    }
    let html = '';
    for (const [tag, val] of entries) {
        html += '<div class="state-row" id="state-' + tag + '">';
        html += '<span class="state-tag">' + tag + '</span>';
        html += '<span class="state-val">' + val + '</span>';
        html += '</div>';
    }
    el.innerHTML = html;
}

function updateStateUI(tag, value) {
    const existing = document.getElementById('state-' + tag);
    if (existing) {
        const valEl = existing.querySelector('.state-val');
        valEl.textContent = value;
        valEl.classList.remove('updated');
        void valEl.offsetWidth;
        valEl.classList.add('updated');
    } else {
        rpc('getAllStates', {}, (resp) => { renderStates(resp.states || {}); });
    }
}

function toggleOverlay() {
    overlayEnabled = !overlayEnabled;
    rpc('toggleLayoutOverlay', {enabled: overlayEnabled});
    document.getElementById('btn-overlay').classList.toggle('active', overlayEnabled);
}

// ── Preview ──
let previewScale = 1;
let previewOffsetX = 0, previewOffsetY = 0;

function refreshPreview() {
    rpc('getScreenshot', {}, (resp) => {
        const panel = document.getElementById('preview');
        if (resp.image) {
            // 用逻辑像素做缩放基准，物理像素的图用 max-width 自动缩放
            const data = JSON.stringify({lw:resp.logWidth, lh:resp.logHeight});
            panel.innerHTML =
                '<div id="preview-container" style="position:relative;display:inline-block;line-height:0;">' +
                '<img id="preview-img" src="data:image/png;base64,' + resp.image + '" ' +
                'style="max-width:300px;border-radius:4px;box-shadow:0 2px 8px rgba(0,0,0,0.3);" ' +
                'onload="onPreviewLoaded(' + resp.logWidth + ',' + resp.logHeight + ')" ' +
                'alt="App screenshot">' +
                '<div id="preview-overlay" style="position:absolute;top:0;left:0;width:100%;height:100%;pointer-events:none;z-index:2;"></div>' +
                '</div>';
        }
    }, () => {
        document.getElementById('preview').innerHTML = '<div class="empty-state">Preview unavailable</div>';
    });
}

function onPreviewLoaded(logW, logH) {
    const img = document.getElementById('preview-img');
    if (!img) return;
    // 缩放比 = 图片实际显示宽度 / 窗口逻辑宽度
    previewScale = img.clientWidth / logW;
    // 重新绘制选中高亮（可能树先加载并选中了节点）
    const sel = document.querySelector('.tree-node-content.selected');
    if (sel) drawHighlight(sel.getAttribute('data-ptr'));
}

function drawHighlight(ptr) {
    console.log('[DevTools] drawHighlight', ptr, 'previewScale=' + previewScale);
    const overlay = document.getElementById('preview-overlay');
    if (!overlay) { console.log('[DevTools] drawHighlight: no overlay element'); return; }
    const info = widgetIndex[ptr];
    if (!info) {
        console.log('[DevTools] drawHighlight: ptr not in index. Keys:', Object.keys(widgetIndex).slice(0,5));
        overlay.innerHTML = '';
        return;
    }
    console.log('[DevTools] drawHighlight info:', info);

    const left = info.x * previewScale;
    const top = info.y * previewScale;
    const w = Math.max(info.w * previewScale, 2);
    const h = Math.max(info.h * previewScale, 2);
    console.log('[DevTools] highlight rect:', left, top, w, h);

    const labelText = info.name + '\n' + info.w + '×' + info.h + ' @ (' + info.x + ',' + info.y + ')';

    overlay.innerHTML =
        '<div style="position:absolute;left:' + left + 'px;top:' + top + 'px;' +
        'width:' + w + 'px;height:' + h + 'px;' +
        'outline:2px solid #89b4fa;outline-offset:-1px;' +
        'background:rgba(137,180,250,0.15);z-index:3;' +
        'pointer-events:none;"></div>' +
        '<div style="position:absolute;left:' + left + 'px;top:' + (top + h + 4) + 'px;' +
        'background:var(--bg);color:var(--text);font-size:10px;font-family:SF Mono,monospace;' +
        'padding:2px 6px;border-radius:3px;white-space:pre;line-height:1.4;' +
        'border:1px solid #89b4fa;pointer-events:none;z-index:10;' +
        'box-shadow:0 2px 6px rgba(0,0,0,0.5);">' + labelText + '</div>';
}

// ── Theme ──
function applyTheme(light) {
    document.documentElement.classList.toggle('light', light);
    document.getElementById('btn-theme').textContent = light ? '☾' : '☀';
    localStorage.setItem('rui-devtools-theme', light ? 'light' : 'dark');
}
function toggleTheme() {
    applyTheme(!document.documentElement.classList.contains('light'));
}
// 启动时从 localStorage 或系统偏好读取
(function initTheme() {
    const saved = localStorage.getItem('rui-devtools-theme');
    if (saved) { applyTheme(saved === 'light'); return; }
    if (window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches) {
        applyTheme(true);
    }
})();

// ── Start ──
connect();
</script>
</body>
</html>
)HTML";
}

} // namespace rui
