/*
 * Calls and runs the actuall interface to the hardware
 *
 *
 */


#ifndef TASKCLASS_H
#define TASKCLASS_H

#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QTimer>

#include <iostream>
#include <string.h>
#include <string>

using namespace std;

#define BODY_SOURCE "USE RS232"
#define TEXT_FAIL_TIME 15000
#define HYT_TEXT_FAIL_TIME 30000

class ContactClass;
class AbstractedSmsClass;
class HyteraInterfaceClass;

struct TaskData;

class TaskClass : public QMainWindow
{
    Q_OBJECT
public:
    explicit TaskClass(QWidget *parent = 0);
    ~TaskClass();
    int PCScommand(TaskData commandAndData, ContactClass *phoneBook);
	int SMScommand(AbstractedSmsClass *smsDevice, TaskData commandAndData, ContactClass *phoneBook);
	int HYTcommand(HyteraInterfaceClass *hytDevice, TaskData commandAndData, ContactClass *phoneBook);
    int StopTask(void); // kills the task?
    int PauseTask(void); // pauses the task?
    void killTask(void);
	QTimer *SmsTextStuckWd;
	QTimer *hytTextStuckWd;
private:
    bool m_kill;
	bool m_abortText;
	
signals:


public slots:
	
private slots:
	void WDsmsStuck(void);
	void WDhytStuck(void);
};

#endif // TASKCLASS_H
