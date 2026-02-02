#pragma once

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>     // DITAMBAHKAN
#include <QJsonValue>     // Optional, tapi baik untuk dittambah
#include <QTimer>
#include <QUrlQuery>
#include <QDebug>
#include <functional>
#include <map>

// Macro untuk kompatibilitas Qt version
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#define CONNECT_WEBSOCKET_ERROR(obj, receiver, slot) \
connect(obj, &QWebSocket::errorOccurred, receiver, slot)
#else
#define CONNECT_WEBSOCKET_ERROR(obj, receiver, slot) \
connect(obj, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), receiver, slot)
#endif

    class SocketIOClient : public QObject
{
    Q_OBJECT

public:
    enum class SocketIOVersion {
        V4,  // EIO=4
        V3,  // EIO=3
        V2   // EIO=3 (v2 juga pakai EIO=3)
    };

    explicit SocketIOClient(QObject *parent = nullptr);

    ~SocketIOClient();

    void connectToServer(const QString &host,
                         quint16 port = 3000,
                         const QString &ns = "/",
                         SocketIOVersion version = SocketIOVersion::V4);

    void disconnectFromServer();
    bool isConnected() const { return m_isConnected; }

    void emitEvent(const QString &eventName, const QJsonObject &data = QJsonObject());
    void emitEventWithAck(const QString &eventName,
                          const QJsonObject &data,
                          std::function<void(const QJsonObject&)> callback);

    // Robot commands
    void sendFallDetected();
    void sendDeviceReady(int brightness = 50, int volume = 10);
    void sendNoResponseFall(const QString &originalTimestamp);
    void sendScheduleEvent(const QString &scheduleType,
                           const QJsonObject &dateParams);

    // Utility
    QString getSocketId() const { return m_socketId; }
    QString getNamespace() const { return m_namespace; }
    QString getHost() const { return m_host; }
    quint16 getPort() const { return m_port; }

signals:
    void connected(const QString &socketId);
    void disconnected();
    void connectionError(const QString &error);
    void eventReceived(const QString &eventName, const QJsonObject &data);

    // Specific events dari frontend
    void screenBrightnessSet(int level);
    void volumeSet(int volume);
    void deviceOffRequested();
    void deviceResetRequested();
    void deviceRestartRequested();
    void pingDeviceUp();
    void helpEvent(const QString &timestamp);
    void okEvent(const QString &timestamp);
    void notOkEvent(const QString &timestamp);

private:
    QWebSocket *m_webSocket;
    QTimer *m_pingTimer;
    QTimer *m_reconnectTimer;
    QString m_socketId;
    QString m_namespace;
    bool m_isConnected;
    int m_packetId;
    int m_reconnectAttempts;
    bool namespaceConnected = false;

    QString m_host;
    quint16 m_port;
    SocketIOVersion m_version;

    std::map<int, std::function<void(const QJsonObject&)>> m_ackCallbacks;

    void setupWebSocket();
    void constructWebSocketUrl();
    void parseSocketIOMessage(const QString &message);
    void handleSocketIOPacket(int type, const QString &data);
    void sendSocketIOPacket(int type, const QString &data = QString());
    void handleIncomingEvent(const QString &eventName,
                             const QJsonObject &payload,
                             int ackId);
    void sendAcknowledgment(const QString &eventName,
                            int ackId,
                            const QJsonObject &originalData);
    void scheduleReconnect();

private slots:
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketTextMessage(const QString &message);
    void onWebSocketError(QAbstractSocket::SocketError error);
    void sendPing();
    void attemptReconnect();
};
