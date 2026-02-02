/****************************************************************************
** Meta object code from reading C++ file 'socketioclient.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../socketioclient.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'socketioclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14SocketIOClientE_t {};
} // unnamed namespace

template <> constexpr inline auto SocketIOClient::qt_create_metaobjectdata<qt_meta_tag_ZN14SocketIOClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SocketIOClient",
        "connected",
        "",
        "socketId",
        "disconnected",
        "connectionError",
        "error",
        "eventReceived",
        "eventName",
        "QJsonObject",
        "data",
        "screenBrightnessSet",
        "level",
        "volumeSet",
        "volume",
        "deviceOffRequested",
        "deviceResetRequested",
        "deviceRestartRequested",
        "pingDeviceUp",
        "helpEvent",
        "timestamp",
        "okEvent",
        "notOkEvent",
        "onWebSocketConnected",
        "onWebSocketDisconnected",
        "onWebSocketTextMessage",
        "message",
        "onWebSocketError",
        "QAbstractSocket::SocketError",
        "sendPing",
        "attemptReconnect"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'connected'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'disconnected'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'connectionError'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'eventReceived'
        QtMocHelpers::SignalData<void(const QString &, const QJsonObject &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'screenBrightnessSet'
        QtMocHelpers::SignalData<void(int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 12 },
        }}),
        // Signal 'volumeSet'
        QtMocHelpers::SignalData<void(int)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 14 },
        }}),
        // Signal 'deviceOffRequested'
        QtMocHelpers::SignalData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'deviceResetRequested'
        QtMocHelpers::SignalData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'deviceRestartRequested'
        QtMocHelpers::SignalData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pingDeviceUp'
        QtMocHelpers::SignalData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'helpEvent'
        QtMocHelpers::SignalData<void(const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Signal 'okEvent'
        QtMocHelpers::SignalData<void(const QString &)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Signal 'notOkEvent'
        QtMocHelpers::SignalData<void(const QString &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Slot 'onWebSocketConnected'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onWebSocketDisconnected'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onWebSocketTextMessage'
        QtMocHelpers::SlotData<void(const QString &)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 26 },
        }}),
        // Slot 'onWebSocketError'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 28, 6 },
        }}),
        // Slot 'sendPing'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'attemptReconnect'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SocketIOClient, qt_meta_tag_ZN14SocketIOClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SocketIOClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SocketIOClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SocketIOClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14SocketIOClientE_t>.metaTypes,
    nullptr
} };

void SocketIOClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SocketIOClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->connected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->disconnected(); break;
        case 2: _t->connectionError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->eventReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 4: _t->screenBrightnessSet((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->volumeSet((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->deviceOffRequested(); break;
        case 7: _t->deviceResetRequested(); break;
        case 8: _t->deviceRestartRequested(); break;
        case 9: _t->pingDeviceUp(); break;
        case 10: _t->helpEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->okEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->notOkEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->onWebSocketConnected(); break;
        case 14: _t->onWebSocketDisconnected(); break;
        case 15: _t->onWebSocketTextMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: _t->onWebSocketError((*reinterpret_cast<std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 17: _t->sendPing(); break;
        case 18: _t->attemptReconnect(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 16:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(const QString & )>(_a, &SocketIOClient::connected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)()>(_a, &SocketIOClient::disconnected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(const QString & )>(_a, &SocketIOClient::connectionError, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(const QString & , const QJsonObject & )>(_a, &SocketIOClient::eventReceived, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(int )>(_a, &SocketIOClient::screenBrightnessSet, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(int )>(_a, &SocketIOClient::volumeSet, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)()>(_a, &SocketIOClient::deviceOffRequested, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)()>(_a, &SocketIOClient::deviceResetRequested, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)()>(_a, &SocketIOClient::deviceRestartRequested, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)()>(_a, &SocketIOClient::pingDeviceUp, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(const QString & )>(_a, &SocketIOClient::helpEvent, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(const QString & )>(_a, &SocketIOClient::okEvent, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (SocketIOClient::*)(const QString & )>(_a, &SocketIOClient::notOkEvent, 12))
            return;
    }
}

const QMetaObject *SocketIOClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SocketIOClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SocketIOClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SocketIOClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void SocketIOClient::connected(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void SocketIOClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SocketIOClient::connectionError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SocketIOClient::eventReceived(const QString & _t1, const QJsonObject & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void SocketIOClient::screenBrightnessSet(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void SocketIOClient::volumeSet(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SocketIOClient::deviceOffRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SocketIOClient::deviceResetRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SocketIOClient::deviceRestartRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void SocketIOClient::pingDeviceUp()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void SocketIOClient::helpEvent(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void SocketIOClient::okEvent(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void SocketIOClient::notOkEvent(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1);
}
QT_WARNING_POP
