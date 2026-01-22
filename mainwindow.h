#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVector>
#include <QRandomGenerator>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QDir>
#include <QListWidget>
#include <QQueue>
#include <QtEndian>
#include <QtMultimedia/QSoundEffect>
//QSoundEffect sound;
#include <QScreen>   // Qt6 pengganti QDesktopWidget
#include "qcustomplot.h"
#include <QWebSocket>
#include <QTimer>
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QWebSocket>

//#define AUTOSTART_ONRPI 1

#define CMD_GET_PRODUCT_MODEL    QByteArray::fromHex("535902A100010F") //53	59	02	A1	00	01	0F
#define CMD_GET_PRODUCT_ID       QByteArray::fromHex("535902A200010F") //53	59	02	A2	00	01	0F
#define CMD_GET_HARDWARE_MODEL   QByteArray::fromHex("535902A300010F") //53	59	02	A3	00	01	0F
#define CMD_GET_FIRMWARE_VERSION QByteArray::fromHex("535902A400010F") //53	59	02	A4	00	01	0F

#define CMD_CEK_INITIALIZATION_COMPLETE QByteArray::fromHex("5359058100010F") //53	59	05	81	00	01	0F
#define CMD_GET_ANGLE_INST_QUERY        QByteArray::fromHex("5359068100010F") //53	59	06	81	00	01	0f
#define CMD_SET_ANGLE_INST              QByteArray::fromHex("535906010006")  //53	59	06	01	00	06	2Byte-X + 2ByteY +2ByteZ sum	54	43
#define CMD_GET_HEIGHT_INST_QUERY       QByteArray::fromHex("5359068200010F") //53	59	06	82	00	01	0F

#define CMD_SET_PRESENCE_ON  QByteArray::fromHex("53598000000101") //53	59	80	00	00	01	00
#define CMD_SET_PRESENCE_OFF QByteArray::fromHex("53598000000100") //53	59	80	00	00	01	01

#define CMD_SET_FALL_DETECTION_ON  QByteArray::fromHex("53598300000101") //53	59	83	00	00	01	01
#define CMD_SET_FALL_DETECTION_OFF QByteArray::fromHex("53598300000100") //53	59	83	00	00	01	00

#define CMD_GET_FALL_DURATION QByteArray::fromHex("5359838C00010F") //53 59 83	8C	00	01	0F
#define CMD_SET_FALL_DURATION QByteArray::fromHex("5359830C0004")   //53 59	83	0C	00	04	duration sum 54	43

#define CMD_SET_HEIGHT QByteArray::fromHex("535906020002") //53	59	06	02	00	02 2ByteHeight

#define CMD_SET_STAND_STILLON     QByteArray::fromHex("5359830B000101") //53 59 83	0B	00	01	01
#define CMD_SET_STAND_STILLOFF    QByteArray::fromHex("5359830B000100") //53 59 83	0B	00	01	00

#define CMD_SET_TRACE_TRACKING_ON  QByteArray::fromHex("53598200000101") //53 59	82	00	00	01	01	sum	54	43
#define CMD_SET_TRACE_TRACKING_OFF QByteArray::fromHex("53598200000100") //53 59	82	00	00	01	00	sum	54	43

#define CMD_SET_MOTION_INFO_REPORTING_NONE "535980020001002F5443"
#define CMD_SET_MOTION_INFO_REPORTING_STATIONARY "53598002000101305443"
#define CMD_SET_MOTION_INFO_REPORTING_ACTIVE "53598002000102315443"
#define CMD_TEST "5359820200160001000023003900C780000101008040006F00C60002"


// ==============================
// Radar Frame Struct
// ==============================
struct FrameRadarData {
    quint8 motion;
    quint16 len;
    quint16 target;
    quint16 x;
    quint16 y;
    quint16 amp;
    quint16 flag1;
    quint16 flag2;
    quint16 extra1;
    quint16 extra2;
    quint16 extra3;
    int16_t fallPosX;
    int16_t fallPosY;

    bool presenceOnFunction;
    bool fallDetectionFunction;
    bool standStillFunction;

    bool fallState;
    bool standStillState;

    bool traceTracking;
    quint8 traceNumber;
};

enum SOCKET_STATE{
    SOCKET_IDDLE,
    SOCKET_CONNECTING,
    SOCKET_WAITING_CONNECTED,
    SOCKET_CONNECTED,

    SOCKET_SENDING,
    SOCKET_WAITING_SENT,
    SOCKET_SENT,
    SOCKET_CLOSING
};

enum SOCKET_REQUEST{
    SOCKET_NULL,
    SOCKET_REQ__RADAR_ERROR,
    SOCKET_REQ_FALL,
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setupRealtimeDataMotion(QCustomPlot *customPlot);
    void setupRealtimeDataVelocity(QCustomPlot *customPlot);
    void setupPlotRadar(QCustomPlot *customPlot);

public slots:
    //void updatePlot(const QVector<double> &values);   //

private slots:
    void init_port();
    void init_port(QString portname);
    void readData();

    // Qt6: SerialPortError → SerialError
    void handleError(QSerialPort::SerialPortError error);

    void closeSerialPort();
    void showPortInfo(int idx);
    void processPayload();
    void processFayloadSocket();
    void writeData(const QByteArray &data);

    void on_btnLoad_clicked();
    void on_btnOpenSerialPort_clicked();

    void on_btnGetProductID_clicked();
    void on_btnGetProductModel_clicked();
    void on_btnGetFirmwareVersion_clicked();

    void realtimeDataSlot(QString value);
    void realtimeDataVelocity(QString value);

    void on_btnHWModel_clicked();
    void on_btnCmdInitCompleteCek_clicked();

    void on_btnGetAngleInst_clicked();
    void on_btnGetHeightInst_clicked();

    void on_cbPresence_toggled(bool checked);
    void on_cbFallDetection_toggled(bool checked);
    void on_btnGetFallDuration_clicked();
    void on_cbStandStill_toggled(bool checked);
    void on_cbTraceTracking_toggled(bool checked);

    void on_btnSetHeight_clicked();

    void on_btnSetFallDuration_clicked();
    void on_btnSetAngle_clicked();

    void on_btnTestFall_clicked();

    void on_cbSocket_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    QString demoName;

    QWebSocket *ws;
    bool isConnected = false;
    bool namespaceConnected = false;

    float radarX = 0;
    float radarY = 0;

    QCPGraph *radarPoint;
    void updateRadarPoint(double x, double y);
    void init_radar();

    QSerialPort *m_serial = nullptr;
    QByteArray m_buffer;

    QQueue<QByteArray> m_payloadQueue;    
    QQueue<quint8> m_socketQueue;
    quint8 socketState;

    QTimer *m_processTimer;
    QTimer *m_socketTimer;
    QTimer *m_initRadarTimer;

    //QCustomPlot *m_plot;

    FrameRadarData radarFrame;

    quint8 calcChecksum(const QByteArray &frame);
    void fillPortsInfo();
    QByteArray makeFrame(const QByteArray &body);
    QString toHexSpace(const QByteArray &data);

    QSoundEffect sound;

    void setupPlotTs();
    void setupPlotTsVelocity();

    void drawRealTimeetsgram(QString motion);
    void drawRealTimeVelocity(QString velocity);

    void socketIO_Client_Prepare();
    void socketIO_Client_Connect();
    void socketIO_Client_Write();
    void socketIO_Client_Disconnect();
};

#endif // MAINWINDOW_H
