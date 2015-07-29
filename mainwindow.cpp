#include "mainwindow.h"
#include "ui_MainWindow.h"
#include "fileaccess.h"
#include "scenarothread.h"
#include "contactclass.h"
#include "smsabstractionclass.h"


#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	m_scenarioRunning = false;
	m_scenarioIndex = 0;
	// create a file object for test purposes
	fileLoader = new FileAccess;
	// lets create a scenario object
	touchButtonScenario = new ScenarioThread;
	firePanelScenario = new ScenarioThread;
	runningScenario = new ScenarioThread;
	resquenceScenario = new ScenarioThread;
	smsContacts = new ContactClass;
	pcsContacts = new ContactClass;
	hytContacts = new ContactClass;
	// set up a timer to service the scenarioes
	serviceScenario = new QTimer;
	//serviceScenario->setSingleShot(true);
	serviceScenario->start(1000);
	blankStatusBar = new QTimer;
	blankStatusBar->setSingleShot(true);
	blankStatusBar->stop();

	    // debug sset up a fake abstraction Object
	testSmsLayer = new SmsAbstractionClass;

	setUpConnections(); // link signals to slots;

	    // test trigger
	testSmsLayer->generateTrigger();

	    // lets start the file load processs
	if (loadConfigFiles())
	{

	}
	else
		updateStatusBar("Load errors please review.");
	qDebug() << "SMS Phone Book Size: " << smsContacts->contactNumbers.size();
}

MainWindow::~MainWindow()
{
	delete ui;
	delete fileLoader;
	delete touchButtonScenario;
	delete firePanelScenario;
	delete smsContacts;
	delete pcsContacts;
	delete hytContacts;
	delete testSmsLayer;
}

void MainWindow::setUpConnections()
{
    // scenarioes
	connect(touchButtonScenario,
		SIGNAL(sendScenarioName(QString)),
		this,
		SLOT(incomingScenarioName(QString)));
	connect(firePanelScenario,
		SIGNAL(sendScenarioName(QString)),
		this,
		SLOT(incomingScenarioName(QString)));
	    // tasks
	connect(touchButtonScenario,
		SIGNAL(sendTaskName(QString)),
		this,
		SLOT(incomingTaskName(QString)));
	connect(firePanelScenario,
		SIGNAL(sendTaskName(QString)),
		this,
		SLOT(incomingTaskName(QString)));

	            // connect the sceanrio end signals
	connect(touchButtonScenario,
		SIGNAL(notifyScenarioFinished()),
		this,
		SLOT(scenarioFinished()));
	connect(firePanelScenario,
		SIGNAL(notifyScenarioFinished()),
		this,
		SLOT(scenarioFinished()));

	            // these are the connections from objects that want to use STATUS bar
	connect(fileLoader,
		SIGNAL(statusUpdate(QString)),
		this,
		SLOT(updateStatusBar(QString)));

		    // timers
	connect(serviceScenario,
		SIGNAL(timeout()),
		this,
		SLOT(checkForNextScenario()));
	connect(blankStatusBar,
		SIGNAL(timeout()),
		this,
		SLOT(blankStatusBarText()));

		    // scenarioes that emit signals are linked to objects - could be messy!!!
	connect(firePanelScenario,
		SIGNAL(scenarioIssuingReset(QChar)),
		this,
		SLOT(intiateReset(QChar)));
	connect(touchButtonScenario,
		SIGNAL(scenarioIssuingReset(QChar)),
		this,
		SLOT(intiateReset(QChar)));

		    // debug trigger from abstraction layer
	connect(testSmsLayer,
		SIGNAL(triggerDetected(int)),
		this,
		SLOT(testRxOfTrigger(int)));
}


void MainWindow::callNextScenario()
{
	if (runningScenario->isFinished()) m_scenarioRunning = false;
	if (m_scenarioRunning)
	{
	    // start a timer to check back here later
		serviceScenario->start(1000);
	}
	else
	{
		if (!scenarioQueue.isEmpty())
		{
			qDebug() << "Starting a scenario...";
			runningScenario = scenarioQueue.dequeue();
			serviceScenario->start(1000);
			// start if not latchable
			if (!runningScenario->isCurrentlyLatched())
			{
				runningScenario->start();
				m_scenarioRunning = true;
			}
			else
				qDebug() << "Didn't start as it is latched out...";
		}
		else
		{
			ui->labelCurrentScenario->setText("None");
			ui->labelCurrentTask->setText("None");
			qDebug() << "Scenario queue empty";
			serviceScenario->stop();
		}
	}
}

void MainWindow::pushAckPressed()
{
	touchButtonScenario->stopThread();
	firePanelScenario->stopThread();
	simulateEndOfTask();
}

int MainWindow::queueScenario(ScenarioThread *scenario)
{
    // this is a generic queue routine!!!
    // need to check priority level and test against items aheead of it in the Q
	int incomingPriority = scenario->getPriority();
	// need to find its place in the world
	//bool resequnceScenario = false;


	if (scenarioQueue.isEmpty() & !m_scenarioRunning)
	{
	    // Q empty and nothing running
		scenarioQueue.enqueue(scenario);
		callNextScenario();
	}
	else
	{
	    // somthing allready in the Q need to handle that!
	    // is therir anything in the list???? N makes life easier
		if (m_scenarioRunning)
		{
		    // resequence it grap a pointer to current object
			resquenceScenario = runningScenario;
			// check priority level of scenario that is currently running
			if (incomingPriority > (runningScenario->getPriority()))
			{
			    // we want to interrupt top of Q
				if (runningScenario->doesHighPriorityKill())
				{
				    // we must terminate current thread, do we re-sequence it?
					if (runningScenario->doesHighPriorityStore())
					{
						bool insertedInQ = false;
						//resequnceScenario = true;
						for (int i = 0; i < scenarioQueue.size(); i++)
						{
							if ((runningScenario->getPriority()) > (scenarioQueue[i]->getPriority()))
							{
							    // found a place for it
								scenarioQueue.insert(i, resquenceScenario);
								insertedInQ = true;
								i = scenarioQueue.size();
								break;
							}
						}
						// its the lowest of the low
						if (!insertedInQ)
						    // add it to end
							scenarioQueue.append(resquenceScenario);

					}
					// ok now kill it
					runningScenario->stopThread();
				}
			}
		}
		bool insertedInQ = false;
		for (int i = 0; i < scenarioQueue.size(); i++)
		{
			scenarioQueue.insert(i, scenario);
			insertedInQ = true;
			i = scenarioQueue.size();
			break;
		}
		if (!insertedInQ)
		    // add it to end
			scenarioQueue.append(scenario);

	}

	//    scenarioQueue.enqueue(scenario);
	//    callNextScenario();
	return 1;
}

// Place scenario on Queue
void MainWindow::queueFire()
{
	queueScenario(firePanelScenario);
}

void MainWindow::queueSoft()
{
	queueScenario(touchButtonScenario);
}

void MainWindow::queueManDown()
{
    //scenarioQueue.enqueue(manDownScenario);
	callNextScenario();

}

// load the config params
int MainWindow::loadConfigFiles()
{
    // try loading a phone book
	QString fileName = "/home/sts/files/sms_contacts.txt"; /*"c:/Users/Phil/Documents/sms_contacts.txt";*/
	if (fileLoader->loadContacts(fileName, smsContacts))
	{
		updateStatusBar((fileName + " Loaded OK"));
		// pass in a reference to sms contact list
		touchButtonScenario->smsContactRef = smsContacts;
		firePanelScenario->smsContactRef = smsContacts;
		fileLoader->smsPhoneBook = smsContacts;
	}
	else updateStatusBar((fileName + " Load Error"));

	fileName = "/home/sts/files/pcs_contacts.txt";
	if (fileLoader->loadContacts(fileName, pcsContacts))
	{
		updateStatusBar((fileName + " Loaded OK"));
		touchButtonScenario->pcsContactRef = pcsContacts;
		firePanelScenario->pcsContactRef = pcsContacts;
		fileLoader->pcsPhoneBook = pcsContacts;
	}
	else updateStatusBar((fileName + " Load Error"));

	    // for test lets try open a file
	fileName = "/home/sts/files/fire.txt" /*"/home/sts/files/fire.txt"*/;
	if (fileLoader->loadFile(fileName, firePanelScenario))
	{
		updateStatusBar((fileName + " Loaded OK"));
	}
	else updateStatusBar("Load error");

	fileName = "/home/sts/files/mandown.txt" /*"/home/sts/files/mandown.txt"*/;
	if (fileLoader->loadFile(fileName, touchButtonScenario))
	{
		updateStatusBar((fileName + " Loaded OK"));
	}
	else updateStatusBar("Load error");

	return 1;
}

// ///////
// SLOTS
//
void MainWindow::incomingTaskName(QString task)
{
    // a task just sent us its name
	ui->labelCurrentTask->setText(task);
	qDebug() << "task name set";
}

void MainWindow::incomingScenarioName(QString scenario)
{
	ui->labelCurrentScenario->setText(scenario);
	qDebug() << "scenario name set";
}

void MainWindow::scenarioFinished()
{
    // get next from the queue
	m_scenarioRunning = false;
	callNextScenario();
}

void MainWindow::checkForNextScenario()
{
    // go back see if anything else in Q
	callNextScenario();
}

void MainWindow::updateStatusBar(QString message)
{
    // update the list
	ui->listStatus->addItem(message);
	// this will display for five seconds then delete
	ui->labelStatus->setText(message);
	blankStatusBar->start(5000);
}

void MainWindow::intiateReset(QChar c)
{
    // this is triggered via an object doing an emit
    // check all scenarioes
	testAndReset(touchButtonScenario, c);
	testAndReset(firePanelScenario, c);
}

void MainWindow::testRxOfTrigger(int i)
{
    // for debug
	qDebug() << "Trigger Code: " << i;
}

void MainWindow::testAndReset(ScenarioThread *s, QChar c)
{
	if (c == s->getResetByGroup())
		s->setIsCurrentlyLatched(false);
}

void MainWindow::blankStatusBarText()
{
	ui->labelStatus->setText("");
	blankStatusBar->stop();
}

void MainWindow::simulateEndOfTask()
{
    //
	firePanelScenario->debugLockoutTask = false;
}
