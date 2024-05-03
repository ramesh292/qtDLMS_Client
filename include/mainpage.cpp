#include "mainpage.h"
#include "ui_mainpage.h"
#include <QDebug>

mainPage::mainPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainPage)
{
    ui->setupUi(this);

    connect(&serialPort, SIGNAL(readyRead()), SLOT(packetGenerate()));
    connect(&stateMachineDLMS, SIGNAL(stateChanged()), this, SLOT(printState()));


    tag = "DLMS";


    QString str1 =  "password";

    US_Client = *new CGXDLMSSecureClient(true, 0x30, 0x1, DLMS_AUTHENTICATION_HIGH, str1.toLocal8Bit().constData(), DLMS_INTERFACE_TYPE_HDLC);
    PC_Client = *new CGXDLMSClient(true, 0x10, 0x01,DLMS_AUTHENTICATION_NONE,NULL,DLMS_INTERFACE_TYPE_HDLC);

    invocationSNRMRequest = new PVState();
    invocationAARQRequest= new PVState();
    invocationOBJData = new PVState();
    invocationLastStage = new PVState();
    secureSNRMRequest = new PVState();
    secureAARQRequest = new PVState();
    secureAuthenticate = new PVState();
    finishDLMS = new QFinalState();
    failed_US_Client = new QFinalState();
    failed_PC_Client = new QFinalState();

    connect(invocationSNRMRequest, SIGNAL(entered()), this, SLOT(sendSNRMrequestInvo()));
    invocationSNRMRequest->addTransition(this, SIGNAL(invocationSNRMRequestPass()), invocationAARQRequest);
    invocationSNRMRequest->addFailTransition(this, SIGNAL(dlmsTestFail()), failed_PC_Client);

    connect(invocationAARQRequest, SIGNAL(entered()), this, SLOT(sendAARQrequestInvo()));
    invocationAARQRequest->addTransition(this, SIGNAL(invocationAARQRequestPass()), invocationOBJData);
    invocationAARQRequest->addFailTransition(this, SIGNAL(dlmsTestFail()), failed_PC_Client);

    connect(invocationOBJData, SIGNAL(entered()), this, SLOT(sendObjDataInvo()));
    invocationOBJData->addTransition(this, SIGNAL(invocationobjDataPass()), invocationLastStage);
    invocationOBJData->addFailTransition(this, SIGNAL(dlmsTestFail()), failed_PC_Client);

    connect(invocationLastStage, SIGNAL(entered()), this, SLOT(getInvocation()));
    invocationLastStage->addTransition(this, SIGNAL(invocationLastStagePass()), secureSNRMRequest);
    invocationLastStage->addFailTransition(this, SIGNAL(dlmsTestFail()), failed_PC_Client, 3000, 2);

    connect(secureSNRMRequest, SIGNAL(entered()), this, SLOT(sendSecureSNRMRequest()));
    secureSNRMRequest->addTransition(this, SIGNAL(secureSNRMRequestPass()), secureAARQRequest);
    secureSNRMRequest->addFailTransition(this, SIGNAL(dlmsTestFail()), failed_US_Client);

    connect(secureAARQRequest, SIGNAL(entered()), this, SLOT(sendSecureAARQRequest()));
    secureAARQRequest->addTransition(this, SIGNAL(secureAARQRequestPass()), secureAuthenticate);
    secureAARQRequest->addFailTransition(this, SIGNAL(dlmsTestFail()), failed_US_Client);

    connect(secureAuthenticate, SIGNAL(entered()), this, SLOT(fetchAuthentication()));
    secureAuthenticate->addTransition(this, SIGNAL(secureAuthenticatePass()), finishDLMS);
    secureAuthenticate->addFailTransition(this, SIGNAL(dlmsTestFail()), failed_US_Client);


    connect(&stateMachineDLMS, SIGNAL(finished()), this, SLOT(processStateMachineStoped()));

    stateMachineDLMS.addState(invocationSNRMRequest, "invocationSNRMRequest");
    stateMachineDLMS.addState(invocationAARQRequest, "invocationAARQRequest");
    stateMachineDLMS.addState(invocationOBJData, "invocationOBJData");
    stateMachineDLMS.addState(invocationLastStage, "invocationLastStage");
    stateMachineDLMS.addState(secureSNRMRequest, "secureSNRMRequest");
    stateMachineDLMS.addState(secureAARQRequest, "secureAARQRequest");
    stateMachineDLMS.addState(secureAuthenticate, "secureAuthenticate");
    stateMachineDLMS.addState(finishDLMS);
    stateMachineDLMS.addState(failed_US_Client);
    stateMachineDLMS.setInitialState(invocationSNRMRequest);
    stateMachineDLMS.addState(failed_PC_Client);
}

mainPage::~mainPage()
{
    delete ui;
}



void mainPage::printState()
{
    qDebug() << tag << "evt" << QDateTime::currentDateTime() << time.msecsTo(QDateTime::currentDateTime()) << "state" << stateMachineDLMS.lastState() << "->" << stateMachineDLMS.state() ;
    if(stateMachineDLMS.lastState() == stateMachineDLMS.state())
    {
    emit dlmsTestFail();

    }
    time = QDateTime::currentDateTime();
}

void mainPage::processStateMachineStoped()
{
    std::vector<CGXByteBuffer> packets;
    serialPort.close();
    if(stateMachineDLMS.configuration().contains(finishDLMS)){
        US_Client.DisconnectRequest(packets);
    }
    else if(stateMachineDLMS.configuration().contains(failed_US_Client)){
        US_Client.DisconnectRequest(packets);
    }
    else if(stateMachineDLMS.configuration().contains(failed_PC_Client)){
        PC_Client.ReleaseRequest(packets);
    }
    packetSend(packets);

}


void mainPage::serialPortopen(QString port){
    serialPort.setPortName(port);
    serialPort.setBaudRate(QSerialPort::Baud9600);
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setFlowControl(QSerialPort::NoFlowControl);

    if(serialPort.open(QIODevice::ReadWrite)){
        qDebug() << tag << "serialPort open: true";
    }else{
        qDebug() << tag << "serialPort open false";
    }
}

void mainPage::packetReceived(QByteArray packet){

    qDebug() << tag << "packet received" << packet.toHex();
    qDebug() << tag << "packet length:" << packet.length();
    if(stateMachineDLMS.configuration().contains(invocationSNRMRequest)){
        serial_data.Set(packet.data(),packet.size());
        emit invocationSNRMRequestPass();

    }
    else if(stateMachineDLMS.configuration().contains(invocationAARQRequest)){
        serial_data.Set(packet.data(),packet.size());
        emit invocationAARQRequestPass();

    }
    else if(stateMachineDLMS.configuration().contains(invocationOBJData)){
        serial_data.Set(packet.data(),packet.size());
        emit invocationobjDataPass();

    }
    else if(stateMachineDLMS.configuration().contains(secureSNRMRequest)){
        serial_data.Set(packet.data(),packet.size());
        emit secureSNRMRequestPass();

    }
    else if(stateMachineDLMS.configuration().contains(secureAARQRequest)){
        serial_data.Set(packet.data(),packet.size());
        emit secureAARQRequestPass();

    }
    else if(stateMachineDLMS.configuration().contains(secureAuthenticate)){
        serial_data.Set(packet.data(),packet.size());
        US_Client.GetData(serial_data, replyData, notify);
        US_Client.ParseApplicationAssociationResponse(replyData.GetData());
        replyData.Clear();
        replyData.GetData().Clear();
        notify.Clear();
        emit secureAuthenticatePass();
    } if(stateMachineDLMS.configuration().contains(finishDLMS)){
//        US_Client.DisconnectRequest(packets);
        emit fetchValuePass();
    }
    else if(stateMachineDLMS.configuration().contains(failed_US_Client)){
//        US_Client.DisconnectRequest(packets);
        emit fetchValueFail();
    }
    else if(stateMachineDLMS.configuration().contains(failed_PC_Client)){
//        PC_Client.ReleaseRequest(packets);
        emit fetchValueFail();
    }
}

void mainPage::packetGenerate(){
    while(serialPort.bytesAvailable()){
        auto tmp = serialPort.readAll();
        receiverBuffer.append(tmp);

        if(receiverBuffer.size()>=3 && receiverBuffer.size() == receiverBuffer.at(2) + 2){

            if(receiverBuffer.endsWith(char(0x7e)) && receiverBuffer.size() >= 6){
                QByteArray data;
                data = receiverBuffer;
                receiverBuffer = "";
                emit packetReceived(data);
            }else{
                receiverBuffer = "";
                //                emit finishdlms();
                emit dlmsTestFail();
                qDebug() << tag << "****************** TestFail";
            }
        }
    }


}

void mainPage::release_US_Channel()
{
    std::vector<CGXByteBuffer> packets;
    US_Client.DisconnectRequest(packets);
    packetSend(packets);
    emit fetchValueFail();
}

void mainPage::release_PC_Channel()
{
    std::vector<CGXByteBuffer> sendpackets;
    PC_Client.ReleaseRequest(sendpackets);
    packetSend(sendpackets);
    emit fetchValueFail();
}

void mainPage::testPassed()
{
    std::vector<CGXByteBuffer> packets;
    US_Client.DisconnectRequest(packets);
    packetSend(packets);
    emit fetchValuePass();
}

void mainPage::getInvocation(){
    PC_Client.GetData(serial_data, replyData, notify);
    CGXDLMSVariant dlmsVariant;
    dlmsVariant = replyData.GetValue();
    if (dlmsVariant.IsNumber()) {
        invo_value = dlmsVariant.ToDouble();
        qDebug() << tag << "Value extracted from CGXDLMSVariant:" << invo_value;
        emit invocationLastStagePass();
    } else {
        emit dlmsTestFail();
        qDebug() << tag << "CGXDLMSVariant does not contain a valid quint32 value.";
    }
}

void mainPage::sendObjDataInvo(){
    replyData.Clear();
    std::vector<CGXByteBuffer> sendpackets;
    PC_Client.GetData(serial_data, replyData, notify);
    PC_Client.ParseAAREResponse(replyData.GetData());
    replyData.Clear();
    CGXDLMSData* obj_data = new CGXDLMSData("0.0.255.144");// location where data to be read.
    PC_Client.Read(obj_data, 2,sendpackets);
    packetSend(sendpackets);
}

void mainPage::sendAARQrequestInvo(){
    replyData.Clear();
    std::vector<CGXByteBuffer> sendpackets;
    PC_Client.GetData(serial_data, replyData, notify);
    PC_Client.ParseUAResponse(replyData.GetData());
    PC_Client.AARQRequest(sendpackets);
    packetSend(sendpackets);
}

void mainPage::sendSNRMrequestInvo(){
    std::vector<CGXByteBuffer> sendpackets;
    PC_Client.SNRMRequest(sendpackets);
    packetSend(sendpackets);
}


void mainPage::sendSecureSNRMRequest(){
    US_Client.GetCiphering()->SetInvocationCounter(invo_value);
    US_Client.GetCiphering()->SetSecurity(DLMS_SECURITY_AUTHENTICATION_ENCRYPTION);
    QString inputString = "12345678"; // The input QByteArray
    QByteArray byteArray;
    CGXByteBuffer systemTitle;
    for (const QChar &c : inputString) {
        byteArray.append(static_cast<char>(c.toLatin1())); // Convert to ASCII values
    }
    systemTitle.Set(byteArray.data(), byteArray.size());
    US_Client.GetCiphering()->SetSystemTitle(systemTitle);
    inputString = "Data1";
    QByteArray byteArray1;
    for (const QChar &c : inputString) {
        byteArray1.append(static_cast<char>(c.toLatin1())); // Convert to ASCII values
    }
    CGXByteBuffer blockCipherKey;
    blockCipherKey.Set(byteArray1.data(), byteArray1.size());
    CGXByteBuffer AuthenticationKey = blockCipherKey;

    US_Client.GetCiphering()->SetBlockCipherKey(blockCipherKey);
    US_Client.GetCiphering()->SetAuthenticationKey(AuthenticationKey);
    std::vector<CGXByteBuffer> reply;
    US_Client.SNRMRequest(reply);
    packetSend(reply);
}

void mainPage::sendSecureAARQRequest(){
    US_Client.GetData(serial_data, replyData, notify);
    US_Client.ParseUAResponse(replyData.GetData());
    replyData.Clear();
    replyData.GetData().Clear();
    notify.Clear();
    std::vector<CGXByteBuffer> reply;
    US_Client.AARQRequest(reply);
    US_Client.GetCiphering()->SetInvocationCounter(US_Client.GetCiphering()->GetInvocationCounter()+1);
    qDebug()<<"Invocation counter"<<US_Client.GetCiphering()->GetInvocationCounter();
    packetSend(reply);
}

void mainPage::fetchAuthentication(){
    US_Client.GetData(serial_data, replyData, notify);
    US_Client.ParseAAREResponse(replyData.GetData());
    DLMS_CONFORMANCE currentConformance = US_Client.GetNegotiatedConformance();
    currentConformance = static_cast<DLMS_CONFORMANCE>(currentConformance & ~DLMS_CONFORMANCE_GENERAL_BLOCK_TRANSFER);
    US_Client.SetNegotiatedConformance(currentConformance);
    replyData.Clear();
    replyData.GetData().Clear();
    notify.Clear();
    if (US_Client.IsAuthenticationRequired())
    {
        US_Client.GetCiphering()->SetInvocationCounter(US_Client.GetCiphering()->GetInvocationCounter()+1);
        std::vector<CGXByteBuffer> reply;
        US_Client.GetApplicationAssociationRequest(reply);
        packetSend(reply);
    }else{
        emit dlmsTestFail();
    }
}


void mainPage::packetSend(std::vector<CGXByteBuffer> sendpackets){
    for (const CGXByteBuffer& databytes : sendpackets) {
        QByteArray dataReceived;
        CGXByteBuffer data = databytes;
        QByteArray byteArray(reinterpret_cast<const char*>(data.GetData()), data.GetSize());
        qDebug() << tag << "packetSend" << byteArray.toHex();
        serialPort.write(byteArray);
    }
}




void mainPage::on_dlmsConnect_clicked()
{
    if(serialPort.isOpen())
        serialPort.close();

    serialPortopen(portName);
    stateMachineDLMS.start();
    qDebug() << tag << "Test dlms start";

}

