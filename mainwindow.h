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
#include <radar.h>
#include <socketioclient.h>

//#define AUTOSTART_ONRPI 1

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

    void setupRealtimeDataMotion(QCustomPlot *plottsgram);
    void setupRealtimeDataVelocity(QCustomPlot *plottsVelocity);
    void setupPlotRadar(QCustomPlot *plotRadar);

    void setupRealtimeDataMotion2(QCustomPlot *plottsgram2);
    void setupRealtimeDataVelocity2(QCustomPlot *plottsVelocity2);
    void setupPlotRadar2(QCustomPlot *plotRadar2);

public slots:
    //void updatePlot(const QVector<double> &values);   //

private slots:
    void init_port();
    void init_port(QString portname);
    void readData();

    void init_port2();
    void init_port2(QString portname);
    void readData2();

    // Qt6: SerialPortError → SerialError
    void handleError(QSerialPort::SerialPortError error);
    void handleError2(QSerialPort::SerialPortError error);

    void closeSerialPort();
    void showPortInfo(int idx);
    void processPayload();
    void processFayloadSocket();
    void writeData(const QByteArray &data);

    void closeSerialPort2();
    void showPortInfo2(int idx);
    void processPayload2();
    void processFayloadSocket2();
    void writeData2(const QByteArray &data);

    void on_btnLoad_clicked();
    void on_btnOpenSerialPort_clicked();

    void on_btnLoad2_clicked();
    void on_btnOpenSerialPort2_clicked();

    void on_btnGetProductID_clicked();
    void on_btnGetProductModel_clicked();
    void on_btnGetFirmwareVersion_clicked();

    void on_btnGetProductID2_clicked();
    void on_btnGetProductModel2_clicked();
    void on_btnGetFirmwareVersion2_clicked();

    void realtimeDataSlot(QString value);
    void realtimeDataVelocity(QString value);

    void realtimeDataSlot2(QString value);
    void realtimeDataVelocity2(QString value);

    void on_btnHWModel_clicked();
    void on_btnCmdInitCompleteCek_clicked();

    void on_btnHWModel2_clicked();
    void on_btnCmdInitCompleteCek2_clicked();

    void on_btnGetAngleInst_clicked();
    void on_btnGetHeightInst_clicked();

    void on_btnGetAngleInst2_clicked();
    void on_btnGetHeightInst2_clicked();

    void on_cbPresence_toggled(bool checked);
    void on_cbFallDetection_toggled(bool checked);
    void on_btnGetFallDuration_clicked();
    void on_cbStandStill_toggled(bool checked);
    void on_cbTraceTracking_toggled(bool checked);

    void on_cbPresence2_toggled(bool checked);
    void on_cbFallDetection2_toggled(bool checked);
    void on_btnGetFallDuration2_clicked();
    void on_cbStandStill2_toggled(bool checked);
    void on_cbTraceTracking2_toggled(bool checked);

    void on_btnSetHeight_clicked();
    void on_btnSetHeight2_clicked();

    void on_btnSetFallDuration_clicked();
    void on_btnSetAngle_clicked();

    void on_btnSetFallDuration2_clicked();
    void on_btnSetAngle2_clicked();

    void on_btnTestFall_clicked();
    void on_btnTestFall2_clicked();

private:
    Ui::MainWindow *ui;
    QString demoName;
    //QString demoName2;


    QWebSocket *ws;
    bool isConnected = false;
    bool namespaceConnected = false;

    float radarX = 0;
    float radarY = 0;

    float radar2X = 0;
    float radar2Y = 0;

    QCPGraph *radarPoint;
    void updateRadarPoint(double x, double y);
    void init_radar();

    QCPGraph *radarPoint2;
    void updateRadarPoint2(double x, double y);
    //void init_radar2();

    QSerialPort *m_serial = nullptr;
    QByteArray m_buffer;

    QSerialPort *m_serial2 = nullptr;
    QByteArray m_buffer2;


    QQueue<QByteArray> m_payloadQueue;    
    QQueue<quint8> m_socketQueue;
    quint8 socketState;

    QQueue<QByteArray> m_payloadQueue2;
    QQueue<quint8> m_socketQueue2;
    quint8 socketState2;

    QTimer *m_processTimer;
    QTimer *m_socketTimer;
    QTimer *m_initRadarTimer;

    QTimer *m_processTimer2;
    QTimer *m_socketTimer2;
    QTimer *m_initRadarTimer2;

    //QCustomPlot *m_plot;

    FrameRadarData radarFrame;
    FrameRadarData radarFrame2;

    quint8 calcChecksum(const QByteArray &frame);

    void fillPortsInfo();
    void fillPortsInfo2();

    QByteArray makeFrame(const QByteArray &body);
    QString toHexSpace(const QByteArray &data);

    QSoundEffect sound;
    QSoundEffect sound2;

    void setupPlotTs();
    void setupPlotTsVelocity();

    void setupPlotTs2();
    void setupPlotTsVelocity2();

    void drawRealTimeetsgram(QString motion);
    void drawRealTimeVelocity(QString velocity);

    void drawRealTimeetsgram2(QString motion);
    void drawRealTimeVelocity2(QString velocity);

    void socketIO_Client_Prepare();
    void socketIO_Client_Connect();
    void socketIO_Client_Write();

    void socketIO_Client_Disconnect();
    void socketIO_Client_Disconnect2();

};

#endif // MAINWINDOW_H
