#include "mainwindow.h"
#include "ui_MainWindow.h"
#include "fileaccess.h"
#include "scenarothread.h"
#include "contactclass.h"
#include "abstractedsmsclass.h"
//#include "notifierPanel.h"
#include "zitonClass.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	initaliseGraphics();
	m_scenarioRunning = false;
	m_scenarioIndex = 0;
	// create a file object for test purposes
	fileLoader = new FileAccess;
	// panel object
	firePanel = new zitonClass;
	// devices reg is passed to scenario then object
	SMSinterface = new AbstractedSmsClass;
	// lets create a scenario object
	touchButtonScenario = new ScenarioThread;
	firePanelScenario = new ScenarioThread;
	runningScenario = new ScenarioThread;
	resquenceScenario = new ScenarioThread;
	firePanelResetScenario = new ScenarioThread;
	firePanelFaultScenario = new ScenarioThread;
	firePanelSilenceScenario = new ScenarioThread;
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
	// timer for checking sms serial
	readSerialSMS = new QTimer;
	readSerialSMS->stop();
	sendATcommand = new QTimer;
	sendATcommand->stop();

	
	

	setUpConnections(); // link signals to slots;
	passRefOfDevicesToScenario(); // link devices to scenario
	
	    // test trigger
	//SMSinterface->generateTrigger();

	    // lets start the file load processs
	if (loadConfigFiles())
	{

	}
	else
		updateStatusBar("Load errors please review.");
	qDebug() << "SMS Phone Book Size: " << smsContacts->contactNumbers.size();
	// start timers
	readSerialSMS->start(10);
	sendATcommand->start(1000);

	
}

MainWindow::~MainWindow()
{
	delete ui;
	delete fileLoader;
	delete touchButtonScenario;
	delete firePanelScenario;
	delete firePanelResetScenario;
	delete firePanelFaultScenario;
	delete firePanelSilenceScenario;
	delete smsContacts;
	delete pcsContacts;
	delete hytContacts;
	delete SMSinterface;
	delete blankStatusBar;
	delete readSerialSMS;
	delete sendATcommand;
}

void MainWindow::setUpConnections()
{
	// add scenario here
	// graphics
    // scenarioes names
	connect(touchButtonScenario,SIGNAL(sendScenarioName(QString)),
		this,SLOT(incomingScenarioName(QString)));
	connect(firePanelScenario,SIGNAL(sendScenarioName(QString)),
		this,SLOT(incomingScenarioName(QString)));
	connect(firePanelResetScenario, SIGNAL(sendScenarioName(QString)),
		this, SLOT(incomingScenarioName(QString)));
	connect(firePanelFaultScenario, SIGNAL(sendScenarioName(QString)),
		this, SLOT(incomingScenarioName(QString)));
	connect(firePanelSilenceScenario, SIGNAL(sendScenarioName(QString)),
		this, SLOT(incomingScenarioName(QString)));
	    // tasks
	connect(touchButtonScenario,SIGNAL(sendTaskName(QString)),
		this,SLOT(incomingTaskName(QString)));
	connect(firePanelScenario,SIGNAL(sendTaskName(QString)),
		this,SLOT(incomingTaskName(QString)));
	connect(firePanelResetScenario, SIGNAL(sendTaskName(QString)),
		this, SLOT(incomingTaskName(QString)));
	connect(firePanelFaultScenario, SIGNAL(sendTaskName(QString)),
		this, SLOT(incomingTaskName(QString)));
	connect(firePanelSilenceScenario, SIGNAL(sendTaskName(QString)),
		this, SLOT(incomingTaskName(QString)));
	     // connect the sceanrio end signals
	connect(touchButtonScenario,SIGNAL(notifyScenarioFinished()),
		this,SLOT(scenarioFinished()));
	connect(firePanelScenario,SIGNAL(notifyScenarioFinished()),
		this,SLOT(scenarioFinished()));
	connect(firePanelResetScenario, SIGNAL(notifyScenarioFinished()),
		this, SLOT(scenarioFinished()));
	connect(firePanelFaultScenario, SIGNAL(notifyScenarioFinished()),
		this, SLOT(scenarioFinished()));
	connect(firePanelSilenceScenario, SIGNAL(notifyScenarioFinished()),
		this, SLOT(scenarioFinished()));

	    // these are the connections from objects that want to use STATUS bar
	connect(fileLoader,SIGNAL(statusUpdate(QString)),
		this,SLOT(updateStatusBar(QString)));

		// timersSmsCTSwd
	connect(serviceScenario,SIGNAL(timeout()),this,SLOT(checkForNextScenario()));
	connect(blankStatusBar,SIGNAL(timeout()),this,SLOT(blankStatusBarText()));
	connect(readSerialSMS, SIGNAL(timeout()), SMSinterface, SLOT(readSerial()));
	connect(sendATcommand, SIGNAL(timeout()), SMSinterface, SLOT(sendNextATcommand()));
	
	// scenarioes that emit signals are linked to objects 
	connect(firePanelScenario,SIGNAL(scenarioIssuingReset(QChar)),this,SLOT(intiateReset(QChar)));
	connect(touchButtonScenario,SIGNAL(scenarioIssuingReset(QChar)),this,SLOT(intiateReset(QChar)));
	connect(firePanelResetScenario, SIGNAL(scenarioIssuingReset(QChar)), this, SLOT(intiateReset(QChar)));
	connect(firePanelFaultScenario, SIGNAL(scenarioIssuingReset(QChar)), this, SLOT(intiateReset(QChar)));
	connect(firePanelSilenceScenario, SIGNAL(scenarioIssuingReset(QChar)), this, SLOT(intiateReset(QChar)));

	// signals for the UI
	connect(SMSinterface,SIGNAL(signalLevelIs(int)), this, SLOT(gotUpdatedSigLvl(int)));
	connect(SMSinterface,SIGNAL(parsedIncomingSMS(QString, QString, QString)),
		this,SLOT(gotNotificationOfSMS(QString, QString, QString)));

	// connections from fire panel triggers
	connect(firePanel, SIGNAL(callFirePanelEvent(QString,int)), this, SLOT(firePanelFIRE(QString,int)));
	connect(firePanel, SIGNAL(callResetPanelEvent(QString)), this, SLOT(firePanelReset(QString)));
	connect(firePanel, SIGNAL(callFaultPanelEvent(QString)), this, SLOT(firePanelFault(QString)));
	connect(firePanel, SIGNAL(callSilencePanelEvent(QString)), this, SLOT(firePanelSilence(QString)));
	connect(firePanel, SIGNAL(firePanelSilenceScenario(QString)), this, SLOT(firePanelSilence(QString)));

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
			ui->labelCurrentScenario->setText("Finished");
			ui->labelCurrentTask->setText("None");
			qDebug() << "Scenario queue empty";
			serviceScenario->stop();
		}
	}
}

void MainWindow::pushAckPressed()
{

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
	// add scenario here
    // try loading a phone book
	QString fileName = "/home/sts/files/sms_contacts.txt"; /*"c:/Users/Phil/Documents/sms_contacts.txt";*/
	if (fileLoader->loadContacts(fileName, smsContacts))
	{
		updateStatusBar((fileName + " Loaded OK"));
		// pass in a reference to sms contact list
		touchButtonScenario->smsContactRef = smsContacts;
		firePanelScenario->smsContactRef = smsContacts;
		firePanelResetScenario->smsContactRef = smsContacts;
		firePanelFaultScenario->smsContactRef = smsContacts;
		firePanelSilenceScenario->smsContactRef = smsContacts;
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
		firePanelResetScenario->pcsContactRef = pcsContacts;
		firePanelFaultScenario->pcsContactRef = pcsContacts;
		firePanelSilenceScenario->pcsContactRef = pcsContacts;
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

	// panel reseceived a reset event
	fileName = "/home/sts/files/fireReset.txt";
	if (fileLoader->loadFile(fileName, firePanelResetScenario))
	{
		updateStatusBar((fileName + " Loaded OK"));
	}
	else updateStatusBar("Load error");

	// add scenario here
	// panel reseceived a fault event
	fileName = "/home/sts/files/fault.txt";
	if (fileLoader->loadFile(fileName, firePanelFaultScenario))
	{
		updateStatusBar((fileName + " Loaded OK"));
	}
	else updateStatusBar("Load error");

	// panel silenced
	fileName = "/home/sts/files/silence.txt";
	if (fileLoader->loadFile(fileName, firePanelSilenceScenario))
	{
		updateStatusBar((fileName + " Loaded OK"));
	}
	else updateStatusBar("Load error");

	return 1;
}

void MainWindow::initaliseGraphics()
{
	// set up the look of the UI at boot
	ui->silence->setVisible(false);
	ui->fault->setVisible(false);
	ui->fire->setVisible(false);
	// Tab graphics
	ui->infoPage->setTabEnabled(1, false);
	ui->infoPage->setTabText(1, "No Faults");
	ui->infoPage->setCurrentIndex(ALARM_STATUS_PAGE);
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
	// add scenario here
    // this is triggered via an object doing an emit
    // check all scenarioes ignore the char '-'
	if (c != '-')
	{
		testAndReset(touchButtonScenario, c);
		testAndReset(firePanelScenario, c);
		testAndReset(firePanelResetScenario, c);
		testAndReset(firePanelFaultScenario, c);
		testAndReset(firePanelSilenceScenario, c);
	}
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


void MainWindow::gotUpdatedSigLvl(int i)
{
	// update UI
	ui->signalLevel->setValue(i);
	
}


void MainWindow::passRefOfDevicesToScenario(void)
{
	// add scenario here
	touchButtonScenario->linkScenarioToMultipleDevices(SMSinterface);
	firePanelScenario->linkScenarioToMultipleDevices(SMSinterface);
	firePanelResetScenario->linkScenarioToMultipleDevices(SMSinterface);
	firePanelFaultScenario->linkScenarioToMultipleDevices(SMSinterface);
	firePanelSilenceScenario->linkScenarioToMultipleDevices(SMSinterface);
}


void MainWindow::gotNotificationOfSMS(QString DTG, QString ID, QString body)
{
	// display it
	ui->caller->setText(ID);
	ui->smsBody->setText(body);
	// now check to see if this text is a privlidged user
	if (!(ID.compare("+353868072505")))
	{
		qDebug() << "Hi Larry!";
		SMSinterface->sendText("+353868072505", "Hi Larry I am alive :-)");
	}
}

// triggers in --- need to call scenarios, one for each
void MainWindow::firePanelFIRE(QString s,int i)
{
	ui->infoPage->setCurrentIndex(ALARM_STATUS_PAGE);
	ui->fire->setVisible(true);
	// this is linked to the Panel FIRE signal
	qDebug() << "Alarm Number: " << i << " " << s;
	ui->listFires->addItem(s);
	panelText = s;
	m_bellsSilenced = false;
	ui->silence->setVisible(false);
	// now call the Fire Scenario but ... if we already called
	// one we must use the boiler text
	if (i == 1)
		firePanelScenario->panelText = panelText;
	else
		firePanelScenario->panelText = firePanelScenario->getSerialBoilerText();
	queueFire();
}

void MainWindow::firePanelReset(QString)
{
	// possible reset tab graphics
	if (ui->infoPage->tabText(FAULTS_PAGE) == "Panel Faults")
	{
		ui->infoPage->setTabText(FAULTS_PAGE, "Panel Faults Cleared");
		ui->infoPage->setTabEnabled(FAULTS_PAGE, false);
		ui->infoPage->setCurrentIndex(ALARM_STATUS_PAGE);
	}
	// clear fire list
	ui->listFires->clear();
	ui->listFaults->clear();
	m_bellsSilenced = false;
	ui->silence->setVisible(false);
	ui->fault->setVisible(false);
	ui->fire->setVisible(false);
	// call the scenario
	queueScenario(firePanelResetScenario);
	
}

void MainWindow::firePanelFault(QString s)
{
	ui->fault->setVisible(true);
	ui->infoPage->setTabEnabled(FAULTS_PAGE, true);
	// add to fault list
	ui->listFaults->addItem(s);
	
	// Tab graphics
	ui->infoPage->setTabText(FAULTS_PAGE, "Panel Faults");
	// if not displaying an alarm switch to faults page
	if (!ui->fire->isVisible())
		ui->infoPage->setCurrentIndex(FAULTS_PAGE);

	// pass through panel text
	// alternative method to FIRE this checks to see
	// if we all ready had an entry in the fault list
	int i = ui->listFaults->count();
	if (i == 1)
		firePanelFaultScenario->panelText = s;
	else
		firePanelFaultScenario->panelText = firePanelFaultScenario->getSerialBoilerText();

	// call the scenario
	queueScenario(firePanelFaultScenario);
}

void MainWindow::firePanelSilence(QString s)
{
	m_bellsSilenced = true;
	ui->silence->setVisible(true);

	// call the scenario
	queueScenario(firePanelSilenceScenario);
}