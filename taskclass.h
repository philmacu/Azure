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
#define MAX_DELAY 1800 // max delay

class ContactClass;
class AbstractedSmsClass;
class SerialInterfaceClass;

struct TaskData;

class TaskClass : public QMainWindow
{
    Q_OBJECT
public:
    explicit TaskClass(QWidget *parent = 0);
    ~TaskClass();
    int PCScommand(TaskData commandAndData, ContactClass *phoneBook);
	int SMScommand(AbstractedSmsClass *smsDevice, TaskData commandAndData, ContactClass *phoneBook);
	int HYTcommand(SerialInterfaceClass *radioDevice, TaskData commandAndData, ContactClass *phoneBook);
	int DLYcommand(QString delayInterval);
	int AUDcommand(QString filePath, QChar pinOut); 
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
