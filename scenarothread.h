#ifndef SCENAROTHREAD_H
#define SCENAROTHREAD_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QMutex>

class TaskClass;
class ContactClass;
class AbstractedSmsClass;
struct TaskData;

class ScenarioThread : public QThread
{

    Q_OBJECT

public:
    ScenarioThread();
    void run();
    ~ScenarioThread();
    void stopThread();
    QMutex scenarioMutex;
    void setName(QString name);
    QString getName(void);
    QString getCurrentAskName(void);
	void setSerialBoilerText(QString s);
	QString getSerialBoilerText(void);
    //QVector<QString> taskVector;
    QVector<TaskData> taskVector;
    bool debugLockoutTask;
    void cleanObject(void); // dumps all old values
    // we will be passed in pointers to the contact objects
    // we dont need to new them as the memory is allocated already
    ContactClass *smsContactRef;
    ContactClass *pcsContactRef;
	AbstractedSmsClass *smsDevice;
    void setPriority(int p);
    int getPriority(void);
    bool isRunning(void); // bool state of scenarrio
    bool doesHighPriorityKill(void);
    bool doesHighPriorityStore(void);
    void setHighPriorityKill(bool b);
    void setHighPriorityStores(bool b);
    void setResetsGroup(QChar c);
    void setResetBy(QChar c);
    void setIfLatchable(bool b);
    bool getIfLatchable(void);
    QChar getResetByGroup(void);
    QChar getResetsGroup(void);
	bool getCanBePaused(void);
	void setCanBePaused(bool b);
    bool isCurrentlyLatched() const;
    void setIsCurrentlyLatched(bool isCurrentlyLatched);
	void linkScenarioToMultipleDevices(AbstractedSmsClass *sms); //increase to allow all interfaces
	TaskClass *taskControl;
	QString panelText;
private:
    QString m_text;
    QString m_name;
    QString m_currentTaskName; // INFO FOR UI
	QString m_serialBoilerText; // for subsequent serial messages
    bool m_stop;
    int m_loop;
    void stopMe(void);
    
    int m_taskThread;
    int priority;
    bool runningState; // true if running;
    bool hihPriorityKills; // if true a higher priority terminates
    bool highPriorityStores; // will be reinserted into Q
    bool m_latchable;
    bool m_isCurrentlyLatched;
    QChar m_resetsGroup;
    QChar m_isResetBy;
	bool m_canBePaused;
signals:
   void sendTaskName(QString);
   void sendScenarioTaskInfo(QString);
   void notifyScenarioFinished(void);
   void scenarioIssuingReset(QChar resetGroup);

public slots:

private slots:

};

#endif // SCENAROTHREAD_H
