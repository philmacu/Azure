#include "scenarothread.h"
#include "taskclass.h"
#include "contactclass.h"
#include "mainwindow.h"
#include "abstractedsmsclass.h"
#include "SerialInterfaceClass.h"

struct TaskData;

ScenarioThread::ScenarioThread():QThread()
{
    m_text = "Created";
    m_stop = false;
    m_loop = 0;
    taskControl = new TaskClass;
    m_isCurrentlyLatched = false;
	connect(taskControl, SIGNAL(tastaskText(QString)), this, SLOT(catchTaskText(QString)));
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
        emit sendScenarioTaskInfo(m_name);
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
			qDebug() << "Task Protocol: " << poppedTask.protocol;
            if ((poppedTask.protocol == "#PCS#") & !m_stop)
            {
				emit sendScenarioTaskInfo("---> Paging: " + (poppedTask.messageBody) +
					" : To group " + (poppedTask.messageGroup));
                taskControl->PCScommand(poppedTask,pcsContactRef);
				emit  sendScenarioTaskInfo("--->Page Sent To Group ");
                sleep(2);
            }
            else if  ((poppedTask.protocol == "#SMS#") & !m_stop)
            {
				poppedTask.panelText = panelText; // this was loaded when fire alarm went off via MainWindow
				if (poppedTask.messageBody == BODY_SOURCE)
					emit sendScenarioTaskInfo("---> Sending a Text: " + (poppedTask.panelText) +
					" : To group " + (poppedTask.messageGroup));
				else
					emit sendScenarioTaskInfo("---> Sending a Text: " + (poppedTask.messageBody) +
					" : To group " + (poppedTask.messageGroup));
                taskControl->SMScommand(smsDevice, poppedTask,smsContactRef);
				emit  sendScenarioTaskInfo("-> Text Sent To Group.");
            }
			else if ((poppedTask.protocol == "#HYT#") & !m_stop)
			{
				poppedTask.panelText = panelText; // this was loaded when fire alarm went off via MainWindow
				if (poppedTask.messageBody == BODY_SOURCE)
					emit sendScenarioTaskInfo("---> Sending To Radio: " + (poppedTask.panelText) +
					" : To group " + (poppedTask.messageGroup));
				else
					emit sendScenarioTaskInfo("---> Sending To Radio: " + (poppedTask.messageBody) +
					" : To group " + (poppedTask.messageGroup));
				taskControl->HYTcommand(hytDevice, poppedTask, hytContactRef);
				emit  sendScenarioTaskInfo("-> Radio Message Sent.");
			}
			else if ((poppedTask.protocol == "#DLY#") & !m_stop)
			{
				emit sendScenarioTaskInfo("---> Starting a Delay of: " + poppedTask.messageBody + "s");
				taskControl->DLYcommand(poppedTask.messageBody);
				//int delay = poppedTask.messageBody.toInt();
				//if (delay > MAX_DELAY)
				//	delay = MAX_DELAY;
				//sleep(delay);
				emit sendScenarioTaskInfo("-> Delay Finished.");
				emit pleaseLogThis("Delay of " + poppedTask.messageBody + "s Finished");
			}
			//else if ((poppedTask.protocol == "#AUD#") & !m_stop)
			//{
			//	poppedTask.panelText = panelText; // this was loaded when fire alarm went off via MainWindow
			//	if (poppedTask.messageBody == BODY_SOURCE)
			//		emit sendScenarioTaskInfo("---> Sending To Radio: " + (poppedTask.panelText) +
			//		" : To group " + (poppedTask.messageGroup));
			//	else
			//		emit sendScenarioTaskInfo("---> Sending To Radio: " + (poppedTask.messageBody) +
			//		" : To group " + (poppedTask.messageGroup));
			//	taskControl->HYTcommand(hytDevice, poppedTask, hytContactRef);
			//	emit  sendScenarioTaskInfo("-> Radio Message Sent.");
			//}
   //         // task is finished
        }
        m_stop = true;
		emit sendScenarioTaskInfo(m_name + " has finished.");
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

void ScenarioThread::setSerialBoilerText(QString s)
{
	m_serialBoilerText = s;
}

QString ScenarioThread::getSerialBoilerText(void)
{
	return m_serialBoilerText;
}

bool ScenarioThread::getCanBePaused(void)
{
	return m_canBePaused;
}

void ScenarioThread::setCanBePaused(bool b)
{
	m_canBePaused = b;
}

// slots



void ScenarioThread::linkScenarioToMultipleDevices(AbstractedSmsClass *sms)
{
	// will copy pointers to the interfaces
	smsDevice = sms;
}

void ScenarioThread::linkScenarioToMultipleDevices(SerialInterfaceClass *hyt)
{
	// will copy pointers to the interfaces
	hytDevice = hyt;
}