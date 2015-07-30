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

#include <iostream>
#include <string.h>
#include <string>

using namespace std;

#define BODY_SOURCE "USE RS232"

class ContactClass;
class AbstractedSmsClass;

struct TaskData;

class TaskClass : public QMainWindow
{
    Q_OBJECT
public:
    explicit TaskClass(QWidget *parent = 0);
    ~TaskClass();
    int PCScommand(TaskData commandAndData, ContactClass *phoneBook);
	int SMScommand(AbstractedSmsClass *smsDevice, TaskData commandAndData, ContactClass *phoneBook);
    int StopTask(void); // kills the task?
    int PauseTask(void); // pauses the task?
    void killTask(void);

private:
    bool m_kill;
	
signals:

public slots:
};

#endif // TASKCLASS_H
