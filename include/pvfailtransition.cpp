#include "pvfailtransition.h"
#include <QStateMachine>
#include <QDebug>

PVFailTransition::PVFailTransition(const QObject *object, const char *signal, int timeoutDuration, int retryCount, QObject *parent) :
    QSignalTransition(object, signal)
{
    passCounter = 0;
    this->retryCount = retryCount;

    timer = new QTimer(this);
    timer->setInterval(timeoutDuration);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    connect(this, SIGNAL(fail()), object, signal);
}

void PVFailTransition::startTimer()
{
    timer->start();
    if(lastDateTime.msecsTo(QDateTime::currentDateTime()) > 200)
        passCounter = 0;
}

void PVFailTransition::stopTimer()
{
    timer->stop();
    lastDateTime = QDateTime::currentDateTime();
}

void PVFailTransition::timeout()
{
    qDebug() << "PVFailTransition::timeout passCounter retryCount" << passCounter << retryCount;
    if(++passCounter < retryCount){
        emit retry();
    }
    else
        emit fail();
}

bool PVFailTransition::eventTest(QEvent *e)
{
    if (!QSignalTransition::eventTest(e))
        return false;

    if(++passCounter < retryCount){
        qDebug() << "passCounter" << passCounter;
        QTimer::singleShot(1000, this, SIGNAL(retry()));
        return false;
    }

    passCounter = 0;
    return true;
}
