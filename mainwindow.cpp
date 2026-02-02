#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtMultimedia/QSoundEffect>
#include <QUrl>
#include <QTime>
#include <QElapsedTimer>
#include <QSharedPointer>

// Jika qcustomplot butuh include spesifik, sudah di header

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Timer untuk proses payload
    m_processTimer = new QTimer(this);
    connect(m_processTimer, &QTimer::timeout, this, &MainWindow::processPayload);
    m_processTimer->start(50); // proses tiap 50 ms

    // Timer untuk proses payload
    m_processTimer2 = new QTimer(this);
    connect(m_processTimer2, &QTimer::timeout, this, &MainWindow::processPayload2);
    m_processTimer2->start(50); // proses tiap 50 ms

    // Setup sound (Qt6: include path QtMultimedia/QSoundEffect)
    sound.setSource(QUrl::fromLocalFile("/Volumes/DATA/wav/alarm1.wav"));
    sound.setLoopCount(1);
    sound.setVolume(1.0f);

    setupRealtimeDataMotion(ui->plottsgram);
    setupRealtimeDataVelocity(ui->plottsVelocity);
    setupPlotRadar(ui->plotRadar);

    setupRealtimeDataMotion2(ui->plottsgram2);
    setupRealtimeDataVelocity2(ui->plottsVelocity2);
    setupPlotRadar2(ui->plotRadar2);

    //Timer utk sokcet
    m_socketTimer = new QTimer(this);
    ws = new QWebSocket();

    connect(m_socketTimer, &QTimer::timeout, this, &MainWindow::processFayloadSocket);
    m_socketTimer->start(500); // proses tiap 500 ms
    socketState = SOCKET_IDDLE;
    socketIO_Client_Prepare();

#ifdef AUTOSTART_ONRPI
    ui->cbSocket->setChecked(true);
    init_port("ttyS0");
    init_port("ttyAMA1");

    // Timer untuk proses payload
    //m_processTimer = new QTimer(this);
    //connect(m_processTimer, &QTimer::timeout, this, &MainWindow::processPayload);
    //m_processTimer->start(50); // proses tiap 50 ms

#endif
}

//---------------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    // Pastikan timer dan serial ditutup dengan rapi
    if (m_processTimer) {
        m_processTimer->stop();
    }
    if (m_serial) {
        if (m_serial->isOpen()) m_serial->close();
        m_serial->deleteLater();
        m_serial = nullptr;
    }

    delete ui;
}

//---------------------------------------------------------------------------------------
void MainWindow::init_port()
{
    // Periksa apakah port sudah diinisialisasi dan terbuka
    if (m_serial && m_serial->isOpen()) {
        qDebug() << "Port already open.";
        return;
    }

    // Jika sudah ada instance `m_serial`, hapus untuk mencegah kebocoran memori
    if (m_serial) {
        m_serial->deleteLater();
        m_serial = nullptr;
    }

    // Buat instance QSerialPort baru; gunakan port name dari combo box
    const QString portName = ui->serialPortInfoListBox->currentText();
    m_serial = new QSerialPort(portName, this);

    // Konfigurasi serial port
    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);

    // Cek apakah port berhasil dibuka
    if (!m_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open port:" << m_serial->errorString();
        m_serial->deleteLater();
        m_serial = nullptr;
    } else {
        qDebug() << "Open Port OK";

        // Hubungkan sinyal ke slot (syntax Qt6 modern)
        connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
        connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

        ui->btnOpenSerialPort->setEnabled(false);
        ui->btnLoad->setEnabled(false);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::init_port(QString portname)
{
    // Periksa apakah port sudah diinisialisasi dan terbuka
    if (m_serial && m_serial->isOpen()) {
        qDebug() << "Port already open.";
        return;
    }

    // Jika sudah ada instance `m_serial`, hapus untuk mencegah kebocoran memori
    if (m_serial) {
        m_serial->deleteLater();
        m_serial = nullptr;
    }

    // Buat instance QSerialPort baru; gunakan port name dari combo box
    //const QString portName = ui->serialPortInfoListBox->currentText();
    m_serial = new QSerialPort(portname, this);

    // Konfigurasi serial port
    m_serial->setPortName(portname);
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);

    // Cek apakah port berhasil dibuka
    if (!m_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open port:" << m_serial->errorString();
        m_serial->deleteLater();
        m_serial = nullptr;
    } else {
        qDebug() << "Open Port OK";

        // Hubungkan sinyal ke slot (syntax Qt6 modern)
        connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
        connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

        ui->btnOpenSerialPort->setEnabled(false);
        ui->btnLoad->setEnabled(false);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::readData()
{
    if (!m_serial) return;

    // Tambahkan data baru ke buffer
    const QByteArray data = m_serial->readAll();
    if (data.isEmpty()) return;

    // Append new data to existing buffer (previous code assigned; safer to append)
    m_buffer.append(data);

    // Parsing frame lengkap
    while (true) {
        int startIdx = m_buffer.indexOf(QByteArray::fromHex("5359")); // "53 59"
        if (startIdx < 0) {
            break;
        }

        int endIdx = m_buffer.indexOf(QByteArray::fromHex("5443"), startIdx + 2); // "54 43"
        if (endIdx < 0) {
            break;
        }

        // Hitung panjang frame (endIdx menunjuk byte '54', +2 termasuk kedua byte)
        int frameLen = endIdx + 2 - startIdx;
        if (frameLen <= 0 || endIdx + 2 > m_buffer.size()) {
            break;
        }

        // Ambil satu frame lengkap
        QByteArray frame = m_buffer.mid(startIdx, frameLen);
        QString strData = frame.toHex().toUpper();

        // Hapus frame dari buffer (hapus sampai setelah endIdx+1)
        m_buffer.remove(0, endIdx + 2);

        // Cek minimal panjang frame
        if (frame.size() < 6)
            continue;

        // Periksa checksum (mengikuti implementasi asli)
        quint8 cs = calcChecksum(frame.mid(0, frame.size() - 3)); // hitung dari byte awal s/d sebelum checksum byte
        quint8 recvCs = static_cast<quint8>(frame.at(frame.size() - 3)); // byte sebelum "54 43"

        if (cs != recvCs) {
            qDebug() << "[ERR] checksum mismatch:" << QByteArray(1, cs).toHex() << "!=" << QByteArray(1, recvCs).toHex();
            continue;
        }

        // Payload adalah data di antara header dan checksum
        QByteArray payload = frame.mid(2, frame.size() - 5);
        m_payloadQueue.enqueue(payload);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::init_port2()
{
    // Periksa apakah port sudah diinisialisasi dan terbuka
    if (m_serial2 && m_serial2->isOpen()) {
        qDebug() << "Port2 already open.";
        return;
    }

    // Jika sudah ada instance `m_serial`, hapus untuk mencegah kebocoran memori
    if (m_serial2) {
        m_serial2->deleteLater();
        m_serial2 = nullptr;
    }

    // Buat instance QSerialPort baru; gunakan port name dari combo box
    const QString portName = ui->serialPortInfoListBox2->currentText();
    m_serial2 = new QSerialPort(portName, this);

    // Konfigurasi serial port
    m_serial2->setPortName(portName);
    m_serial2->setBaudRate(QSerialPort::Baud115200);
    m_serial2->setFlowControl(QSerialPort::NoFlowControl);
    m_serial2->setParity(QSerialPort::NoParity);
    m_serial2->setDataBits(QSerialPort::Data8);
    m_serial2->setStopBits(QSerialPort::OneStop);

    // Cek apakah port berhasil dibuka
    if (!m_serial2->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open port2:" << m_serial2->errorString();
        m_serial2->deleteLater();
        m_serial2 = nullptr;
    } else {
        qDebug() << "Open Port OK";

        // Hubungkan sinyal ke slot (syntax Qt6 modern)
        connect(m_serial2, &QSerialPort::readyRead, this, &MainWindow::readData2);
        connect(m_serial2, &QSerialPort::errorOccurred, this, &MainWindow::handleError2);

        ui->btnOpenSerialPort2->setEnabled(false);
        ui->btnLoad2->setEnabled(false);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::init_port2(QString portname)
{
    // Periksa apakah port sudah diinisialisasi dan terbuka
    if (m_serial2 && m_serial2->isOpen()) {
        qDebug() << "Port already open.";
        return;
    }

    // Jika sudah ada instance `m_serial`, hapus untuk mencegah kebocoran memori
    if (m_serial2) {
        m_serial2->deleteLater();
        m_serial2 = nullptr;
    }

    // Buat instance QSerialPort baru; gunakan port name dari combo box
    //const QString portName = ui->serialPortInfoListBox->currentText();
    m_serial2 = new QSerialPort(portname, this);

    // Konfigurasi serial port
    m_serial2->setPortName(portname);
    m_serial2->setBaudRate(QSerialPort::Baud115200);
    m_serial2->setFlowControl(QSerialPort::NoFlowControl);
    m_serial2->setParity(QSerialPort::NoParity);
    m_serial2->setDataBits(QSerialPort::Data8);
    m_serial2->setStopBits(QSerialPort::OneStop);

    // Cek apakah port berhasil dibuka
    if (!m_serial2->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open port:" << m_serial2->errorString();
        m_serial2->deleteLater();
        m_serial2 = nullptr;
    } else {
        qDebug() << "Open Port OK";

        // Hubungkan sinyal ke slot (syntax Qt6 modern)
        connect(m_serial2, &QSerialPort::readyRead, this, &MainWindow::readData2);
        connect(m_serial2, &QSerialPort::errorOccurred, this, &MainWindow::handleError2);

        ui->btnOpenSerialPort2->setEnabled(false);
        ui->btnLoad2->setEnabled(false);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::readData2()
{
    if (!m_serial2) return;

    // Tambahkan data baru ke buffer
    const QByteArray data = m_serial2->readAll();
    if (data.isEmpty()) return;

    // Append new data to existing buffer (previous code assigned; safer to append)
    m_buffer2.append(data);

    // Parsing frame lengkap
    while (true) {
        int startIdx = m_buffer2.indexOf(QByteArray::fromHex("5359")); // "53 59"
        if (startIdx < 0) {
            break;
        }

        int endIdx = m_buffer2.indexOf(QByteArray::fromHex("5443"), startIdx + 2); // "54 43"
        if (endIdx < 0) {
            break;
        }

        // Hitung panjang frame (endIdx menunjuk byte '54', +2 termasuk kedua byte)
        int frameLen = endIdx + 2 - startIdx;
        if (frameLen <= 0 || endIdx + 2 > m_buffer2.size()) {
            break;
        }

        // Ambil satu frame lengkap
        QByteArray frame = m_buffer2.mid(startIdx, frameLen);
        QString strData = frame.toHex().toUpper();

        // Hapus frame dari buffer (hapus sampai setelah endIdx+1)
        m_buffer2.remove(0, endIdx + 2);

        // Cek minimal panjang frame
        if (frame.size() < 6)
            continue;

        // Periksa checksum (mengikuti implementasi asli)
        quint8 cs = calcChecksum(frame.mid(0, frame.size() - 3)); // hitung dari byte awal s/d sebelum checksum byte
        quint8 recvCs = static_cast<quint8>(frame.at(frame.size() - 3)); // byte sebelum "54 43"

        if (cs != recvCs) {
            qDebug() << "[ERR] checksum mismatch:" << QByteArray(1, cs).toHex() << "!=" << QByteArray(1, recvCs).toHex();
            continue;
        }

        // Payload adalah data di antara header dan checksum
        QByteArray payload2 = frame.mid(2, frame.size() - 5);
        m_payloadQueue2.enqueue(payload2);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return; // Abaikan jika tidak ada error
    if (!m_serial) return;

    qDebug() << "Serial port error occurred:" << m_serial->errorString();

    // Tutup port untuk memastikan tidak digunakan lagi
    if (m_serial->isOpen()) m_serial->close();

    // Hapus objek serial dengan deleteLater() untuk menghindari crash
    m_serial->deleteLater();
    m_serial = nullptr;

    // Enable UI controls kembali
    ui->btnOpenSerialPort->setEnabled(true);
    ui->btnLoad->setEnabled(true);
}

//---------------------------------------------------------------------------------------
void MainWindow::handleError2(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return; // Abaikan jika tidak ada error
    if (!m_serial2) return;

    qDebug() << "Serial port error occurred:" << m_serial2->errorString();

    // Tutup port untuk memastikan tidak digunakan lagi
    if (m_serial2->isOpen()) m_serial2->close();

    // Hapus objek serial dengan deleteLater() untuk menghindari crash
    m_serial2->deleteLater();
    m_serial2 = nullptr;

    // Enable UI controls kembali
    ui->btnOpenSerialPort2->setEnabled(true);
    ui->btnLoad2->setEnabled(true);
}

//---------------------------------------------------------------------------------------
void MainWindow::closeSerialPort()
{
    if (!m_serial) return;

    if (m_serial->isOpen()) {
        m_serial->close();
        qDebug() << "Close OK\n";

        disconnect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
        disconnect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

        ui->btnOpenSerialPort->setEnabled(true);
        ui->btnLoad->setEnabled(true);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::showPortInfo(int idx)
{
    if (idx == -1) return;

    const QStringList list = ui->serialPortInfoListBox->itemData(idx).toStringList();
    Q_UNUSED(list);
}

//---------------------------------------------------------------------------------------
void MainWindow::processPayload()
{
    while (!m_payloadQueue.isEmpty()) {
        QByteArray payload = m_payloadQueue.dequeue();
        if (payload.isEmpty())
            continue;

        const QByteArray hex = payload.toHex(' ').toUpper();

        quint8 cmd = static_cast<quint8>(payload[0]);
        quint8 sub = (payload.size() > 1) ? static_cast<quint8>(payload[1]) : 0;

        switch (cmd)
        {
        // ======================================================
        // 0x01 — SYSTEM
        // ======================================================
        case 0x01:
            switch (sub) {
            case 0x01:
                qDebug() << "System: Heartbeat (reporting)";
                break;
            case 0x02:
                qDebug() << "System: Module Reset (distribution/reporting)";
                break;
            default:
                qDebug() << "System: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x02 — PRODUCT INFO
        // ======================================================
        case 0x02:
            switch (sub) {
            case 0xA1:
                if (payload.size() > 10) {
                    QByteArray model = payload.mid(payload.length()-7, 7);
                    ui->leProductModel->setText(QString::fromUtf8(model));
                }
                break;
            case 0xA2:
                if (payload.size() > 6) {
                    quint16 len = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(payload.constData() + 2));
                    QByteArray pid = payload.mid(4, len);
                    QString productID = QString::fromUtf8(pid);
                    ui->leProductID->setText(productID);
                }
                break;
            case 0xA3:
                if (payload.size() >= 8) {
                    QByteArray hwmodel = payload.mid(payload.length()-4, 4);
                    ui->leHWModel->setText(QString::fromUtf8(hwmodel));
                }
                break;
            case 0xA4:
                if (payload.size() > 6) {
                    QByteArray ver = payload.mid(payload.size() - 16, 16);
                    ui->leFirmwareVersion->setText(QString::fromUtf8(ver));
                }
                break;
            default:
                qDebug() << "Product Info: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x05 — WORKING STATUS / PARAMETER
        // ======================================================
        case 0x05:
            switch (sub) {
            case 0x01:
                qDebug() << "Working Status: Init complete";
                break;
            case 0x02:
                qDebug() << "Working Status: Radar fault";
                if (payload.size() > 6){
                    qDebug() << "Fault code:" << QString::number(static_cast<quint8>(payload[6]));
                }
                break;
            case 0x81:
                if (payload.size() >= 5){
                    quint8 status = static_cast<quint8>(payload[4]);
                    if(status == 1){
                        ui->leInitComplete->setText("Inited");
                    } else {
                        ui->leInitComplete->setText("Uninit");
                    }
                }
                break;
            default:
                qDebug() << "Working Status/Param: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x06 — INSTALLATION INFO
        // ======================================================
        case 0x06:
            switch (sub) {
            case 0x81: {
                if (payload.size() >= 10) {
                    auto read16BigEndian = [&](int offset) {
                        return (static_cast<uchar>(payload[offset]) << 8) | static_cast<uchar>(payload[offset + 1]);
                    };
                    double x_deg = read16BigEndian(4); // /100.0;
                    double y_deg = read16BigEndian(6); // /100.0;
                    double z_deg = read16BigEndian(8); // /100.0;
                    ui->leAngleXInstallation->setText(QString::number(x_deg));
                    ui->leAngleYInstallation->setText(QString::number(y_deg));
                    ui->leAngleZInstallation->setText(QString::number(z_deg));
                    qDebug() << " 06 81 get angle =" << payload << " xyz " << x_deg << "-" << y_deg << "-" << z_deg;
                }
                break;
            }
            case 0x82: {
                if (payload.size() >= 6) {
                    uchar low = payload[4];
                    uchar high = payload[5];
                    qint16 height = (low << 8) | high;
                    ui->leHeightInstallation->setText(QString::number(height));
                    qDebug() << " 06 82 get height =" << payload << " height " << height;
                }
                break;
            }
            case 0x01:{
                if (payload.size() >= 10) {
                    auto read16BigEndian = [&](int offset) {
                        return (static_cast<uchar>(payload[offset]) << 8) | static_cast<uchar>(payload[offset + 1]);
                    };
                    double x_deg = read16BigEndian(4);// / 100.0;
                    double y_deg = read16BigEndian(6);// / 100.0;
                    double z_deg = read16BigEndian(8);// / 100.0;
                    ui->leAngleXInstallation->setText(QString::number(x_deg));
                    ui->leAngleYInstallation->setText(QString::number(y_deg));
                    ui->leAngleZInstallation->setText(QString::number(z_deg));
                    qDebug() << " 06 01 set angle =" << payload << " xyz " << x_deg << "-" << y_deg << "-" << z_deg;
                }
                break;
            }
            case 0x04:{
                if(payload.size() >= 8) {
                    qDebug() << " Hearth Beat " << payload;
                }
                break;
            }
            case 0x02:{
                if (payload.size() >= 6) {
                    uchar low = payload[4];
                    uchar high = payload[5];
                    qint16 height = (low << 8) | high;
                    ui->leHeightInstallation->setText(QString::number(height));
                    qDebug() << " 06 02 set height reply=" << payload << " height " << height;
                }
                break;
            }
            default:
                qDebug() << "Installation: Unknown subcmd" << " payload " << payload;
                break;
            }
            break;

        // ======================================================
        // 0x07 — RADAR RANGE
        // ======================================================
        case 0x07:
            if (sub == 0x09 && payload.size() >= 14) {
                auto read16 = [&](int offset) {
                    return qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(payload.constData() + offset));
                };
                qDebug() << "Radar Range: X+:" << read16(6) << " X-:" << read16(8)
                         << " Y+:" << read16(10) << " Y-:" << read16(12);
            } else {
                qDebug() << "Radar Range: Unknown subcmd" << QString::number(sub,16);
            }
            break;

        // ======================================================
        // 0x80 — PRESENCE
        // ======================================================
        case 0x80:
            switch (sub) {
            case 0x00: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame.presenceOnFunction = (val == 1);
                ui->lePresence->setText(val ? "ON" : "OFF");
                qDebug() << "80 00 Set Presence Activated =" << payload;
                break;
            }
            case 0x01:{
                qDebug() << "80 01 Presence: Presence Info =" << static_cast<quint8>(payload[6]);
                quint8 val = static_cast<quint8>(payload[4]);
                ui->leMotion->setStyleSheet(val ? "background-color: blue; color: yellow;" : "background-color: green; color: black;");
                break;
            }
            case 0x02:{
                qDebug() << "80 02 Presence: Motion Info =" << static_cast<quint8>(payload[6]);
                quint8 val = static_cast<quint8>(payload[4]);
                ui->leMotion->setStyleSheet(val ? "background-color: grey; color: black;" : "background-color: orange; color: black;");
                break;
            }
            case 0x03:{
                if (payload.size() >= 5) {
                    quint8 param = static_cast<quint8>(payload[4]);
                    ui->leMotion->setText(QString::number(param));
                    drawRealTimeetsgram(QString::number(param));
                    qDebug() << "80 03 Presence: Motion Info =" << static_cast<quint8>(param);
                }
                break;
            }
            default:
                qDebug() << "80 Presence: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x82 — RADAR FRAME DATA
        // ======================================================
        case 0x82:
            switch (sub){
            case 0x00:{  //command reply set trace tracking
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame.traceTracking = (val == 1);
                ui->leTraceTracking->setText(val ? "ON" : "OFF");
                qDebug() << "82 00 Reply of Trace Cmd =" << payload;
                break;
            }
            case 0x01:{  //trace number reply
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame.traceNumber = (val == 1);
                ui->leTraceNumber->setText(QString::number(val));
                qDebug() << "82 01 Trace Numbers of =" << payload;
                break;
            }
            case 0x02:{
                qDebug() << "82 02 Trace Tracking Info =" << payload.toHex(' ');

                auto read16s = [&](int offset) -> qint16 {
                    quint16 raw = (quint16(payload[offset]) << 8) |
                                  quint8(payload[offset + 1]);
                    return qint16(raw);
                };

                uint8_t  type_code = payload[0];
                uint8_t  subtype   = payload[1];
                uint16_t len       = read16s(2);

                int offset = 4;   // first target starts at 4
                const int targetSize = 11;

                int targetCount = len / targetSize;

                for (int i = 0; i < targetCount; i++)
                {
                    uint8_t index        = payload[offset + 0];
                    uint8_t target_size  = payload[offset + 1];
                    uint8_t target_feat  = payload[offset + 2];

                    qint16 x_pos        = read16s(offset + 3);
                    qint16 y_pos        = read16s(offset + 5);
                    qint16 height       = read16s(offset + 7);
                    qint16 velocity     = read16s(offset + 9);

                    // Filter invalid velocity
                    if (velocity <= -32000 || velocity >= 32000) {
                        velocity = 0;
                    }

                    // Filter invalid coordinates
                    if (x_pos <= -32000 || x_pos >= 32000 ||
                        y_pos <= -32000 || y_pos >= 32000)
                    {
                        qDebug() << "Invalid coordinate received, skipping" << x_pos << "-" << y_pos;;
                        offset += targetSize;
                        continue;   // skip update UI/plot untuk target ini
                    }

                    // Update UI & Plot
                    ui->leFallPosX->setText(QString::number(x_pos));
                    ui->leFallPosY->setText(QString::number(y_pos));
                    updateRadarPoint(static_cast<double>(x_pos),
                                     static_cast<double>(y_pos));

                    drawRealTimeVelocity(QString::number(velocity));
                    ui->leVelocity->setText(QString::number(velocity));

                    qDebug() << "Target" << i
                             << "tc:" << targetCount
                             << "Index:" << index
                             << "Size:" << target_size
                             << "Feat:" << target_feat
                             << "X:" << x_pos
                             << "Y:" << y_pos
                             << "Z:" << height
                             << "Vel:" << velocity;

                    offset += targetSize;
                }
                break;
            }

            case 0x80:{ //
                qDebug() << "82 80 Trace Tracking Info =" << payload;
                break;
            }
            case 0x81:{
                qDebug() << "82 81 Trace Numbers =" << payload;
                break;
            }
            case 0x82:{
                qDebug() << "82 82 Trace Info query =" << payload;
                break;
            }
            default:
                qDebug() << "82 xx Trace Info query =" << payload;
                break;
            }
            break;

        // ======================================================
        // 0x83 — FALL DETECTION
        // ======================================================
        case 0x83:
            switch (sub) {
            case 0x00: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame.traceTracking = (val == 1);
                ui->leFallDetection->setText(val ? "ON" : "OFF");
                break;
            }
            case 0x01: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame.fallState = (val == 1);
                ui->leFallState->setStyleSheet(val ? "background-color: red; color: yellow;" : "background-color: green; color: black;");
                ui->leFallState->setText(val ? "FALLEN" : "NOT FALLEN");
                qDebug() << "83 01 Fall =" << payload;
                if(val == 1){
                    sound.stop();
                    sound.play();
                    if(ui->cbSocket->isChecked()){
                        m_socketQueue.enqueue(SOCKET_REQ_FALL); //send to socket queue
                    }
                }
                break;
            }
            case 0x05: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame.standStillState = (val == 1);
                ui->leFallState->setStyleSheet(val ? "background-color: blue; color: black;" : "background-color: grey; color: black;");
                ui->leStandStill->setText(val ? "EXIST" : "NO STAND STILL");
                qDebug() << "83 05 standstill =" << payload;
                break;
            }
            case 0x0B:{
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame.standStillState = (val == 1);
                ui->leGetStandStill->setText(val ? "ON" : "OFF");
                qDebug() << "83 0B standstill setting =" << payload;
                break;
            }
            case 0x0C:{
                if (payload.size() >= 8) {
                    quint8 duration = static_cast<quint8>(payload[7]);
                    qDebug() << "0C Fall: Duration Setting =" << duration;
                }
                break;
            }
            case 0x8C:{
                if (payload.size() >= 8) {
                    QByteArray durationBytes = payload.mid(4, 4);
                    quint32 duration = (static_cast<quint8>(durationBytes[0]) << 24) |
                                       (static_cast<quint8>(durationBytes[1]) << 16) |
                                       (static_cast<quint8>(durationBytes[2]) << 8)  |
                                       (static_cast<quint8>(durationBytes[3]));
                    ui->leFallDuration->setText(QString::number(duration));
                    qDebug() << "8C Fall: Duration Setting =" << duration;
                }
                break;
            }
            case 16: {//fall position
                if (payload.size() >= 8){   // safety

                    // Payload:
                    // [0] = 0x83
                    // [1] = 0x16
                    // [2] = length HB
                    // [3] = length LB
                    // [4] = X HB
                    // [5] = X LB
                    // [6] = Y HB
                    // [7] = Y LB

                    quint16 posX = (static_cast<quint8>(payload[4]) << 8) |
                                    static_cast<quint8>(payload[5]);

                    quint16 posY = (static_cast<quint8>(payload[6]) << 8) |
                                    static_cast<quint8>(payload[7]);

                    radarFrame.fallPosX = posX;
                    radarFrame.fallPosY = posY;

                    ui->leFallPosX->setText(QString::number(posX));
                    ui->leFallPosY->setText(QString::number(posY));

                    updateRadarPoint(posX, posY);

                    qDebug() << "Fall Position -> X:" << posX << "Y:" << posY;
                }
                break;
            }
            case 17: {//fall cancel
                if (payload.size() >= 8){   // safety

                    // Payload:
                    // [0] = 0x83
                    // [1] = 0x16
                    // [2] = length HB
                    // [3] = length LB
                    // [4] = X HB
                    // [5] = X LB
                    // [6] = Y HB
                    // [7] = Y LB

                    quint16 posX = (static_cast<quint8>(payload[4]) << 8) |
                                   static_cast<quint8>(payload[5]);

                    quint16 posY = (static_cast<quint8>(payload[6]) << 8) |
                                   static_cast<quint8>(payload[7]);

                    radarFrame.fallPosX = posX;
                    radarFrame.fallPosY = posY;

                    ui->leFallPosX->setText(QString::number(posX));
                    ui->leFallPosY->setText(QString::number(posY));


                    updateRadarPoint(posX, posY);

                    qDebug() << "Fall Cancel Position -> X:" << posX << "Y:" << posY;

                    //ui->leFallState->setStyleSheet("background-color: blue; color: black;");
                    ui->leFallState->setStyleSheet("background-color: grey; color: yellow;");
                    ui->leFallState->setText("FALLEN");
                    qDebug() << "83 17 Fall Cancel=" << payload;
                    sound.stop();
                    sound.play();
                }
                break;
            }
            default:
                qDebug() << "Fall: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        default:
            break;
        }
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::processFayloadSocket()
{
    while (!m_socketQueue.isEmpty()) {
        quint8 sockQueue = m_socketQueue.dequeue();
        if (sockQueue == 0){ //no queue
            break;
        }else{ //queue available, process
            socketState = SOCKET_CONNECTING;
            ui->logEdit->appendPlainText("q0. Queue rcv");
        }
    }

    switch (socketState) {
    case SOCKET_IDDLE:{
        //ui->logEdit->appendPlainText("q1. queue iddle0");
        break;
    }
    case SOCKET_CONNECTING:{
        socketState = SOCKET_WAITING_CONNECTED;
        ui->logEdit->appendPlainText("q2. queue connecting");
        socketIO_Client_Connect();
        break;
    }
    case SOCKET_WAITING_CONNECTED:{
        break;
    }
    case SOCKET_CONNECTED:{
        socketState = SOCKET_SENDING;
        ui->logEdit->appendPlainText("q3. queue connected");
        break;
    }
    case SOCKET_SENDING:{
        socketState = SOCKET_WAITING_SENT;
        ui->logEdit->appendPlainText("q4. queue sending");
        socketIO_Client_Write();
        break;
    }
    case SOCKET_WAITING_SENT:{
        ui->logEdit->appendPlainText("q5. queue waiting sent");
        break;
    }
    case SOCKET_SENT:{
        socketState = SOCKET_CLOSING;
        ui->logEdit->appendPlainText("q6. queue disconnected");
        socketIO_Client_Disconnect();
        break;
    }
    case SOCKET_CLOSING:{
        socketState = SOCKET_IDDLE;
        //ui->logEdit->appendPlainText("q7. queue iddle1");
        break;
    }
    default:
        //ui->logEdit->appendPlainText("q8");
        break;
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::writeData(const QByteArray &data)
{
    if (!m_serial) return;
    m_serial->write(data);
}

//---------------------------------------------------------------------------------------
void MainWindow::closeSerialPort2()
{
    if (!m_serial2) return;

    if (m_serial2->isOpen()) {
        m_serial2->close();
        qDebug() << "Close2 OK\n";

        disconnect(m_serial2, &QSerialPort::readyRead, this, &MainWindow::readData2);
        disconnect(m_serial2, &QSerialPort::errorOccurred, this, &MainWindow::handleError2);

        ui->btnOpenSerialPort2->setEnabled(true);
        ui->btnLoad2->setEnabled(true);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::showPortInfo2(int idx)
{
    if (idx == -1) return;

    const QStringList list = ui->serialPortInfoListBox2->itemData(idx).toStringList();
    Q_UNUSED(list);
}

//---------------------------------------------------------------------------------------
void MainWindow::processPayload2()
{
    while (!m_payloadQueue2.isEmpty()) {
        QByteArray payload = m_payloadQueue2.dequeue();
        if (payload.isEmpty())
            continue;

        const QByteArray hex = payload.toHex(' ').toUpper();

        quint8 cmd = static_cast<quint8>(payload[0]);
        quint8 sub = (payload.size() > 1) ? static_cast<quint8>(payload[1]) : 0;

        switch (cmd)
        {
        // ======================================================
        // 0x01 — SYSTEM
        // ======================================================
        case 0x01:
            switch (sub) {
            case 0x01:
                qDebug() << "System2: Heartbeat (reporting)";
                break;
            case 0x02:
                qDebug() << "System2: Module Reset (distribution/reporting)";
                break;
            default:
                qDebug() << "System2: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x02 — PRODUCT INFO
        // ======================================================
        case 0x02:
            switch (sub) {
            case 0xA1:
                if (payload.size() > 10) {
                    QByteArray model = payload.mid(payload.length()-7, 7);
                    ui->leProductModel2->setText(QString::fromUtf8(model));
                }
                break;
            case 0xA2:
                if (payload.size() > 6) {
                    quint16 len = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(payload.constData() + 2));
                    QByteArray pid = payload.mid(4, len);
                    QString productID = QString::fromUtf8(pid);
                    ui->leProductID2->setText(productID);
                }
                break;
            case 0xA3:
                if (payload.size() >= 8) {
                    QByteArray hwmodel = payload.mid(payload.length()-4, 4);
                    ui->leHWModel2->setText(QString::fromUtf8(hwmodel));
                }
                break;
            case 0xA4:
                if (payload.size() > 6) {
                    QByteArray ver = payload.mid(payload.size() - 16, 16);
                    ui->leFirmwareVersion2->setText(QString::fromUtf8(ver));
                }
                break;
            default:
                qDebug() << "Product Info2: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x05 — WORKING STATUS / PARAMETER
        // ======================================================
        case 0x05:
            switch (sub) {
            case 0x01:
                qDebug() << "Working Status2: Init complete";
                break;
            case 0x02:
                qDebug() << "Working Status2: Radar fault";
                if (payload.size() > 6){
                    qDebug() << "Fault code2:" << QString::number(static_cast<quint8>(payload[6]));
                }
                break;
            case 0x81:
                if (payload.size() >= 5){
                    quint8 status = static_cast<quint8>(payload[4]);
                    if(status == 1){
                        ui->leInitComplete2->setText("Inited");
                    } else {
                        ui->leInitComplete2->setText("Uninit");
                    }
                }
                break;
            default:
                qDebug() << "Working Status/Param2: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x06 — INSTALLATION INFO
        // ======================================================
        case 0x06:
            switch (sub) {
            case 0x81: {
                if (payload.size() >= 10) {
                    auto read16BigEndian = [&](int offset) {
                        return (static_cast<uchar>(payload[offset]) << 8) | static_cast<uchar>(payload[offset + 1]);
                    };
                    double x_deg = read16BigEndian(4); // /100.0;
                    double y_deg = read16BigEndian(6); // /100.0;
                    double z_deg = read16BigEndian(8); // /100.0;
                    ui->leAngleXInstallation2->setText(QString::number(x_deg));
                    ui->leAngleYInstallation2->setText(QString::number(y_deg));
                    ui->leAngleZInstallation2->setText(QString::number(z_deg));
                    qDebug() << " 06 81 get angle2 =" << payload << " xyz " << x_deg << "-" << y_deg << "-" << z_deg;
                }
                break;
            }
            case 0x82: {
                if (payload.size() >= 6) {
                    uchar low = payload[4];
                    uchar high = payload[5];
                    qint16 height = (low << 8) | high;
                    ui->leHeightInstallation2->setText(QString::number(height));
                    qDebug() << " 06 82 get height2 =" << payload << " height " << height;
                }
                break;
            }
            case 0x01:{
                if (payload.size() >= 10) {
                    auto read16BigEndian = [&](int offset) {
                        return (static_cast<uchar>(payload[offset]) << 8) | static_cast<uchar>(payload[offset + 1]);
                    };
                    double x_deg = read16BigEndian(4);// / 100.0;
                    double y_deg = read16BigEndian(6);// / 100.0;
                    double z_deg = read16BigEndian(8);// / 100.0;
                    ui->leAngleXInstallation2->setText(QString::number(x_deg));
                    ui->leAngleYInstallation2->setText(QString::number(y_deg));
                    ui->leAngleZInstallation2->setText(QString::number(z_deg));
                    qDebug() << " 06 01 set angle2 =" << payload << " xyz " << x_deg << "-" << y_deg << "-" << z_deg;
                }
                break;
            }
            case 0x04:{
                if(payload.size() >= 8) {
                    qDebug() << " Hearth Beat2 " << payload;
                }
                break;
            }
            case 0x02:{
                if (payload.size() >= 6) {
                    uchar low = payload[4];
                    uchar high = payload[5];
                    qint16 height = (low << 8) | high;
                    ui->leHeightInstallation2->setText(QString::number(height));
                    qDebug() << " 06 02 set height reply2=" << payload << " height " << height;
                }
                break;
            }
            default:
                qDebug() << "Installation: Unknown subcmd2" << " payload " << payload;
                break;
            }
            break;

        // ======================================================
        // 0x07 — RADAR RANGE
        // ======================================================
        case 0x07:
            if (sub == 0x09 && payload.size() >= 14) {
                auto read16 = [&](int offset) {
                    return qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(payload.constData() + offset));
                };
                qDebug() << "Radar2 Range: X+:" << read16(6) << " X-:" << read16(8)
                         << " Y+:" << read16(10) << " Y-:" << read16(12);
            } else {
                qDebug() << "Radar2 Range: Unknown subcmd" << QString::number(sub,16);
            }
            break;

        // ======================================================
        // 0x80 — PRESENCE
        // ======================================================
        case 0x80:
            switch (sub) {
            case 0x00: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame2.presenceOnFunction = (val == 1);
                ui->lePresence->setText(val ? "ON" : "OFF");
                qDebug() << "80 00 Set Presence2 Activated =" << payload;
                break;
            }
            case 0x01:{
                qDebug() << "80 01 Presence2: Presence Info =" << static_cast<quint8>(payload[6]);
                quint8 val = static_cast<quint8>(payload[4]);
                ui->leMotion2->setStyleSheet(val ? "background-color2: blue; color: yellow;" : "background-color: green; color: black;");
                break;
            }
            case 0x02:{
                qDebug() << "80 02 Presence2: Motion Info =" << static_cast<quint8>(payload[6]);
                quint8 val = static_cast<quint8>(payload[4]);
                ui->leMotion2->setStyleSheet(val ? "background-color2: grey; color: black;" : "background-color: orange; color: black;");
                break;
            }
            case 0x03:{
                if (payload.size() >= 5) {
                    quint8 param = static_cast<quint8>(payload[4]);
                    ui->leMotion2->setText(QString::number(param));
                    drawRealTimeetsgram2(QString::number(param));
                    qDebug() << "80 03 Presence2: Motion Info =" << static_cast<quint8>(param);
                }
                break;
            }
            default:
                qDebug() << "80 Presence2: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        // ======================================================
        // 0x82 — RADAR FRAME DATA
        // ======================================================
        case 0x82:
            switch (sub){
            case 0x00:{  //command reply set trace tracking
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame2.traceTracking = (val == 1);
                ui->leTraceTracking2->setText(val ? "ON" : "OFF");
                qDebug() << "82 00 Reply2 of Trace Cmd =" << payload;
                break;
            }
            case 0x01:{  //trace number reply
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame2.traceNumber = (val == 1);
                ui->leTraceNumber2->setText(QString::number(val));
                qDebug() << "82 01 Trace2 Numbers of =" << payload;
                break;
            }
            case 0x02:{
                qDebug() << "82 02 Trace2 Tracking Info =" << payload.toHex(' ');

                auto read16s = [&](int offset) -> qint16 {
                    quint16 raw = (quint16(payload[offset]) << 8) |
                                  quint8(payload[offset + 1]);
                    return qint16(raw);
                };

                uint8_t  type_code = payload[0];
                uint8_t  subtype   = payload[1];
                uint16_t len       = read16s(2);

                int offset = 4;   // first target starts at 4
                const int targetSize = 11;

                int targetCount = len / targetSize;

                for (int i = 0; i < targetCount; i++)
                {
                    uint8_t index        = payload[offset + 0];
                    uint8_t target_size  = payload[offset + 1];
                    uint8_t target_feat  = payload[offset + 2];

                    qint16 x_pos        = read16s(offset + 3);
                    qint16 y_pos        = read16s(offset + 5);
                    qint16 height       = read16s(offset + 7);
                    qint16 velocity     = read16s(offset + 9);

                    // Filter invalid velocity
                    if (velocity <= -32000 || velocity >= 32000) {
                        velocity = 0;
                    }

                    // Filter invalid coordinates
                    if (x_pos <= -32000 || x_pos >= 32000 ||
                        y_pos <= -32000 || y_pos >= 32000)
                    {
                        qDebug() << "Invalid2 coordinate received, skipping" << x_pos << "-" << y_pos;;
                        offset += targetSize;
                        continue;   // skip update UI/plot untuk target ini
                    }

                    // Update UI & Plot
                    ui->leFallPosX2->setText(QString::number(x_pos));
                    ui->leFallPosY2->setText(QString::number(y_pos));
                    qDebug() << "Begin plotting...";
                    updateRadarPoint2(static_cast<double>(x_pos), static_cast<double>(y_pos));

                    drawRealTimeVelocity2(QString::number(velocity));
                    qDebug() << "end plotting...";

                    ui->leVelocity2->setText(QString::number(velocity));

                    qDebug() << "Target" << i
                             << "tc:" << targetCount
                             << "Index:" << index
                             << "Size:" << target_size
                             << "Feat:" << target_feat
                             << "X:" << x_pos
                             << "Y:" << y_pos
                             << "Z:" << height
                             << "Vel:" << velocity;

                    offset += targetSize;
                }
                break;
            }

            case 0x80:{ //
                qDebug() << "82 80 Trace2 Tracking Info =" << payload;
                break;
            }
            case 0x81:{
                qDebug() << "82 81 Trace2 Numbers =" << payload;
                break;
            }
            case 0x82:{
                qDebug() << "82 82 Trace2 Info query =" << payload;
                break;
            }
            default:
                qDebug() << "82 xx Trace2 Info query =" << payload;
                break;
            }
            break;

        // ======================================================
        // 0x83 — FALL DETECTION
        // ======================================================
        case 0x83:
            switch (sub) {
            case 0x00: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame2.traceTracking = (val == 1);
                ui->leFallDetection2->setText(val ? "ON" : "OFF");
                break;
            }
            case 0x01: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame2.fallState = (val == 1);
                ui->leFallState2->setStyleSheet(val ? "background-color: red; color: yellow;" : "background-color: green; color: black;");
                ui->leFallState2->setText(val ? "FALLEN" : "NOT FALLEN");
                qDebug() << "83 01 Fall =" << payload;
                if(val == 1){
                    sound.stop();
                    sound.play();
                    if(ui->cbSocket2->isChecked()){
                        m_socketQueue2.enqueue(SOCKET_REQ_FALL); //send to socket queue
                    }
                }
                break;
            }
            case 0x05: {
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame2.standStillState = (val == 1);
                ui->leFallState2->setStyleSheet(val ? "background-color: blue; color: black;" : "background-color: grey; color: black;");
                ui->leStandStill2->setText(val ? "EXIST" : "NO STAND STILL");
                qDebug() << "83 05 standstill2 =" << payload;
                break;
            }
            case 0x0B:{
                quint8 val = static_cast<quint8>(payload[4]);
                radarFrame2.standStillState = (val == 1);
                ui->leGetStandStill2->setText(val ? "ON" : "OFF");
                qDebug() << "83 0B standstill setting2 =" << payload;
                break;
            }
            case 0x0C:{
                if (payload.size() >= 8) {
                    quint8 duration = static_cast<quint8>(payload[7]);
                    qDebug() << "0C Fall: Duration Setting2 =" << duration;
                }
                break;
            }
            case 0x8C:{
                if (payload.size() >= 8) {
                    QByteArray durationBytes = payload.mid(4, 4);
                    quint32 duration = (static_cast<quint8>(durationBytes[0]) << 24) |
                                       (static_cast<quint8>(durationBytes[1]) << 16) |
                                       (static_cast<quint8>(durationBytes[2]) << 8)  |
                                       (static_cast<quint8>(durationBytes[3]));
                    ui->leFallDuration2->setText(QString::number(duration));
                    qDebug() << "8C Fall2: Duration Setting =" << duration;
                }
                break;
            }
            case 16: {//fall position
                if (payload.size() >= 8){   // safety

                    // Payload:
                    // [0] = 0x83
                    // [1] = 0x16
                    // [2] = length HB
                    // [3] = length LB
                    // [4] = X HB
                    // [5] = X LB
                    // [6] = Y HB
                    // [7] = Y LB

                    quint16 posX = (static_cast<quint8>(payload[4]) << 8) |
                                   static_cast<quint8>(payload[5]);

                    quint16 posY = (static_cast<quint8>(payload[6]) << 8) |
                                   static_cast<quint8>(payload[7]);

                    radarFrame2.fallPosX = posX;
                    radarFrame2.fallPosY = posY;

                    ui->leFallPosX2->setText(QString::number(posX));
                    ui->leFallPosY2->setText(QString::number(posY));

                    updateRadarPoint2(posX, posY);

                    qDebug() << "Fall Position -> X:" << posX << "Y:" << posY;
                }
                break;
            }
            case 17: {//fall cancel
                if (payload.size() >= 8){   // safety

                    // Payload:
                    // [0] = 0x83
                    // [1] = 0x16
                    // [2] = length HB
                    // [3] = length LB
                    // [4] = X HB
                    // [5] = X LB
                    // [6] = Y HB
                    // [7] = Y LB

                    quint16 posX = (static_cast<quint8>(payload[4]) << 8) |
                                   static_cast<quint8>(payload[5]);

                    quint16 posY = (static_cast<quint8>(payload[6]) << 8) |
                                   static_cast<quint8>(payload[7]);

                    radarFrame2.fallPosX = posX;
                    radarFrame2.fallPosY = posY;

                    ui->leFallPosX2->setText(QString::number(posX));
                    ui->leFallPosY2->setText(QString::number(posY));


                    updateRadarPoint2(posX, posY);

                    qDebug() << "Fall2 Cancel Position -> X:" << posX << "Y:" << posY;

                    //ui->leFallState->setStyleSheet("background-color: blue; color: black;");
                    ui->leFallState2->setStyleSheet("background-color: grey; color: yellow;");
                    ui->leFallState2->setText("FALLEN");
                    qDebug() << "83 17 Fall2 Cancel=" << payload;
                    sound2.stop();
                    sound2.play();
                }
                break;
            }
            default:
                qDebug() << "Fall2: Unknown subcmd" << QString::number(sub,16);
                break;
            }
            break;

        default:
            break;
        }
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::processFayloadSocket2()
{
    while (!m_socketQueue2.isEmpty()) {
        quint8 sockQueue = m_socketQueue2.dequeue();
        if (sockQueue == 0){ //no queue
            break;
        }else{ //queue available, process
            socketState2 = SOCKET_CONNECTING;
            ui->logEdit2->appendPlainText("q0. Queue rcv");
        }
    }

    switch (socketState2) {
    case SOCKET_IDDLE:{
        //ui->logEdit->appendPlainText("q1. queue iddle0");
        break;
    }
    case SOCKET_CONNECTING:{
        socketState2 = SOCKET_WAITING_CONNECTED;
        ui->logEdit2->appendPlainText("q2. queue connecting");
        socketIO_Client_Connect();
        break;
    }
    case SOCKET_WAITING_CONNECTED:{
        break;
    }
    case SOCKET_CONNECTED:{
        socketState2 = SOCKET_SENDING;
        ui->logEdit2->appendPlainText("q3. queue connected");
        break;
    }
    case SOCKET_SENDING:{
        socketState2 = SOCKET_WAITING_SENT;
        ui->logEdit2->appendPlainText("q4. queue sending");
        socketIO_Client_Write();
        break;
    }
    case SOCKET_WAITING_SENT:{
        ui->logEdit2->appendPlainText("q5. queue waiting sent");
        break;
    }
    case SOCKET_SENT:{
        socketState2 = SOCKET_CLOSING;
        ui->logEdit2->appendPlainText("q6. queue disconnected");
        socketIO_Client_Disconnect2();
        break;
    }
    case SOCKET_CLOSING:{
        socketState2 = SOCKET_IDDLE;
        //ui->logEdit->appendPlainText("q7. queue iddle1");
        break;
    }
    default:
        //ui->logEdit->appendPlainText("q8");
        break;
    }
}

void MainWindow::writeData2(const QByteArray &data)
{

}


//---------------------------------------------------------------------------------------
void MainWindow::fillPortsInfo()
{
    ui->serialPortInfoListBox->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        list << info.portName();
        ui->serialPortInfoListBox->addItem(list.first(), list);
    }

    ui->serialPortInfoListBox->addItem(tr("Custom"));
}

//---------------------------------------------------------------------------------------
void MainWindow::fillPortsInfo2()
{
    ui->serialPortInfoListBox2->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        list << info.portName();
        ui->serialPortInfoListBox2->addItem(list.first(), list);
    }

    ui->serialPortInfoListBox2->addItem(tr("Custom"));
}

//---------------------------------------------------------------------------------------
QByteArray MainWindow::makeFrame(const QByteArray &body)
{
    int s = 0;
    for (unsigned char c : body) s += c;
    unsigned char sum = s & 0xFF;
    QByteArray frame = body;
    frame.append(static_cast<char>(sum));
    frame.append(static_cast<char>(0x54));
    frame.append(static_cast<char>(0x43));
    return frame;
}

//---------------------------------------------------------------------------------------
QString MainWindow::toHexSpace(const QByteArray &data)
{
    QStringList parts;
    for (auto b : data) parts << QString("%1").arg((unsigned char)b, 2, 16, QChar('0')).toUpper();
    return parts.join(' ');
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnLoad_clicked()
{
    fillPortsInfo();
    ui->btnOpenSerialPort->setEnabled(true);

    //Test koordinat
    //updateRadarPoint(30, -45);
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnOpenSerialPort_clicked()
{
    if (ui->serialPortInfoListBox->currentText().isEmpty()) return;

#ifndef AUTOSTART_ONRPI
    init_port();
#endif
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnLoad2_clicked()
{
    fillPortsInfo2();
    ui->btnOpenSerialPort2->setEnabled(true);

    //Test koordinat
    //updateRadarPoint(30, -45);
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnOpenSerialPort2_clicked()
{
    if (ui->serialPortInfoListBox2->currentText().isEmpty()) return;

#ifndef AUTOSTART_ONRPI
    init_port2();
#endif
}

//---------------------------------------------------------------------------------------
quint8 MainWindow::calcChecksum(const QByteArray &frame)
{
    quint16 sum = 0;
    for (char c : frame)
        sum += static_cast<quint8>(c);
    return static_cast<quint8>(sum & 0xFF);
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetProductID_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_PRODUCT_ID);
    qDebug() << "Sending frame get Product Id:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetProductModel_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_PRODUCT_MODEL);
    qDebug() << "Sending frame get Production:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetFirmwareVersion_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_FIRMWARE_VERSION);
    qDebug() << "Sending frame get Firmware:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetProductID2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_PRODUCT_ID);
    qDebug() << "Sending frame get Product Id2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetProductModel2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_PRODUCT_MODEL);
    qDebug() << "Sending frame get Production2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetFirmwareVersion2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_FIRMWARE_VERSION);
    qDebug() << "Sending frame get Firmware2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnHWModel_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_HARDWARE_MODEL);
    qDebug() << "Sending frame HW Model:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnCmdInitCompleteCek_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_CEK_INITIALIZATION_COMPLETE);
    qDebug() << "Sending frame Cek Init Complete:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnHWModel2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_HARDWARE_MODEL);
    qDebug() << "Sending frame HW Model2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnCmdInitCompleteCek2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_CEK_INITIALIZATION_COMPLETE);
    qDebug() << "Sending frame Cek Init Complete2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetAngleInst_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_ANGLE_INST_QUERY);
    qDebug() << "Sending frame get Angle:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetHeightInst_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_HEIGHT_INST_QUERY);
    qDebug() << "Sending frame Get Height:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetAngleInst2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_ANGLE_INST_QUERY);
    qDebug() << "Sending frame get Angle2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetHeightInst2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_HEIGHT_INST_QUERY);
    qDebug() << "Sending frame Get Height:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbPresence_toggled(bool checked)
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_PRESENCE_ON) : makeFrame(CMD_SET_PRESENCE_OFF);
    qDebug() << "Sending frame set Presence:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbFallDetection_toggled(bool checked)
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_FALL_DETECTION_ON) : makeFrame(CMD_SET_FALL_DETECTION_OFF);
    qDebug() << "Sending frame Set Fall Duration:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetFallDuration_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_FALL_DURATION);
    qDebug() << "Sending frame Get Fall Duration:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbStandStill_toggled(bool checked)
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_STAND_STILLON) : makeFrame(CMD_SET_STAND_STILLOFF);
    qDebug() << "Sending frame StandStill:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbTraceTracking_toggled(bool checked)
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_TRACE_TRACKING_ON) : makeFrame(CMD_SET_TRACE_TRACKING_OFF);
    qDebug() << "Sending frame Trace Tracking:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbPresence2_toggled(bool checked)
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_PRESENCE_ON) : makeFrame(CMD_SET_PRESENCE_OFF);
    qDebug() << "Sending frame set Presence2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbFallDetection2_toggled(bool checked)
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_FALL_DETECTION_ON) : makeFrame(CMD_SET_FALL_DETECTION_OFF);
    qDebug() << "Sending frame Set Fall Duration2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnGetFallDuration2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = makeFrame(CMD_GET_FALL_DURATION);
    qDebug() << "Sending frame Get Fall Duration2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbStandStill2_toggled(bool checked)
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_STAND_STILLON) : makeFrame(CMD_SET_STAND_STILLOFF);
    qDebug() << "Sending frame StandStill2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_cbTraceTracking2_toggled(bool checked)
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);
    QByteArray frame = checked ? makeFrame(CMD_SET_TRACE_TRACKING_ON) : makeFrame(CMD_SET_TRACE_TRACKING_OFF);
    qDebug() << "Sending frame Trace Tracking2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnSetHeight_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);

    int height = ui->leSetHeight->text().toInt();
    QByteArray cmd = CMD_SET_HEIGHT;

    quint8 hb = static_cast<quint8>((height >> 8) & 0xFF);
    quint8 lb = static_cast<quint8>(height & 0xFF);

    cmd.append(static_cast<char>(hb));
    cmd.append(static_cast<char>(lb));

    QByteArray frame = makeFrame(cmd);
    qDebug() << "Sending frame cmd Height2:" << toHexSpace(frame);
    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnSetHeight2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);

    int height = ui->leSetHeight2->text().toInt();
    QByteArray cmd = CMD_SET_HEIGHT;

    quint8 hb = static_cast<quint8>((height >> 8) & 0xFF);
    quint8 lb = static_cast<quint8>(height & 0xFF);

    cmd.append(static_cast<char>(hb));
    cmd.append(static_cast<char>(lb));

    QByteArray frame = makeFrame(cmd);
    qDebug() << "Sending frame cmd Height2:" << toHexSpace(frame);
    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnSetFallDuration_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);

    uint32_t duration = ui->leSetFallDuration->text().toUInt();

    QByteArray cmd = CMD_SET_FALL_DURATION;

    // 4 byte (big-endian)
    quint8 b1 = static_cast<quint8>((duration >> 24) & 0xFF);
    quint8 b2 = static_cast<quint8>((duration >> 16) & 0xFF);
    quint8 b3 = static_cast<quint8>((duration >> 8) & 0xFF);
    quint8 b4 = static_cast<quint8>(duration & 0xFF);

    cmd.append(static_cast<char>(b1));
    cmd.append(static_cast<char>(b2));
    cmd.append(static_cast<char>(b3));
    cmd.append(static_cast<char>(b4));

    QByteArray frame = makeFrame(cmd);

    qDebug() << "Sending frame cmd Set Fall Duration:" << toHexSpace(frame);

    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnSetAngle_clicked()
{
    if (!m_serial) return;
    m_serial->clear(QSerialPort::Input);

    uint16_t angleX = ui->leAngleX->text().toUInt();
    uint16_t angleY = ui->leAngleY->text().toUInt();
    uint16_t angleZ = ui->leAngleZ->text().toUInt();

    QByteArray cmd = CMD_SET_ANGLE_INST;   // ini harus berisi 06 01 sesuai protokol kamu

    // X axis (2 byte big-endian)
    cmd.append(static_cast<char>((angleX >> 8) & 0xFF));   // HB
    cmd.append(static_cast<char>(angleX & 0xFF));          // LB

    // Y axis
    cmd.append(static_cast<char>((angleY >> 8) & 0xFF));
    cmd.append(static_cast<char>(angleY & 0xFF));

    // Z axis
    cmd.append(static_cast<char>((angleZ >> 8) & 0xFF));
    cmd.append(static_cast<char>(angleZ & 0xFF));

    // Generate full frame (prefix, length, checksum, suffix)
    QByteArray frame = makeFrame(cmd);

    qDebug() << "Sending frame SetAngle:" << toHexSpace(frame);

    m_serial->write(frame);
    m_serial->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnSetFallDuration2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);

    uint32_t duration = ui->leSetFallDuration->text().toUInt();

    QByteArray cmd = CMD_SET_FALL_DURATION;

    // 4 byte (big-endian)
    quint8 b1 = static_cast<quint8>((duration >> 24) & 0xFF);
    quint8 b2 = static_cast<quint8>((duration >> 16) & 0xFF);
    quint8 b3 = static_cast<quint8>((duration >> 8) & 0xFF);
    quint8 b4 = static_cast<quint8>(duration & 0xFF);

    cmd.append(static_cast<char>(b1));
    cmd.append(static_cast<char>(b2));
    cmd.append(static_cast<char>(b3));
    cmd.append(static_cast<char>(b4));

    QByteArray frame = makeFrame(cmd);

    qDebug() << "Sending frame cmd Set Fall Duration:" << toHexSpace(frame);

    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::on_btnSetAngle2_clicked()
{
    if (!m_serial2) return;
    m_serial2->clear(QSerialPort::Input);

    uint16_t angleX = ui->leAngleX2->text().toUInt();
    uint16_t angleY = ui->leAngleY2->text().toUInt();
    uint16_t angleZ = ui->leAngleZ2->text().toUInt();

    QByteArray cmd = CMD_SET_ANGLE_INST;   // ini harus berisi 06 01 sesuai protokol kamu

    // X axis (2 byte big-endian)
    cmd.append(static_cast<char>((angleX >> 8) & 0xFF));   // HB
    cmd.append(static_cast<char>(angleX & 0xFF));          // LB

    // Y axis
    cmd.append(static_cast<char>((angleY >> 8) & 0xFF));
    cmd.append(static_cast<char>(angleY & 0xFF));

    // Z axis
    cmd.append(static_cast<char>((angleZ >> 8) & 0xFF));
    cmd.append(static_cast<char>(angleZ & 0xFF));

    // Generate full frame (prefix, length, checksum, suffix)
    QByteArray frame = makeFrame(cmd);

    qDebug() << "Sending frame SetAngle2:" << toHexSpace(frame);

    m_serial2->write(frame);
    m_serial2->flush();
}

//---------------------------------------------------------------------------------------
void MainWindow::updateRadarPoint(double x, double y)
{
    radarPoint->data()->clear();
    radarPoint->addData(x, y);

    //ui->plotRadar->rescaleAxes();
    ui->plotRadar->replot();
}

//---------------------------------------------------------------------------------------
void MainWindow::init_radar()
{

}

//---------------------------------------------------------------------------------------
void MainWindow::updateRadarPoint2(double x, double y)
{
    radarPoint2->data()->clear();
    radarPoint2->addData(x, y);

    //ui->plotRadar2->rescaleAxes();
    ui->plotRadar2->replot();
}

//---------------------------------------------------------------------------------------
void MainWindow::setupPlotTs()
{
    // Add two graphs and configure appearance
    ui->plottsgram->addGraph();
    ui->plottsgram->graph(0)->setPen(QPen(Qt::yellow));
    ui->plottsgram->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    ui->plottsgram->addGraph();
    ui->plottsgram->graph(1)->setPen(QPen(Qt::red));

    // Configure top and right axes (ticks only, no labels)
    ui->plottsgram->xAxis2->setVisible(true);
    ui->plottsgram->xAxis2->setTickLabels(false);
    ui->plottsgram->yAxis2->setVisible(true);
    ui->plottsgram->yAxis2->setTickLabels(false);

    connect(ui->plottsgram, &QCustomPlot::afterReplot, [=]() {
        ui->plottsgram->xAxis2->setRange(ui->plottsgram->xAxis->range());
        ui->plottsgram->yAxis2->setRange(ui->plottsgram->yAxis->range());
    });

    // Enable interactions
    ui->plottsgram->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

}

//---------------------------------------------------------------------------------------
void MainWindow::setupPlotTsVelocity()
{
    // Add two graphs and configure appearance
    ui->plottsVelocity->addGraph();
    ui->plottsVelocity->graph(0)->setPen(QPen(Qt::yellow));
    ui->plottsVelocity->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    ui->plottsVelocity->addGraph();
    ui->plottsVelocity->graph(1)->setPen(QPen(Qt::red));

    // Configure top and right axes (ticks only, no labels)
    ui->plottsVelocity->xAxis2->setVisible(true);
    ui->plottsVelocity->xAxis2->setTickLabels(false);
    ui->plottsVelocity->yAxis2->setVisible(true);
    ui->plottsVelocity->yAxis2->setTickLabels(false);

    connect(ui->plottsVelocity, &QCustomPlot::afterReplot, [=]() {
        ui->plottsVelocity->xAxis2->setRange(ui->plottsVelocity->xAxis->range());
        ui->plottsVelocity->yAxis2->setRange(ui->plottsVelocity->yAxis->range());
    });

    // Enable interactions
    ui->plottsVelocity->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}

//---------------------------------------------------------------------------------------
void MainWindow::setupPlotTs2()
{
    // Add two graphs and configure appearance
    ui->plottsgram2->addGraph();
    ui->plottsgram2->graph(0)->setPen(QPen(Qt::yellow));
    ui->plottsgram2->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    ui->plottsgram2->addGraph();
    ui->plottsgram2->graph(1)->setPen(QPen(Qt::red));

    // Configure top and right axes (ticks only, no labels)
    ui->plottsgram2->xAxis2->setVisible(true);
    ui->plottsgram2->xAxis2->setTickLabels(false);
    ui->plottsgram2->yAxis2->setVisible(true);
    ui->plottsgram2->yAxis2->setTickLabels(false);

    connect(ui->plottsgram2, &QCustomPlot::afterReplot, [=]() {
        ui->plottsgram2->xAxis2->setRange(ui->plottsgram->xAxis->range());
        ui->plottsgram2->yAxis2->setRange(ui->plottsgram->yAxis->range());
    });

    // Enable interactions
    ui->plottsgram2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

}

//---------------------------------------------------------------------------------------
void MainWindow::setupPlotTsVelocity2()
{
    // Add two graphs and configure appearance
    ui->plottsVelocity2->addGraph();
    ui->plottsVelocity2->graph(0)->setPen(QPen(Qt::yellow));
    ui->plottsVelocity2->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    ui->plottsVelocity2->addGraph();
    ui->plottsVelocity2->graph(1)->setPen(QPen(Qt::red));

    // Configure top and right axes (ticks only, no labels)
    ui->plottsVelocity2->xAxis2->setVisible(true);
    ui->plottsVelocity2->xAxis2->setTickLabels(false);
    ui->plottsVelocity2->yAxis2->setVisible(true);
    ui->plottsVelocity2->yAxis2->setTickLabels(false);

    connect(ui->plottsVelocity2, &QCustomPlot::afterReplot, [=]() {
        ui->plottsVelocity2->xAxis2->setRange(ui->plottsVelocity->xAxis->range());
        ui->plottsVelocity2->yAxis2->setRange(ui->plottsVelocity->yAxis->range());
    });

    // Enable interactions
    ui->plottsVelocity->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}

//---------------------------------------------------------------------------------------
void MainWindow::realtimeDataSlot(QString value)
{
    static QTime timeStart = QTime::currentTime();
    double key = timeStart.msecsTo(QTime::currentTime()) / 1000.0; // seconds
    static double lastPointKey = 0;

    if (key - lastPointKey > 0.002) { // at most add point every 2 ms
        double filteredValue = value.toDouble();
        ui->plottsgram->graph(0)->addData(key, filteredValue);
        lastPointKey = key;
    }

    ui->plottsgram->xAxis->setRange(key, 8, Qt::AlignRight);
    ui->plottsgram->graph(0)->rescaleValueAxis(true);

    static QElapsedTimer replotTimer;
    if (!replotTimer.isValid()) replotTimer.start();

    if (replotTimer.elapsed() >= 50) { // Refresh setiap 50 ms
        ui->plottsgram->replot();
        replotTimer.restart();
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::realtimeDataVelocity(QString value)
{
    static QTime timeStart = QTime::currentTime();
    double key = timeStart.msecsTo(QTime::currentTime()) / 1000.0; // seconds
    static double lastPointKey = 0;

    if (key - lastPointKey > 0.002) { // at most add point every 2 ms
        double filteredValue = value.toDouble();
        ui->plottsVelocity->graph(0)->addData(key, filteredValue);
        lastPointKey = key;
    }

    ui->plottsVelocity->xAxis->setRange(key, 8, Qt::AlignRight);
    ui->plottsVelocity->graph(0)->rescaleValueAxis(true);

    static QElapsedTimer replotTimer;
    if (!replotTimer.isValid()) replotTimer.start();

    if (replotTimer.elapsed() >= 50) { // Refresh setiap 50 ms
        ui->plottsVelocity->replot();
        replotTimer.restart();
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::realtimeDataSlot2(QString value)
{
    static QTime timeStart = QTime::currentTime();
    double key = timeStart.msecsTo(QTime::currentTime()) / 1000.0; // seconds
    static double lastPointKey = 0;

    if (key - lastPointKey > 0.002) { // at most add point every 2 ms
        double filteredValue = value.toDouble();
        ui->plottsgram2->graph(0)->addData(key, filteredValue);
        lastPointKey = key;
    }

    ui->plottsgram2->xAxis->setRange(key, 8, Qt::AlignRight);
    ui->plottsgram2->graph(0)->rescaleValueAxis(true);

    static QElapsedTimer replotTimer;
    if (!replotTimer.isValid()) replotTimer.start();

    if (replotTimer.elapsed() >= 50) { // Refresh setiap 50 ms
        ui->plottsgram2->replot();
        replotTimer.restart();
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::realtimeDataVelocity2(QString value)
{
    static QTime timeStart = QTime::currentTime();
    double key = timeStart.msecsTo(QTime::currentTime()) / 1000.0; // seconds
    static double lastPointKey = 0;

    if (key - lastPointKey > 0.002) { // at most add point every 2 ms
        double filteredValue = value.toDouble();
        ui->plottsVelocity2->graph(0)->addData(key, filteredValue);
        lastPointKey = key;
    }

    ui->plottsVelocity2->xAxis->setRange(key, 8, Qt::AlignRight);
    ui->plottsVelocity2->graph(0)->rescaleValueAxis(true);

    static QElapsedTimer replotTimer;
    if (!replotTimer.isValid()) replotTimer.start();

    if (replotTimer.elapsed() >= 50) { // Refresh setiap 50 ms
        ui->plottsVelocity2->replot();
        replotTimer.restart();
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::drawRealTimeetsgram(QString motion)
{
    realtimeDataSlot(motion);
}

//---------------------------------------------------------------------------------------
void MainWindow::drawRealTimeVelocity(QString velocity)
{
    realtimeDataVelocity(velocity);
}

//---------------------------------------------------------------------------------------
void MainWindow::drawRealTimeetsgram2(QString motion)
{
    realtimeDataSlot2(motion);
}

//---------------------------------------------------------------------------------------
void MainWindow::drawRealTimeVelocity2(QString velocity)
{
    realtimeDataVelocity2(velocity);
}

//---------------------------------------------------------------------------------------
void MainWindow::setupRealtimeDataMotion(QCustomPlot *plottsgram)
{
    demoName = "Real Time Data Demo";

    plottsgram->setNotAntialiasedElements(QCP::aeAll);
    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    plottsgram->xAxis->setTickLabelFont(font);
    plottsgram->yAxis->setTickLabelFont(font);
    plottsgram->legend->setFont(font);

    plottsgram->addGraph();
    plottsgram->graph(0)->setPen(QPen(QColor(40, 255, 255)));

    plottsgram->setBackground(QBrush(QColor(30, 30, 30)));
    plottsgram->axisRect()->setBackground(QBrush(QColor(45, 45, 45)));

    QPen axisPen(QColor(250, 250, 250));
    QPen tickPen(QColor(250, 250, 250));
    QFont labelFont("Arial", 10);

    plottsgram->xAxis->setBasePen(axisPen);
    plottsgram->xAxis->setTickPen(tickPen);
    plottsgram->xAxis->setSubTickPen(tickPen);
    plottsgram->xAxis->setTickLabelColor(QColor(250, 250, 250));
    plottsgram->xAxis->setLabelColor(QColor(250, 250, 250));
    plottsgram->xAxis->setLabelFont(labelFont);

    plottsgram->yAxis->setBasePen(axisPen);
    plottsgram->yAxis->setTickPen(tickPen);
    plottsgram->yAxis->setSubTickPen(tickPen);
    plottsgram->yAxis->setTickLabelColor(QColor(250, 250, 250));
    plottsgram->yAxis->setLabelColor(QColor(250, 250, 250));
    plottsgram->yAxis->setLabelFont(labelFont);

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    plottsgram->xAxis->setRange(0, 10000);
    plottsgram->axisRect()->setupFullAxesBox();
    plottsgram->yAxis->setRange(0, 40);

    // Synchronize top/right axes with bottom/left
    //connect(plottsgram->xAxis, &QCPAxis::rangeChanged, plottsgram->xAxis2, &QCPAxis::setRange);
    //connect(plottsgram->yAxis, &QCPAxis::rangeChanged, plottsgram->yAxis2, &QCPAxis::setRange);
    // Synchronize top/right axes with bottom/left
    //plottsgram->xAxis->setRangeReplotPolicy(QCPAxis::rpImmediate);
    plottsgram->xAxis2->setRange(plottsgram->xAxis->range());
    plottsgram->yAxis2->setRange(plottsgram->yAxis->range());

    // Set update function saat plot direplot
    // Sinkronisasi top/right axes dengan bottom/left
    connect(plottsgram, &QCustomPlot::afterReplot, [=](){
        plottsgram->xAxis2->setRange(plottsgram->xAxis->range());
        plottsgram->yAxis2->setRange(plottsgram->yAxis->range());
    });

    // Vertical Text - left blank intentionally
}

//---------------------------------------------------------------------------------------
void MainWindow::setupRealtimeDataVelocity(QCustomPlot *plottsVelocity)
{
    demoName = "Real Time Data Demo";

    ui->plottsVelocity->setNotAntialiasedElements(QCP::aeAll);

    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    ui->plottsVelocity->xAxis->setTickLabelFont(font);
    ui->plottsVelocity->yAxis->setTickLabelFont(font);
    ui->plottsVelocity->legend->setFont(font);

    ui->plottsVelocity->addGraph();
    ui->plottsVelocity->graph(0)->setPen(QPen(QColor(40, 255, 255)));

    ui->plottsVelocity->setBackground(QBrush(QColor(30, 30, 30)));
    ui->plottsVelocity->axisRect()->setBackground(QBrush(QColor(45, 45, 45)));

    QPen axisPen(QColor(250, 250, 250));
    QPen tickPen(QColor(250, 250, 250));
    QFont labelFont("Arial", 10);

    ui->plottsVelocity->xAxis->setBasePen(axisPen);
    ui->plottsVelocity->xAxis->setTickPen(tickPen);
    ui->plottsVelocity->xAxis->setSubTickPen(tickPen);
    ui->plottsVelocity->xAxis->setTickLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity->xAxis->setLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity->xAxis->setLabelFont(labelFont);

    ui->plottsVelocity->yAxis->setBasePen(axisPen);
    ui->plottsVelocity->yAxis->setTickPen(tickPen);
    ui->plottsVelocity->yAxis->setSubTickPen(tickPen);
    ui->plottsVelocity->yAxis->setTickLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity->yAxis->setLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity->yAxis->setLabelFont(labelFont);

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->plottsVelocity->xAxis->setRange(0, 10000);
    ui->plottsVelocity->axisRect()->setupFullAxesBox();
    ui->plottsVelocity->yAxis->setRange(0, 40);


    ui->plottsVelocity->xAxis2->setRange(ui->plottsVelocity->xAxis->range());
    ui->plottsVelocity->yAxis2->setRange(ui->plottsVelocity->yAxis->range());

    // Set update function saat plot direplot
    // Sinkronisasi top/right axes dengan bottom/left
    connect(ui->plottsVelocity, &QCustomPlot::afterReplot, [=](){
        ui->plottsVelocity->xAxis2->setRange(ui->plottsVelocity->xAxis->range());
        ui->plottsVelocity->yAxis2->setRange(ui->plottsVelocity->yAxis->range());
    });

    // Vertical Text - left blank intentionally
}

//---------------------------------------------------------------------------------------
void MainWindow::setupPlotRadar(QCustomPlot *plotRadar)
{
    //-------------------------- Inisialisasi plotRadar --------------------------
    ui->plotRadar->xAxis->setVisible(true);   // tampilkan axes
    ui->plotRadar->yAxis->setVisible(true);
    ui->plotRadar->xAxis->setRange(-500, 500);
    ui->plotRadar->yAxis->setRange(-500, 500);

    // Grid kotak-kotak putus-putus
    ui->plotRadar->xAxis->grid()->setVisible(true);
    ui->plotRadar->yAxis->grid()->setVisible(true);
    ui->plotRadar->xAxis->grid()->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    ui->plotRadar->yAxis->grid()->setPen(QPen(Qt::gray, 1, Qt::DashLine));

    // Bikin garis utama di tengah (X=0 dan Y=0)
    QCPItemStraightLine *xLine = new QCPItemStraightLine(ui->plotRadar);
    xLine->point1->setCoords(0, -500);
    xLine->point2->setCoords(0, 500);
    xLine->setPen(QPen(Qt::gray, 1, Qt::DashLine));

    QCPItemStraightLine *yLine = new QCPItemStraightLine(ui->plotRadar);
    yLine->point1->setCoords(-500, 0);
    yLine->point2->setCoords(500, 0);
    yLine->setPen(QPen(Qt::gray, 1, Qt::DashLine));

    // Bikin graph untuk titik radar
    radarPoint = ui->plotRadar->addGraph();
    radarPoint->setLineStyle(QCPGraph::lsNone);
    radarPoint->setScatterStyle(QCPScatterStyle(
        QCPScatterStyle::ssCircle,
        QPen(Qt::red),
        QBrush(Qt::red),
        8
        ));
}


//---------------------------------------------------------------------------------------
void MainWindow::setupRealtimeDataMotion2(QCustomPlot *plottsgram2)
{
    demoName = "Real Time Data Demo";

    plottsgram2->setNotAntialiasedElements(QCP::aeAll);
    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    plottsgram2->xAxis->setTickLabelFont(font);
    plottsgram2->yAxis->setTickLabelFont(font);
    plottsgram2->legend->setFont(font);

    plottsgram2->addGraph();
    plottsgram2->graph(0)->setPen(QPen(QColor(40, 255, 255)));

    plottsgram2->setBackground(QBrush(QColor(30, 30, 30)));
    plottsgram2->axisRect()->setBackground(QBrush(QColor(45, 45, 45)));

    QPen axisPen(QColor(250, 250, 250));
    QPen tickPen(QColor(250, 250, 250));
    QFont labelFont("Arial", 10);

    plottsgram2->xAxis->setBasePen(axisPen);
    plottsgram2->xAxis->setTickPen(tickPen);
    plottsgram2->xAxis->setSubTickPen(tickPen);
    plottsgram2->xAxis->setTickLabelColor(QColor(250, 250, 250));
    plottsgram2->xAxis->setLabelColor(QColor(250, 250, 250));
    plottsgram2->xAxis->setLabelFont(labelFont);

    plottsgram2->yAxis->setBasePen(axisPen);
    plottsgram2->yAxis->setTickPen(tickPen);
    plottsgram2->yAxis->setSubTickPen(tickPen);
    plottsgram2->yAxis->setTickLabelColor(QColor(250, 250, 250));
    plottsgram2->yAxis->setLabelColor(QColor(250, 250, 250));
    plottsgram2->yAxis->setLabelFont(labelFont);

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    plottsgram2->xAxis->setRange(0, 10000);
    plottsgram2->axisRect()->setupFullAxesBox();
    plottsgram2->yAxis->setRange(0, 40);

    // Synchronize top/right axes with bottom/left
    //connect(plottsgram->xAxis, &QCPAxis::rangeChanged, plottsgram->xAxis2, &QCPAxis::setRange);
    //connect(plottsgram->yAxis, &QCPAxis::rangeChanged, plottsgram->yAxis2, &QCPAxis::setRange);
    // Synchronize top/right axes with bottom/left
    //plottsgram->xAxis->setRangeReplotPolicy(QCPAxis::rpImmediate);
    plottsgram2->xAxis->setRange(plottsgram2->xAxis2->range());
    plottsgram2->yAxis->setRange(plottsgram2->yAxis2->range());

    // Set update function saat plot direplot
    // Sinkronisasi top/right axes dengan bottom/left
    connect(plottsgram2, &QCustomPlot::afterReplot, [=](){
        plottsgram2->xAxis->setRange(plottsgram2->xAxis->range());
        plottsgram2->yAxis->setRange(plottsgram2->yAxis->range());
    });

    // Vertical Text - left blank intentionally
}

//---------------------------------------------------------------------------------------
void MainWindow::setupRealtimeDataVelocity2(QCustomPlot *plottsVelocity2)
{
    demoName = "Real Time Data Demo";

    ui->plottsVelocity2->setNotAntialiasedElements(QCP::aeAll);

    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    ui->plottsVelocity2->xAxis->setTickLabelFont(font);
    ui->plottsVelocity2->yAxis->setTickLabelFont(font);
    ui->plottsVelocity2->legend->setFont(font);

    ui->plottsVelocity2->addGraph();
    ui->plottsVelocity2->graph(0)->setPen(QPen(QColor(40, 255, 255)));

    ui->plottsVelocity2->setBackground(QBrush(QColor(30, 30, 30)));
    ui->plottsVelocity2->axisRect()->setBackground(QBrush(QColor(45, 45, 45)));

    QPen axisPen(QColor(250, 250, 250));
    QPen tickPen(QColor(250, 250, 250));
    QFont labelFont("Arial", 10);

    ui->plottsVelocity2->xAxis->setBasePen(axisPen);
    ui->plottsVelocity2->xAxis->setTickPen(tickPen);
    ui->plottsVelocity2->xAxis->setSubTickPen(tickPen);
    ui->plottsVelocity2->xAxis->setTickLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity2->xAxis->setLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity2->xAxis->setLabelFont(labelFont);

    ui->plottsVelocity2->yAxis->setBasePen(axisPen);
    ui->plottsVelocity2->yAxis->setTickPen(tickPen);
    ui->plottsVelocity2->yAxis->setSubTickPen(tickPen);
    ui->plottsVelocity2->yAxis->setTickLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity2->yAxis->setLabelColor(QColor(250, 250, 250));
    ui->plottsVelocity2->yAxis->setLabelFont(labelFont);

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->plottsVelocity2->xAxis->setRange(0, 10000);
    ui->plottsVelocity2->axisRect()->setupFullAxesBox();
    ui->plottsVelocity2->yAxis->setRange(0, 40);


    ui->plottsVelocity2->xAxis->setRange(ui->plottsVelocity2->xAxis->range());
    ui->plottsVelocity2->yAxis->setRange(ui->plottsVelocity2->yAxis->range());

    // Set update function saat plot direplot
    // Sinkronisasi top/right axes dengan bottom/left
    connect(ui->plottsVelocity2, &QCustomPlot::afterReplot, [=](){
        ui->plottsVelocity2->xAxis->setRange(ui->plottsVelocity2->xAxis->range());
        ui->plottsVelocity2->yAxis->setRange(ui->plottsVelocity2->yAxis->range());
    });

    // Vertical Text - left blank intentionally
}

//---------------------------------------------------------------------------------------
void MainWindow::setupPlotRadar2(QCustomPlot *plotRadar2)
{
    //-------------------------- Inisialisasi plotRadar --------------------------
    ui->plotRadar2->xAxis->setVisible(true);   // tampilkan axes
    ui->plotRadar2->yAxis->setVisible(true);
    ui->plotRadar2->xAxis->setRange(-500, 500);
    ui->plotRadar2->yAxis->setRange(-500, 500);

    // Grid kotak-kotak putus-putus
    ui->plotRadar2->xAxis->grid()->setVisible(true);
    ui->plotRadar2->yAxis->grid()->setVisible(true);
    ui->plotRadar2->xAxis->grid()->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    ui->plotRadar2->yAxis->grid()->setPen(QPen(Qt::gray, 1, Qt::DashLine));

    // Bikin garis utama di tengah (X=0 dan Y=0)
    QCPItemStraightLine *xLine = new QCPItemStraightLine(ui->plotRadar);
    xLine->point1->setCoords(0, -500);
    xLine->point2->setCoords(0, 500);
    xLine->setPen(QPen(Qt::gray, 1, Qt::DashLine));

    QCPItemStraightLine *yLine = new QCPItemStraightLine(ui->plotRadar);
    yLine->point1->setCoords(-500, 0);
    yLine->point2->setCoords(500, 0);
    yLine->setPen(QPen(Qt::gray, 1, Qt::DashLine));

    // Bikin graph untuk titik radar
    radarPoint2 = ui->plotRadar2->addGraph();
    radarPoint2->setLineStyle(QCPGraph::lsNone);
    radarPoint2->setScatterStyle(QCPScatterStyle(
        QCPScatterStyle::ssCircle,
        QPen(Qt::red),
        QBrush(Qt::red),
        8
        ));
}

//------------------------------------------------------------------------
void MainWindow::socketIO_Client_Prepare()
{
    // Event connected
    connect(ws, &QWebSocket::connected, this, [this]() {
        ui->logEdit->appendPlainText("Connected to server!");
        isConnected = true;

        // Join namespace /radar setelah connected
        ws->sendTextMessage("40");
        ui->logEdit->appendPlainText("Sent namespace connect: 40/radar");
    });

    // Event textMessageReceived
    connect(ws, &QWebSocket::textMessageReceived, this, [this](const QString &msg){
        ui->logEdit->appendPlainText("Received: " + msg);

        // -------------------------------
        // 1. Balas ping Engine.IO otomatis
        if(msg == "2") {           // ping dari server
            ws->sendTextMessage("3"); // pong
            ui->logEdit->appendPlainText("Sent pong: 3");
            return;
        }

        // -------------------------------
        // 2. Deteksi ack namespace connect
        if(msg.startsWith("40")) {
            namespaceConnected = true;
            ui->logEdit->appendPlainText("Namespace connected, ready to send payload");
            socketState = SOCKET_CONNECTED;
        }

        // -------------------------------
        // 3. Bisa tambahkan parser event lain nanti
        // Misal: 42/radar,[...] ΓåÆ event server
        if(msg.startsWith("42")) {
            ui->logEdit->appendPlainText("Payload received: " + msg.mid(9)); // skip "42/radar"
            socketState = SOCKET_SENT;
        }
    });


    // Event error
    connect(ws, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError err){
        Q_UNUSED(err);
        ui->logEdit->appendPlainText("WebSocket error: " + ws->errorString());
    });
}

//------------------------------------------------------------------------
void MainWindow::socketIO_Client_Connect()
{
    if(!isConnected) {
        //ws->open(QUrl("ws://192.168.1.100:3000/socket.io/?EIO=4&transport=websocket"));
        //ws->open(QUrl("ws://localhost:3000/radar/?EIO=4&transport=websocket"));
        ws->open(QUrl("ws://localhost:3000/socket.io/?EIO=4&transport=websocket"));
        ui->logEdit->appendPlainText("Opening WebSocket...");
    } else {
        ui->logEdit->appendPlainText("Already connected");
    }
}

//------------------------------------------------------------------------
void MainWindow::socketIO_Client_Write()
{
    if(isConnected && namespaceConnected) {
        //QString payload = "42[\"navigateCommand\",\"/fall\"]";
        ws->sendTextMessage("42[\"navigateCommand\",\"/fall\"]");
        //ws->sendTextMessage(payload);
        ui->logEdit->appendPlainText("Sent payload: ");// + payload);
    } else if(!isConnected) {
        ui->logEdit->appendPlainText("Not connected yet!");
    } else {
        ui->logEdit->appendPlainText("Namespace not connected yet!");
    }
}

//------------------------------------------------------------------------
void MainWindow::socketIO_Client_Disconnect()
{
    if(isConnected) {

        ui->logEdit->appendPlainText("Sending namespace disconnect...");

        // 1. Kirim paket disconnect namespace
        // Socket.IO format: 41/<namespace>
        ws->sendTextMessage("41/radar");

        ui->logEdit->appendPlainText("Closing WebSocket...");

        // 2. Tutup koneksi WebSocket
        ws->close();

        // 3. Reset status
        isConnected = false;
        namespaceConnected = false;

        ui->logEdit->appendPlainText("Disconnected.");
    }
    else {
        ui->logEdit->appendPlainText("Already disconnected.");
    }
}

//------------------------------------------------------------------------
void MainWindow::socketIO_Client_Disconnect2()
{
    if(isConnected) {

        ui->logEdit2->appendPlainText("Sending namespace disconnect...");

        // 1. Kirim paket disconnect namespace
        // Socket.IO format: 41/<namespace>
        ws->sendTextMessage("41/radar");

        ui->logEdit2->appendPlainText("Closing WebSocket...");

        // 2. Tutup koneksi WebSocket
        ws->close();

        // 3. Reset status
        isConnected = false;
        namespaceConnected = false;

        ui->logEdit2->appendPlainText("Disconnected.");
    }
    else {
        ui->logEdit2->appendPlainText("Already disconnected.");
    }
}

//------------------------------------------------------------------------
void MainWindow::on_btnTestFall_clicked()
{
    if(ui->cbSocket->isChecked()){
       m_socketQueue.enqueue(SOCKET_REQ_FALL); //send to socket queue
       ui->logEdit->appendPlainText("Test FALL");
    }
}


//------------------------------------------------------------------------
void MainWindow::on_btnTestFall2_clicked()
{
    if(ui->cbSocket2->isChecked()){
        m_socketQueue2.enqueue(SOCKET_REQ_FALL); //send to socket queue
        ui->logEdit2->appendPlainText("Test FALL");
    }
}


