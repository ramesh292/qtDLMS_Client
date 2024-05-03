#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>
#include "GXDLMSData.h"
#include "GXReplyData.h"
#include "GXDLMSNotify.h"
#include "GXDLMSSecureClient.h"
#include <QByteArray>
#include <QSerialPort>
#include <QTimer>
#include "QState"
#include "QStateMachine"
#include "qfinalstate.h"
#include "pvstatemachine.h"
#include "pvstate.h"



namespace Ui {
class mainPage;
}

class mainPage : public QWidget
{
    Q_OBJECT

public:
    explicit mainPage(QWidget *parent = nullptr);
    ~mainPage();


private slots:
    void packetReceived(QByteArray packet);
    void sendAARQrequestInvo();
    void sendObjDataInvo();
    void getInvocation();
    void serialPortopen(QString port);
    void packetSend(std::vector<CGXByteBuffer> sendpackets);
    void sendSNRMrequestInvo();
    void sendSecureSNRMRequest();
    void sendSecureAARQRequest();
    void fetchAuthentication();
    void packetGenerate();
    void release_US_Channel();
    void release_PC_Channel();
    void testPassed();
    void printState();


    void processStateMachineStoped();

    void on_dlmsConnect_clicked();

signals:
    void fetchValuePass();
    void fetchValueFail();

    void dlmsTestFail();
    void fetchRFkeyPass();
    void fetchChAddrPass();
    void fetchNwAddrPass();
    void entered();
    void invocationSNRMRequestPass();
    void invocationAARQRequestPass();
    void invocationobjDataPass();
    void invocationLastStagePass();
    void secureSNRMRequestPass();
    void secureAARQRequestPass();
    void secureAuthenticatePass();

public:
    CGXReplyData notify;
    CGXReplyData replyData;
    QSerialPort serialPort;

private:
    quint32 invo_value;
    CGXByteBuffer serial_data;
    QString tag;
    QDateTime time;
    QString portName;

    PVState *invocationSNRMRequest;
    PVState *invocationAARQRequest;
    PVState *invocationOBJData;
    PVState *invocationLastStage;
    PVState *secureSNRMRequest;
    PVState *secureAARQRequest;
    PVState *secureAuthenticate;
    QFinalState *finishDLMS = new QFinalState();
    QFinalState *failed_US_Client = new QFinalState();
    QFinalState *failed_PC_Client = new QFinalState();

    QByteArray receiverBuffer;
    PVStateMachine stateMachineDLMS;
    quint8 choice = 0;

    CGXDLMSSecureClient US_Client;
    CGXDLMSClient PC_Client;

private:
    Ui::mainPage *ui;
};

#endif // MAINPAGE_H
