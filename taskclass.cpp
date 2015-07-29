#include "taskclass.h"
#include "contactclass.h"
#include "mainwindow.h"

TaskClass::TaskClass(QWidget *parent) : QMainWindow(parent)
{
    m_kill = false;
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

int TaskClass::SMScommand(TaskData commandAndData, ContactClass *phoneBook)
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
                    qDebug() << "using serial port text";
                else
                    qDebug() << commandAndData.messageBody;
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
