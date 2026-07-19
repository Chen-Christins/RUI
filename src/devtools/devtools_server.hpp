// devtools_server.hpp
#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QWidget>

#include <functional>
#include <map>

namespace rui {

// 简易 HTTP + WebSocket 服务器（单端口）
// - GET /       → 返回 DevTools 前端页面 (HTML)
// - GET /ws     → WebSocket 升级，建立 JSON-RPC 通道
class DevToolsServer : public QObject {
public:
    static DevToolsServer& instance();

    void start(quint16 port = 9222);
    void stop();
    bool isRunning() const;
    quint16 port() const;

    // 向所有 WebSocket 客户端广播事件
    void broadcast(const QJsonObject& message);

    // 注册 RPC 方法处理器
    using RpcHandler = std::function<QJsonObject(const QJsonObject& params)>;
    void registerMethod(const QString& method, RpcHandler handler);

    // 设置被检查的根 widget
    void setRootWidget(QWidget* widget) { rootWidget_ = widget; }
    QWidget* rootWidget() const { return rootWidget_; }

private:
    DevToolsServer() = default;
    ~DevToolsServer() override = default;

    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

    // HTTP 处理
    void handleHttpRequest(QTcpSocket* socket, const QByteArray& request);
    void serveFrontend(QTcpSocket* socket);
    void sendHttpResponse(QTcpSocket* socket, int code, const QByteArray& body,
                          const QByteArray& contentType = "text/html; charset=utf-8");

    // WebSocket 处理
    bool tryWebSocketUpgrade(QTcpSocket* socket, const QByteArray& request);
    void handleWebSocketFrame(QTcpSocket* socket, const QByteArray& frame);
    void sendWebSocketFrame(QTcpSocket* socket, const QByteArray& payload);
    QJsonObject handleRpcCall(const QJsonObject& request);

    // WebSocket 帧工具
    static QByteArray encodeWebSocketFrame(const QByteArray& payload);
    static QByteArray computeAcceptKey(const QByteArray& key);

private:
    QTcpServer* server_ = nullptr;
    QWidget* rootWidget_ = nullptr;

    // 每个连接的 WS 读缓冲区
    struct ConnState {
        bool isWebSocket = false;
        QByteArray wsReadBuffer;
        QByteArray httpBuffer;
    };
    std::map<QTcpSocket*, ConnState> connections_;

    std::map<QString, RpcHandler> handlers_;

    static const char* frontendHtml();
};

} // namespace rui
