#include "scenarothread.h"
#include "taskclass.h"
#include "contactclass.h"
#include "mainwindow.h"

struct TaskData;

ScenarioThread::ScenarioThread():QThread()
{
    m_text = "Created";
    m_stop = false;
    m_loop = 0;
    taskControl = new TaskClass;
    m_isCurrentlyLatched = false;
}

ScenarioThread::~ScenarioThread()
{
    delete taskControl;
}

void ScenarioThread::run()
{
    // if currently latched do not run scenario
    if (!m_isCurrentlyLatched)
    {
        // is this a latchable scenario?
        if (m_latchable)
            m_isCurrentlyLatched = true;

        scenarioMutex.lock();
        runningState = true;
        m_stop = false;
        qDebug() << "running scenario " << m_name;
        emit sendScenarioName(m_name);
        // first thing we do is emit a reset signal to any group that cares
        emit scenarioIssuingReset(m_resetsGroup);
        qDebug() <<"Emitted a reset: " << m_resetsGroup;
        for(int i = 0; i <taskVector.size(); i++)
        {
            // need a var to break this loop
            // for cancelling or pausing tasks
            if (m_stop)
            {
                i = taskVector.size();
                break;
            }

            TaskData poppedTask = taskVector[i];

            if ((poppedTask.protocol == "#PCS#") & !m_stop)
            {
                taskControl->PCScommand(poppedTask,pcsContactRef);
                sleep(2);
            }
            else if  ((poppedTask.protocol == "#SMS#") & !m_stop)
            {
                taskControl->SMScommand(poppedTask,smsContactRef);
            }


            // task is finished
        }
        m_stop = true;
        qDebug() << "No More tasks";
        // let everyone know scenario is finished
        emit notifyScenarioFinished();
        scenarioMutex.unlock();
        runningState = false;
    }
}



void ScenarioThread::stopThread()
{
    qDebug() << "FORCED!!! --- Stopped --- ";
    m_stop = true;
    m_loop = 0;
}

void ScenarioThread::setName(QString name)
{
    m_name = name;
}

QString ScenarioThread::getName()
{
    return m_name;
}

void ScenarioThread::cleanObject()
{
    m_text = "";
    m_name = "";
    taskVector.clear();
}

// setters and getters

int ScenarioThread::getPriority()
{
    return priority;
}

void ScenarioThread::setPriority(int p)
{
    priority = p;
}

bool ScenarioThread::isRunning()
{
    return runningState;
}


void ScenarioThread::setHighPriorityKill(bool b)
{
    hihPriorityKills = b;
}

void ScenarioThread::setHighPriorityStores(bool b)
{
    highPriorityStores = b;
}



bool ScenarioThread::doesHighPriorityKill()
{
    return hihPriorityKills;
}

bool ScenarioThread::doesHighPriorityStore()
{
    return highPriorityStores;
}

bool ScenarioThread::getIfLatchable()
{
    return m_latchable;
}

QChar ScenarioThread::getResetByGroup()
{
    return m_isResetBy;
}

QChar ScenarioThread::getResetsGroup()
{
    return m_resetsGroup;
}
bool ScenarioThread::isCurrentlyLatched() const
{
    return m_isCurrentlyLatched;
}

void ScenarioThread::setIsCurrentlyLatched(bool isCurrentlyLatched)
{
    m_isCurrentlyLatched = isCurrentlyLatched;
}


void ScenarioThread::setResetsGroup(QChar c)
{
    m_resetsGroup = c;
}

void ScenarioThread::setResetBy(QChar c)
{
    m_isResetBy = c;
}

void ScenarioThread::setIfLatchable(bool b)
{
    m_latchable = b;
}

// slots



