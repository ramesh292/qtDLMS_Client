#include "pvstate.h"

PVState::PVState(QState *parent) : QState(parent)
{
    t1 = nullptr;
}

void PVState::addFailTransition(const QObject *sender, const char *signal, QAbstractState *target, int timeoutDuration, int retry)
{
    connect(this, SIGNAL(entered()), this, SLOT(stateEntered()));
    connect(this, SIGNAL(exited()), this, SLOT(stateExited()));

    t1 = new PVFailTransition(sender, signal, timeoutDuration, retry);
    t1->setTargetState(target);
    addTransition(t1); //in case of fail got to t1 state

    addTransition(t1, SIGNAL(retry()), this); //before going t1state do ntimes retry by entering same state again.
}

void PVState::stateEntered()
{
    if(t1 != nullptr)
        t1->startTimer();
}

void PVState::stateExited()
{
    if(t1 != nullptr)
        t1->stopTimer();
}
