#include "socketioclient.h"
#include <QUrlQuery>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
//#include <cmath>
//#include <iostream>
#include <QRandomGenerator>  // <-- TAMBAHKAN INI

SocketIOClient::SocketIOClient(QObject *parent)
    : QObject(parent)
    , m_webSocket(nullptr)
    , m_pingTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_isConnected(false)
    , m_packetId(0)
    , m_reconnectAttempts(0)
    , m_port(3000)
    , m_version(SocketIOVersion::V4)
{
    //m_webSocket = new QWebSocket();

    m_pingTimer->setInterval(25000);
    connect(m_pingTimer, &QTimer::timeout, this, &SocketIOClient::sendPing);

    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &SocketIOClient::attemptReconnect);
}

//------------------------------------------------------------------------
SocketIOClient::~SocketIOClient()
{
    disconnectFromServer();
}

//------------------------------------------------------------------------
void SocketIOClient::connectToServer(const QString &host,
                                     quint16 port,
                                     const QString &ns,
                                     SocketIOVersion version)
{
    m_host = host;
    m_port = port;
    m_namespace = ns;
    m_version = version;

    if (m_webSocket) {
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    setupWebSocket();
    constructWebSocketUrl();
}

//------------------------------------------------------------------------
void SocketIOClient::disconnectFromServer()
{
    if (m_webSocket) {
        m_webSocket->close();
    }
    m_isConnected = false;
    m_pingTimer->stop();
    m_reconnectTimer->stop();
}

//------------------------------------------------------------------------
void SocketIOClient::setupWebSocket()
{
    //m_webSocket = new QWebSocket();

    m_webSocket = new QWebSocket(QString(),
                      QWebSocketProtocol::VersionLatest,
                      this);

    connect(m_webSocket, &QWebSocket::connected,
            this, &SocketIOClient::onWebSocketConnected);
    connect(m_webSocket, &QWebSocket::disconnected,
            this, &SocketIOClient::onWebSocketDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &SocketIOClient::onWebSocketTextMessage);

    // Gunakan macro untuk kompatibilitas
    CONNECT_WEBSOCKET_ERROR(m_webSocket, this, &SocketIOClient::onWebSocketError);
}

//------------------------------------------------------------------------
void SocketIOClient::constructWebSocketUrl()
{
    QUrl url;
    url.setScheme("ws");
    url.setHost(m_host);
    url.setPort(m_port);
    url.setPath("/socket.io/");

    QUrlQuery query;

    // Parameter EIO berdasarkan versi
    switch (m_version) {
    case SocketIOVersion::V4:
        query.addQueryItem("EIO", "4");
        break;
    case SocketIOVersion::V3:
        query.addQueryItem("EIO", "3");
        break;
    case SocketIOVersion::V2:
        query.addQueryItem("EIO", "3"); // v2 juga pakai EIO=3
        break;
    }

    query.addQueryItem("transport", "websocket");

    // Tambahkan timestamp untuk avoid cache
    query.addQueryItem("t", QString::number(QDateTime::currentMSecsSinceEpoch()));

    // Tambahkan namespace ke query jika bukan root namespace
    if (m_namespace != "/") {
        query.addQueryItem("namespace", m_namespace);
    }

    url.setQuery(query);

    QString urlString = url.toString();
    qDebug() << "Connecting to:" << urlString;
    qDebug() << "Namespace:" << m_namespace;

    m_webSocket->open(url);
}

//------------------------------------------------------------------------
void SocketIOClient::onWebSocketConnected()
{
    qDebug() << "WebSocket connected, waiting for Socket.IO handshake...";
    m_webSocket->sendTextMessage("40");
    qDebug() << "Sent namespaces";
}

//------------------------------------------------------------------------
void SocketIOClient::onWebSocketDisconnected()
{
    m_isConnected = false;
    m_pingTimer->stop();
    qDebug() << "WebSocket disconnected";

    scheduleReconnect();

    emit disconnected();
}

//------------------------------------------------------------------------
void SocketIOClient::onWebSocketTextMessage(const QString &message)
{
    qDebug() << "Received message:" << message;

    parseSocketIOMessage(message);
}

//------------------------------------------------------------------------
void SocketIOClient::parseSocketIOMessage(const QString &message)
{
    if (message.isEmpty()) {
        qDebug() << "Empty message received";
        return;
    }

    bool ok;
    int packetType = QString(message[0]).toInt(&ok);
    if (!ok) {
        qWarning() << "Invalid packet type:" << message;
        return;
    }

    QString data = message.mid(1);
    handleSocketIOJsonPacket(packetType, data);
}

//------------------------------------------------------------------------
void SocketIOClient::handleSocketIOJsonPacket(int type, const QString &data)
{
    switch (type) {
    case 0: { // CONNECT
        if (!data.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                m_socketId = obj["sid"].toString();
                m_isConnected = true;
                m_reconnectAttempts = 0;

                qDebug() << "Socket.IO connected! SID:" << m_socketId;
                qDebug() << "Ping interval:" << obj["pingInterval"].toInt()
                         << "Ping timeout:" << obj["pingTimeout"].toInt();

                // Start ping timer
                int pingInterval = obj["pingInterval"].toInt(25000);
                m_pingTimer->start(pingInterval);

                emit connected(m_socketId);

                // Kirim device ready event setelah connect
                QTimer::singleShot(1000, this, [this]() {
                    sendDeviceReady();
                });
            }
        }
        break;
    }

    case 1: // DISCONNECT
        qDebug() << "Server requested disconnect";
        if (m_webSocket) {
            m_webSocket->close();
        }
        break;

    case 2: // PING
        sendSocketIOPacket(3); // PONG
        qDebug() << "Received PING, sent PONG";
        break;

    case 3: // PONG
        qDebug() << "Received PONG";
        break;

    case 4: { // MESSAGE (Engine.IO)
        if (data.isEmpty()) break;

        // Data seharusnya diawali dengan subtype Socket.IO, misalnya:
        // 2["EVENT","DATA"]
        // 3[ackId,"RESPONSE"]

        QChar subTypeChar = data[0];
        QString subData = data.mid(1);

        int subType = subTypeChar.digitValue();

        if (subType == 2) { // EVENT
            QJsonDocument doc = QJsonDocument::fromJson(subData.toUtf8());
            if (!doc.isArray()) {
                qWarning() << "Invalid event JSON:" << subData;
                return;
            }

            QJsonArray arr = doc.array();
            if (arr.size() < 2) {
                qWarning() << "Invalid event format:" << arr;
                return;
            }

            QString eventName = arr[0].toString();
            QJsonValue dataValue = arr[1];

            int ackId = -1;
            if (arr.size() >= 3 && arr[2].isDouble()) {
                ackId = arr[2].toInt();
            }

            qDebug() << "Parse Event:" << eventName
                     << "Data:" << dataValue
                     << "AckId:" << ackId;

            // ===== DEDUPLICATION LOGIC =====
            if (eventName == m_lastEventName && dataValue == m_lastEventData) {
                qDebug() << "Duplicate event skipped:" << eventName << dataValue;
                return;
            }

            // Update last values
            m_lastEventName = eventName;
            m_lastEventData = dataValue;
            // =================================

            handleIncomingEvent(eventName, dataValue.toObject(), ackId);
            emit eventReceived(eventName, dataValue);
        }

        else if (subType == 3) { // ACK
            // Format: 3[ackId,"RESPONSE"]
            QJsonDocument doc = QJsonDocument::fromJson(subData.toUtf8());
            if (!doc.isArray()) {
                qWarning() << "Invalid ACK JSON:" << subData;
                return;
            }

            QJsonArray arr = doc.array();
            if (arr.size() >= 1) {
                int ackId = arr[0].toInt();
                QJsonValue ackData = (arr.size() >= 2) ? arr[1] : QJsonValue();
                qDebug() << "ACK received:" << ackId << ackData;
                handleIncomingAck(ackId, ackData);
            }
        }
        else if (subType == '0') { // CONNECT
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(subData.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject obj = doc.object();
                QString sid = obj["sid"].toString();
                qDebug() << "Socket.IO connected, SID:" << sid;
                emit connected(sid);
            }
        }else {
            qDebug() << "Unhandled Socket.IO subtype:" << subType << "Data:" << subData;
        }

        break;
    }


    case 5: // UPGRADE (WebSocket upgrade)
        qDebug() << "Upgrade packet received";
        break;

    case 6: // NOOP
        qDebug() << "NOOP packet received";
        break;

    default:
        qWarning() << "Unknown packet type:" << type;
        break;
    }
}


//------------------------------------------------------------------------
void SocketIOClient::handleIncomingAck(int ackId, const QJsonValue &data)
{
    qDebug() << "ACK received for id:" << ackId << "Data:" << data;

    auto it = m_ackCallbacks.find(ackId);
    if (it != m_ackCallbacks.end()) {
        auto callback = it->second;
        m_ackCallbacks.erase(it);
        callback(data);
    } else {
        qWarning() << "No callback found for ACK id:" << ackId;
    }
}


//------------------------------------------------------------------------
void SocketIOClient::handleIncomingEvent(const QString &eventName,
                                         const QJsonValue &payload,
                                         int ackId)
{
    // Kirim acknowledgment jika ada ackId
    if (ackId != -1) {
        // Kirim payload apa adanya (bisa object/string/array)
        sendAcknowledgment(eventName, ackId, payload);
    }

    // Jika payload bukan object, tetap log dan keluar aman
    if (!payload.isObject()) {
        qDebug() << "Event with non-object payload:" << eventName << payload;
        return;
    }

    QJsonObject obj = payload.toObject();

    // Route events ke handler yang sesuai
    if (eventName == "SCREEN_BRIGHTNESS_SET") {
        int level = obj["level"].toInt(75);
        emit screenBrightnessSet(level);
    }
    else if (eventName == "VOLUME_SET") {
        int volume = obj["volume"].toInt(75);
        emit volumeSet(volume);
    }
    else if (eventName == "DEVICE_OFF") {
        emit deviceOffRequested();
    }
    else if (eventName == "DEVICE_RESET") {
        emit deviceResetRequested();
    }
    else if (eventName == "DEVICE_RESTART") {
        emit deviceRestartRequested();
    }
    else if (eventName == "PING_DEVICE_UP") {
        emit pingDeviceUp();
    }
    else if (eventName == "INCIDENT_HELP_EVENT_DETECTED") {
        QString timestamp = obj["timestamp"].toString();
        emit helpEvent(timestamp);
    }
    else if (eventName == "INCIDENT_OK_EVENT_DETECTED") {
        QString timestamp = obj["timestamp"].toString();
        emit okEvent(timestamp);
    }
    else if (eventName == "INCIDENT_NOT_OK_EVENT_DETECTED") {
        QString timestamp = obj["timestamp"].toString();
        emit notOkEvent(timestamp);
    }
    // Handle schedule requests
    else if (eventName.startsWith("SHOW_")) {
        qDebug() << "Schedule request:" << eventName << obj;
        // Event ini akan ditangani oleh eventReceived signal
    }
}


//------------------------------------------------------------------------
void SocketIOClient::sendAcknowledgment(const QString &eventName,
                                        int ackId,
                                        const QJsonValue &originalData)
{
    if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Cannot send acknowledgment, socket not connected";
        return;
    }

    QJsonValue ackPayload;

    // Buat data acknowledgment berdasarkan event
    if (eventName == "SCREEN_BRIGHTNESS_SET" && originalData.isObject()) {
        QJsonObject obj;
        obj["level"] = originalData.toObject()["level"];
        ackPayload = obj;
    }
    else if (eventName == "VOLUME_SET" && originalData.isObject()) {
        QJsonObject obj;
        obj["volume"] = originalData.toObject()["volume"];
        ackPayload = obj;
    }
    else {
        // Default ACK payload
        QJsonObject obj;
        obj["ack"] = true;
        obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        ackPayload = obj;
    }

    // Format Socket.IO ACK packet:
    // 42["EVENT_ACK", data, ackId]
    QJsonArray eventArray;
    eventArray.append(eventName + "_ACK");
    eventArray.append(ackPayload);
    eventArray.append(ackId);

    QJsonDocument doc(eventArray);
    QString message = "42" + doc.toJson(QJsonDocument::Compact);

    m_webSocket->sendTextMessage(message);
    qDebug() << "Sent acknowledgment for" << eventName << "ackId:" << ackId;
}


//------------------------------------------------------------------------
void SocketIOClient::sendSocketIOPacket(int type, const QString &data)
{
    if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Cannot send packet, socket not connected";
        return;
    }

    QString packet = QString::number(type);
    if (!data.isEmpty()) {
        packet += data;
    }

    m_webSocket->sendTextMessage(packet);
}

//------------------------------------------------------------------------
void SocketIOClient::emitEvent1(const QString &eventName,const QJsonValue &data,std::function<void(QJsonValue)> ackCallback)
{
    if (!m_isConnected || !m_webSocket) {
        qWarning() << "Not connected, cannot emit:" << eventName;
        return;
    }

    // Format Socket.IO message: 42["eventName", {data}]
    QJsonArray eventArray;
    eventArray.append(eventName);
    eventArray.append(data);

    QJsonDocument doc(eventArray);
    QString message = "42" + doc.toJson(QJsonDocument::Compact);

    m_webSocket->sendTextMessage(message);
    qDebug() << "Emitted event:" << eventName << "data:" << data;
}

//------------------------------------------------------------------------
void SocketIOClient::emitEvent2(const QString &eventName, const QString message)
{
    if (!m_isConnected || !m_webSocket) {
        qWarning() << "Not connected, cannot emit:" << eventName;
        return;
    }

    // Format Socket.IO message: 42["eventName"]
    QString data = "42[\"" + message + "\"" + "]";

    m_webSocket->sendTextMessage(message);
    qDebug() << "Emitted event:" << eventName << "message:" << data;
}

//------------------------------------------------------------------------
void SocketIOClient::emitEvent3(const QString &eventName,
                               const QJsonValue &data)
{
    if (!m_isConnected || !m_webSocket) {
        qWarning() << "Not connected, cannot emit:" << eventName;
        return;
    }

    QJsonArray packetArray;
    packetArray.append(eventName);
    packetArray.append(data);

    QJsonDocument doc(packetArray);
    QString message = "42" + QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    m_webSocket->sendTextMessage(message);
    qDebug() << "Emitted event:" << eventName;
}


//------------------------------------------------------------------------
/*void SocketIOClient::emitEventWithAck(const QString &eventName,
                                      const QJsonObject &data,
                                      std::function<void(const QJsonObject&)> callback)
{
    if (!m_isConnected || !m_webSocket) {
        qWarning() << "Not connected, cannot emit:" << eventName;
        return;
    }

    int ackId = m_nextAckId++;
    m_ackCallbacks[ackId] = callback;

    QJsonArray packetArray;
    packetArray.append(eventName);
    packetArray.append(data);
    packetArray.append(ackId);

    QJsonDocument doc(packetArray);
    QString message = "42" + doc.toJson(QJsonDocument::Compact);

    m_webSocket->sendTextMessage(message);
    qDebug() << "Emitted event with ack:" << eventName << "ackId:" << ackId;
}
*/

//------------------------------------------------------------------------
void SocketIOClient::emitEventWithAck(const QString &eventName,
                                      const QString &data,
                                      std::function<void(const QString&)> callback)
{
    if (!m_isConnected || !m_webSocket) {
        qWarning() << "Not connected, cannot emit:" << eventName;
        return;
    }

    int ackId = m_nextAckId++;
    m_ackCallbacksQString[ackId] = callback;

    QJsonArray packetArray;
    packetArray.append(eventName);
    packetArray.append(data);
    packetArray.append(ackId);

    QJsonDocument doc(packetArray);
    QString message = "42" + doc.toJson(QJsonDocument::Compact);

    m_webSocket->sendTextMessage(message);
    qDebug() << "Emitted event with ack:" << eventName << "ackId:" << ackId;
}

//------------------------------------------------------------------------
void SocketIOClient::emitEventWithAck(const QString &eventName,
                                      const QJsonObject &data,
                                      std::function<void(QJsonValue)> callback)
{
    if (!m_isConnected || !m_webSocket) {
        qWarning() << "Not connected, cannot emit:" << eventName;
        return;
    }

    int ackId = m_nextAckId++;
    m_ackCallbacks[ackId] = callback;

    QJsonArray packetArray;
    packetArray.append(eventName);
    packetArray.append(data);
    packetArray.append(ackId);

    QJsonDocument doc(packetArray);
    QString message = "42" + QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    m_webSocket->sendTextMessage(message);
    qDebug() << "Emitted event with ack:" << eventName << "ackId:" << ackId;
}


//------------------------------------------------------------------------
void SocketIOClient::sendFallDetected()
{
    QJsonObject payload{
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };
    //emitEvent("INCIDENT_FALL_DOWN_DETECTED", payload);
    emitEvent1("INCIDENT_FALL_DOWN_DETECTED", payload, [](QJsonValue ackVal) {
        qDebug() << ackVal;
    });

}

//------------------------------------------------------------------------
void SocketIOClient::sendNoResponseFall(const QString &originalTimestamp)
{
    QJsonObject payload{
        {"timestamp", originalTimestamp}
    };
    //emitEvent("INCIDENT_FALL_DOWN_NO_RESPONSE", payload);
    emitEvent1("INCIDENT_FALL_DOWN_NO_RESPONSE", payload, [](QJsonValue ackVal) {
        qDebug() << ackVal;
    });
}

//------------------------------------------------------------------------
void SocketIOClient::sendDeviceReady(int brightness, int volume)
{
    // Gunakan QRandomGenerator untuk Qt 6
    // Generate device ID yang unik
    QString deviceId = "sentikoo_" +
                       QString::number(QRandomGenerator::global()->generate() % 1000);

    QJsonObject payload{
        {"device_id", deviceId},
        {"brightness", brightness},
        {"volume", volume}
    };
    //emitEvent("DEVICE_READY", payload);
    emitEvent1("DEVICE_READY", payload, [](QJsonValue ackVal) {
        qDebug() << ackVal;
    });
}

//------------------------------------------------------------------------
void SocketIOClient::sendScheduleEvent(const QString &scheduleType,
                                       const QJsonObject &dateParams)
{
    emitEvent3(scheduleType, dateParams);
}

//------------------------------------------------------------------------
void SocketIOClient::onWebSocketError(QAbstractSocket::SocketError error)
{
    QString errorStr = QString("WebSocket error [%1]: %2")
    .arg(error)
        .arg(m_webSocket ? m_webSocket->errorString() : "No socket");
    qWarning() << errorStr;
    emit connectionError(errorStr);
}

//------------------------------------------------------------------------
void SocketIOClient::sendPing()
{
    sendSocketIOPacket(2); // Type 2 = PING
    qDebug() << "Sent PING";
}

//------------------------------------------------------------------------
void SocketIOClient::scheduleReconnect()
{
    m_reconnectAttempts++;

    // Exponential backoff: 1s, 2s, 4s, 8s, 16s, max 30s
    int delay = qMin(30000, (1 << qMin(m_reconnectAttempts, 5)) * 1000);

    qDebug() << "Reconnecting in" << delay << "ms (attempt"
             << m_reconnectAttempts << ")";

    m_reconnectTimer->start(delay);
}

//------------------------------------------------------------------------
void SocketIOClient::attemptReconnect()
{
    if (!m_webSocket || m_webSocket->state() == QAbstractSocket::UnconnectedState) {
        qDebug() << "Attempting reconnect to" << m_host << ":" << m_port;
        constructWebSocketUrl();
    }
}


