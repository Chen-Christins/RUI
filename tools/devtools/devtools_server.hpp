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

#include "utils/noncopyable.hpp"
#include "utils/singleton.hpp"

namespace rui {

// 简易 HTTP + WebSocket 服务器（单端口）
// - GET /       → 返回 DevTools 前端页面 (HTML)
// - GET /ws     → WebSocket 升级，建立 JSON-RPC 通道
class DevToolsServer : public QObject,
                       public Singleton<DevToolsServer>, Noncopyable {
public:
    using RpcHandler = std::function<QJsonObject(const QJsonObject& params)>;

    void start(quint16 port = 9222);
    void stop();
    bool isRunning() const;
    quint16 port() const;

    void broadcast(const QJsonObject& message);
    void registerMethod(const QString& method, RpcHandler handler);

    void setRootWidget(QWidget* widget) { rootWidget_ = widget; }
    QWidget* rootWidget() const { return rootWidget_; }

private:
    friend class Singleton<DevToolsServer>;
    DevToolsServer() = default;
    ~DevToolsServer() override = default;

    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

    void handleHttpRequest(QTcpSocket* socket, const QByteArray& request);
    void serveFrontend(QTcpSocket* socket);
    void sendHttpResponse(QTcpSocket* socket, int code, const QByteArray& body,
                          const QByteArray& contentType = "text/html; charset=utf-8");

    bool tryWebSocketUpgrade(QTcpSocket* socket, const QByteArray& request);
    void handleWebSocketFrame(QTcpSocket* socket, const QByteArray& frame);
    void sendWebSocketFrame(QTcpSocket* socket, const QByteArray& payload);
    QJsonObject handleRpcCall(const QJsonObject& request);

    static QByteArray encodeWebSocketFrame(const QByteArray& payload);
    static QByteArray computeAcceptKey(const QByteArray& key);

private:
    // QTcpServer 用于监听 DevTools 连接
    QTcpServer* server_ = nullptr;
    // 当前连接的根窗口指针，用于在 DevTools 中显示窗口层级结构
    QWidget* rootWidget_ = nullptr;
    // 当前连接的套接字及其状态信息
    struct ConnState {
        bool isWebSocket = false;
        QByteArray wsReadBuffer;
        QByteArray httpBuffer;
    };
    // 维护所有连接的状态信息，key 为 QTcpSocket*，value 为 ConnState
    std::map<QTcpSocket*, ConnState> connections_;
    // 注册的 JSON-RPC 方法及其处理函数
    std::map<QString, RpcHandler> handlers_;
};

} // namespace rui
