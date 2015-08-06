#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQueue>
#include <QThread>
#include <QString>
#include <QTimer>
#include <QListWidget>
#include <QStringList>
#include <QListIterator>
#include <QFile>
#include <QTextStream>

#define ALARM_STATUS_PAGE 0
#define FAULTS_PAGE 1


namespace Ui {
	class MainWindow;
}

class ScenarioThread;
class FileAccess;
class ContactClass;
class AbstractedSmsClass;
class notifierPanel;
class zitonClass;
class LogClass;

struct TaskData {
	QString protocol;
	QChar messageGroup;
	QString messageBody;
	QString panelText;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public : explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	QQueue<ScenarioThread*> scenarioQueue;
	QQueue<QString> testQ; 
	QString m_string;
	ScenarioThread *touchButtonScenario;
	ScenarioThread *firePanelScenario;
	ScenarioThread *firePanelResetScenario;
	ScenarioThread *firePanelFaultScenario;
	ScenarioThread *firePanelSilenceScenario;
	ScenarioThread *firePanelEvacuateScenario;
	ScenarioThread *lockoutKey;
	FileAccess *fileLoader;
	ContactClass *smsContacts;
	ContactClass *pcsContacts;
	ContactClass *hytContacts;
	AbstractedSmsClass *SMSinterface;
	zitonClass *firePanel;
	QString panelText;
	LogClass *theLogs;
private:
	Ui::MainWindow *ui;
	ScenarioThread *runningScenario;
	ScenarioThread *resquenceScenario;
	void callNextScenario(void);
	bool m_scenarioRunning;
	int m_scenarioIndex;
	QTimer *serviceScenario;
	QTimer *blankStatusBar;
	QTimer *readSerialSMS;
	QTimer *sendATcommand;
	void initaliseGraphics(void);
	int loadConfigFiles(void);
	int queueScenario(ScenarioThread *scenario); // the generic scenario loader
	void setUpConnections(void);   // use to link signals to slots
	void testAndReset(ScenarioThread *s, QChar c); // test and reset the latch flag
	bool m_bellsSilenced; // goes high when rx's a trigger, cleared on alarm or reset
	bool m_keyLockOut;
	bool m_panelLinkOk;
	bool m_smsLinkOk;
	bool m_hagenLinkOk;
public slots :
	void queueFire(void);
	void queueSoft(void);
	void queueManDown(void);
	void pushAckPressed();
	void incomingTaskName(QString);
	void incomingScenarioTaskInfo(QString);
	void scenarioFinished(void);
	void blankStatusBarText(void);
	void simulateEndOfTask(void);
	void passRefOfDevicesToScenario(void);
	void gotUpdatedSigLvl(int i);
	void gotNotificationOfSMS(QString DTG, QString ID, QString body);
	void firePanelFIRE(QString,int);
	void firePanelReset(QString);
	void firePanelFault(QString);
	void firePanelSilence(QString);
	void firePanelEvacuate(QString);
	bool getKeyLockoutState(void);
	void setKeyLockoutState(bool b);
	void peripheralFail(QString s, int i); // a peripheral device has died
	void clearSystemFaults(void);
private slots :
	void checkForNextScenario(void);
	void updateStatusBar(QString message);
	void intiateReset(QChar); // checks scenarioes and resets latch flags
	void testRxOfTrigger(int i); // for test triggerred by an obj in abstraction layer
};

#endif // MAINWINDOW_H
