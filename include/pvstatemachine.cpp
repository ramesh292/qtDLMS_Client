#include "pvstatemachine.h"
#include <QAbstractState>
#include <QState>
#include <QDebug>

PVStateMachine::PVStateMachine(QObject *parent)
    : QStateMachine{parent}
{
//    connect(this, &PVStateMachine::started, [this]() {
////        qDebug() << tag << "********************** SM Started ******************";
//        setState("started");
//    });

    connect(this, &PVStateMachine::finished, [this]() {
//         qDebug() << tag << "********* SM Stoped StarteClicked";
         setState("finished");
    });
}

void PVStateMachine::addState(QState *state, QString stateName)
{
    state->assignProperty(this, "state", stateName);
    QStateMachine::addState(state);
}

void PVStateMachine::addState(QFinalState *state)
{
    QStateMachine::addState(state);
}

void PVStateMachine::start()
{
    setState("started");
    QStateMachine::start();
}

void PVStateMachine::setState(QString state)
{
    mLastState = mState;
    mState = state;
    emit stateChanged();
}

QString PVStateMachine::state()
{
    return mState;
}

QString PVStateMachine::lastState()
{
    return mLastState;
}

void PVStateMachine::removeAllStates()
{
    QList<QAbstractState*> statesToRemove = findChildren<QAbstractState*>();
    for (QAbstractState* state : statesToRemove) {
        removeState(state);
        delete state; // Optional if you want to delete the state object
    }
}
