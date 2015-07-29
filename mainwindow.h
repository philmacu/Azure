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

namespace Ui {
	class MainWindow;
}

class ScenarioThread;
class FileAccess;
class ContactClass;
class SmsAbstractionClass;

struct TaskData {
	QString protocol;
	QChar messageGroup;
	QString messageBody;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

	public :
	    explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	QQueue<ScenarioThread*> scenarioQueue;
	QQueue<QString> testQ; 
	QString m_string;
	ScenarioThread *touchButtonScenario;
	ScenarioThread *firePanelScenario;
	FileAccess *fileLoader;
	ContactClass *smsContacts;
	ContactClass *pcsContacts;
	ContactClass *hytContacts;
	SmsAbstractionClass *testSmsLayer;

private:
	Ui::MainWindow *ui;
	ScenarioThread *runningScenario;
	ScenarioThread *resquenceScenario;
	void callNextScenario(void);
	bool m_scenarioRunning;
	int m_scenarioIndex;
	QTimer *serviceScenario;
	QTimer *blankStatusBar;
	int loadConfigFiles(void);
	int queueScenario(ScenarioThread *scenario); // the generic scenario loader
	void setUpConnections(void);   // use to link signals to slots
	void testAndReset(ScenarioThread *s, QChar c); // test and reset the latch flag
	public slots :
	    void queueFire(void);
	void queueSoft(void);
	void queueManDown(void);
	void pushAckPressed();
	void incomingTaskName(QString);
	void incomingScenarioName(QString);
	void scenarioFinished(void);
	void blankStatusBarText(void);
	void simulateEndOfTask(void);

	private slots :
	    void checkForNextScenario(void);
	void updateStatusBar(QString message);
	void intiateReset(QChar); // checks scenarioes and resets latch flags

	void testRxOfTrigger(int i); // for test triggerred by an obj in abstraction layer
};

#endif // MAINWINDOW_H
