#include "taskclass.h"
#include "contactclass.h"
#include "mainwindow.h"
#include "abstractedsmsclass.h"
#include "SerialInterfaceClass.h"

TaskClass::TaskClass(QWidget *parent) : QMainWindow(parent)
{
    m_kill = false;
	SmsTextStuckWd = new QTimer;
	hytTextStuckWd = new QTimer;
	SmsTextStuckWd->stop();
	SmsTextStuckWd->setSingleShot(true);
	connect(SmsTextStuckWd, SIGNAL(timeout()), this, SLOT(WDsmsStuck()));
	hytTextStuckWd->stop();
	hytTextStuckWd->setSingleShot(true);
	connect(hytTextStuckWd, SIGNAL(timeout()), this, SLOT(WDhytStuck()));
}

TaskClass::~TaskClass()
{

}

int TaskClass::PCScommand(TaskData commandAndData,ContactClass *phoneBook)
{
    qDebug() << commandAndData.protocol;
    qDebug() << "PCS... group access from task??";
    phoneBook->getContactsForGroup(commandAndData.messageGroup);
    return 1;

}

int TaskClass::SMScommand(AbstractedSmsClass *smsDevice, TaskData commandAndData, ContactClass *phoneBook)
{
    m_kill = false;
    QStack<QString> callList =  phoneBook->getContactsForGroup(commandAndData.messageGroup);
    // now call the numbers!!!
    QString name;
    QString number;
    while (!m_kill)
    {
        if (callList.size() > 0)
        {
            // need to take the number in the string and text it...
            QString numberName = callList.pop();
            int nameEndsAt = numberName.indexOf(',');
            if (nameEndsAt > -1)
            {
                name = numberName.mid(0,nameEndsAt);
                number = numberName.mid((nameEndsAt+1),numberName.length());
                qDebug() << number << " " << name;
                if (commandAndData.messageBody == BODY_SOURCE)
		            smsDevice->sendText(number.toUtf8().data(), commandAndData.panelText.toUtf8().data());
                else
		            smsDevice->sendText(number.toUtf8().data(), commandAndData.messageBody.toUtf8().data());
				// block here until text sent
	            // need to keep an eye that we dont stay for ever
	            SmsTextStuckWd->start(TEXT_FAIL_TIME);
	            m_abortText = false;
	            while (!smsDevice->isTextCycleFinished())
	            {
		            if (m_abortText) smsDevice->killText();   
	            }
	            SmsTextStuckWd->stop();
            }
        }
        else
            m_kill = true;
    }
    return 1;
}

int TaskClass::HYTcommand(SerialInterfaceClass *hytDevice, TaskData commandAndData, ContactClass *phoneBook)
{
	qDebug() << "Hyt Command Execution called";
	qDebug() << commandAndData.messageBody + " " << commandAndData.messageGroup << " " << commandAndData.panelText;
	m_kill = false;
	QStack<QString> callList = phoneBook->getContactsForGroup(commandAndData.messageGroup);
	// now call the numbers!!!
	QString name;
	QString number;
	while (!m_kill)
	{
		if (callList.size() > 0)
		{
			// need to take the number in the string and text it...
			QString numberName = callList.pop();
			int nameEndsAt = numberName.indexOf(',');
			if (nameEndsAt > -1)
			{
				name = numberName.mid(0, nameEndsAt);
				number = numberName.mid((nameEndsAt + 1), numberName.length());
				qDebug() << number << " " << name;

				if (commandAndData.messageBody == BODY_SOURCE)
					hytDevice->sendHYTmessage(number, commandAndData.panelText);
				else
					hytDevice->sendHYTmessage(number, commandAndData.messageBody);
				// block here until text sent
				// need to keep an eye that we dont stay for ever
				hytTextStuckWd->start(HYT_TEXT_FAIL_TIME);
				m_abortText = false;
				while (!hytDevice->isTextCycleFinished())
				{
					if (m_abortText) hytDevice->killText();
				}
				hytTextStuckWd->stop();
			}
		}
		else
			m_kill = true;
	}
	return 1;
}

void TaskClass::killTask()
{
    m_kill = true;
}


void TaskClass::WDsmsStuck(void)
{
	// text never completed sending;
	m_abortText = true;
	qDebug() << "Aborting Current Text --- Modem unresponsive!";
}

void TaskClass::WDhytStuck(void)
{
	// text never completed sending;
	m_abortText = true;
	qDebug() << "Aborting Current Text --- Hytera unresponsive!";
}

int TaskClass::DLYcommand(QString delayInterval)
{
	// put the thread to sleep? check its within limits
	int delay = delayInterval.toInt();
	if (delay > MAX_DELAY)
		delay = MAX_DELAY;
	sleep(delay);
	return 1;
}